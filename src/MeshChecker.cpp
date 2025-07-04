#include "MeshChecker.h"
#include "UvChecker.h"
#include "Logger.h"

#include <CGAL/Simple_cartesian.h>
#include <CGAL/Surface_mesh.h>
#include <CGAL/Polygon_mesh_processing/orient_polygon_soup.h>
#include <CGAL/Polygon_mesh_processing/repair_polygon_soup.h>
#include <CGAL/Polygon_mesh_processing/polygon_soup_to_polygon_mesh.h>
#include <CGAL/Polygon_mesh_processing/border.h>
#include <CGAL/Polygon_mesh_processing/manifoldness.h>
#include <CGAL/Polygon_mesh_processing/self_intersections.h>
#include <CGAL/Polygon_mesh_processing/repair.h>
#include <CGAL/Polygon_mesh_processing/corefinement.h>


typedef CGAL::Simple_cartesian<double> K;
typedef K::Point_3 Point;
typedef CGAL::Surface_mesh<Point> CGALMesh;
typedef CGALMesh::Face_index face_descriptor;
typedef CGALMesh::Halfedge_index halfedge_descriptor;

MeshChecker::CheckResult MeshChecker::check(const Mesh& mesh, const std::set<CheckType>& checksToPerform)
{
    CheckResult result;
    // Initialize results
    result.is_watertight = false;
    result.non_manifold_vertices_count = 0;
    result.self_intersections_count = 0;
    result.holes_count = 0;
    result.degenerate_faces_count = 0;
    result.has_uvs = false;
    result.overlapping_uv_islands_count = 0;
    result.uvs_out_of_bounds_count = 0;

    try {
        Logger::log("Starting mesh check...");

        std::vector<Point> points;
        points.reserve(mesh.vertices.size());
        for (const auto& v : mesh.vertices) {
            points.emplace_back(v.x, v.y, v.z);
        }

        std::vector<std::vector<std::size_t>> polygons;
        polygons.reserve(mesh.vertex_indices.size() / 3);
        for (size_t i = 0; i < mesh.vertex_indices.size(); i += 3) {
            polygons.push_back({mesh.vertex_indices[i], mesh.vertex_indices[i+1], mesh.vertex_indices[i+2]});
        }

        try {
            CGAL::Polygon_mesh_processing::repair_polygon_soup(points, polygons);
            CGAL::Polygon_mesh_processing::orient_polygon_soup(points, polygons);
        } catch (const std::exception& e) {
            Logger::log("CGAL Exception during soup processing: " + std::string(e.what()));
        }

        CGALMesh cgal_mesh;
        CGAL::Polygon_mesh_processing::polygon_soup_to_polygon_mesh(points, polygons, cgal_mesh);
        
        // Create a property map to store original face indices
        CGALMesh::Property_map<face_descriptor, std::size_t> original_face_indices;
        bool created;
        boost::tie(original_face_indices, created) = cgal_mesh.add_property_map<face_descriptor, std::size_t>("f:original_index", 0);

        // We need to re-associate faces in the cgal_mesh with original indices.
        // This is tricky after soup processing. A robust way is to map vertex indices.
        // For now, we assume the order is maintained by polygon_soup_to_polygon_mesh.
        std::size_t face_idx_counter = 0;
        for(face_descriptor fd : faces(cgal_mesh)) {
            original_face_indices[fd] = face_idx_counter++;
        }


        if (checksToPerform.count(CheckType::Watertight)) {
            Logger::log("Checking watertightness...");
            result.is_watertight = CGAL::is_closed(cgal_mesh);
        }

        if (checksToPerform.count(CheckType::NonManifold)) {
            Logger::log("Checking non-manifold vertices...");
            std::vector<halfedge_descriptor> non_manifold_halfedges;
            CGAL::Polygon_mesh_processing::non_manifold_vertices(cgal_mesh, std::back_inserter(non_manifold_halfedges));
            result.non_manifold_vertices_count = non_manifold_halfedges.size();
            std::set<std::size_t> non_manifold_faces_set;
            for (const auto& h : non_manifold_halfedges) {
                 if(!is_border(h, cgal_mesh))
                    non_manifold_faces_set.insert(original_face_indices[face(h, cgal_mesh)]);
            }
            result.non_manifold_faces.assign(non_manifold_faces_set.begin(), non_manifold_faces_set.end());
        }

        if (checksToPerform.count(CheckType::SelfIntersect)) {
            Logger::log("Checking self-intersections...");
            std::vector<std::pair<face_descriptor, face_descriptor>> self_intersections;
            CGAL::Polygon_mesh_processing::self_intersections(cgal_mesh, std::back_inserter(self_intersections));
            result.self_intersections_count = self_intersections.size();
            std::set<std::size_t> intersecting_faces_set;
            for(const auto& pair : self_intersections) {
                intersecting_faces_set.insert(original_face_indices[pair.first]);
                intersecting_faces_set.insert(original_face_indices[pair.second]);
            }
            result.intersecting_faces.assign(intersecting_faces_set.begin(), intersecting_faces_set.end());
        }

        if (checksToPerform.count(CheckType::Holes)) {
            Logger::log("Checking for holes...");
            std::vector<halfedge_descriptor> border_edges;
            CGAL::Polygon_mesh_processing::border_halfedges(faces(cgal_mesh), cgal_mesh, std::back_inserter(border_edges));
            
            result.hole_loops.clear();
            if (!border_edges.empty()) {
                std::map<CGALMesh::Vertex_index, unsigned int> cgal_to_orig_v;
                unsigned int v_idx_counter = 0;
                for(CGALMesh::Vertex_index vd : vertices(cgal_mesh)) {
                    cgal_to_orig_v[vd] = v_idx_counter++;
                }

                std::map<CGALMesh::Vertex_index, halfedge_descriptor> successor_map;
                for(const auto& h : border_edges) {
                    successor_map[source(h, cgal_mesh)] = h;
                }

                std::set<CGALMesh::Vertex_index> visited_vertices;
                for(const auto& start_v : cgal_to_orig_v) {
                    if(successor_map.count(start_v.first) && visited_vertices.find(start_v.first) == visited_vertices.end()) {
                        std::vector<unsigned int> current_loop;
                        CGALMesh::Vertex_index current_v = start_v.first;
                        do {
                            visited_vertices.insert(current_v);
                            current_loop.push_back(start_v.second);
                            halfedge_descriptor h = successor_map.at(current_v);
                            current_v = target(h, cgal_mesh);
                        } while(current_v != start_v.first && visited_vertices.find(current_v) == visited_vertices.end());
                        result.hole_loops.push_back(current_loop);
                    }
                }
            }
            result.holes_count = result.hole_loops.size();
        }

        if (checksToPerform.count(CheckType::DegenerateFaces)) {
            Logger::log("Checking for degenerate faces...");
            result.degenerate_faces_count = CGAL::Polygon_mesh_processing::remove_degenerate_faces(cgal_mesh);
        }

        Logger::log("Checking UVs...");
        result.has_uvs = UvChecker::hasUvs(mesh);
        if (result.has_uvs) {
            if (checksToPerform.count(CheckType::UVOverlap)) {
                result.overlapping_uv_islands_count = UvChecker::countOverlappingUvIslands(mesh, result.overlapping_uv_faces);
            }
            if (checksToPerform.count(CheckType::UVBounds)) {
                result.uvs_out_of_bounds_count = UvChecker::countUvsOutOfBounds(mesh);
            }
        }

        Logger::log("Mesh check finished.");
    } catch (const std::exception& e) {
        Logger::log("CGAL Exception: " + std::string(e.what()));
    } catch (...) {
        Logger::log("An unknown exception occurred during mesh check.");
    }

    return result;
}

bool MeshChecker::intersects(const Mesh& mesh1, const Mesh& mesh2)
{
    try {
        std::vector<Point> points1;
        for (const auto& v : mesh1.vertices) {
            points1.emplace_back(v.x, v.y, v.z);
        }
        std::vector<std::vector<std::size_t>> polygons1;
        for (size_t i = 0; i < mesh1.vertex_indices.size(); i += 3) {
            polygons1.push_back({mesh1.vertex_indices[i], mesh1.vertex_indices[i+1], mesh1.vertex_indices[i+2]});
        }
        CGALMesh cgal_mesh1;
        CGAL::Polygon_mesh_processing::polygon_soup_to_polygon_mesh(points1, polygons1, cgal_mesh1);

        std::vector<Point> points2;
        for (const auto& v : mesh2.vertices) {
            points2.emplace_back(v.x, v.y, v.z);
        }
        std::vector<std::vector<std::size_t>> polygons2;
        for (size_t i = 0; i < mesh2.vertex_indices.size(); i += 3) {
            polygons2.push_back({mesh2.vertex_indices[i], mesh2.vertex_indices[i+1], mesh2.vertex_indices[i+2]});
        }
        CGALMesh cgal_mesh2;
        CGAL::Polygon_mesh_processing::polygon_soup_to_polygon_mesh(points2, polygons2, cgal_mesh2);

        return CGAL::Polygon_mesh_processing::do_intersect(cgal_mesh1, cgal_mesh2);
    } catch (const std::exception& e) {
        Logger::log("CGAL Exception in intersects: " + std::string(e.what()));
        return false;
    } catch (...) {
        Logger::log("An unknown exception occurred during intersection check.");
        return false;
    }
}



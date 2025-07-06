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
#include <CGAL/Polygon_mesh_processing/remesh.h>
#include <CGAL/Polygon_mesh_processing/measure.h>
#include <CGAL/boost/graph/helpers.h>
#include <CGAL/Side_of_triangle_mesh.h>
#include <CGAL/AABB_tree.h>
#include <CGAL/AABB_traits.h>
#include <CGAL/AABB_triangle_primitive.h>
#include <set>
#include <numeric>
#include <thread>
#include <vector>

namespace PMP = CGAL::Polygon_mesh_processing;

typedef CGAL::Simple_cartesian<double> K;
typedef K::Point_3 Point;
typedef K::Triangle_3 Triangle;
typedef CGAL::Surface_mesh<Point> CGALMesh;
typedef CGALMesh::Face_index face_descriptor;
typedef CGALMesh::Halfedge_index halfedge_descriptor;

typedef std::vector<Triangle>::iterator Triangle_iterator;
typedef CGAL::AABB_triangle_primitive<K, Triangle_iterator> Primitive;
typedef CGAL::AABB_traits<K, Primitive> Traits;
typedef CGAL::AABB_tree<Traits> Tree;

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
        Logger::getInstance().log("Starting mesh conversion to CGAL format...");

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
            Logger::getInstance().log("Repairing polygon soup...");
            CGAL::Polygon_mesh_processing::repair_polygon_soup(points, polygons);
            Logger::getInstance().log("Orienting polygon soup...");
            CGAL::Polygon_mesh_processing::orient_polygon_soup(points, polygons);
        } catch (const std::exception& e) {
            Logger::getInstance().log("CGAL Exception during soup processing: " + std::string(e.what()));
        }

        CGALMesh cgal_mesh;
        CGAL::Polygon_mesh_processing::polygon_soup_to_polygon_mesh(points, polygons, cgal_mesh);
        Logger::getInstance().log("Mesh conversion finished.");
        
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

        std::vector<std::thread> threads;

        if (checksToPerform.count(CheckType::Watertight)) {
            threads.emplace_back([&]() {
                Logger::getInstance().log("Checking watertightness...");
                result.is_watertight = CGAL::is_closed(cgal_mesh);
                Logger::getInstance().log(std::string("Watertight: ") + (result.is_watertight ? "Yes" : "No"));
            });
        }

        if (checksToPerform.count(CheckType::NonManifold)) {
            threads.emplace_back([&]() {
                Logger::getInstance().log("Checking non-manifold vertices...");
                std::vector<halfedge_descriptor> non_manifold_halfedges;
                CGAL::Polygon_mesh_processing::non_manifold_vertices(cgal_mesh, std::back_inserter(non_manifold_halfedges));
                result.non_manifold_vertices_count = non_manifold_halfedges.size();
                std::set<std::size_t> non_manifold_faces_set;
                for (const auto& h : non_manifold_halfedges) {
                     if(!is_border(h, cgal_mesh))
                        non_manifold_faces_set.insert(original_face_indices[face(h, cgal_mesh)]);
                }
                result.non_manifold_faces.assign(non_manifold_faces_set.begin(), non_manifold_faces_set.end());
                Logger::getInstance().log("Non-manifold vertices found: " + std::to_string(result.non_manifold_vertices_count));
            });
        }

        if (checksToPerform.count(CheckType::SelfIntersect)) {
            threads.emplace_back([&]() {
                Logger::getInstance().log("Checking self-intersections...");
                std::vector<std::pair<face_descriptor, face_descriptor>> self_intersections;
                CGAL::Polygon_mesh_processing::self_intersections(cgal_mesh, std::back_inserter(self_intersections));
                result.self_intersections_count = self_intersections.size();
                std::set<std::size_t> intersecting_faces_set;
                for(const auto& pair : self_intersections) {
                    intersecting_faces_set.insert(original_face_indices[pair.first]);
                    intersecting_faces_set.insert(original_face_indices[pair.second]);
                }
                result.intersecting_faces.assign(intersecting_faces_set.begin(), intersecting_faces_set.end());
                Logger::getInstance().log("Self-intersections found: " + std::to_string(result.self_intersections_count));
            });
        }

        if (checksToPerform.count(CheckType::Holes)) {
            threads.emplace_back([&]() {
                Logger::getInstance().log("Checking for holes...");
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
                Logger::getInstance().log("Holes found: " + std::to_string(result.holes_count));
            });
        }

        if (checksToPerform.count(CheckType::DegenerateFaces)) {
            threads.emplace_back([&]() {
                Logger::getInstance().log("Checking for degenerate faces with relative tolerance...");
                result.degenerate_faces_count = 0;
                const double epsilon_sq = 1e-12; // A small tolerance for the squared ratio

                for(face_descriptor fd : faces(cgal_mesh)) {
                    auto h = halfedge(fd, cgal_mesh);
                    const auto& p1 = cgal_mesh.point(source(h, cgal_mesh));
                    const auto& p2 = cgal_mesh.point(target(h, cgal_mesh));
                    const auto& p3 = cgal_mesh.point(target(next(h, cgal_mesh), cgal_mesh));

                    K::Vector_3 v1 = p2 - p1;
                    K::Vector_3 v2 = p3 - p1;
                    K::Vector_3 v3 = p3 - p2;

                    double a_sq = v1.squared_length();
                    double b_sq = v2.squared_length();
                    double c_sq = v3.squared_length();

                    // Avoid division by zero for zero-length edges (which are degenerate)
                    if (a_sq == 0 || b_sq == 0 || c_sq == 0) {
                        result.degenerate_faces_count++;
                        continue;
                    }
                    
                    K::Vector_3 cross_prod = CGAL::cross_product(v1, v2);
                    double area_sq_x4 = cross_prod.squared_length(); // This is 4 * (triangle area)^2

                    // Find the longest edge squared
                    double max_edge_sq = std::max({a_sq, b_sq, c_sq});

                    // Check if area is negligible compared to the longest edge
                    if (area_sq_x4 / max_edge_sq < epsilon_sq) {
                        result.degenerate_faces_count++;
                    }
                }
                Logger::getInstance().log("Degenerate faces found: " + std::to_string(result.degenerate_faces_count));
            });
        }

        threads.emplace_back([&]() {
            Logger::getInstance().log("Checking UVs...");
            result.has_uvs = UvChecker::hasUvs(mesh);
            Logger::getInstance().log(std::string("Has UVs: ") + (result.has_uvs ? "Yes" : "No"));
            if (result.has_uvs) {
                if (checksToPerform.count(CheckType::UVOverlap)) {
                    Logger::getInstance().log("Checking for overlapping UVs...");
                    result.overlapping_uv_islands_count = UvChecker::countOverlappingUvIslands(mesh, result.overlapping_uv_faces);
                    Logger::getInstance().log("Overlapping UV islands found: " + std::to_string(result.overlapping_uv_islands_count));
                }
                if (checksToPerform.count(CheckType::UVBounds)) {
                    Logger::getInstance().log("Checking for UVs out of bounds...");
                    result.uvs_out_of_bounds_count = UvChecker::countUvsOutOfBounds(mesh);
                    Logger::getInstance().log("UVs out of bounds found: " + std::to_string(result.uvs_out_of_bounds_count));
                }
            }
        });

        for (auto& thread : threads) {
            thread.join();
        }

    } catch (const std::exception& e) {
        Logger::getInstance().log("CGAL Exception: " + std::string(e.what()));
    } catch (...) {
        Logger::getInstance().log("An unknown exception occurred during mesh check.");
    }

    return result;
}

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
#include <CGAL/Polygon_mesh_processing/measure.h>
#include <CGAL/boost/graph/helpers.h>
#include <set>
#include <numeric>

namespace PMP = CGAL::Polygon_mesh_processing;

typedef CGAL::Simple_cartesian<double> K;
typedef K::Point_3 Point;
typedef CGAL::Surface_mesh<Point> CGALMesh;
typedef CGALMesh::Face_index face_descriptor;
typedef CGALMesh::Halfedge_index halfedge_descriptor;



bool MeshChecker::intersects(const Mesh& mesh1, const Mesh& mesh2, std::vector<int>& intersecting_faces)
{
    try {
        Logger::getInstance().log("Building triangle lists for intersection check...");

        // --- Process Mesh 1 (Mannequin) ---
        std::vector<Triangle> triangles1;
        triangles1.reserve(mesh1.vertex_indices.size() / 3);
        for (size_t i = 0; i < mesh1.vertex_indices.size(); i += 3) {
            const auto& p1 = mesh1.vertices[mesh1.vertex_indices[i]];
            const auto& p2 = mesh1.vertices[mesh1.vertex_indices[i+1]];
            const auto& p3 = mesh1.vertices[mesh1.vertex_indices[i+2]];
            triangles1.emplace_back(Point(p1.x, p1.y, p1.z), Point(p2.x, p2.y, p2.z), Point(p3.x, p3.y, p3.z));
        }

        // --- Process Mesh 2 (Apparel) ---
        std::vector<Triangle> triangles2;
        triangles2.reserve(mesh2.vertex_indices.size() / 3);
        for (size_t i = 0; i < mesh2.vertex_indices.size(); i += 3) {
            const auto& p1 = mesh2.vertices[mesh2.vertex_indices[i]];
            const auto& p2 = mesh2.vertices[mesh2.vertex_indices[i+1]];
            const auto& p3 = mesh2.vertices[mesh2.vertex_indices[i+2]];
            triangles2.emplace_back(Point(p1.x, p1.y, p1.z), Point(p2.x, p2.y, p2.z), Point(p3.x, p3.y, p3.z));
        }

        Logger::getInstance().log("Building AABB tree for mannequin...");
        Tree tree1(triangles1.begin(), triangles1.end());
        tree1.accelerate_distance_queries();

        intersecting_faces.clear();
        Logger::getInstance().log("Checking apparel faces for intersection...");
        for(size_t i = 0; i < triangles2.size(); ++i) {
            if(tree1.do_intersect(triangles2[i])) {
                intersecting_faces.push_back(i);
            }
        }

        Logger::getInstance().log("Found " + std::to_string(intersecting_faces.size()) + " intersecting faces on apparel.");
        return !intersecting_faces.empty();

    } catch (const std::exception& e) {
        Logger::getInstance().log("CGAL Exception in intersects: " + std::string(e.what()));
        return false;
    } catch (...) {
        Logger::getInstance().log("An unknown exception occurred during intersection check.");
        return false;
    }
}






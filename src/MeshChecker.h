#ifndef MESHCHECKER_H
#define MESHCHECKER_H

#include "Mesh.h"
#include <vector>
#include <set>

class MeshChecker
{
public:
    enum class CheckType {
        Watertight,
        NonManifold,
        SelfIntersect,
        Holes,
        DegenerateFaces,
        UVOverlap,
        UVBounds
    };

    struct CheckResult {
        bool is_watertight = false;
        int non_manifold_vertices_count;
        int self_intersections_count;
        int holes_count;
        int degenerate_faces_count;
        bool has_uvs;
        int overlapping_uv_islands_count;
        int uvs_out_of_bounds_count;

        // For visualization
        std::vector<unsigned int> intersecting_faces;
        std::vector<unsigned int> non_manifold_faces;
        std::vector<std::vector<unsigned int>> hole_loops;
        std::vector<unsigned int> overlapping_uv_faces;

        void clear() {
            is_watertight = false;
            non_manifold_vertices_count = 0;
            self_intersections_count = 0;
            holes_count = 0;
            degenerate_faces_count = 0;
            has_uvs = false;
            overlapping_uv_islands_count = 0;
            uvs_out_of_bounds_count = 0;
            intersecting_faces.clear();
            non_manifold_faces.clear();
            hole_loops.clear();
            overlapping_uv_faces.clear();
        }
    };

    static CheckResult check(const Mesh& mesh, const std::set<CheckType>& checksToPerform);
    static bool intersects(const Mesh& mesh1, const Mesh& mesh2);
};

#endif // MESHCHECKER_H

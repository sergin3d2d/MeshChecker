#include <iostream>
#include <string>
#include <vector>
#include "Mesh.h"
#include "ObjLoader.h"
#include "MeshChecker.h"

void printResult(const MeshChecker::CheckResult& result) {
    std::cout << "  Watertight: " << (result.is_watertight ? "Yes" : "No") << std::endl;
    std::cout << "  Non-manifold vertices: " << result.non_manifold_vertices_count << std::endl;
    std::cout << "  Self-intersections: " << result.self_intersections_count << std::endl;
    std::cout << "  Holes: " << result.holes_count << std::endl;
    std::cout << "  Degenerate faces: " << result.degenerate_faces_count << std::endl;
    std::cout << "  Has UVs: " << (result.has_uvs ? "Yes" : "No") << std::endl;
    if (result.has_uvs) {
        std::cout << "  Overlapping UVs: " << result.overlapping_uv_islands_count << std::endl;
        std::cout << "  UVs out of bounds: " << result.uvs_out_of_bounds_count << std::endl;
    }
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Usage: ApparelMeshChecker-cli --single <file.obj>" << std::endl;
        std::cerr << "       ApparelMeshChecker-cli --batch <folder_path> [--output <results.csv>]" << std::endl;
        std::cerr << "       ApparelMeshChecker-cli --intersect --mannequin <mannequin.obj> --apparel <apparel1.obj> ..." << std::endl;
        return 1;
    }

    std::string mode = argv[1];

    if (mode == "--single") {
        std::string filePath = argv[2];
        Mesh mesh;
        if (ObjLoader::load(filePath, mesh)) {
            std::cout << "Checking " << filePath << "..." << std::endl;
            std::set<MeshChecker::CheckType> allChecks = {
                MeshChecker::CheckType::Watertight,
                MeshChecker::CheckType::NonManifold,
                MeshChecker::CheckType::SelfIntersect,
                MeshChecker::CheckType::Holes,
                MeshChecker::CheckType::DegenerateFaces,
                MeshChecker::CheckType::UVOverlap,
                MeshChecker::CheckType::UVBounds
            };
            MeshChecker::CheckResult result = MeshChecker::check(mesh, allChecks);
            printResult(result);
        } else {
            std::cerr << "Error loading file: " << filePath << std::endl;
            return 1;
        }
    } else {
        std::cerr << "Mode not yet implemented." << std::endl;
        return 1;
    }

    return 0;
}

#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
#include <fstream>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include "Mesh.h"
#include "ObjLoader.h"
#include "MeshChecker.h"

namespace fs = std::filesystem;


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
        std::cerr << "       ApparelMeshChecker-cli --batch <folder_path> [--output <results.csv>] [--threads <N|auto>]" << std::endl;
        std::cerr << "       ApparelMeshChecker-cli --intersect --mannequin <mannequin.obj> --apparel <apparel1.obj> ..." << std::endl;
        return 1;
    }

    std::string mode = argv[1];

    if (mode == "--single") {
        std::string filePath = argv[2];
        Mesh mesh;
        if (ObjLoader::load_indexed(filePath, mesh)) {
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
    } else if (mode == "--batch") {
        std::string folderPath = argv[2];
        std::string outputPath = "results.csv";
        unsigned int num_threads = std::thread::hardware_concurrency();

        for (int i = 3; i < argc; ++i) {
            std::string arg = argv[i];
            if (arg == "--output" && i + 1 < argc) {
                outputPath = argv[++i];
            } else if (arg == "--threads" && i + 1 < argc) {
                std::string val = argv[++i];
                if (val == "auto") {
                    num_threads = std::thread::hardware_concurrency();
                } else {
                    num_threads = std::stoi(val);
                }
            }
        }

        std::ofstream outputFile(outputPath);
        outputFile << "File,Watertight,NonManifoldVertices,SelfIntersections,Holes,DegenerateFaces,HasUVs,OverlappingUVs,UVsOutOfBounds\n";

        std::queue<std::string> fileQueue;
        for (const auto& entry : fs::directory_iterator(folderPath)) {
            if (entry.is_regular_file() && entry.path().extension() == ".obj") {
                fileQueue.push(entry.path().string());
            }
        }

        std::mutex queueMutex;
        std::mutex outputMutex;
        std::condition_variable cv;
        bool done = false;

        auto worker = [&]() {
            while (true) {
                std::string filePath;
                {
                    std::unique_lock<std::mutex> lock(queueMutex);
                    cv.wait(lock, [&] { return !fileQueue.empty() || done; });
                    if (fileQueue.empty() && done) {
                        return;
                    }
                    filePath = fileQueue.front();
                    fileQueue.pop();
                }

                Mesh mesh;
                if (ObjLoader::load_indexed(filePath, mesh)) {
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

                    std::lock_guard<std::mutex> lock(outputMutex);
                    outputFile << filePath << ","
                               << (result.is_watertight ? "Yes" : "No") << ","
                               << result.non_manifold_vertices_count << ","
                               << result.self_intersections_count << ","
                               << result.holes_count << ","
                               << result.degenerate_faces_count << ","
                               << (result.has_uvs ? "Yes" : "No") << ","
                               << result.overlapping_uv_islands_count << ","
                               << result.uvs_out_of_bounds_count << "\n";
                } else {
                    std::cerr << "Error loading file: " << filePath << std::endl;
                }
            }
        };

        std::vector<std::thread> threads;
        for (unsigned int i = 0; i < num_threads; ++i) {
            threads.emplace_back(worker);
        }

        {
            std::unique_lock<std::mutex> lock(queueMutex);
            done = true;
        }
        cv.notify_all();

        for (auto& thread : threads) {
            thread.join();
        }

    } else if (mode == "--intersect") {
        // ... intersection logic
    }
    else {
        std::cerr << "Mode not yet implemented." << std::endl;
        return 1;
    }

    return 0;
}

#include "UvChecker.h"
#include <vector>
#include <queue>
#include <set>
#include <algorithm>
#include <cmath>
#include <map>

// --- Configuration ---
const int GRID_RESOLUTION = 1024; // Trade-off between precision and memory/speed

// --- Helper Data Structures ---
struct Edge {
    unsigned int v1, v2;
    bool operator<(const Edge& other) const {
        return std::min(v1, v2) < std::min(other.v1, other.v2) ||
               (std::min(v1, v2) == std::min(other.v1, other.v2) &&
                std::max(v1, v2) < std::max(other.v1, other.v2));
    }
};

// --- Function Prototypes ---
void findUVIslands(const Mesh& mesh, std::vector<std::vector<unsigned int>>& islands, std::vector<int>& face_to_island_map);
bool is_inside(const glm::vec2& p1, const glm::vec2& p2, const glm::vec2& p3, const glm::vec2& test_p);

// --- Public Methods ---

bool UvChecker::hasUvs(const Mesh& mesh)
{
    return !mesh.uvs.empty() && !mesh.uv_indices.empty();
}

int UvChecker::countUvsOutOfBounds(const Mesh& mesh)
{
    int count = 0;
    for (const auto& uv : mesh.uvs) {
        if (uv.x < 0.0f || uv.x > 1.0f || uv.y < 0.0f || uv.y > 1.0f) {
            count++;
        }
    }
    return count;
}

int UvChecker::countOverlappingUvIslands(const Mesh& mesh, std::vector<unsigned int>& overlapping_faces)
{
    if (!hasUvs(mesh)) {
        return 0;
    }

    // 1. Find connected UV islands to distinguish between inter-island and intra-island overlaps
    std::vector<std::vector<unsigned int>> islands;
    std::vector<int> face_to_island_map(mesh.vertex_indices.size() / 3, -1);
    findUVIslands(mesh, islands, face_to_island_map);

    // 2. Rasterize each face and check for overlaps at the pixel level
    std::vector<int> grid(GRID_RESOLUTION * GRID_RESOLUTION, -1); // Stores face_idx, -1 is empty
    std::set<unsigned int> culprit_faces;

    int num_faces = mesh.vertex_indices.size() / 3;
    for (int face_idx = 0; face_idx < num_faces; ++face_idx) {
        glm::vec2 uv1 = mesh.uvs[mesh.uv_indices[face_idx * 3 + 0]];
        glm::vec2 uv2 = mesh.uvs[mesh.uv_indices[face_idx * 3 + 1]];
        glm::vec2 uv3 = mesh.uvs[mesh.uv_indices[face_idx * 3 + 2]];

        // Get bounding box of the UV triangle
        int min_x = std::max(0, (int)floor(std::min({uv1.x, uv2.x, uv3.x}) * GRID_RESOLUTION));
        int max_x = std::min(GRID_RESOLUTION - 1, (int)ceil(std::max({uv1.x, uv2.x, uv3.x}) * GRID_RESOLUTION));
        int min_y = std::max(0, (int)floor(std::min({uv1.y, uv2.y, uv3.y}) * GRID_RESOLUTION));
        int max_y = std::min(GRID_RESOLUTION - 1, (int)ceil(std::max({uv1.y, uv2.y, uv3.y}) * GRID_RESOLUTION));

        // Rasterize the triangle
        for (int y = min_y; y <= max_y; ++y) {
            for (int x = min_x; x <= max_x; ++x) {
                glm::vec2 test_p((float)x / GRID_RESOLUTION, (float)y / GRID_RESOLUTION);
                if (is_inside(uv1, uv2, uv3, test_p)) {
                    int grid_index = y * GRID_RESOLUTION + x;
                    int colliding_face_idx = grid[grid_index];

                    if (colliding_face_idx != -1 && colliding_face_idx != face_idx) {
                        // An overlap is detected.
                        // It's an overlap regardless of whether it's the same island or not.
                        culprit_faces.insert(face_idx);
                        culprit_faces.insert(colliding_face_idx);
                    }
                    grid[grid_index] = face_idx;
                }
            }
        }
    }

    // 3. Populate the output vector with the specific overlapping faces
    overlapping_faces.clear();
    overlapping_faces.assign(culprit_faces.begin(), culprit_faces.end());

    // The return value is the number of overlapping islands, which is a bit ambiguous now.
    // Let's return the number of culprit faces for a more accurate metric.
    return culprit_faces.size();
}


// --- Private Helper Implementations ---

void findUVIslands(const Mesh& mesh, std::vector<std::vector<unsigned int>>& islands, std::vector<int>& face_to_island_map) {
    int num_faces = mesh.vertex_indices.size() / 3;
    std::vector<bool> visited(num_faces, false);
    std::vector<std::vector<int>> adj(num_faces);

    // Build adjacency list based on shared UV vertices
    std::map<unsigned int, std::vector<unsigned int>> uv_to_face_map;
    for(int i = 0; i < num_faces; ++i) {
        for(int j = 0; j < 3; ++j) {
            uv_to_face_map[mesh.uv_indices[i*3 + j]].push_back(i);
        }
    }

    for(auto const& [uv_idx, faces] : uv_to_face_map) {
        for(size_t i = 0; i < faces.size(); ++i) {
            for(size_t j = i + 1; j < faces.size(); ++j) {
                adj[faces[i]].push_back(faces[j]);
                adj[faces[j]].push_back(faces[i]);
            }
        }
    }

    // Find connected components (islands) using BFS
    for (int i = 0; i < num_faces; ++i) {
        if (!visited[i]) {
            std::vector<unsigned int> current_island;
            std::queue<int> q;

            q.push(i);
            visited[i] = true;
            int island_id = islands.size();
            face_to_island_map[i] = island_id;

            while (!q.empty()) {
                int u = q.front();
                q.pop();
                current_island.push_back(u);

                for (int v : adj[u]) {
                    if (!visited[v]) {
                        visited[v] = true;
                        face_to_island_map[v] = island_id;
                        q.push(v);
                    }
                }
            }
            islands.push_back(current_island);
        }
    }
}

// Barycentric coordinate check
float sign(const glm::vec2& p1, const glm::vec2& p2, const glm::vec2& p3) {
    return (p1.x - p3.x) * (p2.y - p3.y) - (p2.x - p3.x) * (p1.y - p3.y);
}

bool is_inside(const glm::vec2& p1, const glm::vec2& p2, const glm::vec2& p3, const glm::vec2& test_p) {
    float d1, d2, d3;
    bool has_neg, has_pos;

    d1 = sign(test_p, p1, p2);
    d2 = sign(test_p, p2, p3);
    d3 = sign(test_p, p3, p1);

    has_neg = (d1 < 0) || (d2 < 0) || (d3 < 0);
    has_pos = (d1 > 0) || (d2 > 0) || (d3 > 0);

    return !(has_neg && has_pos);
}

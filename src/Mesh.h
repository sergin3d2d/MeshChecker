#ifndef MESH_H
#define MESH_H

#include <vector>
#include <glm/glm.hpp>
#include <limits>

struct BoundingBox {
    glm::vec3 min = glm::vec3(std::numeric_limits<float>::max());
    glm::vec3 max = glm::vec3(std::numeric_limits<float>::lowest());
};

struct Mesh
{
    std::vector<glm::vec3> vertices;
    std::vector<glm::vec2> uvs;
    std::vector<glm::vec3> colors;
    std::vector<glm::vec3> normals;

    std::vector<unsigned int> vertex_indices;
    std::vector<unsigned int> uv_indices;
    std::vector<unsigned int> normal_indices;

    BoundingBox getBoundingBox() const {
        BoundingBox box;
        if (vertices.empty()) {
            box.min = glm::vec3(0.0f);
            box.max = glm::vec3(0.0f);
            return box;
        }

        for (const auto& vertex : vertices) {
            box.min.x = std::min(box.min.x, vertex.x);
            box.min.y = std::min(box.min.y, vertex.y);
            box.min.z = std::min(box.min.z, vertex.z);
            box.max.x = std::max(box.max.x, vertex.x);
            box.max.y = std::max(box.max.y, vertex.y);
            box.max.z = std::max(box.max.z, vertex.z);
        }
        return box;
    }
};

#endif // MESH_H

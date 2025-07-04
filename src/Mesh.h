#ifndef MESH_H
#define MESH_H

#include <vector>
#include <glm/glm.hpp>

struct Mesh
{
    std::vector<glm::vec3> vertices;
    std::vector<glm::vec2> uvs;
    std::vector<glm::vec3> colors;
    std::vector<glm::vec3> normals;

    std::vector<unsigned int> vertex_indices;
    std::vector<unsigned int> uv_indices;
    std::vector<unsigned int> normal_indices;
};

#endif // MESH_H

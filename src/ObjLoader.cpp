#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"
#include "ObjLoader.h"
#include <iostream>
#include <cstring>

bool ObjLoader::load(const std::string& path, Mesh& mesh)
{
    // This function is deprecated and now calls the indexed loader.
    return load_indexed(path, mesh);
}

bool ObjLoader::load_indexed(const std::string& path, Mesh& mesh)
{
    mesh.vertices.clear();
    mesh.uvs.clear();
    mesh.normals.clear();
    mesh.vertex_indices.clear();
    mesh.uv_indices.clear();
    mesh.normal_indices.clear();

    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, path.c_str())) {
        std::cerr << "tinyobjloader: " << warn << err << std::endl;
        return false;
    }

    // Loop over shapes
    for (const auto& shape : shapes) {
        // Loop over faces
        for (size_t f = 0; f < shape.mesh.num_face_vertices.size(); f++) {
            int fv = shape.mesh.num_face_vertices[f];

            // Loop over vertices in the face
            for (size_t v = 0; v < fv; v++) {
                tinyobj::index_t idx = shape.mesh.indices[f * fv + v];

                // Vertex
                mesh.vertices.push_back(glm::vec3(
                    attrib.vertices[3 * idx.vertex_index + 0],
                    attrib.vertices[3 * idx.vertex_index + 1],
                    attrib.vertices[3 * idx.vertex_index + 2]
                ));

                // Normal
                if (idx.normal_index >= 0) {
                    mesh.normals.push_back(glm::vec3(
                        attrib.normals[3 * idx.normal_index + 0],
                        attrib.normals[3 * idx.normal_index + 1],
                        attrib.normals[3 * idx.normal_index + 2]
                    ));
                } else {
                    // If no normals are present, we'll need to compute them.
                    // For now, we'll just add a zero vector.
                    mesh.normals.push_back(glm::vec3(0.0f, 0.0f, 0.0f));
                }

                // UV
                if (idx.texcoord_index >= 0) {
                    mesh.uvs.push_back(glm::vec2(
                        attrib.texcoords[2 * idx.texcoord_index + 0],
                        attrib.texcoords[2 * idx.texcoord_index + 1]
                    ));
                } else {
                    mesh.uvs.push_back(glm::vec2(0.0f, 0.0f));
                }
            }
        }
    }

    // The mesh is now "flat" and de-indexed. The indices are just a simple sequence.
    mesh.vertex_indices.resize(mesh.vertices.size());
    for(size_t i = 0; i < mesh.vertices.size(); ++i) {
        mesh.vertex_indices[i] = i;
    }
    mesh.normal_indices = mesh.vertex_indices;
    mesh.uv_indices = mesh.vertex_indices;

    // If no normals were loaded, compute flat normals
    if (attrib.normals.empty()) {
        for (size_t i = 0; i < mesh.vertices.size(); i += 3) {
            glm::vec3 v1 = mesh.vertices[i + 1] - mesh.vertices[i];
            glm::vec3 v2 = mesh.vertices[i + 2] - mesh.vertices[i];
            glm::vec3 normal = glm::normalize(glm::cross(v1, v2));
            mesh.normals[i] = normal;
            mesh.normals[i + 1] = normal;
            mesh.normals[i + 2] = normal;
        }
    }

    return true;
}

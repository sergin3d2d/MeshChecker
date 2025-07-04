#include "ObjLoader.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>

namespace {
    struct FaceVertex {
        unsigned int v_idx = 0;
        unsigned int vt_idx = 0;
        unsigned int vn_idx = 0;
    };

    void parse_face_vertex(const std::string& face_data, FaceVertex& fv) {
        std::stringstream face_ss(face_data);
        char slash;

        face_ss >> fv.v_idx;
        if (face_ss.peek() == '/') {
            face_ss >> slash;
            if (face_ss.peek() != '/') {
                face_ss >> fv.vt_idx;
            }
            if (face_ss.peek() == '/') {
                face_ss >> slash >> fv.vn_idx;
            }
        }
    }
}

bool ObjLoader::load(const std::string& path, Mesh& mesh)
{
    mesh.vertices.clear();
    mesh.uvs.clear();
    mesh.normals.clear();
    mesh.vertex_indices.clear();
    mesh.uv_indices.clear();
    mesh.normal_indices.clear();

    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file " << path << std::endl;
        return false;
    }

    // Use temporary vectors to store the raw data from the OBJ file
    std::vector<glm::vec3> temp_vertices;
    std::vector<glm::vec2> temp_uvs;
    std::vector<glm::vec3> temp_normals;
    std::vector<FaceVertex> faces;

    std::string line;
    while (std::getline(file, line)) {
        std::stringstream ss(line);
        std::string type;
        ss >> type;

        if (type == "v") {
            glm::vec3 vertex;
            ss >> vertex.x >> vertex.y >> vertex.z;
            temp_vertices.push_back(vertex);
        } else if (type == "vt") {
            glm::vec2 uv;
            ss >> uv.x >> uv.y;
            temp_uvs.push_back(uv);
        } else if (type == "vn") {
            glm::vec3 normal;
            ss >> normal.x >> normal.y >> normal.z;
            temp_normals.push_back(normal);
        } else if (type == "f") {
            std::string face_data;
            while(ss >> face_data) {
                FaceVertex fv;
                parse_face_vertex(face_data, fv);
                faces.push_back(fv);
            }
            // Add a marker for the end of the face
            faces.push_back({0,0,0}); 
        }
    }

    // --- Post-processing: De-index for faceted normals and VBOs ---
    for (size_t i = 0; i < faces.size(); ) {
        std::vector<FaceVertex> face;
        while(i < faces.size() && faces[i].v_idx != 0) {
            face.push_back(faces[i]);
            i++;
        }
        i++; // Skip the end-of-face marker

        if(face.size() >= 3) {
            // Triangulate the polygon
            for(size_t j = 1; j < face.size() - 1; ++j) {
                FaceVertex fv[3] = {face[0], face[j], face[j+1]};

                glm::vec3 v[3];
                for(int k=0; k<3; ++k) {
                    v[k] = temp_vertices[fv[k].v_idx - 1];
                }

                glm::vec3 face_normal = glm::normalize(glm::cross(v[1] - v[0], v[2] - v[0]));

                for(int k=0; k<3; ++k) {
                    mesh.vertices.push_back(v[k]);
                    mesh.normals.push_back(face_normal);
                    if(!temp_uvs.empty() && fv[k].vt_idx > 0) {
                        mesh.uvs.push_back(temp_uvs[fv[k].vt_idx - 1]);
                    } else {
                        mesh.uvs.push_back(glm::vec2(0,0)); // Default UV
                    }
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

    return true;
}
#ifndef OBJLOADER_H
#define OBJLOADER_H

#include "Mesh.h"
#include <string>

class ObjLoader
{
public:
    static bool load(const std::string& path, Mesh& mesh);
    static bool load_indexed(const std::string& path, Mesh& mesh);
};

#endif // OBJLOADER_H

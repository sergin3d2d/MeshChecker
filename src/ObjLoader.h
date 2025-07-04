#ifndef OBJLOADER_H
#define OBJLOADER_H

#include "Mesh.h"
#include <string>

class ObjLoader
{
public:
    static bool load(const std::string& path, Mesh& mesh);
};

#endif // OBJLOADER_H

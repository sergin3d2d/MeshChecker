#ifndef UVCHECKER_H
#define UVCHECKER_H

#include "Mesh.h"
#include <vector>

class UvChecker
{
public:
    static bool hasUvs(const Mesh& mesh);
    static int countOverlappingUvIslands(const Mesh& mesh, std::vector<unsigned int>& overlapping_faces);
    static int countUvsOutOfBounds(const Mesh& mesh);
};

#endif // UVCHECKER_H

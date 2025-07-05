#ifndef INTERSECTIONRESULT_H
#define INTERSECTIONRESULT_H

#include <vector>

struct IntersectionResult {
    bool intersects;
    std::vector<int> intersecting_faces;
};

#endif // INTERSECTIONRESULT_H
# Apparel Mesh Checker

A Windows desktop application for quality assurance of 3D apparel scans. This tool automatically checks 3D meshes for common geometry and UV problems, helping to ensure asset correctness before production.

## Features

- **Single Mesh Check:** Analyze a single .OBJ file for issues like watertightness, non-manifold geometry, self-intersections, and UV errors.
- **Batch Check:** Process an entire folder of .OBJ files and summarize the results in a table.
- **Intersection Check:** Test apparel meshes for intersections against a base mannequin mesh.
- **3D Preview:** Visualize errors directly on the 3D model in an interactive viewer.

## Tech Stack

- **Language:** C++
- **Framework:** Qt 6 for the GUI and 3D rendering.
- **Geometry Processing:** CGAL (Computational Geometry Algorithms Library)
- **3D Math:** GLM (OpenGL Mathematics)
- **Build System:** CMake

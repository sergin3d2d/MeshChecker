#!/bin/bash
# Create a build directory if it doesn't exist
mkdir -p build
# Navigate into the build directory
cd build
# Configure the project with CMake
cmake .. -DCMAKE_BUILD_TYPE=Release
# Build the project
make

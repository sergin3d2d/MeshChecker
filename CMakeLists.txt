cmake_minimum_required(VERSION 3.16)
project(ApparelMeshChecker)

# --- Standard Settings ---
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(MSVC)
    add_compile_options("/execution_character_set:utf-8")
    add_compile_options("/wd4251") 
    add_compile_options("/wd4275") 
endif()

# --- Find Dependencies ---
find_package(Qt6 REQUIRED COMPONENTS Widgets OpenGL OpenGLWidgets Concurrent)
find_package(OpenGL REQUIRED)
find_package(CGAL REQUIRED)
find_package(glm REQUIRED)
find_package(GLU REQUIRED)
find_package(Threads REQUIRED)

# --- Project Sources ---
add_executable(ApparelMeshChecker WIN32
    src/main.cpp
    src/MainWindow.h
    src/MainWindow.cpp
    src/ViewerWidget.h
    src/ViewerWidget.cpp
    src/Mesh.h
    src/ObjLoader.h
    src/ObjLoader.cpp
    src/MeshChecker.h
    src/MeshChecker.cpp
    src/UvChecker.h
    src/UvChecker.cpp
    src/Logger.h
    src/Logger.cpp
)

add_executable(ApparelMeshChecker-cli
    src/main_cli.cpp
    src/Mesh.h
    src/ObjLoader.h
    src/ObjLoader.cpp
    src/MeshChecker.h
    src/MeshChecker.cpp
    src/UvChecker.h
    src/UvChecker.cpp
    src/Logger.h
    src/Logger.cpp
)

# --- Link Dependencies ---
target_link_libraries(ApparelMeshChecker
    PRIVATE
    Qt6::Widgets
    Qt6::OpenGL
    Qt6::OpenGLWidgets
    Qt6::Concurrent
    ${OPENGL_LIBRARIES}
    ${GLU_LIBRARIES}
    CGAL::CGAL
    glm::glm
    Threads::Threads
)

# --- Include Directories ---
target_include_directories(ApparelMeshChecker
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CGAL_INCLUDE_DIRS}
    ${glm_INCLUDE_DIRS}
    ${GLU_INCLUDE_DIRS}
)

target_link_libraries(ApparelMeshChecker-cli
    PRIVATE
    Qt6::Core
    CGAL::CGAL
    glm::glm
    Threads::Threads
)

# --- CPack Configuration ---
include(InstallRequiredSystemLibraries)
set(CPACK_GENERATOR "ZIP")
set(CPACK_PACKAGE_NAME ${PROJECT_NAME})
set(CPACK_PACKAGE_VERSION "1.0.0")
set(CPACK_PACKAGE_CONTACT "user@example.com")
set(CPACK_PACKAGE_DESCRIPTION "Apparel Mesh Checker")
set(CPACK_PACKAGE_VENDOR "Gemini")

install(TARGETS ${PROJECT_NAME} DESTINATION bin)
install(FILES
    ${QT_LIBRARIES}
    DESTINATION bin
    COMPONENT Runtime
)

include(CPack)

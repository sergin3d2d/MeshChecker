cmake_minimum_required(VERSION 3.16)
project(ApparelMeshChecker)

# --- Standard Settings ---
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# --- Find Dependencies ---
find_package(Qt6 REQUIRED COMPONENTS Widgets OpenGL OpenGLWidgets)
find_package(OpenGL REQUIRED)
find_package(CGAL REQUIRED)
find_package(glm REQUIRED)
find_package(GLU REQUIRED)

# --- Project Sources ---
if(WIN32)
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
else()
    add_executable(ApparelMeshChecker
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
endif()

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
    ${OPENGL_LIBRARIES}
    ${GLU_LIBRARIES}
    CGAL::CGAL
    glm::glm
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

#include "ViewerWidget.h"
#include <QOpenGLContext>
#include <QMouseEvent>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <limits>
#include <set>
#include <QOpenGLFunctions>
#include <algorithm>

ViewerWidget::ViewerWidget(QWidget *parent)
    : QOpenGLWidget(parent)
{
}

ViewerWidget::~ViewerWidget()
{
    clearMeshes();
}

void ViewerWidget::setMeshes(const std::vector<const Mesh*>& newMeshes, const MeshChecker::CheckResult* result, const std::vector<IntersectionResult>* intResult)
{
    meshes = newMeshes;
    checkResult = result;
    intersectionResults = intResult;
    update(); // Trigger a repaint, which will call paintGL() on the right thread
}

void ViewerWidget::clearMeshes()
{
    meshes.clear();
    checkResult = nullptr;
    intersectionResults = nullptr;
    update();
}

void ViewerWidget::focusOnMesh()
{
    if (meshes.empty()) {
        return;
    }

    glm::vec3 min_v(std::numeric_limits<float>::max());
    glm::vec3 max_v(std::numeric_limits<float>::lowest());

    for (const auto& mesh : meshes) {
        for (const auto& v : mesh->vertices) {
            min_v.x = std::min(min_v.x, v.x);
            min_v.y = std::min(min_v.y, v.y);
            min_v.z = std::min(min_v.z, v.z);
            max_v.x = std::max(max_v.x, v.x);
            max_v.y = std::max(max_v.y, v.y);
            max_v.z = std::max(max_v.z, v.z);
        }
    }

    modelCenter = (min_v + max_v) / 2.0f;
    modelRadius = glm::distance(min_v, max_v) / 2.0f;

    fov = 45.0f;
    rotationX = 0.0f;
    rotationY = 0.0f;

    update();
    updateCameraStatus();
}



void ViewerWidget::initializeGL()
{
    initializeOpenGLFunctions();
    glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_COLOR_MATERIAL);

    // Enable two-sided lighting
    glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);

    // Set a fixed light position in world space
    GLfloat light_position[] = { 1.0, 1.0, 1.0, 0.0 }; // Directional light
    glLightfv(GL_LIGHT0, GL_POSITION, light_position);

    // Set light properties for a brighter, more ambient feel
    GLfloat ambient[] = { 0.4f, 0.4f, 0.4f, 1.0f };
    GLfloat diffuse[] = { 0.8f, 0.8f, 0.8f, 1.0f };
    GLfloat specular[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, specular);

    // Set global ambient light
    GLfloat global_ambient[] = { 0.3f, 0.3f, 0.3f, 1.0f };
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, global_ambient);
}

void ViewerWidget::resizeGL(int w, int h)
{
    glViewport(0, 0, w, h);
}

void ViewerWidget::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // If meshes are empty, there's nothing to do.
    if (meshes.empty()) {
        // Clean up old buffers if they exist
        if (!vbo_vertices_list.empty()) {
            glDeleteBuffers(vbo_vertices_list.size(), vbo_vertices_list.data());
            glDeleteBuffers(vbo_normals_list.size(), vbo_normals_list.data());
            glDeleteBuffers(vbo_colors_list.size(), vbo_colors_list.data());
            glDeleteBuffers(ibo_indices_list.size(), ibo_indices_list.data());
            vbo_vertices_list.clear();
            vbo_normals_list.clear();
            vbo_colors_list.clear();
            ibo_indices_list.clear();
        }
        return;
    }

    // If the number of meshes has changed, we need to re-generate buffers
    if (vbo_vertices_list.size() != meshes.size()) {
        // Clean up old buffers
        if (!vbo_vertices_list.empty()) {
            glDeleteBuffers(vbo_vertices_list.size(), vbo_vertices_list.data());
            glDeleteBuffers(vbo_normals_list.size(), vbo_normals_list.data());
            glDeleteBuffers(vbo_colors_list.size(), vbo_colors_list.data());
            glDeleteBuffers(ibo_indices_list.size(), ibo_indices_list.data());
        }

        // Allocate new buffer IDs
        vbo_vertices_list.resize(meshes.size(), 0);
        vbo_normals_list.resize(meshes.size(), 0);
        vbo_colors_list.resize(meshes.size(), 0);
        ibo_indices_list.resize(meshes.size(), 0);
        glGenBuffers(meshes.size(), vbo_vertices_list.data());
        glGenBuffers(meshes.size(), vbo_normals_list.data());
        glGenBuffers(meshes.size(), vbo_colors_list.data());
        glGenBuffers(meshes.size(), ibo_indices_list.data());
    }

    // --- Upload data to buffers ---
    for (size_t i = 0; i < meshes.size(); ++i) {
        const auto& mesh = meshes[i];
        if (!mesh) continue;

        // Vertex Buffer
        glBindBuffer(GL_ARRAY_BUFFER, vbo_vertices_list[i]);
        glBufferData(GL_ARRAY_BUFFER, mesh->vertices.size() * sizeof(glm::vec3), mesh->vertices.data(), GL_DYNAMIC_DRAW);

        // Normal Buffer
        if (!mesh->normals.empty()) {
            glBindBuffer(GL_ARRAY_BUFFER, vbo_normals_list[i]);
            glBufferData(GL_ARRAY_BUFFER, mesh->normals.size() * sizeof(glm::vec3), mesh->normals.data(), GL_DYNAMIC_DRAW);
        }

        // Color Buffer
        if (!mesh->colors.empty()) {
            glBindBuffer(GL_ARRAY_BUFFER, vbo_colors_list[i]);
            glBufferData(GL_ARRAY_BUFFER, mesh->colors.size() * sizeof(glm::vec3), mesh->colors.data(), GL_DYNAMIC_DRAW);
        }

        // Index Buffer
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_indices_list[i]);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh->vertex_indices.size() * sizeof(unsigned int), mesh->vertex_indices.data(), GL_DYNAMIC_DRAW);
    }


    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glm::mat4 projection = glm::perspective(glm::radians(fov), (float)width() / (float)height(), 0.1f, 100.0f * modelRadius);
    glLoadMatrixf(glm::value_ptr(projection));

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // Set up the view transformation (camera)
    glTranslatef(0.0f, 0.0f, -modelRadius * 2.5f);
    glRotatef(rotationX, 1.0f, 0.0f, 0.0f);
    glRotatef(rotationY, 0.0f, 1.0f, 0.0f);

    // Apply the model transformation
    glTranslatef(-modelCenter.x, -modelCenter.y, -modelCenter.z);

    for (size_t i = 0; i < meshes.size(); ++i) {
        const auto& mesh = meshes[i];
        if (!mesh || i >= vbo_vertices_list.size() || vbo_vertices_list[i] == 0 || i >= ibo_indices_list.size() || ibo_indices_list[i] == 0) {
            continue;
        }

        // --- Draw the main mesh using VBOs ---
        glEnable(GL_LIGHTING);
        
        glEnableClientState(GL_VERTEX_ARRAY);
        glBindBuffer(GL_ARRAY_BUFFER, vbo_vertices_list[i]);
        glVertexPointer(3, GL_FLOAT, 0, nullptr);

        if (i < vbo_normals_list.size() && vbo_normals_list[i] && !mesh->normals.empty()) {
            glEnableClientState(GL_NORMAL_ARRAY);
            glBindBuffer(GL_ARRAY_BUFFER, vbo_normals_list[i]);
            glNormalPointer(GL_FLOAT, 0, nullptr);
        }

        if (i < vbo_colors_list.size() && vbo_colors_list[i] && !mesh->colors.empty()) {
            glEnableClientState(GL_COLOR_ARRAY);
            glBindBuffer(GL_ARRAY_BUFFER, vbo_colors_list[i]);
            glColorPointer(3, GL_FLOAT, 0, nullptr);
        } else {
            glColor3f(0.7f, 0.7f, 0.7f); // Default color if no color buffer
        }

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_indices_list[i]);
        glDrawElements(GL_TRIANGLES, mesh->vertex_indices.size(), GL_UNSIGNED_INT, nullptr);
        glDisableClientState(GL_VERTEX_ARRAY);
        glDisableClientState(GL_NORMAL_ARRAY);
        glDisableClientState(GL_COLOR_ARRAY);
    }

    // --- Draw hole edges if they exist ---
    if (checkResult && !checkResult->hole_loops.empty()) {
        glDisable(GL_LIGHTING);
        glLineWidth(3.0f);
        glColor3f(0.0f, 1.0f, 1.0f); // Bright Cyan
        
        for (const auto& loop : checkResult->hole_loops) {
            glBegin(GL_LINE_LOOP);
            for (const auto& vertex_idx : loop) {
                const auto& v = meshes[0]->vertices[vertex_idx];
                glVertex3f(v.x, v.y, v.z);
            }
            glEnd();
        }
        
        glLineWidth(1.0f);
        glEnable(GL_LIGHTING);
    }
}

void ViewerWidget::mousePressEvent(QMouseEvent *event)
{
    lastMousePos = event->pos();
}

void ViewerWidget::mouseMoveEvent(QMouseEvent *event)
{
    int dx = event->pos().x() - lastMousePos.x();
    int dy = event->pos().y() - lastMousePos.y();

    if (event->buttons() & Qt::LeftButton) {
        rotationX += dy;
        rotationY += dx;
        update();
        updateCameraStatus();
    }
    lastMousePos = event->pos();
}

void ViewerWidget::wheelEvent(QWheelEvent *event)
{
    fov -= event->angleDelta().y() / 120.0f;
    fov = std::clamp(fov, 1.0f, 90.0f); // Clamp FOV to reasonable values
    update();
    updateCameraStatus();
}

void ViewerWidget::updateCameraStatus()
{
    QString status = QString("FOV: %1, Rot: %2, %3").arg(fov, 0, 'f', 1).arg(rotationX).arg(rotationY);
    emit cameraChanged(status);
}

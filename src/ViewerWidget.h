#ifndef VIEWERWIDGET_H
#define VIEWERWIDGET_H

#include <QtOpenGLWidgets/QOpenGLWidget>
#include <QOpenGLFunctions>
#include <glm/glm.hpp>
#include "Mesh.h"
#include "MeshChecker.h"

class ViewerWidget : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT

public:
    ViewerWidget(QWidget *parent = nullptr);
    ~ViewerWidget();

    void setMesh(const Mesh* mesh, const MeshChecker::CheckResult* result = nullptr);
    void clearMesh();
    void focusOnMesh();

signals:
    void cameraChanged(const QString& status);

protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

private:
    void setupBuffers();
    void updateCameraStatus();

    const Mesh* currentMesh = nullptr;
    const MeshChecker::CheckResult* checkResult = nullptr;

    // VBO IDs
    GLuint vbo_vertices = 0;
    GLuint vbo_normals = 0;
    GLuint vbo_colors = 0;
    GLuint ibo_indices = 0;

    glm::vec3 modelCenter = glm::vec3(0.0f);
    float modelRadius = 1.0f;

    float rotationX = 0.0f;
    float rotationY = 0.0f;
    float fov = 45.0f;
    QPoint lastMousePos;
};

#endif // VIEWERWIDGET_H

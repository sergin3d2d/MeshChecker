#ifndef VIEWERWIDGET_H
#define VIEWERWIDGET_H

#include <QtOpenGLWidgets/QOpenGLWidget>
#include <QOpenGLFunctions>
#include <glm/glm.hpp>
#include "Mesh.h"
#include "MeshChecker.h"
#include "IntersectionResult.h"


class ViewerWidget : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT

public:
    ViewerWidget(QWidget *parent = nullptr);
    ~ViewerWidget();

    void setMeshes(const std::vector<const Mesh*>& meshes, const MeshChecker::CheckResult* result, const std::vector<IntersectionResult>* intResult);
    void clearMeshes();
    void focusOnMesh();

    std::vector<glm::vec3> highlight_vertices;
    float highlight_radius = 0.01f;

public slots:
    void setHighlightRadius(float radius);

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
    void drawSphere(const glm::vec3& center, float radius);

    std::vector<const Mesh*> meshes;
    const std::vector<IntersectionResult>* intersectionResults = nullptr;
    const MeshChecker::CheckResult* checkResult = nullptr;

    // VBO IDs
    std::vector<GLuint> vbo_vertices_list;
    std::vector<GLuint> vbo_normals_list;
    std::vector<GLuint> vbo_colors_list;
    std::vector<GLuint> ibo_indices_list;

    glm::vec3 modelCenter = glm::vec3(0.0f);
    float modelRadius = 1.0f;

    float rotationX = 0.0f;
    float rotationY = 0.0f;
    float fov = 45.0f;
    QPoint lastMousePos;
};

#endif // VIEWERWIDGET_H

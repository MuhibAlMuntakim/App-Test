#pragma once

#include <QOpenGLWidget>
#include <QOpenGLFunctions_3_3_Core>
#include <QGestureEvent>
#include <QTouchEvent>
#include <QRotationGesture>
#include <QOpenGLShaderProgram>
#include <QMatrix4x4>
#include <QVector3D>
#include <QVector2D>
#include <QElapsedTimer>
#include <QImage>
#include <vector>
#include <memory>

#include "MazeController.h"
#include "MazeTypes.h"
#include "MazeSolver.h"

class MazeGLWidget : public QOpenGLWidget, protected QOpenGLFunctions_3_3_Core {
    Q_OBJECT
public:
    explicit MazeGLWidget(QWidget* parent = nullptr);
    ~MazeGLWidget() override;

public slots:
    void regenerateMaze(const MazeConfig& cfg);
    void startPreview(const MazeConfig& cfg);
    void stepPreview();
    void runSolver();
    void clearPath();
    void setStartGoal(Coord s, Coord g);
    void loadGridAndConfig(const MazeGrid& grid, const MazeConfig& cfg);

    const MazeGrid& currentGrid() const { return controller_->grid(); }
    const MazeConfig& currentConfig() const { return cfg_; }
    QImage snapshot() { return grabFramebuffer(); }

    // Theme toggle
    void setThemeTextured(bool enabled);

signals:
    void fpsUpdated(float fps);
    void previewProgress(float p);
    void mazeConfigLoaded(const MazeConfig& cfg);

protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;

    void mousePressEvent(QMouseEvent* e) override;
    void mouseMoveEvent(QMouseEvent* e) override;
    void wheelEvent(QWheelEvent* e) override;
    void keyPressEvent(QKeyEvent* e) override;
    bool event(QEvent* e) override; // gestures/touch routing
    void touchEvent(QTouchEvent* e);

public:
    struct Sensitivity {
        float orbitRotate = 0.3f;
        float orbitPan = 0.1f;
        float orbitZoom = 1.0f;
        float fpRotate = 0.2f;
        float fpMove = 0.5f;
        float pinchFactor = 1.0f;
    };
    void setSensitivity(const Sensitivity& s) { sens_ = s; }
    void setCollisionRadius(float r) { collisionRadius_ = r; }

private:
    bool gestureEvent(QGestureEvent* e);
    void pinchTriggered(QPinchGesture* gesture);
    void panTriggered(QPanGesture* gesture);
    void rotationTriggered(QRotationGesture* gesture);

    enum class CameraMode { Orbit, FirstPerson };
    enum class ThemeMode { Flat, Textured };
    CameraMode cameraMode_ = CameraMode::Orbit;
    ThemeMode themeMode_ = ThemeMode::Flat;

    // Orbit camera
    float yaw_ = 30.0f;
    float pitch_ = 20.0f;
    float distance_ = 30.0f;

    // First-person
    QVector3D fpPos_{0.5f, 0.0f, 0.5f};
    float fpYaw_ = 0.0f;

    // Collision
    float collisionRadius_ = 0.2f; // player radius in maze units

    QPoint lastPos_;

    // GL resources
    QOpenGLShaderProgram program_;
    GLuint vao_ = 0;
    GLuint vbo_ = 0;
    GLuint wallTex_ = 0;
    GLuint floorTex_ = 0;
    struct Vertex { QVector3D pos; QVector3D normal; QVector3D color; QVector2D uv; };
    std::vector<Vertex> vertices_;

    Sensitivity sens_{};
    QElapsedTimer frameTimer_;
    int frameCount_ = 0;

    // Maze and solver
    std::unique_ptr<MazeController> controller_;
    MazeConfig cfg_;
    std::vector<Coord> path_;
    Coord start_{0,0};
    Coord goal_{0,0};

    // Geometry builders
    void buildMaze();
    void buildPathGeometry();
    void addQuad(const QVector3D& a, const QVector3D& b, const QVector3D& c, const QVector3D& d, const QVector3D& normal, const QVector3D& color);
    void uploadGeometry();
    void createProceduralTextures();

    QMatrix4x4 computeViewMatrix() const;
    QMatrix4x4 computeProjMatrix() const;

    // FP movement helpers
    void moveForward(float delta);
    void strafeRight(float delta);
    bool canMoveTo(const QVector3D& next) const;
    bool resolveCollision(QVector3D& pos, const QVector3D& desiredDelta) const;
};

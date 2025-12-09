#include "MazeGLWidget.h"
#include <QMouseEvent>
#include <QWheelEvent>
#include <QKeyEvent>
#include <cmath>

static const char* kVertexShader = R"GLSL(
#version 330 core
layout(location=0) in vec3 aPos;
layout(location=1) in vec3 aNormal;
layout(location=2) in vec3 aColor;
layout(location=3) in vec2 aUV;

uniform mat4 uProj;
uniform mat4 uView;
uniform mat4 uModel;

out vec3 vColor;
out vec3 vNormal;
out vec3 vFragPos;
out vec2 vUV;

void main(){
    vec4 worldPos = uModel * vec4(aPos, 1.0);
    gl_Position = uProj * uView * worldPos;
    vFragPos = worldPos.xyz;
    vNormal = mat3(uModel) * aNormal;
    vColor = aColor;
    vUV = aUV;
}
)GLSL";

static const char* kFragmentShader = R"GLSL(
#version 330 core
in vec3 vColor;
in vec3 vNormal;
in vec3 vFragPos;
in vec2 vUV;

out vec4 FragColor;

uniform vec3 uLightDir = normalize(vec3(-0.5, -1.0, -0.3));
uniform vec3 uAmbient = vec3(0.15);
uniform vec3 uViewPos;
uniform int uTheme; // 0 = flat, 1 = textured

uniform sampler2D uWallTex;
uniform sampler2D uFloorTex;

void main(){
    vec3 n = normalize(vNormal);
    float diff = max(dot(n, -uLightDir), 0.0);
    // Specular
    vec3 viewDir = normalize(uViewPos - vFragPos);
    vec3 reflectDir = reflect(uLightDir, n);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 16.0);

    vec3 baseColor = vColor;
    if (uTheme == 1) {
        // Choose texture based on base color hint: floor uses darker base
        bool isFloor = (vColor.r < 0.2 && vColor.g < 0.25);
        vec3 wallTint = vec3(0.9, 0.9, 0.9);
        vec3 floorTint = vec3(0.8, 0.8, 0.8);
        vec3 texColor = isFloor ? texture(uFloorTex, vUV).rgb * floorTint : texture(uWallTex, vUV).rgb * wallTint;
        baseColor = mix(baseColor, texColor, 0.85);
    }
    vec3 color = baseColor * (uAmbient + diff * 0.85) + vec3(0.2) * spec;
    // Gamma correction
    color = pow(color, vec3(1.0/2.2));
    FragColor = vec4(color, 1.0);
}
)GLSL";

MazeGLWidget::MazeGLWidget(QWidget* parent)
    : QOpenGLWidget(parent) {
    setFocusPolicy(Qt::StrongFocus);
    setAttribute(Qt::WA_AcceptTouchEvents, true);
    grabGesture(Qt::PinchGesture);
    grabGesture(Qt::PanGesture);
    grabGesture(Qt::RotationGesture);
}

MazeGLWidget::~MazeGLWidget() {
    makeCurrent();
    if (vbo_) glDeleteBuffers(1, &vbo_);
    if (vao_) glDeleteVertexArrays(1, &vao_);
    if (wallTex_) glDeleteTextures(1, &wallTex_);
    if (floorTex_) glDeleteTextures(1, &floorTex_);
    doneCurrent();
}

void MazeGLWidget::initializeGL() {
    initializeOpenGLFunctions();
    sens_ = Sensitivity();

    glClearColor(0.08f, 0.1f, 0.12f, 1.0f);
    glEnable(GL_DEPTH_TEST);

    program_.addShaderFromSourceCode(QOpenGLShader::Vertex, kVertexShader);
    program_.addShaderFromSourceCode(QOpenGLShader::Fragment, kFragmentShader);
    program_.link();

    createProceduralTextures();

    #if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
    cfg_.width = 15; cfg_.height = 15;
#else
    cfg_.width = 20; cfg_.height = 20;
#endif
    cfg_.seed = 0; cfg_.algorithm = MazeAlgorithm::RecursiveBacktracking;
    controller_ = std::make_unique<MazeController>(cfg_.width, cfg_.height);
    controller_->regenerate(cfg_);

    buildMaze();
    uploadGeometry();
}

void MazeGLWidget::resizeGL(int w, int h) {
    glViewport(0, 0, w, h);
}

void MazeGLWidget::paintGL() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    program_.bind();
    if (!frameTimer_.isValid()) frameTimer_.start();
    frameCount_++;
    if (frameTimer_.elapsed() >= 1000) {
        float fps = frameCount_ * 1000.0f / float(frameTimer_.elapsed());
        emit fpsUpdated(fps);
        frameTimer_.restart();
        frameCount_ = 0;
    }

    QMatrix4x4 proj = computeProjMatrix();
    QMatrix4x4 view = computeViewMatrix();
    QMatrix4x4 model; // identity

    program_.setUniformValue("uProj", proj);
    program_.setUniformValue("uView", view);
    program_.setUniformValue("uModel", model);

    // Extract view position from inverted view matrix
    QMatrix4x4 invView = view.inverted();
    QVector3D viewPos = invView.column(3).toVector3D();
    program_.setUniformValue("uViewPos", viewPos);
    program_.setUniformValue("uTheme", themeMode_==ThemeMode::Textured ? 1 : 0);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, wallTex_);
    program_.setUniformValue("uWallTex", 0);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, floorTex_);
    program_.setUniformValue("uFloorTex", 1);

    glBindVertexArray(vao_);
    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLint>(vertices_.size()));
    glBindVertexArray(0);

    program_.release();
}

void MazeGLWidget::mousePressEvent(QMouseEvent* e) {
    lastPos_ = e->pos();
}

void MazeGLWidget::mouseMoveEvent(QMouseEvent* e) {
    QPoint delta = e->pos() - lastPos_;
    lastPos_ = e->pos();
    if (cameraMode_ == CameraMode::Orbit) {
        yaw_ += delta.x() * sens_.orbitRotate;
        pitch_ += delta.y() * sens_.orbitRotate;
        if (pitch_ > 89.0f) pitch_ = 89.0f;
        if (pitch_ < -10.0f) pitch_ = -10.0f;
    } else {
        fpYaw_ += delta.x() * sens_.fpRotate;
    }
    update();
}

void MazeGLWidget::wheelEvent(QWheelEvent* e) {
    if (cameraMode_ == CameraMode::Orbit) {
        distance_ -= (e->angleDelta().y() / 120.0f) * sens_.orbitZoom;
        if (distance_ < 5.0f) distance_ = 5.0f;
        if (distance_ > 200.0f) distance_ = 200.0f;
    } else {
        float delta = e->angleDelta().y() / 120.0f;
        moveForward(delta * (sens_.fpMove/2.0f));
    }
    update();
}

void MazeGLWidget::keyPressEvent(QKeyEvent* e) {
    if (e->key() == Qt::Key_C) {
        cameraMode_ = (cameraMode_ == CameraMode::Orbit) ? CameraMode::FirstPerson : CameraMode::Orbit;
        update();
        return;
    }
    if (cameraMode_ == CameraMode::FirstPerson) {
        switch (e->key()) {
            case Qt::Key_W: moveForward(sens_.fpMove); break;
            case Qt::Key_S: moveForward(-sens_.fpMove); break;
            case Qt::Key_A: strafeRight(-sens_.fpMove); break;
            case Qt::Key_D: strafeRight(sens_.fpMove); break;
            default: break;
        }
        update();
    }
}

bool MazeGLWidget::event(QEvent* e) {
    if (e->type() == QEvent::Gesture) {
        return gestureEvent(static_cast<QGestureEvent*>(e));
    }
    if (e->type() == QEvent::TouchBegin || e->type() == QEvent::TouchUpdate || e->type() == QEvent::TouchEnd) {
        touchEvent(static_cast<QTouchEvent*>(e));
        return true;
    }
    return QOpenGLWidget::event(e);
}

void MazeGLWidget::touchEvent(QTouchEvent* e) {
    const auto pts = e->points();
    if (pts.size() == 1 && cameraMode_ == CameraMode::Orbit) {
        auto p = pts.first();
        QPointF delta = p.velocity();
        yaw_ += static_cast<float>(delta.x()) * (sens_.orbitRotate * 0.02f);
        pitch_ += static_cast<float>(delta.y()) * (sens_.orbitRotate * 0.02f);
        if (pitch_ > 89.0f) pitch_ = 89.0f;
        if (pitch_ < -10.0f) pitch_ = -10.0f;
        update();
    }
    e->accept();
}

bool MazeGLWidget::gestureEvent(QGestureEvent* e) {
    if (auto* pinch = static_cast<QPinchGesture*>(e->gesture(Qt::PinchGesture))) {
        pinchTriggered(pinch);
    }
    if (auto* pan = static_cast<QPanGesture*>(e->gesture(Qt::PanGesture))) {
        panTriggered(pan);
    }
    if (auto* rot = static_cast<QRotationGesture*>(e->gesture(Qt::RotationGesture))) {
        rotationTriggered(rot);
    }
    return true;
}

void MazeGLWidget::pinchTriggered(QPinchGesture* gesture) {
    if (gesture->changeFlags() & QPinchGesture::ScaleFactorChanged) {
        qreal scale = gesture->scaleFactor();
        if (cameraMode_ == CameraMode::Orbit) {
            distance_ /= static_cast<float>(scale);
            if (distance_ < 5.0f) distance_ = 5.0f;
            if (distance_ > 200.0f) distance_ = 200.0f;
        } else {
            moveForward(static_cast<float>((scale-1.0) * 1.0));
        }
        update();
    }
}

void MazeGLWidget::panTriggered(QPanGesture* gesture) {
    QPointF delta = gesture->delta();
    if (cameraMode_ == CameraMode::Orbit) {
        yaw_ += static_cast<float>(delta.x()) * sens_.orbitPan;
        pitch_ += static_cast<float>(delta.y()) * sens_.orbitPan;
        if (pitch_ > 89.0f) pitch_ = 89.0f;
        if (pitch_ < -10.0f) pitch_ = -10.0f;
    } else {
        fpYaw_ += static_cast<float>(delta.x()) * sens_.fpRotate;
    }
    update();
}

void MazeGLWidget::rotationTriggered(QRotationGesture* gesture) {
    qreal angle = gesture->angleDelta();
    if (cameraMode_ == CameraMode::Orbit) {
        yaw_ += static_cast<float>(angle);
    } else {
        fpYaw_ += static_cast<float>(angle);
    }
    update();
}

void MazeGLWidget::buildMaze() {
    vertices_.clear();
    const MazeGrid& grid = controller_->grid();

    const float wallH = 1.8f;
    const float baseY = 0.0f;
    const QVector3D wallColor(0.6f, 0.7f, 0.8f);
    const QVector3D floorColor(0.15f, 0.18f, 0.22f);

    // Floor
    addQuad(QVector3D(0, baseY, 0), QVector3D(grid.width(), baseY, 0), QVector3D(grid.width(), baseY, grid.height()), QVector3D(0, baseY, grid.height()), QVector3D(0,1,0), floorColor);

    // Walls
    for (int y = 0; y < grid.height(); ++y) {
        for (int x = 0; x < grid.width(); ++x) {
            const Cell& c = grid.at(x,y);
            float fx = static_cast<float>(x);
            float fz = static_cast<float>(y);
            if (c.wallN) {
                QVector3D a(fx, baseY, fz);
                QVector3D b(fx+1, baseY, fz);
                QVector3D c1(fx+1, baseY+wallH, fz);
                QVector3D d(fx, baseY+wallH, fz);
                addQuad(a,b,c1,d, QVector3D(0,0,-1), wallColor);
            }
            if (c.wallS) {
                QVector3D a(fx, baseY, fz+1);
                QVector3D b(fx+1, baseY, fz+1);
                QVector3D c1(fx+1, baseY+wallH, fz+1);
                QVector3D d(fx, baseY+wallH, fz+1);
                addQuad(d,c1,b,a, QVector3D(0,0,1), wallColor);
            }
            if (c.wallW) {
                QVector3D a(fx, baseY, fz);
                QVector3D b(fx, baseY, fz+1);
                QVector3D c1(fx, baseY+wallH, fz+1);
                QVector3D d(fx, baseY+wallH, fz);
                addQuad(d,c1,b,a, QVector3D(-1,0,0), wallColor);
            }
            if (c.wallE) {
                QVector3D a(fx+1, baseY, fz);
                QVector3D b(fx+1, baseY, fz+1);
                QVector3D c1(fx+1, baseY+wallH, fz+1);
                QVector3D d(fx+1, baseY+wallH, fz);
                addQuad(a,b,c1,d, QVector3D(1,0,0), wallColor);
            }
        }
    }

    // Path overlay
    buildPathGeometry();
}

void MazeGLWidget::buildPathGeometry(){
    if (path_.empty()) return;
    const float baseY = 0.02f;
    const QVector3D pathColor(1.0f, 0.3f, 0.3f);
    for (const auto &c : path_) {
        float fx = float(c.x);
        float fz = float(c.y);
        QVector3D a(fx+0.15f, baseY, fz+0.15f);
        QVector3D b(fx+0.85f, baseY, fz+0.15f);
        QVector3D c1(fx+0.85f, baseY, fz+0.85f);
        QVector3D d(fx+0.15f, baseY, fz+0.85f);
        addQuad(a,b,c1,d, QVector3D(0,1,0), pathColor);
    }
}

void MazeGLWidget::addQuad(const QVector3D& a, const QVector3D& b, const QVector3D& c, const QVector3D& d, const QVector3D& normal, const QVector3D& color) {
    vertices_.push_back({a, normal, color, QVector2D(0.0f,0.0f)});
    vertices_.push_back({b, normal, color, QVector2D(1.0f,0.0f)});
    vertices_.push_back({c, normal, color, QVector2D(1.0f,1.0f)});
    vertices_.push_back({a, normal, color, QVector2D(0.0f,0.0f)});
    vertices_.push_back({c, normal, color, QVector2D(1.0f,1.0f)});
    vertices_.push_back({d, normal, color, QVector2D(0.0f,1.0f)});
}

}
void MazeGLWidget::uploadGeometry() {
    if (!vao_) glGenVertexArrays(1, &vao_);
    if (!vbo_) glGenBuffers(1, &vbo_);

    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, vertices_.size() * sizeof(Vertex), vertices_.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex,pos)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex,normal)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex,color)));
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex,uv)));

    glBindVertexArray(0);
}

QMatrix4x4 MazeGLWidget::computeViewMatrix() const {
    QMatrix4x4 view;
    if (cameraMode_ == CameraMode::Orbit) {
        float cx = controller_->grid().width() * 0.5f;
        float cz = controller_->grid().height() * 0.5f;
        QVector3D target(cx, 0.6f, cz);
        float yawRad = yaw_ * float(M_PI/180.0);
        float pitchRad = pitch_ * float(M_PI/180.0);
        float x = target.x() + distance_ * cosf(pitchRad) * cosf(yawRad);
        float y = target.y() + distance_ * sinf(pitchRad);
        float z = target.z() + distance_ * cosf(pitchRad) * sinf(yawRad);
        QVector3D eye(x,y,z);
        view.lookAt(eye, target, QVector3D(0,1,0));
    } else {
        float yawRad = fpYaw_ * float(M_PI/180.0);
        QVector3D forward(cosf(yawRad), 0.0f, sinf(yawRad));
        QVector3D eye(fpPos_.x(), 0.6f, fpPos_.z());
        QVector3D target = eye + forward;
        view.lookAt(eye, target, QVector3D(0,1,0));
    }
    return view;
}

QMatrix4x4 MazeGLWidget::computeProjMatrix() const {
    QMatrix4x4 proj;
    float aspect = float(width()) / float(std::max(1, height()));
    proj.perspective(60.0f, aspect, 0.1f, 500.0f);
    return proj;
}

void MazeGLWidget::moveForward(float delta) {
    float yawRad = fpYaw_ * float(M_PI/180.0);
    QVector3D dir(cosf(yawRad), 0.0f, sinf(yawRad));
    QVector3D desired = dir * delta;
    resolveCollision(fpPos_, desired);
}

void MazeGLWidget::strafeRight(float delta) {
    float yawRad = fpYaw_ * float(M_PI/180.0);
    QVector3D right(-sinf(yawRad), 0.0f, cosf(yawRad));
    QVector3D desired = right * delta;
    resolveCollision(fpPos_, desired);
}

bool MazeGLWidget::canMoveTo(const QVector3D& next) const {
    const MazeGrid& grid = controller_->grid();
    float r = collisionRadius_;
    if (next.x() < r || next.z() < r || next.x() > grid.width()-r || next.z() > grid.height()-r) return false;
    int cx = int(std::floor(next.x()));
    int cz = int(std::floor(next.z()));
    if (!grid.inBounds(cx, cz)) return false;
    const Cell &c = grid.at(cx, cz);
    float dxL = next.x() - float(cx);
    float dxR = float(cx+1) - next.x();
    float dzN = next.z() - float(cz);
    float dzS = float(cz+1) - next.z();
    if (c.wallW && dxL < r) return false;
    if (c.wallE && dxR < r) return false;
    if (c.wallN && dzN < r) return false;
    if (c.wallS && dzS < r) return false;
    if (dxL < r && grid.inBounds(cx-1, cz)) { const Cell &wc = grid.at(cx-1, cz); if (wc.wallE) return false; }
    if (dxR < r && grid.inBounds(cx+1, cz)) { const Cell &ec = grid.at(cx+1, cz); if (ec.wallW) return false; }
    if (dzN < r && grid.inBounds(cx, cz-1)) { const Cell &nc = grid.at(cx, cz-1); if (nc.wallS) return false; }
    if (dzS < r && grid.inBounds(cx, cz+1)) { const Cell &sc = grid.at(cx, cz+1); if (sc.wallN) return false; }
    return true;
}

bool MazeGLWidget::resolveCollision(QVector3D& pos, const QVector3D& desiredDelta) const {
    QVector3D next = pos + desiredDelta;
    if (canMoveTo(next)) { pos = next; return true; }
    QVector3D dx(desiredDelta.x(), 0.0f, 0.0f);
    QVector3D dz(0.0f, 0.0f, desiredDelta.z());
    bool moved = false;
    QVector3D test = pos + dx;
    if (canMoveTo(test)) { pos = test; moved = true; }
    test = pos + dz;
    if (canMoveTo(test)) { pos = test; moved = true; }
    return moved;
}

void MazeGLWidget::regenerateMaze(const MazeConfig& cfg){
    cfg_ = cfg;
    controller_ = std::make_unique<MazeController>(cfg_.width, cfg_.height);
    controller_->regenerate(cfg_);
    path_.clear();
    buildMaze();
    uploadGeometry();
    update();
}

void MazeGLWidget::startPreview(const MazeConfig& cfg){
    cfg_ = cfg;
    controller_ = std::make_unique<MazeController>(cfg_.width, cfg_.height);
    controller_->startPreview(cfg_);
    path_.clear();
    buildMaze();
    uploadGeometry();
    update();
}

void MazeGLWidget::stepPreview(){
    if (!controller_) return;
    if (controller_->previewActive()){
        controller_->stepPreview();
        emit previewProgress(controller_->previewProgress());
        buildMaze();
        uploadGeometry();
        update();
    }
}

void MazeGLWidget::runSolver(){
    const MazeGrid& g = controller_->grid();
    if (goal_.x == 0 && goal_.y == 0){ goal_ = {g.width()-1, g.height()-1}; }
    auto res = MazeSolver::solveBFS(g, start_, goal_);
    if (res){ path_ = res->nodes; } else { path_.clear(); }
    buildMaze();
    uploadGeometry();
    update();
}

void MazeGLWidget::clearPath(){
    path_.clear();
    buildMaze();
    uploadGeometry();
    update();
}

void MazeGLWidget::setStartGoal(Coord s, Coord g){ start_=s; goal_=g; }

void MazeGLWidget::loadGridAndConfig(const MazeGrid& grid, const MazeConfig& cfg){
    cfg_ = cfg;
    controller_ = std::make_unique<MazeController>(grid.width(), grid.height());
    controller_->regenerate(cfg_);
    controller_->applyFromGrid(grid);
    emit mazeConfigLoaded(cfg_);
    path_.clear();
    buildMaze();
    uploadGeometry();
    update();
}

void MazeGLWidget::createProceduralTextures(){
    const int W=128,H=128;
    QImage wallImg(W,H,QImage::Format_RGBA8888);
    for(int y=0;y<H;++y){ for(int x=0;x<W;++x){ int c = (((x/8)+(y/8))%2)?220:180; wallImg.setPixelColor(x,y,QColor(c,c,c)); }}
    QImage floorImg(W,H,QImage::Format_RGBA8888);
    for(int y=0;y<H;++y){ for(int x=0;x<W;++x){ int c = (((x/16)+(y/16))%2)?90:70; floorImg.setPixelColor(x,y,QColor(c,c,c)); }}
    auto uploadTex = [&](QImage img, GLuint &tex){
        if (!tex) glGenTextures(1,&tex);
        glBindTexture(GL_TEXTURE_2D, tex);
        QImage glImg = img.convertToFormat(QImage::Format_RGBA8888);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, glImg.width(), glImg.height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, glImg.constBits());
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glGenerateMipmap(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, 0);
    };
    uploadTex(wallImg, wallTex_);
    uploadTex(floorImg, floorTex_);
}

void MazeGLWidget::setThemeTextured(bool enabled){
    themeMode_ = enabled ? ThemeMode::Textured : ThemeMode::Flat;
    update();
}

#include "MainWindow.h"
#include "MazeGLWidget.h"
#include "MazeTypes.h"
#include "MazeIO.h"

#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
#include <QAction>
#include <QLabel>
#include <QMessageBox>
#include <QComboBox>
#include <QSpinBox>
#include <QLineEdit>
#include <QDockWidget>
#include <QProgressBar>
#include <QTimer>
#include <QGridLayout>
#include <QLabel>
#include <QFileDialog>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent), glWidget_(new MazeGLWidget(this)) {
    setWindowTitle("3D Maze Generator");
    setCentralWidget(glWidget_);

    createMenus();
    createToolbars();
    createStatusBar();
    createControlsPanel();

    connect(glWidget_, &MazeGLWidget::fpsUpdated, this, &MainWindow::onFpsUpdated);
    connect(glWidget_, &MazeGLWidget::previewProgress, this, [this](float p){ progressLabel_->setText(QString("Progress: %1% ").arg(int(p*100))); });
    previewTimer_ = new QTimer(this);
    previewTimer_->setInterval(50);
    connect(previewTimer_, &QTimer::timeout, this, &MainWindow::onPreviewStep);
}

MainWindow::~MainWindow() = default;

void MainWindow::createMenus() {
    auto* fileMenu = menuBar()->addMenu("&File");
    auto* saveJsonAct = new QAction("Save JSON", this);
    auto* loadJsonAct = new QAction("Load JSON", this);
    auto* exportPngAct = new QAction("Export PNG", this);
    fileMenu->addAction(saveJsonAct);
    fileMenu->addAction(loadJsonAct);
    fileMenu->addAction(exportPngAct);

    auto* exitAct = new QAction("E&xit", this);
    connect(exitAct, &QAction::triggered, this, &QWidget::close);
    fileMenu->addAction(exitAct);

    connect(saveJsonAct, &QAction::triggered, this, [this](){
        QString fn = QFileDialog::getSaveFileName(this, "Save Maze JSON", QString(), "JSON Files (*.json)");
        if (fn.isEmpty()) return;
        const auto &cfg = glWidget_->currentConfig();
        const auto &grid = glWidget_->currentGrid();
        if (MazeIO::saveToJson(grid, cfg, fn)) statusBar()->showMessage("Saved JSON", 2000);
        else statusBar()->showMessage("Failed to save JSON", 2000);
    });
    connect(loadJsonAct, &QAction::triggered, this, [this](){
        QString fn = QFileDialog::getOpenFileName(this, "Load Maze JSON", QString(), "JSON Files (*.json)");
        if (fn.isEmpty()) return;
        std::unique_ptr<MazeGrid> g; MazeConfig cfg;
        if (!MazeIO::loadCreate(g, cfg, fn)) {
            QMessageBox::warning(this, "Load JSON", "Failed to parse JSON or invalid file.");
            return;
        }
        widthSpin_->setValue(g->width());
        heightSpin_->setValue(g->height());
        int algIndex = (cfg.algorithm == MazeAlgorithm::Prims) ? 1 : 0;
        algoCombo_->setCurrentIndex(algIndex);
        seedEdit_->setText(QString::number(cfg.seed));
        glWidget_->loadGridAndConfig(*g, cfg);
        statusBar()->showMessage("Loaded JSON", 2000);
    });
    connect(exportPngAct, &QAction::triggered, this, [this](){
        QString fn = QFileDialog::getSaveFileName(this, "Export PNG", QString(), "PNG Files (*.png)");
        if (fn.isEmpty()) return;
        QImage img = glWidget_->snapshot();
        if (MazeIO::saveSnapshotPNG(img, fn)) statusBar()->showMessage("Exported PNG", 2000);
        else statusBar()->showMessage("Failed to export PNG", 2000);
    });

    auto* helpMenu = menuBar()->addMenu("&Help");
    auto* aboutAct = new QAction("&About", this);
    connect(aboutAct, &QAction::triggered, this, [this]() {
        QMessageBox::about(this, "About", "3D Maze Generator (C++)\nQt6 + OpenGL + Touch Support");
    });
    helpMenu->addAction(aboutAct);
}

void MainWindow::createToolbars() {
    auto* mainTb = addToolBar("Main");
    mainTb->setMovable(false);
    genAct_ = new QAction("Generate", this);
    mainTb->addAction(genAct_);
    previewAct_ = new QAction("Preview", this);
    previewAct_->setCheckable(true);
    mainTb->addAction(previewAct_);
    auto* solveAct_ = new QAction("Solve", this);
    auto* clearPathAct = new QAction("Clear Path", this);
    mainTb->addAction(solveAct_);
    mainTb->addAction(clearPathAct);

    connect(genAct_, &QAction::triggered, this, &MainWindow::onGenerate);
    connect(previewAct_, &QAction::toggled, this, &MainWindow::onPreviewToggled);
    connect(solveAct_, &QAction::triggered, this, [this](){ glWidget_->runSolver(); });
    connect(clearPathAct, &QAction::triggered, this, [this](){ glWidget_->clearPath(); });
}

void MainWindow::createStatusBar() {
    fpsLabel_ = new QLabel("FPS: --", this);
    progressLabel_ = new QLabel("Progress: 0%", this);
    statusBar()->addPermanentWidget(fpsLabel_);
    statusBar()->addPermanentWidget(progressLabel_);
    statusBar()->showMessage("Ready");
}

void MainWindow::createControlsPanel(){
    auto* dock = new QDockWidget("Controls", this);
    QWidget* panel = new QWidget(dock);
    auto* layout = new QGridLayout(panel);

    algoCombo_ = new QComboBox(panel);
    algoCombo_->addItem("Recursive Backtracking", static_cast<int>(MazeAlgorithm::RecursiveBacktracking));
    algoCombo_->addItem("Prim's", static_cast<int>(MazeAlgorithm::Prims));

    widthSpin_ = new QSpinBox(panel); widthSpin_->setRange(5, 200); widthSpin_->setValue(20);
    heightSpin_ = new QSpinBox(panel); heightSpin_->setRange(5, 200); heightSpin_->setValue(20);
    seedEdit_ = new QLineEdit(panel); seedEdit_->setPlaceholderText("0 = random"); seedEdit_->setText("0");

    int r=0;
    layout->addWidget(new QLabel("Algorithm:"), r,0); layout->addWidget(algoCombo_, r,1); r++;
    auto* themeCombo_ = new QComboBox(panel); themeCombo_->addItem("Flat"); themeCombo_->addItem("Textured");
    layout->addWidget(new QLabel("Theme:"), r,0); layout->addWidget(themeCombo_, r,1); r++;
    QObject::connect(themeCombo_, &QComboBox::currentTextChanged, [this](const QString& t){ glWidget_->setThemeTextured(t=="Textured"); });
    layout->addWidget(new QLabel("Width:"), r,0); layout->addWidget(widthSpin_, r,1); r++;
    layout->addWidget(new QLabel("Height:"), r,0); layout->addWidget(heightSpin_, r,1); r++;
    layout->addWidget(new QLabel("Seed:"), r,0); layout->addWidget(seedEdit_, r,1); r++;

    panel->setLayout(layout);
    dock->setWidget(panel);
    addDockWidget(Qt::LeftDockWidgetArea, dock);
}

void MainWindow::onGenerate(){
    MazeConfig cfg;
    cfg.width = widthSpin_->value();
    cfg.height = heightSpin_->value();
    cfg.algorithm = static_cast<MazeAlgorithm>(algoCombo_->currentData().toInt());
    bool ok=false; uint64_t seed = seedEdit_->text().toULongLong(&ok);
    cfg.seed = ok ? seed : 0;
    glWidget_->regenerateMaze(cfg);
    statusBar()->showMessage("Maze regenerated", 2000);
}

void MainWindow::onPreviewToggled(bool checked){
    MazeConfig cfg;
    cfg.width = widthSpin_->value();
    cfg.height = heightSpin_->value();
    cfg.algorithm = static_cast<MazeAlgorithm>(algoCombo_->currentData().toInt());
    bool ok=false; uint64_t seed = seedEdit_->text().toULongLong(&ok);
    cfg.seed = ok ? seed : 0;
    if (checked){
        glWidget_->startPreview(cfg);
        previewTimer_->start();
        statusBar()->showMessage("Preview started", 2000);
    } else {
        previewTimer_->stop();
        statusBar()->showMessage("Preview stopped", 2000);
    }
}

void MainWindow::onPreviewStep(){
    glWidget_->stepPreview();
}

void MainWindow::onFpsUpdated(float fps){
    fpsLabel_->setText(QString("FPS: %1").arg(QString::number(fps, 'f', 1)));
}

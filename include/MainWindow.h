#pragma once

#include <QMainWindow>
#include <QComboBox>
#include <QSpinBox>
#include <QLineEdit>
#include <QAction>
#include <QLabel>
#include <QTimer>

class MazeGLWidget;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

private:
    void createMenus();
    void createToolbars();
    void createStatusBar();
    void createControlsPanel();

    MazeGLWidget* glWidget_;
    // Controls
    QComboBox* algoCombo_ = nullptr;
    QSpinBox* widthSpin_ = nullptr;
    QSpinBox* heightSpin_ = nullptr;
    QLineEdit* seedEdit_ = nullptr;
    QAction* genAct_ = nullptr;
    QAction* previewAct_ = nullptr;
    QLabel* fpsLabel_ = nullptr;
    QLabel* progressLabel_ = nullptr;
    QTimer* previewTimer_ = nullptr;

private slots:
    void onGenerate();
    void onPreviewToggled(bool checked);
    void onPreviewStep();
    void onFpsUpdated(float fps);
};

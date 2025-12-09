#include <QApplication>
#include <QSurfaceFormat>
#include "MainWindow.h"

int main(int argc, char *argv[]) {
    // High-DPI scaling and touch synthesis
#if QT_VERSION >= QT_VERSION_CHECK(5, 6, 0)
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif
    QCoreApplication::setAttribute(Qt::AA_SynthesizeTouchForUnhandledMouseEvents);
    QCoreApplication::setAttribute(Qt::AA_SynthesizeMouseForUnhandledTouchEvents);

    QApplication app(argc, argv);

    // Configure default OpenGL surface format
    QSurfaceFormat fmt;
#ifdef Q_OS_ANDROID
    fmt.setSamples(2);
#elif defined(Q_OS_IOS)
    fmt.setSamples(2);
#else
    fmt.setSamples(4);
#endif
    fmt.setVersion(3, 3);
    fmt.setProfile(QSurfaceFormat::CoreProfile);
    fmt.setSamples(4);
    QSurfaceFormat::setDefaultFormat(fmt);

    MainWindow w;
    w.resize(1280, 800);
    w.show();

    return app.exec();
}

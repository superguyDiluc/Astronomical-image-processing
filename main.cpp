#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>

#include "src/FitsImageProvider.h"
#include "src/FitsManager.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    QQuickStyle::setStyle("Material");

    QQmlApplicationEngine engine;

    // Create image provider and manager before QML loads
    auto *imageProvider = new FitsImageProvider;
    engine.addImageProvider(QStringLiteral("fits"), imageProvider);

    auto *fitsManager = new FitsManager(&app);
    fitsManager->setImageProvider(imageProvider);
    engine.rootContext()->setContextProperty(QStringLiteral("fitsManager"), fitsManager);

    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);

    engine.loadFromModule("test", "Main");

    return QGuiApplication::exec();
}

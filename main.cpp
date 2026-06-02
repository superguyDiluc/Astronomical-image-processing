#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickStyle>

#include "src/FitsImageProvider.h"
#include "src/FitsManager.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    QQuickStyle::setStyle("Material");

    QQmlApplicationEngine engine;

    auto *imageProvider = new FitsImageProvider;
    engine.addImageProvider(QStringLiteral("fits"), imageProvider);

    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);

    engine.loadFromModule("test", "Main");

    // Connect FitsManager instances to the shared image provider
    const auto rootObjects = engine.rootObjects();
    for (auto *root : rootObjects) {
        auto *manager = root->findChild<FitsManager *>();
        if (manager) {
            manager->setImageProvider(imageProvider);
        }
    }

    return QGuiApplication::exec();
}

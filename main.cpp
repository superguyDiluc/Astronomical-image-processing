#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>

#include "src/AstrometryManager.h"
#include "src/FitsImageProvider.h"
#include "src/FitsManager.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    QCoreApplication::setOrganizationName(QStringLiteral("AstronomicalImageProcessing"));
    QCoreApplication::setApplicationName(QStringLiteral("FitsViewer"));

    QQuickStyle::setStyle("Material");

    QQmlApplicationEngine engine;

    // Create image provider and manager before QML loads
    auto *imageProvider = new FitsImageProvider;
    engine.addImageProvider(QStringLiteral("fits"), imageProvider);

    auto *fitsManager = new FitsManager(&app);
    fitsManager->setImageProvider(imageProvider);
    engine.rootContext()->setContextProperty(QStringLiteral("fitsManager"), fitsManager);

    auto *astrometryManager = new AstrometryManager(&app);
    engine.rootContext()->setContextProperty(QStringLiteral("astrometryManager"), astrometryManager);

    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);

    engine.loadFromModule("test", "Main");

    return QGuiApplication::exec();
}

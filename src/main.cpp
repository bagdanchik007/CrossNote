#include "application/NoteController.h"

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QUrl>

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    QGuiApplication::setApplicationName(QStringLiteral("CrossNote"));
    QGuiApplication::setOrganizationName(QStringLiteral("CrossNote"));
    QGuiApplication::setOrganizationDomain(QStringLiteral("crossnote.local"));
    QGuiApplication::setApplicationVersion(QStringLiteral("0.2.0"));

    QQuickStyle::setStyle(QStringLiteral("Basic"));

    CrossNote::NoteController noteController;
    noteController.initialize();

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("noteController"), &noteController);
    const QUrl mainUrl(QStringLiteral("qrc:/qt/qml/CrossNote/qml/Main.qml"));
    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        [] { QCoreApplication::exit(EXIT_FAILURE); },
        Qt::QueuedConnection);
    engine.load(mainUrl);

    return app.exec();
}

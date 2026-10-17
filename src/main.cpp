#include "application/NoteController.h"

#include <QGuiApplication>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QTextStream>
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
    const auto writeQmlDiagnostic = [](const QString &message) {
        QFile logFile(QDir::temp().filePath(QStringLiteral("CrossNote-startup.log")));
        if (!logFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text))
            return;

        QTextStream log(&logFile);
        log << message << Qt::endl;
    };
    QObject::connect(
        &engine,
        &QQmlApplicationEngine::warnings,
        &app,
        [writeQmlDiagnostic](const QList<QQmlError> &warnings) {
            for (const QQmlError &warning : warnings)
                writeQmlDiagnostic(warning.toString());
        });
    engine.rootContext()->setContextProperty(QStringLiteral("noteController"), &noteController);
    const QUrl mainUrl(QStringLiteral("qrc:/qt/qml/CrossNote/qml/Main.qml"));
    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        [writeQmlDiagnostic] {
            writeQmlDiagnostic(QStringLiteral("CrossNote konnte das Hauptfenster nicht erstellen."));
            QCoreApplication::exit(EXIT_FAILURE);
        },
        Qt::QueuedConnection);
    engine.load(mainUrl);

    return app.exec();
}

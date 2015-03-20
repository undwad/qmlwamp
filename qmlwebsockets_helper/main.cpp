#include <QApplication>
#include <QQmlApplicationEngine>
#include <qqml.h>

#include "websocketclient.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QQmlApplicationEngine engine;
    qmlRegisterType<WebSocketClient>("qmlwebsockets", 1, 0, "WebSocketClient");
    engine.load(QUrl(QStringLiteral("qrc:/qmlwamp.qml")));

    return app.exec();
}

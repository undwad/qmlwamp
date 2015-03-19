#include "qmlwebsockets_plugin.h"
#include "websocketclient.h"

#include <qqml.h>

void QmlwebsocketsPlugin::registerTypes(const char *uri)
{
    // @uri qmlwebsockets
    qmlRegisterType<WebSocketClient>(uri, 1, 0, "WebSocketClient");
}



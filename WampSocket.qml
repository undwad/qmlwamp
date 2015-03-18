/*
** wamp client for qml
** https://github.com/undwad/qmlwamp mailto:undwad@mail.ru
** see copyright notice in ./LICENCE
*/

import QtQuick 2.4
import Qt.WebSockets 1.0

Item
{
    property bool log
    property alias active: _ws.active
    property alias errorString: _ws.errorString
    property alias status: _ws.status
    property string sessionId

    property alias url: _ws.url
    property string realm
    property bool serverIsBroker
    property bool serverIsDealer

    property bool clientIsPublisher
    property bool clientIsSubscriber
    property bool clientIsCaller
    property bool clientIsCallee

    signal welcome(string sessionId, var details)
    signal abort()

    WebSocket
    {
        id: _ws

        property int _HELLO: 1
        property int _WELCOME: 2
        property int _ABORT: 3
        property int _CHALLENGE: 4
        property int _AUTHENTICATE: 5
        property int _GOODBYE: 6
        property int _ERROR: 8
        property int _PUBLISH: 16
        property int _PUBLISHED: 17
        property int _SUBSCRIBE: 32
        property int _SUBSCRIBED: 33
        property int _UNSUBSCRIBE: 34
        property int _UNSUBSCRIBED: 35
        property int _EVENT: 36
        property int _CALL: 48
        property int _CANCEL: 49
        property int _RESULT: 50
        property int _REGISTER: 64
        property int _REGISTERED: 65
        property int _UNREGISTER: 66
        property int _UNREGISTERED: 67
        property int _INVOCATION: 68
        property int _INTERRUPT: 69
        property int _YIELD: 70

        function send()
        {
            var msg = JSON.stringify(Array.prototype.slice.call(arguments))
            if(log) print('>>>', msg)
            sendTextMessage(msg)
        }

        onStatusChanged:
        {
            if(WebSocket.Open === status)
                send
                (
                    _HELLO,
                    server.realm,
                    {
                        roles:
                        {
                            publisher: clientIsPublisher ? {} : null,
                            subscriber: clientIsPublisher ? {} : null,
                            caller: clientIsPublisher ? {} : null,
                            callee: clientIsPublisher ? {} : null,
                        }
                    }
                )
        }

        onTextMessageReceived:
        {
            if(log) print('<<<', message)
            var msg = JSON.parse(message)
            switch(msg[0])
            {
                case _WELCOME:
                {
                    sessionId = msg[1]
                    var details = msg[2]
                    serverIsBroker = 'broker' in details.roles
                    serverIsDealer = 'dealer' in details.roles
                    welcome(sessionId, details)
                    break;
                }
                case _ABORT:
                {
                    break;
                }
                case _CHALLENGE:
                {
                    break;
                }
                case _GOODBYE:
                {
                    break;
                }
                case _ERROR:
                {
                    break;
                }
                case _PUBLISHED:
                {
                    break;
                }
                case _SUBSCRIBED:
                {
                    break;
                }
                case _UNSUBSCRIBED:
                {
                    break;
                }
                case _EVENT:
                {
                    break;
                }
                case _RESULT:
                {
                    break;
                }
                case _REGISTERED:
                {
                    break;
                }
                case _UNREGISTERED:
                {
                    break;
                }
                case _INVOCATION:
                {
                    break;
                }
                case _INTERRUPT:
                {
                    break;
                }
            }
        }
    }

    function pprint() { print(Array.prototype.slice.call(arguments).map(JSON.stringify)) }
}

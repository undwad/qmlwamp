/*
** wamp client for qml
** https://github.com/undwad/qmlwamp mailto:undwad@mail.ru
** see copyright notice in ./LICENCE
*/

import QtQuick 2.4
import qmlwebsockets 1.0

Item
{
    id: _
    property bool log
    property alias url: _ws.url
    property alias origin: _ws.origin

    property string realm

    property var clientRoles:
    ({
         publisher: {},
         subscriber: {},
         caller: {},
         callee: {},
    })

    property string username
    property string password

    property string sessionId
    property var serverRoles

    signal welcome(var sessionId, var details)
    signal abort(var details, var reason)
    signal closed()

    WebSocketClient
    {
        id: _ws

        protocol: 'wamp.2.json'

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

        function send_()
        {
            var msg = JSON.stringify(Array.prototype.slice.call(arguments))
            if(log) print('<<<', msg)
            send(msg)
        }

        onStateChanged:
        {
            print('WS', ['CLOSING', 'CLOSED', 'CONNECTING', 'OPEN'][state])
            switch(state)
            {
            case WebSocketClient.OPEN: send_(_HELLO, realm, { roles: clientRoles }); break;
            case WebSocketClient.CLOSED: closed(); break;
            }
        }

        onMessage:
        {
            if(log) print('>>>', text)
            var msg = JSON.parse(text)
            switch(msg[0])
            {
                case _WELCOME:
                {
                    sessionId = msg[1]
                    var details = msg[2]
                    serverRoles = details.roles
                    welcome(sessionId, details)
                    break;
                }
                case _ABORT: abort(msg[1], msg[2]); break;
                case _CHALLENGE:
                {
                    send_(_AUTHENTICATE, username + ':' + password, {})
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

    function open() { _ws.open() }
    function ping() { _ws.ping() }
    function send(text) { _ws.send(text) }
    function close() { _ws.close() }

    function pprint() { print(Array.prototype.slice.call(arguments).map(JSON.stringify)) }
}

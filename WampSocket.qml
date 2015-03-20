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

    signal welcome(var params)
    signal abort(var params)
    signal goodbye(var params)
    signal error(var params)
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

        property int requestId: 0
        property var requests: ([])
        property var subscriptions: ([])
        property var onevents: ([])

        function sendArgs()
        {
            var msg = JSON.stringify(Array.prototype.slice.call(arguments))
            if(log) print('<<<', msg)
            send(msg)
        }

        onSocketError: error({ message: message, details: details })

        onStateChanged:
        {
            print('WS', ['CLOSING', 'CLOSED', 'CONNECTING', 'INITIALIZING', 'OPEN'][state])
            switch(state)
            {
            case WebSocketClient.OPEN: sendArgs(_HELLO, realm, { roles: clientRoles }); break;
            case WebSocketClient.CLOSED: closed(); break;
            }
        }

        onHeaderReceived: print(header)

        onMessageReceived:
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
                    welcome({ id: sessionId, details: details })
                    break;
                }
                case _ABORT: abort({ details: msg[1], reason: msg[2] }); break;
                case _CHALLENGE:
                {
                    sendArgs(_AUTHENTICATE, username + ':' + password, {})
                    break;
                }
                case _GOODBYE: goodbye({ details: msg[1], reason: msg[2] }); break;
                case _ERROR:
                {
                    var type = msg[1]
                    var requestId = msg[2]
                    var details = msg[3]
                    var error = msg[4]
                    var args = msg[5]
                    var kwargs = msg[6]
                    if(requestId in requests)
                    {
                        var onerror = requests[requestId].onerror
                        requests[requestId] = null
                        if(onerror) onerror({ details: details, error: error, args: args, kwargs: kwargs })
                    }
                    break
                }
                case _SUBSCRIBED:
                {
                    var requestId = msg[1]
                    var subscriptionId = msg[2]
                    if(requestId in requests)
                    {
                        var request = requests[requestId]
                        subscriptions[request.topic] = subscriptionId
                        onevents[subscriptionId] = request.onevent
                        var onsuccess = request.onsuccess
                        requests[requestId] = null
                        if(onsuccess) onsuccess()
                    }
                    break;
                }
                case _PUBLISHED:
                case _UNSUBSCRIBED:
                {
                    var requestId = msg[1]
                    var details = msg[2]
                    if(requestId in requests)
                    {
                        var onsuccess = requests[requestId].onsuccess
                        requests[requestId] = null
                        if(onsuccess) onsuccess(details)
                    }
                    break;
                }
                case _EVENT:
                {
                    var subscriptionId = msg[1]
                    var publicationId = msg[2]
                    var details = msg[3]
                    var args = msg[4]
                    var kwargs = msg[5]
                    if(subscriptionId in onevents) onevents[subscriptionId](publicationId, details, args, kwargs)
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

        function subscribe(options, topic, onevent, onsuccess, onerror)
        {
            requestId++
            requests[requestId] = { topic: topic, onevent: onevent, onsuccess: onsuccess, onerror: onerror }
            sendArgs(_SUBSCRIBE, requestId, options, topic)
        }

        function unsubscribe(topic, onsuccess, onerror)
        {
            if(topic in subscriptions)
            {
                var subscriptionId = subscriptions[topic]
                onevents[subscriptionId] = subscriptions[topic] = null
                requestId++
                requests[requestId] = { onsuccess: onsuccess, onerror: onerror }
                sendArgs(_UNSUBSCRIBE, requestId, subscriptionId)
            }
        }

        function publish(options, topic, args, kwargs, onsuccess, onerror)
        {
            requestId++
            requests[requestId] = { onsuccess: onsuccess, onerror: onerror }
            sendArgs(_PUBLISH, requestId, options, topic, args, kwargs)
        }
    }

    function open() { _ws.open() }
    function ping() { _ws.ping() }
    function close() { _ws.close() }
    function subscribe(options, topic, onevent, onsuccess, onerror) { _ws.subscribe(options, topic, onevent, onsuccess, onerror) }
    function unsubscribe(topic, onsuccess, onerror) { _ws.unsubscribe(topic, onsuccess, onerror) }
    function publish(options, topic, args, kwargs, onsuccess, onerror) { _ws.publish(options, topic, args, kwargs, onsuccess, onerror) }

    function pprint() { print(Array.prototype.slice.call(arguments).map(JSON.stringify)) }
}

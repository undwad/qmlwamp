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
    property bool dump
    property alias url: _ws.url
    property alias origin: _ws.origin
    property bool compress

    property string realm

    property var clientRoles:
    ({
         publisher: {},
         subscriber: {},
         caller: {},
         callee: {},
    })

    property string sessionId
    property var serverRoles

    signal header(var header)
    signal challenge(var params)
    signal welcome(var params)
    signal abort(var params)
    signal goodbye(var params)
    signal error(var params)
    signal closed()

    WorkerScript
    {
        id: _worker
        source: "AsyncJSONParser.js"
        onMessage: _ws.parsed(messageObject)
    }

    WebSocketClient
    {
        id: _ws

        extensions: compress ? 'permessage-deflate' : ''
        protocol: 'wamp.2.json'
        key: 'x3JJHMbDL1EzLkh9GBhXDw=='

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
        property var requests: ({})
        property var uris: ({})
        property var callbacks: ({})
        property var results: ({})
        property var cancels: ({})

        function sendArgs()
        {
            var msg = JSON.stringify(Array.prototype.slice.call(arguments).filter(function(arg) { return arg }))
            if(dump) print('<<<', msg)
            send(msg)
        }

        onSocketError: error({ message: message, details: details })

        onStateChanged:
        {
            if(dump) print('^^^', ['CLOSING', 'CLOSED', 'CONNECTING', 'INITIALIZING', 'OPEN'][state])
            switch(state)
            {
            case WebSocketClient.OPEN: sendArgs(_HELLO, realm, { roles: clientRoles }); break;
            case WebSocketClient.CLOSED: closed(); break;
            }
        }

        onHeaderReceived: _.header(header)

        onMessageReceived: _worker.sendMessage({dump: dump, text: text})

        function parsed(msg)
        {
            if(msg.result)
            {
                msg = msg.result
                var code = msg[0]
                switch(code)
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
                    case _CHALLENGE: challenge({ method: msg[1], extra: msg[2] }); break;
                    case _GOODBYE: goodbye({ details: msg[1], reason: msg[2] }); break;
                    case _ERROR:
                    {
                        var type = msg[1]
                        var requestId = msg[2]
                        var details = msg[3]
                        var error = msg[4]
                        var args = msg[5]
                        var kwargs = msg[6]
                        results[requestId] = null
                        if(requestId in requests)
                        {
                            var onerror = requests[requestId].onerror
                            requests[requestId] = null
                            if(onerror) onerror({ details: details, error: error, args: args, kwargs: kwargs })
                        }
                        break
                    }
                    case _REGISTERED:
                    case _SUBSCRIBED:
                    case _PUBLISHED:
                    case _UNSUBSCRIBED:
                    case _UNREGISTERED:
                    {
                        var requestId = msg[1]
                        var callbackId = msg[2]
                        if(requestId in requests)
                        {
                            var request = requests[requestId]
                            callbacks[callbackId] = request.callback
                            cancels[callbackId] = request.cancel
                            var onsuccess = request.onsuccess
                            requests[requestId] = null
                            if(onsuccess) onsuccess(callbackId)
                        }
                        break;
                    }
                    case _EVENT:
                    case _INVOCATION:
                    {
                        var callbackId = msg[1]
                        var responseId = msg[2]
                        var details = msg[3]
                        var args = msg[4]
                        var kwargs = msg[5]
                        if(_INVOCATION === code) var temp = callbackId, callbackId = responseId, responseId = temp
                        if(callbackId in callbacks)
                            callbacks[callbackId]
                            ({
                                 id: responseId,
                                 details: details,
                                 args: args,
                                 kwargs: kwargs,
                                 yield: _INVOCATION === code ? sendArgs.bind(_ws, _YIELD, responseId) : null
                             })
                        break;
                    }
                    case _RESULT:
                    {
                        var resultId = msg[1]
                        var details = msg[2]
                        var args = msg[3]
                        var kwargs = msg[4]
                        requests[resultId] = null
                        if(resultId in results)
                        {
                            var result = results[resultId]
                            if(result)
                                if(!result
                                ({
                                     details: details,
                                     args: args,
                                     kwargs: kwargs,
                                 }))
                                    results[resultId] = null
                        }
                        break;
                    }
                    case _INTERRUPT:
                    {
                        var cancelId = msg[1]
                        var options = msg[2]
                        if(cancelId in cancels) cancels[cancelId]({ id: cancelId, options: options })
                        break;
                    }
                }
            }
            else if(msg.exception) error({ message: msg.exception.message, details: 'JSON' })
        }

        function authenticate(signature, extra) { sendArgs(_AUTHENTICATE, signature, extra || {}) }

        function enable(type, uri, options, callback, onsuccess, onerror, oncancel)
        {
            requests[++requestId] =
            {
                callback: callback,
                onsuccess: function(callbackId)
                {
                    uris[uri] = callbackId
                    if(onsuccess) onsuccess(callbackId)
                },
                onerror: onerror,
                oncancel: oncancel
            }
            sendArgs(type, requestId, options, uri)
            return requestId
        }

        function disable(type, uri, onsuccess, onerror)
        {
            if(uri in uris)
            {
                var callbackId = uris[uri]
                cancels[callbackId] = callbacks[callbackId] = uris[uri] = null
                requests[++requestId] = { onsuccess: onsuccess, onerror: onerror }
                sendArgs(type, requestId, callbackId)
                return requestId
            }
        }

        function publish(uri, options, args, kwargs, onsuccess, onerror)
        {
            requests[++requestId] = { onsuccess: onsuccess, onerror: onerror }
            sendArgs(_PUBLISH, requestId, options, uri, args, kwargs)
            return requestId
        }

        function call(uri, options, args, kwargs, callback, onerror)
        {
            requests[++requestId] = { onerror: onerror }
            results[requestId] = callback
            sendArgs(_ws._CALL, requestId, options, uri, args, kwargs)
            return requestId
        }

        function cancel(id, options) { sendArgs(_ws._CANCEL, id, options) }
    }

    property var open: _ws.open
    property var authenticate: _ws.authenticate
    property var ping: _ws.ping
    property var close: _ws.close
    property var abort: _ws.abort
    property var subscribe: _ws.enable.bind(_ws, _ws._SUBSCRIBE)
    property var register: _ws.enable.bind(_ws, _ws._REGISTER)
    property var unsubscribe: _ws.disable.bind(_ws, _ws._UNSUBSCRIBE)
    property var unregister: _ws.disable.bind(_ws, _ws._UNREGISTER)
    property var publish: _ws.publish
    property var call: _ws.call
    property var cancel: _ws.cancel

    function pprint() { print(Array.prototype.slice.call(arguments).map(JSON.stringify)) }
}

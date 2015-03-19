import QtQuick 2.4
import QtQuick.Controls 1.3
import QtQuick.Window 2.2
import QtQuick.Dialogs 1.2
import Qt.WebSockets 1.0
import atnix.utils 1.0
import qmlwebsockets 1.0

Window
{
    width: 640
    height: 480

    WebSocketClient
    {
        //host: 'ws://echo.websocket.org'
        //port: 80
        host: 'ws://atnix'
        port: 8080

        onTextReceived: print('FRAME', frame)
        onPong: print('PONG', elapsedTime)
        onEncrypted: print('ENCRYPTED')
        onConnected:
        {
            print('CONNECTED')
            write("[1, 'integra-s', {'role': {'caller': {}}}]")
        }
        onDisconnected: print('DISCONNECTED')
        onError: print('ERROR', error)

        Component.onCompleted: connect()
    }

//    WampSocket
//    {
//        active: true
//        log: true
//        //url: 'ws://192.168.10.72:22222/ws'
//        //url: 'ws://192.168.10.26:8080/ws'
//        //url: 'ws://192.168.10.29:8080/ws'
//        url: 'ws://atnix:8080/ws'
//        //url: 'ws://echo.websocket.org'
//        realm: 'integra-s'
//        clientIsPublisher: true
//        clientIsSubscriber: true
//        clientIsCaller: true
//        clientIsCallee: true

//        onErrorStringChanged: if(errorString) print('ERROR', errorString)
//        onStatusChanged: print('STATUS', status)

//        onWelcome: pprint('welcome', sessionId, details, serverIsBroker, serverIsDealer)
//    }


    Component.onCompleted:
    {
    }


    /*********************************************/

    MessageDialog
    {
        id: _msg
        icon: StandardIcon.Information
        title: 'test message'

        function show()
        {
            if(arguments.length > 0)
            {
                _msg.text = arguments[0]
                _msg.informativeText = Array.prototype.slice.call(arguments, 1).map(JSON.stringify).join(', ')
                _msg.open()
            }
        }
    }

    function pprint() { print(Array.prototype.slice.call(arguments).map(JSON.stringify)) }
}

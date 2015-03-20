import QtQuick 2.4
import QtQuick.Controls 1.3
import QtQuick.Window 2.2
import QtQuick.Dialogs 1.2
import Qt.WebSockets 1.0
import qmlwebsockets 1.0

Window
{
    visible: true
    width: 640
    height: 480

    WampSocket
    {
        id: _ws
        log: true
        url: 'ws://192.168.10.26:8080/ws'
        realm: 'integra-s'
        username: 'admin'
        password: 'admin'

        onWelcome: pprint('WELCOME', params)
        onAbort: pprint('ABORT', params)
        onGoodbye: pprint('GOODBYE', params)
        onError: pprint('ERROR', params)
        onClosed: print('CLOSED')


        Component.onCompleted: open()
    }

    Button
    {
        text: 'CLOSE'
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        onClicked: _ws.close()
    }

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

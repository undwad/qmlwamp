import QtQuick 2.4
import QtQuick.Controls 1.3
import QtQuick.Window 2.2
import QtQuick.Dialogs 1.2
import qmlwebsockets 1.0

ApplicationWindow
{
    title: qsTr("Hello World")
    width: 640
    height: 480
    visible: true

    WampSocket
    {
        id: _ws
        log: true
        url: 'ws://192.168.10.26:8080/ws'
        realm: 'integra-s'
        username: 'admin'
        password: 'admin'

        onWelcome: pprint(details)
        onClosed: print('CLOSED')


        Component.onCompleted: open()
    }


    Button
    {
        text: 'CONNECT'
        anchors.centerIn: parent
        onClicked: _ws.open()

    }

    function pprint() { print(Array.prototype.slice.call(arguments).map(JSON.stringify)) }
}

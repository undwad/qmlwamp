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
        url: 'ws://echo.websocket.org'
        //url: 'ws://192.168.10.26:8080/ws'
        realm: 'integra-s'
        clientIsPublisher: true
        clientIsSubscriber: true
        clientIsCaller: true
        clientIsCallee: true

        Component.onCompleted: open()
    }


    Button
    {
        text: 'CONNECT'
        anchors.centerIn: parent
        onClicked: _ws.open()

    }

}

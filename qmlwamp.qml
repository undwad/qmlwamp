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
        dump: true
        url: 'ws://192.168.10.84:8080/ws'
        realm: 'integra-s'
        //compress: true
        property string username: 'admin'
        property string password: 'admin'

        onHeader: print('HEADER', header)
        onChallenge:
        {
            pprint('CHALLENGE', params)
            authenticate(username + ':' + password)
        }
        onWelcome: pprint('WELCOME', params)
        onAbort: pprint('ABORT', params)
        onGoodbye: pprint('GOODBYE', params)
        onError: pprint('ERROR', params)
        onClosed: print('CLOSED')


        Component.onCompleted: open()
    }

    Column
    {
        anchors.right: parent.right
        Button { text: 'OPEN'; onClicked: _ws.open() }
        Button { text: 'PING'; onClicked: _ws.ping() }
        Row
        {
            TextInput { id: _subscription; text: 'subscription' }
            Button
            {
                text: 'SUBSCRIBE'
                onClicked: _ws.subscribe
                (
                    _subscription.text,
                    {},
                    pprint.bind(null, text, 'EVENT'),
                    pprint.bind(null, text, 'SUCCEEDED'),
                    pprint.bind(null, text, 'FAILED')
                )
            }
            Button
            {
                text: 'UNSUBSCRIBE'
                onClicked: _ws.unsubscribe
                (
                    _subscription.text,
                   pprint.bind(null, text, 'SUCCEEDED'),
                   pprint.bind(null, text, 'FAILED')
                )
            }
            Button
            {
                text: 'PUBLISH'
                onClicked: _ws.publish
                (
                    _subscription.text,
                    {},
                    [],
                    {},
                    pprint.bind(null, text, 'SUCCEEDED'),
                    pprint.bind(null, text, 'FAILED')
                )
            }
        }

        Row
        {
            TextInput { id: _procedure; text: 'procedure' }
            Button
            {
                text: 'REGISTER'
                onClicked: _ws.register
                (
                    _procedure.text,
                    {},
                    function(params)
                    {
                        pprint(text, 'INVOKE', params)
                        params.yield({}, params.args, params.kwargs)
                    },
                    pprint.bind(null, text, 'SUCCEEDED'),
                    pprint.bind(null, text, 'FAILED')
                )
            }
            Button
            {
                text: 'UNREGISTER'
                onClicked: _ws.unregister
                (
                    _procedure.text,
                   pprint.bind(null, text, 'SUCCEEDED'),
                   pprint.bind(null, text, 'FAILED')
                )
            }
            Button
            {
                text: 'CALL'
                onClicked: _ws.call
                (
                    _procedure.text,
                    {},
                    [1,2,3],
                    {param1: true, param2: 'joder'},
                    pprint.bind(null, text, 'RESULT'),
                    pprint.bind(null, text, 'FAILED')
                )
            }

        }
        Button { text: 'CLOSE'; onClicked: _ws.close() }
        Button { text: 'ABORT'; onClicked: _ws.abort() }
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

/*
** websocket client for qml
** https://github.com/undwad/qmlwamp mailto:undwad@mail.ru
** see copyright notice in ./LICENCE
*/

#pragma once

#include <vector>
#include <QObject>
#include <QThread>
#include <QString>
#include <QQuickItem>
#include <QAbstractSocket>
#include <QTcpSocket>
#include <QSslSocket>
#include <QTextStream>
#include <QDataStream>
#include <QByteArray>
#include <QList>
#include <QSslError>
#include <QDebug>

//#include <gunzip.h>

#define EMIT_ERROR_AND_RETURN(MESSAGE, DETAILS, RESULT) \
    { \
        emit socketError(MESSAGE, DETAILS); \
        return RESULT; \
    }

class WebSocketWorker : public QObject
{
    Q_OBJECT

    enum ReadyState { CLOSING, CLOSED, CONNECTING, INITIALIZING, OPEN };

    struct wsheader_type
    {
        unsigned header_size;
        bool fin;
        bool mask;
        enum opcode_type
        {
            CONTINUATION = 0x0,
            TEXT_FRAME = 0x1,
            BINARY_FRAME = 0x2,
            CLOSE = 8,
            PING = 9,
            PONG = 0xa,
        } opcode;
        int N0;
        quint64 N;
        quint8 masking_key[4];
    };

signals:
    void stateChanged(int state);
    void headerReceived(const QString& header);
    void messageReceived(const QString& message);
    void socketError(const QString& message, const QString& details);

public slots:
    void open
    (
        const QString& url,
        const QString& key,
        const QString& origin,
        const QString& extensions,
        const QString& protocol,
        bool mask,
        bool ignoreSslErrors
    )
    {
        socket().abort();

        _mask = mask;
        _ignoreSslErrors = ignoreSslErrors;
        _perMessageDeflate = extensions.contains("permessage-deflate");
        _output_header = _input_header = QString();
        _input_data.clear();

        QUrl url_(url);
        if("ws" == url_.scheme()) _ssl = false;
        else if("wss" == url_.scheme()) _ssl = true;
        else EMIT_ERROR_AND_RETURN("invalid url scheme", url_.scheme(),);

        QTextStream(&_output_header, QIODevice::WriteOnly)
            << "GET " << url_.path() << " HTTP/1.1\r\n"
            << "Host: " << url_.host() << (80 == url_.port() ? "" : QString(":%1").arg(url_.port())) << "\r\n"
            << "Upgrade: websocket\r\n"
            << "Connection: Upgrade\r\n"
            << (origin.isEmpty() ? "" : QString("Origin: %1\r\n").arg(origin))
            << (extensions.isEmpty() ? "" : QString("Sec-WebSocket-Extensions: %1\r\n").arg(extensions))
            << (protocol.isEmpty() ? "" : QString("Sec-WebSocket-Protocol: %1\r\n").arg(protocol))
            << "Sec-WebSocket-Key: " << key << "\r\n"
            << "Sec-WebSocket-Version: 13\r\n"
            << "\r\n";

#       if defined(QT_NO_SSL)
        if(_ssl) EMIT_ERROR_AND_RETURN("ssl not supported", "rebuild qt with openssl",);
#       endif

        emit stateChanged(_state = ReadyState::CONNECTING);

#       if !defined(QT_NO_SSL)
        if(_ssl) _sslsocket.connectToHostEncrypted(url_.host(), url_.port());
        else
#       endif
        _socket.connectToHost(url_.host(), url_.port());

    }

    void ping() { sendData(wsheader_type::PING, QByteArray()); }

    void send(const QString& message) { sendData(wsheader_type::TEXT_FRAME, message.toUtf8()); }

    void close()
    {
        if (ReadyState::OPEN != _state) return;
        emit stateChanged(_state = ReadyState::CLOSING);
        quint8 closeFrame[6] = {0x88, 0x80, 0x00, 0x00, 0x00, 0x00};
        std::vector<quint8> buffer(closeFrame, closeFrame+6);
        socket().write((char*)buffer.data(), buffer.size());
    }

    void abort() { socket().abort(); }

public:
    WebSocketWorker()
    {
        connect(&_socket, &QTcpSocket::connected, this, &WebSocketWorker::connected);
        connect(&_socket, &QTcpSocket::readyRead, this, &WebSocketWorker::readyRead);
        connect(&_socket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(error(QAbstractSocket::SocketError)));
        connect(&_socket, &QTcpSocket::aboutToClose, this, &WebSocketWorker::aboutToClose);
        connect(&_socket, &QTcpSocket::disconnected, this, &WebSocketWorker::disconnected);
#       if !defined(QT_NO_SSL)
        connect(&_sslsocket, &QSslSocket::connected, this, &WebSocketWorker::handshake);
        connect(&_sslsocket, &QSslSocket::encrypted, this, &WebSocketWorker::connected);
        connect(&_sslsocket, &QSslSocket::readyRead, this, &WebSocketWorker::readyRead);
        connect(&_sslsocket,SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(error(QAbstractSocket::SocketError)));
        connect(&_sslsocket, &QSslSocket::aboutToClose, this, &WebSocketWorker::aboutToClose);
        connect(&_sslsocket, &QSslSocket::disconnected, this, &WebSocketWorker::disconnected);
        connect(&_sslsocket, SIGNAL(sslErrors(const QList<QSslError>&)), this, SLOT(sslErrors(const QList<QSslError>&)));
#       endif

    }

    void moveToThread(QThread *thread)
    {
        QObject::moveToThread(thread);
        _socket.moveToThread(thread);
#       if !defined(QT_NO_SSL)
        _sslsocket.moveToThread(thread);
#       endif
    }

private slots:
#   if !defined(QT_NO_SSL)
    void handshake() { if(_ignoreSslErrors) _sslsocket.ignoreSslErrors(); }
#   endif

    void connected()
    {
        emit stateChanged(_state = ReadyState::INITIALIZING);
        socket().setSocketOption(QAbstractSocket::SocketOption::LowDelayOption, QVariant(1));
        socket().write(_output_header.toUtf8());
    }

    void readyRead()
    {
        switch(_state)
        {
        case ReadyState::INITIALIZING:
        {
            while(socket().canReadLine())
            {
                QString line = socket().readLine();
                if("\r\n" == line)
                {
                    emit stateChanged(_state = ReadyState::OPEN);
                    emit headerReceived(_input_header);
                }
                else QTextStream(&_input_header, QIODevice::WriteOnly) << line;
            }
            break;
        }
        case ReadyState::OPEN:
        {
            QByteArray packet = socket().readAll();
            _input_data.insert(_input_data.end(), packet.begin(), packet.end());
            parseInputData();
            break;
        }
        }
    }

#   if !defined(QT_NO_SSL)
    void sslErrors(const QList<QSslError>& errors)
    {
        QString message;
        QTextStream stream(&message);
        for(const QSslError& error : errors) stream << error.errorString() << "\r\n";
        emit socketError(message, "ssl");
    }
#   endif

    void error(QAbstractSocket::SocketError code) { emit socketError(socket().errorString(), QString(code)); }
    void aboutToClose() { emit stateChanged(_state = ReadyState::CLOSING); }
    void disconnected() { emit stateChanged(_state = ReadyState::CLOSED); }

private:
    QTcpSocket _socket;
#if !defined(QT_NO_SSL)
    QSslSocket _sslsocket;
#endif
    bool _ssl = false;
    bool _ignoreSslErrors = false;
    bool _perMessageDeflate = false;
    QString _output_header;
    QString _input_header;
    ReadyState _state = ReadyState::CLOSED;
    bool _mask;
    std::vector<qint8> _input_data;

    inline QAbstractSocket& socket()
    {
#   if !defined(QT_NO_SSL)
        if(_ssl) return _sslsocket;
#   endif
        return _socket;
    }

    void sendData(wsheader_type::opcode_type type, QByteArray data)
    {
        const quint8 masking_key[4] = { 0x12, 0x34, 0x56, 0x78 };

        if (ReadyState::OPEN != _state) return;

        std::vector<quint8> buffer;
        qint64 size = data.size();
        buffer.assign(2 + (size >= 126 ? 2 : 0) + (size >= 65536 ? 6 : 0) + (_mask ? 4 : 0), 0);
        buffer[0] = 0x80 | type;
        if (size < 126)
        {
            buffer[1] = (size & 0xff) | (_mask ? 0x80 : 0);
            if (_mask)
            {
                buffer[2] = masking_key[0];
                buffer[3] = masking_key[1];
                buffer[4] = masking_key[2];
                buffer[5] = masking_key[3];
            }
        }
        else if (size < 65536)
        {
            buffer[1] = 126 | (_mask ? 0x80 : 0);
            buffer[2] = (size >> 8) & 0xff;
            buffer[3] = (size >> 0) & 0xff;
            if (_mask)
            {
                buffer[4] = masking_key[0];
                buffer[5] = masking_key[1];
                buffer[6] = masking_key[2];
                buffer[7] = masking_key[3];
            }
        }
        else
        {
            buffer[1] = 127 | (_mask ? 0x80 : 0);
            buffer[2] = (size >> 56) & 0xff;
            buffer[3] = (size >> 48) & 0xff;
            buffer[4] = (size >> 40) & 0xff;
            buffer[5] = (size >> 32) & 0xff;
            buffer[6] = (size >> 24) & 0xff;
            buffer[7] = (size >> 16) & 0xff;
            buffer[8] = (size >>  8) & 0xff;
            buffer[9] = (size >>  0) & 0xff;
            if (_mask)
            {
                buffer[10] = masking_key[0];
                buffer[11] = masking_key[1];
                buffer[12] = masking_key[2];
                buffer[13] = masking_key[3];
            }
        }

        buffer.insert(buffer.end(), data.begin(), data.end());

        if (_mask)
            for (size_t i = 0; i != size; ++i)
                *(buffer.end() - size + i) ^= masking_key[i&0x3];

        socket().write((char*)buffer.data(), buffer.size());
    }

    void parseInputData()
    {
        while (true)
        {
            wsheader_type ws;
            if (_input_data.size() < 2) return; // Need at least 2
            const quint8 * data = (quint8*) &_input_data[0]; // peek, but don't consume
            ws.fin = (data[0] & 0x80) == 0x80;
            ws.opcode = (wsheader_type::opcode_type) (data[0] & 0x0f);
            ws.mask = (data[1] & 0x80) == 0x80;
            ws.N0 = (data[1] & 0x7f);
            ws.header_size = 2 + (ws.N0 == 126? 2 : 0) + (ws.N0 == 127? 8 : 0) + (ws.mask? 4 : 0);
            if (_input_data.size() < ws.header_size) return; // Need: ws.header_size - _input_data.size()
            int i;
            if (ws.N0 < 126)
            {
                ws.N = ws.N0;
                i = 2;
            }
            else if (ws.N0 == 126)
            {
                ws.N = 0;
                ws.N |= ((quint64) data[2]) << 8;
                ws.N |= ((quint64) data[3]) << 0;
                i = 4;
            }
            else if (ws.N0 == 127)
            {
                ws.N = 0;
                ws.N |= ((quint64) data[2]) << 56;
                ws.N |= ((quint64) data[3]) << 48;
                ws.N |= ((quint64) data[4]) << 40;
                ws.N |= ((quint64) data[5]) << 32;
                ws.N |= ((quint64) data[6]) << 24;
                ws.N |= ((quint64) data[7]) << 16;
                ws.N |= ((quint64) data[8]) << 8;
                ws.N |= ((quint64) data[9]) << 0;
                i = 10;
            }
            if (ws.mask)
            {
                ws.masking_key[0] = ((quint8) data[i+0]) << 0;
                ws.masking_key[1] = ((quint8) data[i+1]) << 0;
                ws.masking_key[2] = ((quint8) data[i+2]) << 0;
                ws.masking_key[3] = ((quint8) data[i+3]) << 0;
            }
            else
            {
                ws.masking_key[0] = 0;
                ws.masking_key[1] = 0;
                ws.masking_key[2] = 0;
                ws.masking_key[3] = 0;
            }
            if (_input_data.size() < ws.header_size+ws.N) return; // Need: ws.header_size+ws.N - _input_data.size()

            // We got a whole message, now do something with it:
            if
            (
                ws.opcode == wsheader_type::TEXT_FRAME
                ||
                ws.opcode == wsheader_type::BINARY_FRAME
                ||
                ws.opcode == wsheader_type::CONTINUATION
            )
            {
                if (ws.mask) for (size_t i = 0; i != ws.N; ++i) _input_data[i+ws.header_size] ^= ws.masking_key[i&0x3];
                QByteArray message((char*)_input_data.data()+ws.header_size, ws.N);
                if(_perMessageDeflate)
                {
                    //QByteArray decompressed;
                    //GUnzip::gzipDecompress(message, decompressed);
                    //emit messageReceived(QString::fromUtf8(decompressed));
                    emit socketError("compression not supported", "websockets");
                }
                else emit messageReceived(QString::fromUtf8(message));
            }
            else if (ws.opcode == wsheader_type::PING)
            {
                if (ws.mask) for (size_t i = 0; i != ws.N; ++i) _input_data[i+ws.header_size] ^= ws.masking_key[i&0x3];
                sendData(wsheader_type::PONG, QByteArray((char*)_input_data.data()+ws.header_size, ws.N));
            }
            else if (ws.opcode == wsheader_type::PONG) ;
            else if (ws.opcode == wsheader_type::CLOSE) close();
            else
            {
                emit socketError("invalid websocket message", "websockets");
                close();
            }

            _input_data.erase(_input_data.begin(), _input_data.begin() + ws.header_size+(size_t)ws.N);
        }
    }
};

class WebSocketClient : public QObject
{
public:
    enum ReadyState { CLOSING, CLOSED, CONNECTING, INITIALIZING, OPEN };

    Q_OBJECT

    Q_ENUMS(ReadyState)

    Q_PROPERTY(QString url MEMBER _url)
    Q_PROPERTY(QString origin MEMBER _origin)
    Q_PROPERTY(QString extensions MEMBER _extensions)
    Q_PROPERTY(QString protocol MEMBER _protocol)
    Q_PROPERTY(QString key MEMBER _key)
    Q_PROPERTY(bool mask MEMBER _mask)
    Q_PROPERTY(bool ignoreSslErrors MEMBER _ignoreSslErrors)
    Q_PROPERTY(ReadyState state READ state NOTIFY stateChanged)

    Q_DISABLE_COPY(WebSocketClient)

signals:
    void toOpen
    (
        const QString& url,
        const QString& key,
        const QString& origin,
        const QString& extensions,
        const QString& protocol,
        bool mask,
        bool ignoreSslErrors
    );
    void toPing();
    void toSend(const QString& message);
    void toClose();
    void toAbort();

    void stateChanged(ReadyState state);
    void messageReceived(const QString& text);
    void socketError(const QString& message, const QString& details);
    void headerReceived(const QString& header);

public:
    WebSocketClient(QQuickItem *parent = 0) : QObject(parent), _worker(new WebSocketWorker)
    {
        _worker->moveToThread(&_thread);
        connect(&_thread, &QThread::finished, _worker, &QObject::deleteLater);
        connect(_worker, &WebSocketWorker::stateChanged, this, &WebSocketClient::onStateChanged);
        connect(_worker, &WebSocketWorker::headerReceived, this, &WebSocketClient::onHeaderReceived);
        connect(_worker, &WebSocketWorker::messageReceived, this, &WebSocketClient::onMessageReceived);
        connect(_worker, &WebSocketWorker::socketError, this, &WebSocketClient::onSocketError);
        connect(this, &WebSocketClient::toOpen, _worker, &WebSocketWorker::open);
        connect(this, &WebSocketClient::toPing, _worker, &WebSocketWorker::ping);
        connect(this, &WebSocketClient::toSend, _worker, &WebSocketWorker::send);
        connect(this, &WebSocketClient::toClose, _worker, &WebSocketWorker::close);
        connect(this, &WebSocketClient::toAbort, _worker, &WebSocketWorker::abort);
        _thread.start();
    }

    ReadyState state() const { return _state; }

    ~WebSocketClient()
    {
        _thread.quit();
        _thread.wait();
    }

public slots:
    void open() { emit toOpen(_url, _key, _origin, _extensions, _protocol, _mask, _ignoreSslErrors); }
    void ping() { emit toPing(); }
    void send(const QString& text) { emit toSend(text); }
    void close() { emit toClose(); }
    void abort() { emit toAbort(); }

private slots:
    void onStateChanged(int state) { if((ReadyState)state != _state) emit stateChanged(_state = (ReadyState)state); }
    void onHeaderReceived(const QString& header) { emit headerReceived(header); }
    void onMessageReceived(const QString& message) { emit messageReceived(message); }
    void onSocketError(const QString& message, const QString& details) { emit socketError(message, details); }

private:
    WebSocketWorker* _worker;
    QThread _thread;
    QString _url;
    QString _origin;
    QString _extensions;
    QString _protocol;
    QString _key;
    bool _mask = true;
    bool _ignoreSslErrors = true;
    ReadyState _state = ReadyState::CLOSED;
};

#undef EMIT_ERROR_AND_RETURN

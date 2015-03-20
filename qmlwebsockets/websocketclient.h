#ifndef WEBSOCKETCLIENT_H
#define WEBSOCKETCLIENT_H

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
    void open(const QString& url, const QString& origin, const QString& extensions, const QString& protocol, bool mask)
    {
        close();

        _mask = mask;
        _input_header = QString();

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
            << "Sec-WebSocket-Key: x3JJHMbDL1EzLkh9GBhXDw==\r\n"
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
            qDebug() << socket().readAll();
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
    QString _output_header;
    QString _input_header;
    ReadyState _state = ReadyState::CLOSED;
    bool _mask;

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
    Q_PROPERTY(bool mask MEMBER _mask)
    Q_PROPERTY(ReadyState state READ state NOTIFY stateChanged)

    Q_DISABLE_COPY(WebSocketClient)

signals:
    void toOpen(const QString& url, const QString& origin, const QString& extensions, const QString& protocol, bool mask);
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
    void open() { emit toOpen(_url, _origin, _extensions, _protocol, _mask); }
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
    bool _mask = true;
    ReadyState _state = ReadyState::CLOSED;
};

#endif // WEBSOCKETCLIENT_H


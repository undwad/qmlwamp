#ifndef WEBSOCKETCLIENT_H
#define WEBSOCKETCLIENT_H

#include <QObject>
#include <QThread>
#include <QQuickItem>
#include <QAbstractSocket>
#include <QTcpSocket>
#include <QSslSocket>
#include <QTextStream>
#include <QDebug>

#define EMIT_ERROR_AND_RETURN(MESSAGE, DETAILS, RESULT) \
    { \
        emit socketError(MESSAGE, DETAILS); \
        return RESULT; \
    }

enum WebSocketReadyState { CLOSING, CLOSED, CONNECTING, INITIALIZING, OPEN };

Q_DECLARE_METATYPE(WebSocketReadyState)

class WebSocketWorker : public QObject
{
    Q_OBJECT

signals:
    void stateChanged(WebSocketReadyState state);
    void received(const QString& message);
    void socketError(const QString& message, const QString& details);

public slots:
    void open(const QString& url, const QString& origin, const QString& extensions, const QString& protocol)
    {
        close();

        QUrl url_(url);
        if("ws" == url_.scheme()) _ssl = false;
        else if("wss" == url_.scheme()) _ssl = true;
        else EMIT_ERROR_AND_RETURN("invalid url scheme", url_.scheme(),);

        QString header;
        QTextStream(&header)
            << "GET " << url_.path() << " HTTP/1.1\r\n"
            << "Host: " << url_.host() << (80 == url_.port() ? "" : QString(":%1").arg(url_.port())) << "\r\n"
            << "Upgrade: websocket\r\n"
            << "Connection: Upgrade\r\n"
            << (origin.isEmpty() ? "" : QString("Origin: %1\r\n").arg(origin))
            << (extensions.isEmpty() ? "" : QString("Sec-WebSocket-Extensions: %1\r\n").arg(extensions))
            << (protocol.isEmpty() ? "" : QString("Sec-WebSocket-Protocol: %1\r\n").arg(protocol))
            << "Sec-WebSocket-Key: x3JJHMbDL1EzLkh9GBhXDw==\r\n"
            << "Sec-WebSocket-Version: 13\r\n";

#       if defined(QT_NO_SSL)
        if(_ssl) EMIT_ERROR_AND_RETURN("ssl not supported", "rebuild qt with openssl",);
#       endif

        emit stateChanged(_state = WebSocketReadyState::CONNECTING);

#       if !defined(QT_NO_SSL)
        if(_ssl) _sslsocket.connectToHostEncrypted(url_.host(), url_.port());
        else
#       endif
        _socket.connectToHost(url_.host(), url_.port());

    }

    void ping() {  }

    void send(const QString& message)
    {
    }

    void close() { socket().close(); }

public:
    WebSocketWorker()
    {
        connect(&_socket, &QTcpSocket::connected, this, &WebSocketWorker::connected);
        connect(&_socket, &QTcpSocket::readyRead, this, &WebSocketWorker::readyRead);
        connect(&_socket, &QTcpSocket::aboutToClose, this, &WebSocketWorker::aboutToClose);
        connect(&_socket, &QTcpSocket::disconnected, this, &WebSocketWorker::disconnected);
#       if !defined(QT_NO_SSL)
        connect(&_sslsocket, &QSslSocket::encrypted, this, &WebSocketWorker::connected);
        connect(&_sslsocket, &QSslSocket::readyRead, this, &WebSocketWorker::readyRead);
        connect(&_sslsocket, &QSslSocket::aboutToClose, this, &WebSocketWorker::aboutToClose);
        connect(&_sslsocket, &QSslSocket::disconnected, this, &WebSocketWorker::disconnected);
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
        emit stateChanged(_state = WebSocketReadyState::INITIALIZING);
        socket().write(_header.toUtf8());
    }

    void readyRead()
    {
        switch(_state)
        {
        case WebSocketReadyState::INITIALIZING:
        {
            qDebug() << QString::fromUtf8(socket().readAll());
            break;
        }
        case WebSocketReadyState::OPEN:
        {
            break;
        }
        }
    }

    void aboutToClose() { emit stateChanged(_state = WebSocketReadyState::CLOSING); }
    void disconnected() { emit stateChanged(_state = WebSocketReadyState::CLOSED); }

private:
    QTcpSocket _socket;
#if !defined(QT_NO_SSL)
    QSslSocket _sslsocket;
#endif
    bool _ssl = false;
    QString _header;
    WebSocketReadyState _state = WebSocketReadyState::CLOSED;

    inline QAbstractSocket& socket()
    {
#   if !defined(QT_NO_SSL)
        if(_ssl) return _sslsocket;
#   endif
        return _socket;
    }

};

class WebSocketClient : public QObject
{
    Q_OBJECT

    Q_ENUMS(WebSocketReadyState)

    Q_PROPERTY(QString url MEMBER _url)
    Q_PROPERTY(QString origin MEMBER _origin)
    Q_PROPERTY(QString extensions MEMBER _extensions)
    Q_PROPERTY(QString protocol MEMBER _protocol)
    Q_PROPERTY(WebSocketReadyState state READ state NOTIFY stateChanged)

    Q_DISABLE_COPY(WebSocketClient)

signals:
    void toOpen(const QString& url, const QString& origin, const QString& extensions, const QString& protocol);
    void toPing();
    void toSend(const QString& message);
    void toClose();

    void stateChanged(WebSocketReadyState state);
    void received(const QString& text);
    void socketError(const QString& message, const QString& details);

public:
    WebSocketClient(QQuickItem *parent = 0) : QObject(parent), _worker(new WebSocketWorker)
    {
        _worker->moveToThread(&_thread);
        connect(&_thread, &QThread::finished, _worker, &QObject::deleteLater);
        connect(_worker, &WebSocketWorker::stateChanged, this, &WebSocketClient::onStateChanged);
        connect(_worker, &WebSocketWorker::received, this, &WebSocketClient::onReceived);
        connect(_worker, &WebSocketWorker::socketError, this, &WebSocketClient::onSocketError);
        connect(this, &WebSocketClient::toOpen, _worker, &WebSocketWorker::open);
        connect(this, &WebSocketClient::toPing, _worker, &WebSocketWorker::ping);
        connect(this, &WebSocketClient::toSend, _worker, &WebSocketWorker::send);
        connect(this, &WebSocketClient::toClose, _worker, &WebSocketWorker::close);
        _thread.start();
    }

    WebSocketReadyState state() const { return _state; }

    ~WebSocketClient()
    {
        _thread.quit();
        _thread.wait();
    }

public slots:
    void open() { emit toOpen(_url, _origin, _extensions, _protocol); }
    void ping() { emit toPing(); }
    void send(const QString& text) { emit toSend(text); }
    void close() { emit toClose(); }

private slots:
    void onStateChanged(WebSocketReadyState state) { if(state != _state) emit stateChanged(_state = state); }
    void onReceived(const QString& message) { emit received(message); }
    void onSocketError(const QString& message, const QString& details) { emit socketError(message, details); }

private:
    WebSocketWorker* _worker;
    QThread _thread;
    QString _url;
    QString _origin;
    QString _extensions;
    QString _protocol;
    WebSocketReadyState _state = WebSocketReadyState::CLOSED;
};

#endif // WEBSOCKETCLIENT_H

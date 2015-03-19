#ifndef WEBSOCKETCLIENT_H
#define WEBSOCKETCLIENT_H

#include <QObject>
#include <QThread>
#include <QQuickItem>
#include <QSharedPointer>
#include <QDebug>

#include "easywsclient.hpp"

class WebSocketWorker : public QObject
{
    Q_OBJECT

signals:
    void toPoll();
    void stateChanged(int state);
    void message(const QString& msg);

public slots:
    void open(const QString& url, const QString& origin, const QString& extensions, const QString& protocol)
    {
        emit stateChanged(_state = easywsclient::WebSocket::CLOSED);
        _ws = QSharedPointer<easywsclient::WebSocket>(easywsclient::WebSocket::from_url(url.toStdString(), origin.toStdString(), extensions.toStdString(), protocol.toStdString()));
        if(_ws) emit toPoll();
    }

    void ping() { _ws->sendPing(); }

    void send(const QString& msg)
    {
        qDebug() << "send begin";
        _ws->send(msg.toStdString());
        qDebug() << "send end";
    }

    void close() { _ws->close(); }

public:
    WebSocketWorker()
    {
        connect(this, &WebSocketWorker::toPoll, this, &WebSocketWorker::poll, Qt::ConnectionType::QueuedConnection);
    }

private slots:
    void poll()
    {
        auto state = _ws->getReadyState();
        if(state != _state) emit stateChanged(_state = state);
        if(easywsclient::WebSocket::CLOSED != _state)
        {
            _ws->poll();
            _ws->dispatch([this](const std::string& msg) { emit message(QString::fromStdString(msg)); });
            emit toPoll();
        }
    }

private:
    QSharedPointer<easywsclient::WebSocket> _ws;
    easywsclient::WebSocket::readyStateValues _state = easywsclient::WebSocket::CLOSED;
};

class WebSocketClient : public QObject
{
    Q_OBJECT

    Q_ENUMS(ReadyState)

    Q_PROPERTY(QString url MEMBER _url)
    Q_PROPERTY(QString origin MEMBER _origin)
    Q_PROPERTY(QString extensions MEMBER _extensions)
    Q_PROPERTY(QString protocol MEMBER _protocol)
    Q_PROPERTY(ReadyState state READ state NOTIFY stateChanged)

    Q_DISABLE_COPY(WebSocketClient)

public:
    enum ReadyState { CLOSING, CLOSED, CONNECTING, OPEN };

signals:
    void toOpen(const QString& url, const QString& origin, const QString& extensions, const QString& protocol);
    void toPing();
    void toSend(const QString& msg);
    void toClose();

    void stateChanged(ReadyState state);
    void message(const QString& text);

public:
    WebSocketClient(QQuickItem *parent = 0) : QObject(parent), _worker(new WebSocketWorker)
    {
        _worker->moveToThread(&_thread);
        connect(&_thread, &QThread::finished, _worker, &QObject::deleteLater);
        connect(_worker, &WebSocketWorker::stateChanged, this, &WebSocketClient::onStateChanged);
        connect(_worker, &WebSocketWorker::message, this, &WebSocketClient::onMessage);
        connect(this, &WebSocketClient::toOpen, _worker, &WebSocketWorker::open);
        connect(this, &WebSocketClient::toPing, _worker, &WebSocketWorker::ping);
        connect(this, &WebSocketClient::toSend, _worker, &WebSocketWorker::send);
        connect(this, &WebSocketClient::toClose, _worker, &WebSocketWorker::close);
        _thread.start();
    }

    ReadyState state() const { return _state; }

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
    void onStateChanged(int state)
    {
        if((ReadyState)state != _state) emit stateChanged(_state = (ReadyState)state);
    }

    void onMessage(const QString& msg) { emit message(msg); }

private:
    WebSocketWorker* _worker;
    QThread _thread;
    QString _url;
    QString _origin;
    QString _extensions;
    QString _protocol;
    ReadyState _state = ReadyState::CLOSED;
};

#endif // WEBSOCKETCLIENT_H


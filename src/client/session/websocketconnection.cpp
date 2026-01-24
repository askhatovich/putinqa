#include "websocketconnection.h"

#include <QDebug>
#include <QNetworkCookie>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTimer>

namespace {
constexpr auto MAX_RECONNECTS = 30;
}

WebSocketConnection::WebSocketConnection(QNetworkCookieJar *cookieJar, const QUrl& url, QObject *parent)
    : QObject{parent}
    , m_ws(new QWebSocket(QString(), QWebSocketProtocol::VersionLatest, this))
    , m_url(url)
    , m_cookieJar(cookieJar)
    , m_reconnectTtl(MAX_RECONNECTS)
{
    m_url.setPath("/api/ws");
    m_url.setScheme(m_url.scheme() == "https" ? "wss" : "ws");

    QObject::connect(m_ws, &QWebSocket::binaryMessageReceived, this, &WebSocketConnection::onBinaryMessageReceived);
    QObject::connect(m_ws, &QWebSocket::textMessageReceived, this, &WebSocketConnection::onTextMessageReceived);
    QObject::connect(m_ws, &QWebSocket::connected, this, &WebSocketConnection::onConnected);
    QObject::connect(m_ws, &QWebSocket::disconnected, this, &WebSocketConnection::onDisconnected);
    QObject::connect(m_ws, &QWebSocket::errorOccurred, this, &WebSocketConnection::onWsError);
}

void WebSocketConnection::connect()
{
    const auto cookies = m_cookieJar->cookiesForUrl(m_url);
    if (cookies.isEmpty()) {
        emit error("No cookies found");
        return;
    }

    QNetworkRequest request(m_url);

    QStringList cookieStrings;
    for (const QNetworkCookie& cookie : cookies) {
        cookieStrings << QString::fromUtf8(cookie.name()) + "=" + QString::fromUtf8(cookie.value());
    }

    QString cookieHeader = cookieStrings.join("; ");
    request.setRawHeader("Cookie", cookieHeader.toUtf8());

    m_ws->open(request);
}

void WebSocketConnection::sendBinary(const QByteArray &data)
{
    const auto sent = m_ws->sendBinaryMessage(data);
    if (sent != data.size()) {
        qWarning() << "WebSocketConnection::sendBinary data size is" << data.size() << "but sent" << sent;
    }
}

void WebSocketConnection::sendText(const QString &message)
{
    m_ws->sendTextMessage(message);
}

void WebSocketConnection::onConnected()
{
    if (m_reconnectTtl.attemptsLeft() != MAX_RECONNECTS) {
        m_reconnectTtl.release();
    }

    emit connected();
}

void WebSocketConnection::onDisconnected()
{
    if (m_reconnectTtl.attemptsLeft() == 0) {
        qDebug() << "WebSocketConnection::onDisconnected";
        emit disconnected();
    }
}

void WebSocketConnection::onWsError(QAbstractSocket::SocketError error)
{
    if (m_reconnectTtl.isAliveIfIncrement()) {
        QTimer::singleShot(0, this, &WebSocketConnection::connect);
        emit reconnecting(m_reconnectTtl.attemptsLeft());
        return;
    }

    QString description;
    if (error == QAbstractSocket::RemoteHostClosedError) {
        description = "Remote host closed connection";
    } else if (error == QAbstractSocket::NetworkError) {
        description = "Network error occurred";
    } else if (error == QAbstractSocket::ConnectionRefusedError) {
        description = "Connection refused";
    }
    emit WebSocketConnection::error("WebSocket closed: " + description);
}

void WebSocketConnection::onTextMessageReceived(const QString &string)
{
    emit newTextMessage(string);
}

void WebSocketConnection::onBinaryMessageReceived(const QByteArray &data)
{
    emit newBinaryMessage(data);
}

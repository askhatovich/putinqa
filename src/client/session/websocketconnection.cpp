// Copyright (C) 2026  Roman Lyubimov
// SPDX-License-Identifier: GPL-3.0-or-later
// For full license text, see <https://www.gnu.org/licenses/gpl-3.0.txt>

#include "websocketconnection.h"

#include <QDebug>
#include <QNetworkCookie>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTimer>

namespace {
constexpr auto MAX_RECONNECTS = 30;
constexpr auto RECONNECT_DELAY_MS = 2000;
}

WebSocketConnection::WebSocketConnection(const QSharedPointer<QNetworkCookieJar> cookieJar, const QUrl& url, QObject *parent)
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
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    QObject::connect(m_ws, &QWebSocket::errorOccurred, this, &WebSocketConnection::onWsError);
#else
    QObject::connect(m_ws, QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::error),
                     this, &WebSocketConnection::onWsError);
#endif
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
    const auto sent = m_ws->sendTextMessage(message);
    if (sent == 0) {
        qWarning() << "WebSocketConnection::sendText data was not sent";
    }
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
    auto closeCode = m_ws->closeCode();

    // Normal close (1000) or server-initiated close — don't reconnect
    bool normalClose = (closeCode == QWebSocketProtocol::CloseCodeNormal ||
                        closeCode == QWebSocketProtocol::CloseCodeGoingAway);

    if (normalClose) {
        emit disconnected(true);
        return;
    }

    // Abnormal close — try reconnect if TTL allows
    if (m_reconnectTtl.attemptsLeft() > 0) {
        if (m_reconnectTtl.isAliveIfIncrement()) {
            emit reconnecting(m_reconnectTtl.attemptsLeft());
            QTimer::singleShot(RECONNECT_DELAY_MS, this, &WebSocketConnection::connect);
            return;
        }
    }

    emit disconnected(false);
}

void WebSocketConnection::onWsError(QAbstractSocket::SocketError wsError)
{
    qWarning() << "WebSocket error:" << wsError << m_ws->errorString();

    // RemoteHostClosedError during active session = server closed intentionally
    if (wsError == QAbstractSocket::RemoteHostClosedError) {
        // Don't reconnect — the disconnect handler will emit disconnected
        // after processing the close frame
        return;
    }

    // For connection-level errors (refused, network), try reconnect
    if (wsError == QAbstractSocket::ConnectionRefusedError ||
        wsError == QAbstractSocket::NetworkError ||
        wsError == QAbstractSocket::HostNotFoundError) {
        if (m_reconnectTtl.isAliveIfIncrement()) {
            emit reconnecting(m_reconnectTtl.attemptsLeft());
            QTimer::singleShot(RECONNECT_DELAY_MS, this, &WebSocketConnection::connect);
            return;
        }
    }

    emit error("WebSocket error: " + m_ws->errorString());
}

void WebSocketConnection::onTextMessageReceived(const QString &string)
{
    emit newTextMessage(string);
}

void WebSocketConnection::onBinaryMessageReceived(const QByteArray &data)
{
    emit newBinaryMessage(data);
}

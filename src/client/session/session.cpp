// Copyright (C) 2026  Roman Lyubimov
// SPDX-License-Identifier: GPL-3.0-or-later
// For full license text, see <https://www.gnu.org/licenses/gpl-3.0.txt>

#include "session.h"
#include "actions.h"

#include <QJsonObject>
#include <QJsonDocument>
#include <QDebug>
#include <QTimer>

namespace {
constexpr auto NETWORK_TIMEOUT_SECS = 10;
}

Session::Session(const QUrl &url, const QSharedPointer<QNetworkCookieJar> cookieJar, QObject *parent)
    : QObject{parent}
    , m_url(url)
    , m_cookieJar(cookieJar)
    , m_state(new SessionState(this))
    , m_downloadManager(new QNetworkAccessManager(this))
{
    m_downloadManager->setTransferTimeout(NETWORK_TIMEOUT_SECS * 1000);
    m_downloadManager->setCookieJar(m_cookieJar.data());
    m_cookieJar->setParent(nullptr);

    QObject::connect(this, &Session::joined, this, &Session::onJoined);
    QObject::connect(m_state, &SessionState::updated, this, &Session::stateUpdated);
    QObject::connect(m_state, &SessionState::complete, this, &Session::onComplete);
}

void Session::join(const QString &id)
{
    m_role = Role::receiver;

    QUrl url(m_url);
    url.setPath("/api/session/join");
    url.setQuery(QStringLiteral("id=%1").arg(id));

    auto *manager = new QNetworkAccessManager(this);
    manager->setTransferTimeout(NETWORK_TIMEOUT_SECS * 1000);
    manager->setCookieJar(m_cookieJar.data());
    m_cookieJar->setParent(nullptr);

    auto *reply = manager->get(QNetworkRequest(url));
    QObject::connect(reply, &QNetworkReply::finished, this, [this, manager, reply]() {
        processReply(reply);
        manager->deleteLater();
    });
}

void Session::create(bool autoDropFreeze)
{
    m_role = Role::sender;

    QUrl url(m_url);
    url.setPath("/api/session/create");

    auto *manager = new QNetworkAccessManager(this);
    manager->setTransferTimeout(NETWORK_TIMEOUT_SECS * 1000);
    manager->setCookieJar(m_cookieJar.data());
    m_cookieJar->setParent(nullptr);

    QNetworkRequest request(url);
    QByteArray body;
    if (autoDropFreeze) {
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
        body = QJsonDocument(QJsonObject{{"auto_drop_freeze", true}}).toJson(QJsonDocument::Compact);
    }

    auto *reply = manager->post(request, body);
    QObject::connect(reply, &QNetworkReply::finished, this, [this, manager, reply]() {
        processReply(reply);
        manager->deleteLater();
    });
}

void Session::sendJsonMessage(const QJsonObject &json)
{
    if (!m_wsConnection) {
        qWarning() << "Session::sendJsonMessage: no WebSocket";
        return;
    }
    m_wsConnection->sendText(QJsonDocument(json).toJson(QJsonDocument::Compact));
}

void Session::sendBinaryMessage(const QByteArray &data)
{
    if (!m_wsConnection) {
        qWarning() << "Session::sendBinaryMessage: no WebSocket";
        return;
    }
    m_wsConnection->sendBinary(data);
}

void Session::downloadChunkHttp(qint64 index)
{
    QUrl url(m_url);
    url.setPath("/api/session/chunk");
    url.setQuery(QStringLiteral("id=%1").arg(index));

    auto *reply = m_downloadManager->get(QNetworkRequest(url));
    QObject::connect(reply, &QNetworkReply::finished, this, [this, reply, index]() {
        const int code = reply->attribute(QNetworkRequest::Attribute::HttpStatusCodeAttribute).toInt();
        if (code == 200) {
            emit chunkDataReceived(index, reply->readAll());
        } else {
            emit chunkDownloadFailed(index, QStringLiteral("HTTP %1").arg(code));
        }
        reply->deleteLater();
    });
}

void Session::forceQuit()
{
    m_forceQuit = true;

    if (m_role == Role::sender) {
        sendJsonMessage(Action::TerminateSession().json());
        return;
    }

    QUrl url(m_url);
    url.setPath("/api/me/leave");

    auto *manager = new QNetworkAccessManager(this);
    manager->setTransferTimeout(NETWORK_TIMEOUT_SECS * 1000);
    manager->setCookieJar(m_cookieJar.data());
    m_cookieJar->setParent(nullptr);

    auto *reply = manager->post(QNetworkRequest(url), QByteArray());
    QObject::connect(reply, &QNetworkReply::finished, this, [this, manager, reply]() {
        processReply(reply);
        manager->deleteLater();
    });
}

void Session::onJoined()
{
    m_wsConnection = new WebSocketConnection(m_cookieJar, m_url, this);

    QObject::connect(m_wsConnection, &WebSocketConnection::connected, this, &Session::onWsConnected);
    QObject::connect(m_wsConnection, &WebSocketConnection::disconnected, this, &Session::onWsDisconnected);
    QObject::connect(m_wsConnection, &WebSocketConnection::newTextMessage, this, &Session::onWsText);

    m_wsConnection->connect();
}

void Session::onWsConnected()
{
    emit webSocketConnection(true, false);
}

void Session::onWsDisconnected(bool serverClosed)
{
    emit webSocketConnection(false, serverClosed);
}

void Session::onWsText(const QString &string)
{
    const auto obj = QJsonDocument::fromJson(string.toUtf8()).object();

    // Events carrying a top-level "id" require an explicit ACK before the
    // server closes the WebSocket (terminal events: complete, kicked).
    // Reply immediately; the state handler runs right after.
    if (obj.contains("id")) {
        const auto idValue = obj.value("id");
        if (idValue.isDouble() && m_wsConnection) {
            sendJsonMessage(Action::Ack(idValue.toInteger()).json());
        }
    }

    m_state->processEventJson(obj);
}

void Session::onComplete(const QString &status)
{
    emit complete(status);
}

void Session::processReply(QNetworkReply *reply)
{
    if (!reply) return;

    const int code = reply->attribute(QNetworkRequest::Attribute::HttpStatusCodeAttribute).toInt();
    if (code == 0) {
        emit complete("connection_error");
        return;
    }

    if (m_forceQuit && code == 200) {
        emit complete("terminated_by_you");
        return;
    }

    if (code != 201 && code != 202) {
        qWarning() << "Session::processReply code" << code << reply->readAll();
        emit complete(QStringLiteral("error_%1").arg(code));
        return;
    }

    const auto json = QJsonDocument::fromJson(reply->readAll()).object();
    m_id = json["id"].toString();

    if (m_id.isEmpty()) {
        emit complete("error_no_id");
        return;
    }

    emit joined();
}

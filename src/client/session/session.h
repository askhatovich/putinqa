// Copyright (C) 2026  Roman Lyubimov
// SPDX-License-Identifier: GPL-3.0-or-later
// For full license text, see <https://www.gnu.org/licenses/gpl-3.0.txt>

#pragma once

#include <QObject>
#include <QString>
#include <QUrl>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkCookieJar>

#include "sessionstate.h"
#include "websocketconnection.h"

class Session : public QObject
{
    Q_OBJECT
public:
    explicit Session(const QUrl &url, const QSharedPointer<QNetworkCookieJar> cookieJar, QObject *parent = nullptr);

    void join(const QString &id);
    // If autoDropFreeze is true, the server drops the initial freeze on
    // the first confirmed chunk and treats "last receiver left" as a
    // successful completion. Sent as JSON body to POST /api/session/create.
    void create(bool autoDropFreeze = false);
    SessionState *state() { return m_state; }
    const SessionState &getState() const { return *m_state; }
    const QString &getId() const { return m_id; }
    QSharedPointer<QNetworkCookieJar> getCookieJar() const { return m_cookieJar; }

public slots:
    void sendJsonMessage(const QJsonObject &json);
    void sendBinaryMessage(const QByteArray &data);
    void downloadChunkHttp(qint64 index);
    void forceQuit();

signals:
    void joined();
    void stateUpdated();
    void webSocketConnection(bool connected, bool serverClosed);
    void complete(const QString &status);
    void chunkDataReceived(qint64 index, const QByteArray &data);
    void chunkDownloadFailed(qint64 index, const QString &error);

private slots:
    void onJoined();
    void onWsConnected();
    void onWsDisconnected(bool serverClosed);
    void onWsText(const QString &string);
    void onComplete(const QString &status);

private:
    void processReply(QNetworkReply *reply);

    QUrl m_url;
    QString m_id;
    QSharedPointer<QNetworkCookieJar> m_cookieJar;
    enum class Role { undefined, receiver, sender } m_role = Role::undefined;
    WebSocketConnection *m_wsConnection = nullptr;
    SessionState *m_state = nullptr;
    QNetworkAccessManager *m_downloadManager = nullptr;
    bool m_forceQuit = false;
};

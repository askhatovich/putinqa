// Copyright (C) 2026  Roman Lyubimov
// SPDX-License-Identifier: GPL-3.0-or-later
// For full license text, see <https://www.gnu.org/licenses/gpl-3.0.txt>

#pragma once

#include <QObject>
#include <QNetworkAccessManager>
#include <QTimer>

struct ServerWorkloadInfo
{
    friend bool operator!=(const ServerWorkloadInfo& lhs, const ServerWorkloadInfo& rhs) {
        return lhs.currentSessionCount != rhs.currentSessionCount or
            lhs.currentUserCount != rhs.currentUserCount or
            lhs.maxSessionCount != rhs.maxSessionCount or
            lhs.maxUserCount != rhs.maxUserCount;
    }
    int maxSessionCount = 0;
    int maxUserCount = 0;
    int currentSessionCount = 0;
    int currentUserCount = 0;
};

Q_DECLARE_METATYPE(ServerWorkloadInfo)

class ServerWorkload : public QObject
{
    Q_OBJECT
public:
    explicit ServerWorkload(QObject *parent = nullptr);

public slots:
    void onServerHostUpdated(const QUrl& url);

signals:
    void updated(const ServerWorkloadInfo& info);
    void connectionFailed();

private slots:
    void onTimeout();
    void onRequestFinished(QNetworkReply* reply);

private:
    QNetworkAccessManager* m_networkAccessManager;
    QTimer* m_timer;
    QUrl m_url;
    ServerWorkloadInfo m_info;
};

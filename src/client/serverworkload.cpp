#include "serverworkload.h"

#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonObject>
#include <QJsonDocument>
#include <QDebug>

namespace
{
constexpr auto INTERVAL_SECS = 1;
constexpr auto NETWORK_TIMEOUT_SECS = 5;
}

ServerWorkload::ServerWorkload(QObject *parent)
    : QObject{parent}
    , m_networkAccessManager(new QNetworkAccessManager(this))
    , m_timer(new QTimer(this))
{
    m_networkAccessManager->setTransferTimeout(NETWORK_TIMEOUT_SECS*1000);
    m_timer->setInterval(INTERVAL_SECS*1000);
    m_timer->setSingleShot(true);
    QObject::connect(m_timer, &QTimer::timeout, this, &ServerWorkload::onTimeout);
    m_timer->start();
}

void ServerWorkload::onServerHostUpdated(const QUrl &url)
{
    m_url = url;
    m_url.setPath("/api/statistics/current");
}

void ServerWorkload::onTimeout()
{
    if (m_url.isEmpty()) return;

    QNetworkRequest request(m_url);
    auto reply = m_networkAccessManager->get(request);
    QObject::connect(reply, &QNetworkReply::finished, this, [this, reply]() { onRequestFinished(reply); });
    QObject::connect(reply, &QNetworkReply::finished, m_timer, QOverload<>::of(&QTimer::start));
}

void ServerWorkload::onRequestFinished(QNetworkReply *reply)
{
    if (reply == nullptr) {
        qWarning() << "ServerWorkload::onRequestFinished reply is nullptr";
        return;
    }

    const auto code = reply->attribute(QNetworkRequest::Attribute::HttpStatusCodeAttribute).toInt();
    if (code != 200) {
        qWarning() << "ServerWorkload::onRequestFinished code is" << code;
        return;
    }

    const auto json = QJsonDocument::fromJson(reply->readAll()).object();
    if (json.isEmpty()) {
        qWarning() << "ServerWorkload::onRequestFinished JSON is empty (or invalid)";
        return;
    }

    ServerWorkloadInfo info;
    info.maxSessionCount = json["max_session_count"].toInt();
    info.maxUserCount = json["max_user_count"].toInt();
    info.currentSessionCount = json["current_session_count"].toInt();
    info.currentUserCount = json["current_user_count"].toInt();

    if (info != m_info) {
        m_info = info;
        emit updated(m_info);
    }

    reply->deleteLater();
}

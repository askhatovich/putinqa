// Copyright (C) 2026  Roman Lyubimov
// SPDX-License-Identifier: GPL-3.0-or-later
// For full license text, see <https://www.gnu.org/licenses/gpl-3.0.txt>

#include "authorization.h"

#include <QDebug>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonObject>
#include <QJsonDocument>

namespace {
constexpr auto NETWORK_TIMEOUT_SECS = 10;
}

Authorization::Authorization(QObject *parent)
    : QObject{parent}
    , m_cookieJar(new QNetworkCookieJar)
{
}

void Authorization::setUrl(const QUrl &url)
{
    m_url = url;
    m_url.setPath("");
}

const QUrl &Authorization::getUrl() const
{
    return m_url;
}

void Authorization::setName(const QString &name)
{
    m_clientName = name;
}

const QString &Authorization::getName() const
{
    return m_clientName;
}

const QString &Authorization::getId() const
{
    return m_clientId;
}

const QSharedPointer<QNetworkCookieJar> Authorization::getCookieJar()
{
    return m_cookieJar;
}

bool Authorization::isAuthorized() const
{
    return m_authorized;
}

void Authorization::connect()
{
    if (m_url.isEmpty()) {
        emit error("Server URL is not set");
        return;
    }

    if (m_clientName.isEmpty()) {
        m_clientName = "Noname";
    }

    emit connecting();

    auto *manager = new QNetworkAccessManager(this);
    manager->setCookieJar(m_cookieJar.data());
    m_cookieJar->setParent(nullptr);
    manager->setTransferTimeout(NETWORK_TIMEOUT_SECS * 1000);

    QUrl url(m_url);
    url.setPath("/api/identity/request");
    url.setQuery(QStringLiteral("name=%1").arg(m_clientName));

    auto *reply = manager->get(QNetworkRequest(url));
    QObject::connect(reply, &QNetworkReply::finished, this, [this, manager, reply]() {
        processReply(reply);
        manager->deleteLater();
    });
}

void Authorization::confirmCaptcha(const QString &answer)
{
    auto *manager = new QNetworkAccessManager(this);
    manager->setCookieJar(m_cookieJar.data());
    m_cookieJar->setParent(nullptr);
    manager->setTransferTimeout(NETWORK_TIMEOUT_SECS * 1000);

    QUrl url(m_url);
    url.setPath("/api/identity/confirmation");

    QJsonObject body;
    body["name"] = m_clientName;
    body["client_id"] = m_pendingClientId;
    body["captcha_token"] = m_captchaToken;
    body["captcha_answer"] = answer;

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    auto *reply = manager->post(request, QJsonDocument(body).toJson(QJsonDocument::Compact));
    QObject::connect(reply, &QNetworkReply::finished, this, [this, manager, reply]() {
        processConfirmReply(reply);
        manager->deleteLater();
    });
}

void Authorization::processReply(QNetworkReply *reply)
{
    if (!reply) {
        emit error("No reply from server");
        return;
    }

    const int code = reply->attribute(QNetworkRequest::Attribute::HttpStatusCodeAttribute).toInt();
    if (code == 0) {
        emit error("Connection error");
        return;
    }

    if (code == 400) {
        m_authorized = true;
        emit authorized();
        return;
    }

    if (code == 503) {
        emit error("Server is full, try again later");
        return;
    }

    if (code == 401) {
        const auto json = QJsonDocument::fromJson(reply->readAll()).object();
        if (json.contains("captcha_image")) {
            m_captchaToken = json["captcha_token"].toString();
            m_pendingClientId = json["client_id"].toString();
            emit captchaRequired(json["captcha_image"].toString(),
                                 json["captcha_answer_length"].toInt());
        } else {
            emit error("Captcha required but no image received");
        }
        return;
    }

    if (code != 201) {
        emit error(QStringLiteral("Unexpected response: %1").arg(code));
        return;
    }

    const auto json = QJsonDocument::fromJson(reply->readAll()).object();
    m_clientName = json["name"].toString();
    m_clientId = json["id"].toString();

    if (m_clientId.isEmpty()) {
        emit error("Server returned empty client ID");
        return;
    }

    m_authorized = true;
    emit authorized();
}

void Authorization::processConfirmReply(QNetworkReply *reply)
{
    if (!reply) {
        emit error("Captcha confirmation failed");
        return;
    }

    const int code = reply->attribute(QNetworkRequest::Attribute::HttpStatusCodeAttribute).toInt();

    if (code == 201) {
        const auto json = QJsonDocument::fromJson(reply->readAll()).object();
        m_clientName = json["name"].toString();
        m_clientId = json["id"].toString();
        m_authorized = true;
        emit authorized();
        return;
    }

    if (code == 403) {
        emit error("Wrong captcha answer or expired");
        return;
    }

    emit error(QStringLiteral("Captcha confirmation failed: code %1").arg(code));
}

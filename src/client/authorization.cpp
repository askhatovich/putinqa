#include "authorization.h"

#include <QDebug>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonObject>
#include <QJsonDocument>

namespace
{
constexpr auto NETWORK_TIMEOUT_SECS = 10;
}

Authorization::Authorization(QObject *parent)
    : QObject{parent}
    , m_cookieJar(new QNetworkCookieJar(this))
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

QNetworkCookieJar *Authorization::getCookieJar()
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
        qWarning() << "Authorization::connect url is empty";
        return;
    }

    if (m_clientName.isEmpty()) {
        m_clientName = "Noname";
        qDebug() << "Authorization::connect empty client name changed to" << m_clientName;
    }

    emit connecting();

    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    manager->setCookieJar(m_cookieJar);
    manager->setTransferTimeout(NETWORK_TIMEOUT_SECS*1000);

    QUrl url(m_url);
    url.setPath("/api/identity/request");
    url.setQuery(QStringLiteral("name=%1").arg(m_clientName));
    QNetworkRequest request(url);
    auto reply = manager->get(request);
    QObject::connect(reply, &QNetworkReply::finished, this, [this, manager, reply]() { onRequestFinished(manager, reply); });
}

void Authorization::onRequestFinished(QNetworkAccessManager *manager, QNetworkReply *reply)
{
    if (manager == nullptr) {
        qFatal("Authorization::onRequestFinished manager is nullptr");
    }

    processReply(reply);

    manager->deleteLater();
}

void Authorization::processReply(QNetworkReply *reply)
{
    if (reply == nullptr) {
        qWarning() << "Authorization::processReply reply is nullptr";
        return;
    }

    const auto code = reply->attribute(QNetworkRequest::Attribute::HttpStatusCodeAttribute).toInt();
    if (code == 0) {
        emit error("Connection error");
        return;
    }

    if (code == 400) {
        qWarning() << "Authorization::processReply code is" << code << "(already is authorized)";
        return;
    }

    if (code == 503) {
        qWarning() << "Authorization::processReply server is busy";
        return;
    }

    if (code == 401) {
        qWarning() << "Authorization::processReply CAPTCHA! Not implemented yet";
        return;
    }

    if (code != 201) {
        error(QStringLiteral("Unexpected code: %1").arg(code));
        qWarning() << "Authorization::processReply unexpected code:" << code;
        return;
    }

    const auto json = QJsonDocument::fromJson(reply->readAll()).object();
    if (json.isEmpty()) {
        qWarning() << "Authorization::processReply JSON is empty (or invalid)";
        return;
    }

    m_clientName = json["name"].toString();
    m_clientId = json["id"].toString();

    if (m_clientId.isEmpty()) {
        qWarning() << "Authorization::processReply code 201, but id is emtpy:" << json;
        return;
    }

    m_authorized = true;
    emit authorized();
}


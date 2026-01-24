#include "session.h"

#include <QJsonObject>
#include <QJsonDocument>
#include <QDebug>
#include <QFile>
#include <QFileInfo>

namespace
{
constexpr auto NETWORK_TIMEOUT_SECS = 10;

} // namespace

Session::Session(const QUrl &url, QNetworkCookieJar *cookieJar,QObject *parent)
    : QObject{parent}
    , m_url(url)
    , m_cookieJar(cookieJar)
    , m_state(new SessionState(this))
{
    QObject::connect(this, &Session::joined, this, &Session::onJoined);
    QObject::connect(m_state, &SessionState::updated, this, &Session::stateUpdated);
    QObject::connect(m_state, &SessionState::updated, this, &Session::onStateUpdated);
}

void Session::join(const QString &id)
{
    m_role = Role::receiver;

    QUrl url(m_url);
    url.setPath("/api/session/join");
    url.setQuery(QStringLiteral("id=%1").arg(id));

    QNetworkRequest request(url);
    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    manager->setTransferTimeout(NETWORK_TIMEOUT_SECS*1000);
    manager->setCookieJar(m_cookieJar);
    auto reply = manager->get(request);
    QObject::connect(reply, &QNetworkReply::finished, this, [this, manager, reply]() { onRequestFinished(manager, reply); });
}

void Session::create(const QString &localFilePath)
{
    const auto fileInfo = getLocalFileInfo(localFilePath);
    if (!fileInfo.has_value()) {
        emit finished(true, "No local file found");
        return;
    }
    m_fileInfo = fileInfo.value();

    m_role = Role::sender;

    QUrl url(m_url);
    url.setPath("/api/session/create");

    QNetworkRequest request(url);
    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    manager->setTransferTimeout(NETWORK_TIMEOUT_SECS*1000);
    manager->setCookieJar(m_cookieJar);
    auto reply = manager->post(request, nullptr);
    QObject::connect(reply, &QNetworkReply::finished, this, [this, manager, reply]() { onRequestFinished(manager, reply); });
}

const SessionState &Session::getState() const
{
    return *m_state;
}

void Session::sendJsonMessage(const QJsonObject &json)
{
    if (m_wsConnection == nullptr) {
        qWarning() << "Session::sendJsonMessage WebSocket connection is nullptr";
        return;
    }
    m_wsConnection->sendText(QJsonDocument(json).toJson(QJsonDocument::Compact));
}

void Session::sendBinaryMessage(const QByteArray &data)
{

}

void Session::onRequestFinished(QNetworkAccessManager *manager, QNetworkReply *reply)
{
    if (manager == nullptr) {
        qFatal("Session::onRequestFinished manager is nullptr");
    }

    processReply(reply);

    manager->deleteLater();
}

void Session::onJoined()
{
    m_wsConnection = new WebSocketConnection(m_cookieJar, m_url, this);

    QObject::connect(m_wsConnection, &WebSocketConnection::connected, this, &Session::onWsConnected);
    QObject::connect(m_wsConnection, &WebSocketConnection::disconnected, this, &Session::onWsDisconnected);
    QObject::connect(m_wsConnection, &WebSocketConnection::newBinaryMessage, this, &Session::onWsBinary);
    QObject::connect(m_wsConnection, &WebSocketConnection::newTextMessage, this, &Session::onWsText);

    m_wsConnection->connect();
}

void Session::onWsConnected()
{
    emit webSocketConnection(true);
}

void Session::onWsDisconnected()
{
    emit webSocketConnection(false);
}

void Session::onWsText(const QString &string)
{
    m_state->processEventJson(QJsonDocument::fromJson(string.toUtf8()).object());
}

void Session::onWsBinary(const QByteArray &data)
{
    qInfo() << "on bin " << data.toBase64();
}

void Session::onStateUpdated()
{
    if (m_role == Role::sender) {
        if (m_state->getFileInfo()->name.isEmpty() && m_state->getFileInfo()->size == 0) {
            QJsonObject json;
            json["action"] = "set_file_info";
            QJsonObject data;
            data["name"] = m_fileInfo.name;
            data["size"] = m_fileInfo.size;
            json["data"] = data;

            sendJsonMessage(json);
        }
    }
}

std::optional<Session::FileInfo> Session::getLocalFileInfo(const QString &path)
{
    QFile file(path);
    if (!file.exists()) {
        return std::nullopt;
    }

    QFileInfo fileInfo(file);
    Session::FileInfo info;
    info.name = fileInfo.fileName();
    info.size = fileInfo.size();
    return info;
}

void Session::processReply(QNetworkReply *reply)
{
    if (reply == nullptr) {
        qWarning() << "Session::processReply reply is nullptr";
        return;
    }

    const auto code = reply->attribute(QNetworkRequest::Attribute::HttpStatusCodeAttribute).toInt();
    if (code == 0) {
        emit finished(true, "Connection error");
        return;
    }

    if (code != 201 && code != 202) {
        qWarning() << "Session::processReply code" << code << "/ body" << reply->readAll();
        emit finished(true, QStringLiteral("Code %1").arg(code));
        return;
    }

    const auto json = QJsonDocument::fromJson(reply->readAll()).object();
    if (json.isEmpty()) {
        qWarning() << "Session::processReply JSON is empty (or invalid)";
        return;
    }

    m_id = json["id"].toString();

    if (m_id.isEmpty()) {
        qWarning() << "Session::processReply code 201, but id is emtpy:" << json;
        return;
    }

    emit joined();
}


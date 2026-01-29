#include "session.h"
#include "actions.h"

#include <QJsonObject>
#include <QJsonDocument>
#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <QTimer>

namespace
{
constexpr auto NETWORK_TIMEOUT_SECS = 10;

} // namespace

Session::Session(const QUrl &url, const QSharedPointer<QNetworkCookieJar> cookieJar, QObject *parent)
    : QObject{parent}
    , m_url(url)
    , m_cookieJar(cookieJar)
    , m_state(new SessionState(this))
{
    QObject::connect(this, &Session::joined, this, &Session::onJoined);
    QObject::connect(m_state, &SessionState::updated, this, &Session::stateUpdated);
    QObject::connect(m_state, &SessionState::updated, this, &Session::onStateUpdated);
    QObject::connect(m_state, &SessionState::complete, this, &Session::onComplete);
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
    manager->setCookieJar(m_cookieJar.data());
    m_cookieJar->setParent(nullptr);
    auto reply = manager->get(request);
    QObject::connect(reply, &QNetworkReply::finished, this, [this, manager, reply]() { onRequestFinished(manager, reply); });
}

void Session::create(const QString &localFilePath)
{
    const auto fileInfo = getLocalFileInfo(localFilePath);
    if (!fileInfo.has_value()) {
        emit complete("No local file found", false);
        return;
    }
    m_fileInfo = fileInfo.value();

    m_role = Role::sender;

    QUrl url(m_url);
    url.setPath("/api/session/create");

    QNetworkRequest request(url);
    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    manager->setTransferTimeout(NETWORK_TIMEOUT_SECS*1000);
    manager->setCookieJar(m_cookieJar.data());
    m_cookieJar->setParent(nullptr);
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
    if (m_wsConnection == nullptr) {
        qWarning() << "Session::sendBinaryMessage WebSocket connection is nullptr";
        return;
    }
    m_wsConnection->sendBinary(data);
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

    QNetworkRequest request(url);
    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    manager->setTransferTimeout(NETWORK_TIMEOUT_SECS*1000);
    manager->setCookieJar(m_cookieJar.data());
    m_cookieJar->setParent(nullptr);
    auto reply = manager->post(request, nullptr);
    QObject::connect(reply, &QNetworkReply::finished, this, [this, manager, reply]() { onRequestFinished(manager, reply); });
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
            Action::SetFileInfo action {m_fileInfo.name, m_fileInfo.size};
            sendJsonMessage(action.json());
        }
    }
}

void Session::onComplete(const QString &status)
{
    const auto success = status == "ok";
    QString text;
    if (status == "ok") {
        text = "Completed successfully";
    }
    else if (status == "timeout") {
        text = "The maximum duration has been reached";
    }
    else if (status == "sender_is_gone") {
        text = "Sender is gone";
    }
    else if (status == "no_receivers") {
        text = "There are no receivers";
    }
    else {
        text = QStringLiteral("Unknown (code=%1)").arg(status);
    }
    emit complete(text, success);
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
        emit complete("Connection error", false);
        return;
    }

    if (m_forceQuit && code == 200) {
        emit complete("Terminated by you", false);
        return;
    }

    if (code != 201 && code != 202) {
        qWarning() << "Session::processReply code" << code << "/ body" << reply->readAll();
        emit complete(QStringLiteral("Code %1").arg(code), false);
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

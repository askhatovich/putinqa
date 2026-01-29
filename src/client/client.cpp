#include "client.h"

Client::Client(const QString &internalId, QObject *parent)
    : QObject{parent}
    , m_internalId(internalId)
    , m_serverWorkload(new ServerWorkload(this))
    , m_authorization(new Authorization(this))
{
    // m_serverWorkload->onServerHostUpdated(QUrl("http://127.0.0.1:2233"));
    // QObject::connect(m_serverWorkload, &ServerWorkload::updated, this, &Client::onServerWorkloadUpdate);

}

void Client::joinToSession(const QString &id)
{
    if (!m_authorization) {
        qCritical() << "Client::joinToSession called with null authorization";
        return;
    }
    if (!m_authorization->isAuthorized()) {
        qWarning() << "Client::joinToSession called in unauthorized state";
        return;
    }

    m_session->join(id);
}

void Client::createSession(const QString &file)
{
    if (!m_authorization->isAuthorized()) {
        qWarning() << "Client::createSession called in unauthorized state";
        return;
    }

    m_session->create(file);
}

void Client::forceQuit()
{
    m_session->forceQuit();
}

void Client::onServerWorkloadUpdate(const ServerWorkloadInfo &info)
{
    // qInfo() << "---" << m_internalId;
    // qInfo() << "currentSessionCount:" << info.currentSessionCount;
    // qInfo() << "maxSessionCount:" << info.maxSessionCount;
    // qInfo() << "currentUserCount:" << info.currentUserCount;
    // qInfo() << "maxUserCount:" << info.maxUserCount;
    // qInfo() << "---";
}

void Client::onAuthorized()
{
    // qInfo() << "---" << m_internalId;
    // qInfo() << "Authorized";
    // qInfo() << "Client ID" << m_authorization->getId();
    // qInfo() << "Name:" << m_authorization->getName();
    // qInfo() << "---";

    m_session = new Session(m_authorization->getUrl(), m_authorization->getCookieJar(), this);
    QObject::connect(m_session, &Session::stateUpdated, this, &Client::stateUpdated);
    QObject::connect(m_session, &Session::joined, this, &Client::onSessionJoined);
    QObject::connect(m_session, &Session::complete, this, &Client::onSessionFinished);
    QObject::connect(m_session, &Session::webSocketConnection, this, &Client::webSocketConnection);
    QObject::connect(m_session, &Session::webSocketConnection, this, [this](bool connected){
        qInfo().noquote() << m_internalId << "WebSocket connection" << connected;
    });
    QObject::connect(m_session, &Session::stateUpdated, this, [this](){
        // const auto text = m_session->getState().dump();
        // qInfo().noquote() << m_internalId << "\n" << text << "\n";
    });

    emit authorized();
}

void Client::onSessionJoined()
{
    // qInfo() << "---" << m_internalId;
    // qInfo() << "Joined to session";
    // qInfo() << "Session ID" << m_session->getId();
    // qInfo() << "---";

    emit joinedToSession();
}

void Client::onSessionFinished(const QString &description, bool success)
{
    qInfo().noquote() << m_internalId << "FINISHED" << (success ? "success" : "error") << description;
    emit complete(description, success);
}

void Client::onSessionFinishedWithError(const QString &description)
{
    emit complete(description, false);
}


Client::~Client()
{
    // TODO logout
}

const QString Client::getSessionId() const
{
    return m_session->getId();
}

const SessionState &Client::getSessionState() const
{
    return m_session->getState();
}

void Client::authorize()
{
    m_authorization->setUrl(QUrl("http://127.0.0.1:2233"));
    m_authorization->setName("test user");
    m_authorization->connect();

    QObject::connect(m_authorization, &Authorization::authorized, this, &Client::onAuthorized);
    QObject::connect(m_authorization, &Authorization::error, this, &Client::onSessionFinishedWithError);
}


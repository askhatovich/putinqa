#pragma once

#include "authorization.h"
#include "serverworkload.h"
#include "session/session.h"

#include <QObject>

class Client : public QObject
{
    Q_OBJECT
public:
    explicit Client(const QString &internalId, QObject *parent = nullptr);
    ~Client();
    const QString getSessionId() const;
    const SessionState &getSessionState() const;

signals:
    void authorized();
    void joinedToSession();
    void stateUpdated();
    void complete(bool error, const QString &description);

public slots:
    void authorize();
    void joinToSession(const QString &id);
    void createSession(const QString &file);

private slots:
    void onServerWorkloadUpdate(const ServerWorkloadInfo& info);
    void onAuthorized();
    void onSessionJoined();
    void onSessionFinished(bool error, const QString &description);
    void onSessionFinishedWithError(const QString &description);

private:
    QString m_internalId;
    ServerWorkload *m_serverWorkload = nullptr;
    Authorization *m_authorization = nullptr;
    Session *m_session = nullptr;
};

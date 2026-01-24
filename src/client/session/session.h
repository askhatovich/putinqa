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
    explicit Session(const QUrl &url, QNetworkCookieJar *cookieJar, QObject *parent = nullptr);
    virtual ~Session() = default;

    void join(const QString &id);
    void create(const QString &localFilePath);
    const SessionState &getState() const;
    const QString &getId() const { return m_id; }

public slots:
    void sendJsonMessage(const QJsonObject &json);
    void sendBinaryMessage(const QByteArray &data);

signals:
    void joined();
    void stateUpdated();
    void webSocketConnection(bool connected);

    void finished(bool error, const QString& description);

private slots:
    void onRequestFinished(QNetworkAccessManager *manager, QNetworkReply *reply);
    void onJoined();
    void onWsConnected();
    void onWsDisconnected();
    void onWsText(const QString &string);
    void onWsBinary(const QByteArray &data);
    void onStateUpdated();

protected:
    struct FileInfo {
        QString name;
        qint64 size = 0;
    };
    enum class Role {
        undefined,
        receiver,
        sender
    };

    static std::optional<Session::FileInfo> getLocalFileInfo(const QString &path);
    void processReply(QNetworkReply *reply);

    const QUrl& m_url;
    QString m_id;
    FileInfo m_fileInfo;
    QNetworkCookieJar *m_cookieJar;
    Role m_role = Role::undefined;
    WebSocketConnection *m_wsConnection = nullptr;
    SessionState *m_state = nullptr;
};

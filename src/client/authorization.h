#pragma once

#include <QObject>
#include <QUrl>
#include <QTimer>
#include <QNetworkCookieJar>
#include <QNetworkReply>
#include <QNetworkAccessManager>

class Authorization : public QObject
{
    Q_OBJECT
public:
    explicit Authorization(QObject *parent = nullptr);
    void setUrl(const QUrl& url);
    const QUrl& getUrl() const;
    void setName(const QString& name);
    const QString& getName() const;
    const QString& getId() const;
    const QSharedPointer<QNetworkCookieJar> getCookieJar();
    bool isAuthorized() const;

public slots:
    void connect();

signals:
    void connecting();
    void authorized();
    void error(const QString& reason);

private slots:
    void onRequestFinished(QNetworkAccessManager* manager, QNetworkReply* reply);

private:
    void processReply(QNetworkReply* reply);

    QUrl m_url;
    QString m_clientName;
    QString m_clientId;
    bool m_authorized = false;

    QSharedPointer<QNetworkCookieJar> m_cookieJar;
};

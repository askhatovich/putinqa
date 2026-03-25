// Copyright (C) 2026  Roman Lyubimov
// SPDX-License-Identifier: GPL-3.0-or-later
// For full license text, see <https://www.gnu.org/licenses/gpl-3.0.txt>

#pragma once

#include <QObject>
#include <QUrl>
#include <QNetworkCookieJar>
#include <QNetworkReply>
#include <QNetworkAccessManager>

class Authorization : public QObject
{
    Q_OBJECT
public:
    explicit Authorization(QObject *parent = nullptr);
    void setUrl(const QUrl &url);
    const QUrl &getUrl() const;
    void setName(const QString &name);
    const QString &getName() const;
    const QString &getId() const;
    const QSharedPointer<QNetworkCookieJar> getCookieJar();
    bool isAuthorized() const;

public slots:
    void connect();
    void confirmCaptcha(const QString &answer);

signals:
    void connecting();
    void authorized();
    void captchaRequired(const QString &imageBase64, int answerLength);
    void error(const QString &reason);

private:
    void processReply(QNetworkReply *reply);
    void processConfirmReply(QNetworkReply *reply);

    QUrl m_url;
    QString m_clientName;
    QString m_clientId;
    bool m_authorized = false;

    QString m_captchaToken;
    QString m_pendingClientId;

    QSharedPointer<QNetworkCookieJar> m_cookieJar;
};

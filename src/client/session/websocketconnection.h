#pragma once

#include <QObject>
#include <QWebSocket>
#include <QNetworkCookieJar>

class WebSocketConnection : public QObject
{
    Q_OBJECT

    class Ttl {
    public:
        Ttl(int limit) : m_limit(limit) {}
        int attemptsLeft() const {
            if (m_counter >= m_limit) {
                return 0;
            }
            return m_limit - m_counter;
        }
        bool isAliveIfIncrement() {
            ++m_counter;
            return m_counter < m_limit;
        }
        void release() {
            m_counter = 0;
        }

    private:
        int m_counter = 0;
        const int m_limit;
    };

public:
    explicit WebSocketConnection(const QSharedPointer<QNetworkCookieJar> cookieJar, const QUrl &url, QObject *parent = nullptr);

public slots:
    void connect();
    void sendText(const QString &message);
    void sendBinary(const QByteArray &data);

signals:
    void connected();
    void disconnected();
    void newTextMessage(const QString &message);
    void newBinaryMessage(const QByteArray &data);
    void reconnecting(int attemptsLeft);
    void error(const QString &description);

private slots:
    void onConnected();
    void onDisconnected();
    void onWsError(QAbstractSocket::SocketError error);
    void onTextMessageReceived(const QString &string);
    void onBinaryMessageReceived(const QByteArray &data);

private:
    QWebSocket* m_ws;
    QUrl m_url;
    const QSharedPointer<QNetworkCookieJar> m_cookieJar;
    Ttl m_reconnectTtl;
};

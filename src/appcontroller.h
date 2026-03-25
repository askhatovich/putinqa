// Copyright (C) 2026  Roman Lyubimov
// SPDX-License-Identifier: GPL-3.0-or-later
// For full license text, see <https://www.gnu.org/licenses/gpl-3.0.txt>

#pragma once

#include <QObject>
#include <QSettings>
#include <QVariantList>
#include <QVariantMap>
#include <QFile>
#include <QMap>
#include <QTimer>
#include <QQueue>
#include <QUrl>
#include <QLocale>
#include <QNetworkProxy>

#include "client/authorization.h"
#include "client/serverworkload.h"
#include "client/session/session.h"

class AppController : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString appVersion READ appVersion CONSTANT)
    Q_PROPERTY(QString screen READ screen NOTIFY screenChanged)
    Q_PROPERTY(QString userName READ userName NOTIFY userNameChanged)
    Q_PROPERTY(QString serverUrl READ serverUrl NOTIFY serverUrlChanged)
    Q_PROPERTY(QString activeServer READ activeServer NOTIFY activeServerChanged)
    Q_PROPERTY(bool inSession READ inSession NOTIFY inSessionChanged)
    Q_PROPERTY(QString errorMsg READ errorMsg NOTIFY errorMsgChanged)
    Q_PROPERTY(QString shareLink READ shareLink NOTIFY shareLinkChanged)
    Q_PROPERTY(QString fileName READ fileName NOTIFY fileNameChanged)
    Q_PROPERTY(qint64 fileSize READ fileSize NOTIFY fileSizeChanged)
    Q_PROPERTY(double progress READ progress NOTIFY progressChanged)
    Q_PROPERTY(qint64 bytesTransferred READ bytesTransferred NOTIFY bytesTransferredChanged)
    Q_PROPERTY(bool uploadFinished READ uploadFinished NOTIFY uploadFinishedChanged)
    Q_PROPERTY(int bufferUsed READ bufferUsed NOTIFY bufferUsedChanged)
    Q_PROPERTY(int bufferMax READ bufferMax NOTIFY bufferMaxChanged)
    Q_PROPERTY(bool isSender READ isSender NOTIFY isSenderChanged)
    Q_PROPERTY(bool frozen READ frozen NOTIFY frozenChanged)
    Q_PROPERTY(int freezeRemaining READ freezeRemaining NOTIFY freezeRemainingChanged)
    Q_PROPERTY(int sessionExpirationIn READ sessionExpirationIn NOTIFY sessionExpirationInChanged)
    Q_PROPERTY(QString senderName READ senderName NOTIFY senderNameChanged)
    Q_PROPERTY(bool senderOnline READ senderOnline NOTIFY senderOnlineChanged)
    Q_PROPERTY(QVariantList receivers READ receivers NOTIFY receiversChanged)
    Q_PROPERTY(QString captchaImage READ captchaImage NOTIFY captchaImageChanged)
    Q_PROPERTY(int captchaAnswerLength READ captchaAnswerLength NOTIFY captchaAnswerLengthChanged)
    Q_PROPERTY(QString completeStatus READ completeStatus NOTIFY completeStatusChanged)
    Q_PROPERTY(bool hasDownloadedFile READ hasDownloadedFile NOTIFY hasDownloadedFileChanged)
    Q_PROPERTY(QVariantMap stats READ stats NOTIFY statsChanged)
    Q_PROPERTY(int chunksConfirmed READ chunksConfirmed NOTIFY chunksConfirmedChanged)
    Q_PROPERTY(int highestKnownChunk READ highestKnownChunk NOTIFY highestKnownChunkChanged)
    Q_PROPERTY(QUrl suggestedSavePath READ suggestedSavePath NOTIFY fileNameChanged)
    Q_PROPERTY(bool receiversPresent READ receiversPresent NOTIFY receiversChanged)
    Q_PROPERTY(QString myClientId READ myClientId NOTIFY myClientIdChanged)
    Q_PROPERTY(QString language READ language NOTIFY languageChanged)
    Q_PROPERTY(QVariantMap t READ translations NOTIFY languageChanged)
    Q_PROPERTY(QString proxyType READ proxyType NOTIFY proxyChanged)
    Q_PROPERTY(QString proxyHost READ proxyHost NOTIFY proxyChanged)
    Q_PROPERTY(quint16 proxyPort READ proxyPort NOTIFY proxyChanged)

public:
    explicit AppController(QObject *parent = nullptr);

    QString appVersion() const { return QStringLiteral(APP_VERSION); }

    QString screen() const { return m_screen; }
    QString userName() const { return m_userName; }
    QString serverUrl() const { return m_serverUrl; }
    QString activeServer() const { return m_activeServer; }
    bool inSession() const { return m_screen == "sender" || m_screen == "receiver" || m_screen == "connecting"; }
    QString errorMsg() const { return m_errorMsg; }
    QString shareLink() const { return m_shareLink; }
    QString fileName() const { return m_fileName; }
    qint64 fileSize() const { return m_fileSize; }
    double progress() const { return m_progress; }
    qint64 bytesTransferred() const { return m_bytesTransferred; }
    bool uploadFinished() const { return m_uploadFinished; }
    int bufferUsed() const { return m_bufferUsed; }
    int bufferMax() const { return m_bufferMax; }
    bool isSender() const { return m_isSender; }
    bool frozen() const { return m_frozen; }
    int freezeRemaining() const { return m_freezeRemaining; }
    int sessionExpirationIn() const { return m_sessionExpirationIn; }
    QString senderName() const { return m_senderName; }
    bool senderOnline() const { return m_senderOnline; }
    QVariantList receivers() const { return m_receivers; }
    QString captchaImage() const { return m_captchaImage; }
    int captchaAnswerLength() const { return m_captchaAnswerLength; }
    QString completeStatus() const { return m_completeStatus; }
    bool hasDownloadedFile() const { return m_hasDownloadedFile; }
    QVariantMap stats() const { return m_stats; }
    int chunksConfirmed() const { return m_chunksConfirmed; }
    int highestKnownChunk() const { return m_highestKnownChunk; }
    QUrl suggestedSavePath() const;
    bool receiversPresent() const { return !m_receivers.isEmpty(); }
    QString myClientId() const { return m_auth ? m_auth->getId() : QString(); }
    QString language() const { return m_language; }
    QVariantMap translations() const;
    QString proxyType() const { return m_proxyType; }
    QString proxyHost() const { return m_proxyHost; }
    quint16 proxyPort() const { return m_proxyPort; }

    Q_INVOKABLE void startSend();
    Q_INVOKABLE void selectFile(const QUrl &fileUrl);
    Q_INVOKABLE void startReceive(const QString &link);
    Q_INVOKABLE void solveCaptcha(const QString &answer);
    Q_INVOKABLE void copyShareLink();
    Q_INVOKABLE void openSettings();
    Q_INVOKABLE void saveSettings(const QString &url, const QString &name, const QString &language,
                                      const QString &proxyType, const QString &proxyHost, quint16 proxyPort);
    Q_INVOKABLE void dropFreeze();
    Q_INVOKABLE void kickReceiver(const QString &id);
    Q_INVOKABLE void terminateSession();
    Q_INVOKABLE void leaveSession();
    Q_INVOKABLE void restart();
    Q_INVOKABLE void changeName(const QString &name);
    Q_INVOKABLE void saveReceivedFile(const QUrl &path);
    Q_INVOKABLE void minimizeToTray();
    Q_INVOKABLE QString formatBytes(qint64 bytes) const;
    Q_INVOKABLE QString qrDataUrl() const;

signals:
    void screenChanged();
    void activeServerChanged();
    void inSessionChanged();
    void userNameChanged();
    void serverUrlChanged();
    void errorMsgChanged();
    void shareLinkChanged();
    void fileNameChanged();
    void fileSizeChanged();
    void progressChanged();
    void bytesTransferredChanged();
    void uploadFinishedChanged();
    void bufferUsedChanged();
    void bufferMaxChanged();
    void isSenderChanged();
    void frozenChanged();
    void freezeRemainingChanged();
    void sessionExpirationInChanged();
    void senderNameChanged();
    void senderOnlineChanged();
    void receiversChanged();
    void captchaImageChanged();
    void captchaAnswerLengthChanged();
    void completeStatusChanged();
    void hasDownloadedFileChanged();
    void statsChanged();
    void chunksConfirmedChanged();
    void highestKnownChunkChanged();
    void languageChanged();
    void myClientIdChanged();

    void proxyChanged();
    void showWindowRequested();
    void trayRequested();

private slots:
    void onAuthorized();
    void onCaptchaRequired(const QString &imageBase64, int answerLength);
    void onAuthError(const QString &reason);
    void onSessionComplete(const QString &status);
    void onWsConnection(bool connected, bool serverClosed);
    void onSessionInitialized();
    void onNewChunkEvent(qint64 index, qint64 size);
    void onChunkRemovedEvent();
    void onNewChunkAllowed(bool status);
    void onFreezeDropped();
    void onUploadFinishedEvent();
    void onFileInfoEvent(const QString &name, qint64 size);
    void onBytesCountEvent(const QString &direction, qint64 value);
    void onPersonalReceivedEvent(qint64 bytes);
    void onNewReceiverEvent(const QString &id, const QString &name);
    void onReceiverRemovedEvent(const QString &id);
    void onOnlineEvent(const QString &id, bool online);
    void onNameChangedEvent(const QString &id, const QString &name);
    void onChunkDataReceived(qint64 index, const QByteArray &data);
    void onChunkDownloadFailed(qint64 index, const QString &error);
    void onChunkDownloadFinished(const QString &receiverId, qint64 index);
    void onServerWorkloadUpdated(const ServerWorkloadInfo &info);

private:
    void setScreen(const QString &screen);
    void setError(const QString &msg);
    void loadSettings();
    void authorize();
    void proceedAfterAuth();
    void startSenderSession();
    void startReceiverSession();
    void connectSessionSignals();
    void uploadNextChunk();
    void processDownloadQueue();
    void checkReceiverDone();
    void updateReceiversList();
    void buildShareLink();
    void resetSessionState();
    void applyProxy();

    QSettings m_settings;
    QString m_serverUrl;
    QString m_activeServer;
    QString m_userName;
    QString m_language;
    QString m_proxyType = "none";
    QString m_proxyHost;
    quint16 m_proxyPort = 0;

    QString m_screen = "entry";
    QString m_screenBeforeSettings;
    QString m_errorMsg;
    bool m_isSender = false;
    QString m_pendingRole;
    QString m_pendingSessionId;

    Authorization *m_auth = nullptr;
    Session *m_session = nullptr;
    ServerWorkload *m_serverWorkload = nullptr;

    QByteArray m_encryptionKey;
    QString m_shareLink;
    QString m_filePath;
    QString m_fileName;
    qint64 m_fileSize = 0;
    double m_progress = 0.0;
    qint64 m_bytesTransferred = 0;
    bool m_uploadFinished = false;
    int m_bufferUsed = 0;
    int m_bufferMax = 10;

    QFile *m_uploadFile = nullptr;
    bool m_waitingForChunkAccepted = false;
    bool m_canSendChunk = true;
    qint64 m_maxChunkPayload = 0;

    QFile *m_downloadTmpFile = nullptr;
    QString m_downloadTmpPath;
    QMap<qint64, QByteArray> m_chunkBuffer;   // out-of-order chunks waiting to be flushed
    QSet<qint64> m_writtenChunks;             // all chunks written to disk or buffered
    qint64 m_nextWriteIndex = 1;              // next sequential chunk to write to tmp file
    QQueue<qint64> m_downloadQueue;
    int m_activeDownloads = 0;
    static constexpr int MAX_PARALLEL_DOWNLOADS = 4;
    int m_chunksConfirmed = 0;
    int m_highestKnownChunk = 0;
    int m_pendingConfirms = 0;
    QMap<QString, int> m_receiverChunksDone;
    bool m_hasDownloadedFile = false;

    void openDownloadTmpFile();
    void flushChunksToDisk(qint64 index, const QByteArray &data);
    void cleanupDownloadTmpFile();

    bool m_frozen = true;
    int m_freezeRemaining = 0;
    int m_sessionExpirationIn = 0;
    QString m_senderName;
    bool m_senderOnline = false;
    QVariantList m_receivers;
    QString m_completeStatus;

    QString m_captchaImage;
    int m_captchaAnswerLength = 0;
    QVariantMap m_stats;

    bool m_terminateRequested = false;

    QTimer *m_freezeTimer = nullptr;
    QTimer *m_expirationTimer = nullptr;
};

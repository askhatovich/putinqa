// Copyright (C) 2026  Roman Lyubimov
// SPDX-License-Identifier: GPL-3.0-or-later
// For full license text, see <https://www.gnu.org/licenses/gpl-3.0.txt>

#pragma once

#include <QObject>
#include <QJsonObject>

namespace SessionStateStructures {

class UpdatableStructure : public QObject {
    Q_OBJECT
public:
    UpdatableStructure(QObject *);
    virtual ~UpdatableStructure() = default;

signals:
    void updated();
};

struct Member : public UpdatableStructure {
    Member(QObject *parent) : UpdatableStructure(parent) {}

    QString name;
    QString id;
    bool isOnline = false;
    struct CurrentChunk {
        qint64 index = 0;
        bool inProgress = false;
    } currentChunk;
};

struct FileInfo : public UpdatableStructure {
    FileInfo(QObject *parent) : UpdatableStructure(parent) {}

    QString name;
    qint64 size = 0;
};

struct TransferCounter : public UpdatableStructure {
    TransferCounter(QObject *parent) : UpdatableStructure(parent) {}

    qint64 fromSender = 0;
    qint64 toReceivers = 0;
};

template <typename T>
struct OneValue : public UpdatableStructure {
    OneValue(QObject *parent) : UpdatableStructure(parent) {}

    T value {};
};

struct Limits {
    qint64 maxChunkSize = 0;
    qint64 maxChunkQueue = 0;
    qint64 maxInitialFreeze = 0;
    qint64 maxReceiverCount = 0;
};

struct Chunk {
    qint64 index = 0;
    qint64 size = 0;
};

} // namespace SessionStateStructures

class SessionState : public QObject
{
    Q_OBJECT
public:
    explicit SessionState(QObject *parent = nullptr);

    const QString &getSessionId() const;
    const SessionStateStructures::Limits &getLimits() const;

    const SessionStateStructures::OneValue<qint64> *getLastUploadedChunk() const;
    const SessionStateStructures::OneValue<qint64> *getExpireTimestamp() const;
    const SessionStateStructures::OneValue<qint64> *getReceivedByMe() const;
    const SessionStateStructures::OneValue<bool> *getInitialFreeze() const;
    const SessionStateStructures::OneValue<bool> *getSomeChunkWasRemoved() const;
    const SessionStateStructures::OneValue<bool> *getUploadFinished() const;
    const SessionStateStructures::OneValue<bool> *getNewChunkIsAllowed() const;
    const SessionStateStructures::Member *getSender() const;
    const SessionStateStructures::FileInfo *getFileInfo() const;
    const SessionStateStructures::TransferCounter *getTransferCounter() const;
    const SessionStateStructures::OneValue<QMap<QString, SessionStateStructures::Member*>> *getReceivers() const;
    const SessionStateStructures::OneValue<QMap<qint64, SessionStateStructures::Chunk>> *getChunks() const;

    QString dump() const;

signals:
    void updated();
    void complete(const QString &status);

    // Specific event signals for fine-grained control
    void sessionInitialized();
    void newChunkEvent(qint64 index, qint64 size);
    void chunkRemovedEvent();
    void newChunkAllowedEvent(bool status);
    void freezeDroppedEvent();
    void uploadFinishedEvent();
    void fileInfoEvent(const QString &name, qint64 size);
    void bytesCountEvent(const QString &direction, qint64 value);
    void personalReceivedEvent(qint64 bytes);
    void newReceiverEvent(const QString &id, const QString &name);
    void receiverRemovedEvent(const QString &id);
    void onlineEvent(const QString &id, bool online);
    void nameChangedEvent(const QString &id, const QString &name);
    void chunkDownloadFinishedEvent(const QString &receiverId, qint64 index);

public slots:
    void processEventJson(const QJsonObject &event);

private:
    void onStartInit(const QJsonObject &data);
    void onOnline(const QJsonObject &data);
    void onNameChanged(const QJsonObject &data);
    void onNewReceiver(const QJsonObject &data);
    void onReceiverRemoved(const QJsonObject &data);
    void onFileInfo(const QJsonObject &data);
    void onNewChunk(const QJsonObject &data);
    void onChunkDownload(const QJsonObject &data);
    void onChunkRemoved(const QJsonObject &data);
    void onBytesCount(const QJsonObject &data);
    void onBytesReceived(const QJsonObject &data);
    void onChunksUnfrozen();
    void onUploadFinished();
    void onComplete(const QJsonObject &data);
    void onNewChunkIsAllowed(const QJsonObject &data);

    QString m_sessionId;
    SessionStateStructures::Limits m_limits;
    SessionStateStructures::OneValue<qint64> *m_lastUploadedChunk;
    SessionStateStructures::OneValue<qint64> *m_expireTimestamp;
    SessionStateStructures::OneValue<qint64> *m_receivedByMe;
    SessionStateStructures::OneValue<bool> *m_initialFreeze;
    SessionStateStructures::OneValue<bool> *m_someChunksWasRemoved;
    SessionStateStructures::OneValue<bool> *m_uploadFinished;
    SessionStateStructures::OneValue<bool> *m_newChunkIsAllowed;
    SessionStateStructures::Member *m_sender;
    SessionStateStructures::FileInfo *m_fileInfo;
    SessionStateStructures::TransferCounter *m_transferCounter;
    SessionStateStructures::OneValue<QMap<QString, SessionStateStructures::Member*>> *m_receivers;
    SessionStateStructures::OneValue<QMap<qint64, SessionStateStructures::Chunk>> *m_chunks;
};

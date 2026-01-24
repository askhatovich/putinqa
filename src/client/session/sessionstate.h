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
    qint64 currentChunk = 0;
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
    const SessionStateStructures::Member *getSender() const;
    const SessionStateStructures::FileInfo *getFileInfo() const;
    const SessionStateStructures::TransferCounter *getTransferCounter() const;
    const QMap<QString, SessionStateStructures::Member*> &getReceivers() const;
    const QMap<qint64, SessionStateStructures::Chunk> &getChunks() const;

signals:
    void updated();

public slots:
    void processEventJson(const QJsonObject &event);

private:
    void onStartInit(const QJsonObject &data);
    void onNewReceiver(const QJsonObject &data);
    void onFileInfo(const QJsonObject &data);

    QString m_sessionId;
    SessionStateStructures::Limits m_limits;
    SessionStateStructures::OneValue<qint64> *m_lastUploadedChunk;
    SessionStateStructures::OneValue<qint64> *m_expireTimestamp;
    SessionStateStructures::OneValue<qint64> *m_receivedByMe;
    SessionStateStructures::OneValue<bool> *m_initialFreeze;
    SessionStateStructures::OneValue<bool> *m_someChunksWasRemoved;
    SessionStateStructures::OneValue<bool> *m_uploadFinished;
    SessionStateStructures::Member *m_sender;
    SessionStateStructures::FileInfo *m_fileInfo;
    SessionStateStructures::TransferCounter *m_transferCounter;
    QMap<QString, SessionStateStructures::Member*> m_receivers; // id, user
    QMap<qint64, SessionStateStructures::Chunk> m_chunks; // index, chunk
};

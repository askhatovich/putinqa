#include "sessionstate.h"

#include <QDebug>
#include <QJsonDocument>
#include <QJsonArray>
#include <QDateTime>

SessionStateStructures::UpdatableStructure::UpdatableStructure(QObject *parent) : QObject(parent)
{
}

SessionState::SessionState(QObject *parent)
    : QObject{parent}
    , m_lastUploadedChunk(new SessionStateStructures::OneValue<qint64>(this))
    , m_expireTimestamp(new SessionStateStructures::OneValue<qint64>(this))
    , m_receivedByMe(new SessionStateStructures::OneValue<qint64>(this))
    , m_initialFreeze(new SessionStateStructures::OneValue<bool>(this))
    , m_someChunksWasRemoved(new SessionStateStructures::OneValue<bool>(this))
    , m_uploadFinished(new SessionStateStructures::OneValue<bool>(this))
    , m_sender(new SessionStateStructures::Member(this))
    , m_fileInfo(new SessionStateStructures::FileInfo(this))
    , m_transferCounter(new SessionStateStructures::TransferCounter(this))
{
}

const QString &SessionState::getSessionId() const
{
    return m_sessionId;
}

const SessionStateStructures::OneValue<qint64> *SessionState::getLastUploadedChunk() const
{
    return m_lastUploadedChunk;
}

const SessionStateStructures::OneValue<qint64> *SessionState::getExpireTimestamp() const
{
    return m_expireTimestamp;
}

const SessionStateStructures::OneValue<qint64> *SessionState::getReceivedByMe() const
{
    return m_receivedByMe;
}

const SessionStateStructures::OneValue<bool> *SessionState::getInitialFreeze() const
{
    return m_initialFreeze;
}

const SessionStateStructures::OneValue<bool> *SessionState::getSomeChunkWasRemoved() const
{
    return m_someChunksWasRemoved;
}

const SessionStateStructures::OneValue<bool> *SessionState::getUploadFinished() const
{
    return m_uploadFinished;
}

const SessionStateStructures::Limits &SessionState::getLimits() const
{
    return m_limits;
}

const SessionStateStructures::Member *SessionState::getSender() const
{
    return m_sender;
}

const SessionStateStructures::FileInfo *SessionState::getFileInfo() const
{
    return m_fileInfo;
}

const SessionStateStructures::TransferCounter *SessionState::getTransferCounter() const
{
    return m_transferCounter;
}

const QMap<QString, SessionStateStructures::Member*> &SessionState::getReceivers() const
{
    return m_receivers;
}

const QMap<qint64, SessionStateStructures::Chunk> &SessionState::getChunks() const
{
    return m_chunks;
}

void SessionState::processEventJson(const QJsonObject &event)
{
    const auto type = event.value("event").toString();
    if (type.isEmpty()) {
        qWarning().noquote() << "SessionState::processEventJson: Empty 'event' in" << QJsonDocument(event).toJson(QJsonDocument::Compact);
        return;
    }

    if (type == "start_init") {
        onStartInit(event.value("data").toObject());
    }
    else if (type == "new_receiver") {
        onNewReceiver(event.value("data").toObject());
    }
    else if (type == "file_info") {
        onFileInfo(event.value("data").toObject());
    }
    else {
        qWarning().noquote() << "SessionState::processEventJson: Unknown event type:" << type;
        return;
    }

    emit updated();
}

void SessionState::onStartInit(const QJsonObject &data)
{
    // TODO: Логирование ошибок при нарушении сигнатур

    m_sessionId = data.value("session_id").toString();

    const auto limits = data.value("limits").toObject();
    m_limits.maxChunkQueue = limits.value("max_chunk_queue").toInteger();
    m_limits.maxChunkSize = limits.value("max_chunk_size").toInteger();
    m_limits.maxInitialFreeze = limits.value("max_initial_freeze").toInteger();
    m_limits.maxReceiverCount = limits.value("max_receiver_count").toInteger();

    const auto members = data.value("members").toObject();
    const auto sender = members.value("sender").toObject();
    m_sender->currentChunk = 0; // unused
    m_sender->id = sender.value("id").toString();
    m_sender->isOnline = sender.value("is_online").toBool();
    m_sender->name = sender.value("name").toString();

    const auto receiversArray = members.value("receivers").toArray();
    m_receivers.clear();
    for (const auto &node: receiversArray) {
        const auto object = node.toObject();

        auto *member = new SessionStateStructures::Member(this);
        member->currentChunk = object.value("current_chunk").toInteger();
        member->id = object.value("id").toString();
        member->isOnline = object.value("is_online").toBool();
        member->name = object.value("name").toString();

        m_receivers.insert(member->id, member);
    }

    const auto state = data.value("state").toObject();
    const auto chunks = state.value("chunks").toArray();
    m_chunks.clear();
    for (const auto &node: chunks) {
        const auto object = node.toObject();

        SessionStateStructures::Chunk chunk;
        chunk.index = object.value("index").toInteger();
        chunk.size = object.value("size").toInteger();

        m_chunks.insert(chunk.index, chunk);
    }
    m_lastUploadedChunk->value = state.value("current_chunk").toInteger();
    m_expireTimestamp->value = QDateTime::currentSecsSinceEpoch() + state.value("expiration_in").toInteger();
    m_initialFreeze->value = state.value("initial_freeze").toBool();
    m_someChunksWasRemoved->value = state.value("some_chunk_was_removed").toBool();
    m_uploadFinished->value = state.value("upload_finished").toBool();
    const auto fileInfo = state.value("file").toObject();
    m_fileInfo->name = fileInfo.value("name").toString();
    m_fileInfo->size = fileInfo.value("size").toInteger();

    const auto transferred = data.value("transferred").toObject();
    const auto transferredGlobal = transferred.value("global").toObject();
    m_transferCounter->fromSender = transferredGlobal.value("from_sender").toInteger();
    m_transferCounter->toReceivers = transferredGlobal.value("to_receivers").toInteger();
    m_receivedByMe->value = transferred.value("received_by_you").toInteger();
}

void SessionState::onNewReceiver(const QJsonObject &data)
{
    auto *member = new SessionStateStructures::Member(this);
    member->currentChunk = 0;
    member->isOnline = true;
    member->name = data.value("name").toString();
    member->id = data.value("id").toString();
    m_receivers.insert(member->id, member);
}

void SessionState::onFileInfo(const QJsonObject &data)
{
    m_fileInfo->name = data.value("name").toString();
    m_fileInfo->size = data.value("size").toInteger();
    emit m_fileInfo->updated();
}



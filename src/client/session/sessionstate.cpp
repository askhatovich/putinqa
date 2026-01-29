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
    , m_newChunkIsAllowed(new SessionStateStructures::OneValue<bool>(this))
    , m_sender(new SessionStateStructures::Member(this))
    , m_fileInfo(new SessionStateStructures::FileInfo(this))
    , m_transferCounter(new SessionStateStructures::TransferCounter(this))
    , m_receivers(new SessionStateStructures::OneValue<QMap<QString, SessionStateStructures::Member*>>(this))
    , m_chunks(new SessionStateStructures::OneValue<QMap<qint64, SessionStateStructures::Chunk>>(this))
{
    m_newChunkIsAllowed->value = true;
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

const SessionStateStructures::OneValue<bool> *SessionState::getNewChunkIsAllowed() const
{
    return m_newChunkIsAllowed;
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

const SessionStateStructures::OneValue<QMap<QString, SessionStateStructures::Member*>> *SessionState::getReceivers() const
{
    return m_receivers;
}

const SessionStateStructures::OneValue<QMap<qint64, SessionStateStructures::Chunk>> *SessionState::getChunks() const
{
    return m_chunks;
}

QString SessionState::dump() const
{
    QString string;

    string += QStringLiteral("ID: %1\n").arg(m_sessionId);
    string += QStringLiteral("Limits: Chunks[size=%1;queue=%2] Receivers=%3 Freeze=%4\n")
                  .arg(m_limits.maxChunkSize)
                  .arg(m_limits.maxChunkQueue)
                  .arg(m_limits.maxReceiverCount)
                  .arg(m_limits.maxInitialFreeze);
    string += QStringLiteral("Last uploaded chunk: %1\n").arg(m_lastUploadedChunk->value);
    string += QStringLiteral("Expire: %1\n").arg(QDateTime::fromSecsSinceEpoch(m_expireTimestamp->value).toString("yyyy-MM-dd hh:mm:ss"));
    string += QStringLiteral("Received bytes: %1\n").arg(m_receivedByMe->value);
    string += QStringLiteral("Initial freeze: %1\n").arg(m_initialFreeze->value ? "yes" : "no");
    string += QStringLiteral("Some chunk was removed: %1\n").arg(m_someChunksWasRemoved->value ? "yes" : "no");
    string += QStringLiteral("Upload finished: %1\n").arg(m_uploadFinished->value ? "yes" : "no");
    string += QStringLiteral("New chunk is allowed: %1\n").arg(m_newChunkIsAllowed->value ? "yes" : "no");
    string += QStringLiteral("Sender: id=%1 online=%2 name=%3\n")
                  .arg(m_sender->id)
                  .arg(m_sender->isOnline)
                  .arg(m_sender->name);
    string += QStringLiteral("File info: size=%1 name=%2\n").arg(m_fileInfo->size).arg(m_fileInfo->name);
    string += QStringLiteral("Transfer count: from_sender=%1 to_receivers=%2\n")
                  .arg(m_transferCounter->fromSender)
                  .arg(m_transferCounter->toReceivers);

    string += QStringLiteral("Receivers (%1):\n").arg(m_receivers->value.size());
    for (auto iter = m_receivers->value.begin(); iter != m_receivers->value.end(); ++iter) {
        string += QStringLiteral("  id=%1 online=%2 chunk=%3%4 name=%5\n")
                      .arg(iter.value()->id)
                      .arg(iter.value()->isOnline)
                      .arg(iter.value()->currentChunk.index)
                      .arg(iter.value()->currentChunk.inProgress ? "^" : ".")
                      .arg(iter.value()->name);
    }

    string += QStringLiteral("Chunks (%1):\n").arg(m_chunks->value.size());
    for (auto iter = m_chunks->value.begin(); iter != m_chunks->value.end(); ++iter) {
        string += QStringLiteral("  index=%1 size=%2\n")
                      .arg(iter.value().index)
                      .arg(iter.value().size);
    }

    return string;
}

void SessionState::processEventJson(const QJsonObject &event)
{
    const auto type = event.value("event").toString();
    if (type.isEmpty()) {
        qWarning().noquote() << "SessionState::processEventJson: Empty 'event' in" << QJsonDocument(event).toJson(QJsonDocument::Compact);
        return;
    }
    qInfo().noquote() << "SessionState::processEventJson" << type; // DEBUG

    const auto data = event.value("data").toObject();

    if (type == "start_init") {
        onStartInit(data);
    }
    else if (type == "online") {
        onOnline(data);
    }
    else if (type == "name_changed") {
        onNameChanged(data);
    }
    else if (type == "new_receiver") {
        onNewReceiver(data);
    }
    else if (type == "receiver_removed") {
        onReceiverRemoved(data);
    }
    else if (type == "file_info") {
        onFileInfo(data);
    }
    else if (type == "new_chunk") {
        onNewChunk(data);
    }
    else if (type == "chunk_download") {
        onChunkDownload(data);
    }
    else if (type == "chunk_removed") {
        onChunkRemoved(data);
    }
    else if (type == "bytes_count") {
        onBytesCount(data);
    }
    else if (type == "personal_received") {
        onBytesReceived(data);
    }
    else if (type == "chunks_unfrozen") {
        onChunksUnfrozen();
    }
    else if (type == "upload_finished") {
        onUploadFinished();
    }
    else if (type == "complete") {
        onComplete(data);
    }
    else if (type == "new_chunk_allowed") {
        onNewChunkIsAllowed(data);
    }
    else {
        qWarning().noquote() << "SessionState::processEventJson: Unknown event type:" << type;
        return;
    }

    emit updated();
}

void SessionState::onOnline(const QJsonObject &data)
{
    const auto id = data.value("id").toString();
    const bool status = data.value("status").toBool();

    if (m_sender->id == id) {
        if (m_sender->isOnline == status) {
            return;
        }
        m_sender->isOnline = status;
        emit m_sender->updated();
        return;
    }

    auto iter = m_receivers->value.find(id);
    if (iter == m_receivers->value.end()) {
        qWarning().noquote() << "SessionState::onOnline no member found with id" << id;
        return;
    }
    if (iter.value()->isOnline == status) {
        return;
    }

    iter.value()->isOnline = status;
    emit iter.value()->updated();
}

void SessionState::onNameChanged(const QJsonObject &data)
{
    const auto id = data.value("id").toString();
    const auto name = data.value("name").toString();

    if (m_sender->id == id) {
        if (m_sender->name == name) {
            return;
        }
        m_sender->name = name;
        emit m_sender->updated();
        return;
    }

    auto iter = m_receivers->value.find(id);
    if (iter == m_receivers->value.end()) {
        qWarning().noquote() << "SessionState::onNameChanged no member found with id" << id;
        return;
    }
    if (iter.value()->name == name) {
        return;
    }

    iter.value()->name = name;
    emit iter.value()->updated();
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
    m_sender->id = sender.value("id").toString();
    m_sender->isOnline = sender.value("is_online").toBool();
    m_sender->name = sender.value("name").toString();

    const auto receiversArray = members.value("receivers").toArray();
    m_receivers->value.clear();
    for (const auto &node: receiversArray) {
        const auto object = node.toObject();

        auto *member = new SessionStateStructures::Member(this);
        member->currentChunk.index = object.value("current_chunk").toInteger();
        member->id = object.value("id").toString();
        member->isOnline = object.value("is_online").toBool();
        member->name = object.value("name").toString();

        m_receivers->value.insert(member->id, member);
    }

    const auto state = data.value("state").toObject();
    const auto chunks = state.value("chunks").toArray();
    m_chunks->value.clear();
    for (const auto &node: chunks) {
        const auto object = node.toObject();

        SessionStateStructures::Chunk chunk;
        chunk.index = object.value("index").toInteger();
        chunk.size = object.value("size").toInteger();

        m_chunks->value.insert(chunk.index, chunk);
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
    member->isOnline = true;
    member->name = data.value("name").toString();
    member->id = data.value("id").toString();
    m_receivers->value.insert(member->id, member);
}

void SessionState::onReceiverRemoved(const QJsonObject &data)
{
    const auto id = data.value("id").toString();

    auto iter = m_receivers->value.find(id);
    if (iter == m_receivers->value.end()) {
        qWarning().noquote() << "SessionState::onReceiverRemoved no member found with id" << id;
        return;
    }

    m_receivers->value.erase(iter);
    emit m_receivers->updated();
}

void SessionState::onFileInfo(const QJsonObject &data)
{
    m_fileInfo->name = data.value("name").toString();
    m_fileInfo->size = data.value("size").toInteger();
    emit m_fileInfo->updated();
}

void SessionState::onNewChunk(const QJsonObject &data)
{
    SessionStateStructures::Chunk chunk;
    chunk.index = data.value("index").toInteger();
    chunk.size = data.value("size").toInteger();
    m_chunks->value.insert(chunk.index, chunk);
    emit m_chunks->updated();
}

void SessionState::onChunkDownload(const QJsonObject &data)
{
    const auto id = data.value("id").toString();
    const auto index = data.value("index").toInteger();
    const auto action = data.value("action").toString();

    auto iter = m_receivers->value.find(id);
    if (iter == m_receivers->value.end()) {
        qWarning().noquote() << "SessionState::onChunkDownload no member found with id" << id;
        return;
    }


}

void SessionState::onChunkRemoved(const QJsonObject &data)
{
    const auto jArray = data.value("id").toArray();
    bool removed = false;
    for (auto iter = jArray.begin(); iter != jArray.end(); ++iter) {
        removed = removed || m_chunks->value.remove(iter->toInteger());
    }
    if (removed) {
        emit m_chunks->updated();
    }
}

void SessionState::onBytesCount(const QJsonObject &data)
{
    const auto value = data.value("value").toInteger();
    const auto direction = data.value("direction").toString();

    if (direction == "from_sender") {
        m_transferCounter->fromSender += value;
    } else { // to_receivers
        m_transferCounter->toReceivers += value;
    }

    emit m_transferCounter->updated();
}

void SessionState::onBytesReceived(const QJsonObject &data)
{
    m_receivedByMe->value = data.value("bytes").toInteger();
    emit m_receivedByMe->updated();
}

void SessionState::onChunksUnfrozen()
{
    m_initialFreeze->value = false;
    emit m_initialFreeze->updated();
}

void SessionState::onUploadFinished()
{
    m_uploadFinished->value = true;
    emit m_uploadFinished->updated();
}

void SessionState::onComplete(const QJsonObject &data)
{
    const auto status = data.value("status").toString();
    emit complete(status);
}

void SessionState::onNewChunkIsAllowed(const QJsonObject &data)
{
    const bool status = data.value("status").toBool();
    if (m_newChunkIsAllowed->value == status) {
        return;
    }
    m_newChunkIsAllowed->value = status;
    emit m_newChunkIsAllowed->updated();
}



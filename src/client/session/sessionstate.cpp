// Copyright (C) 2026  Roman Lyubimov
// SPDX-License-Identifier: GPL-3.0-or-later
// For full license text, see <https://www.gnu.org/licenses/gpl-3.0.txt>

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

const QString &SessionState::getSessionId() const { return m_sessionId; }
const SessionStateStructures::Limits &SessionState::getLimits() const { return m_limits; }
const SessionStateStructures::OneValue<qint64> *SessionState::getLastUploadedChunk() const { return m_lastUploadedChunk; }
const SessionStateStructures::OneValue<qint64> *SessionState::getExpireTimestamp() const { return m_expireTimestamp; }
const SessionStateStructures::OneValue<qint64> *SessionState::getReceivedByMe() const { return m_receivedByMe; }
const SessionStateStructures::OneValue<bool> *SessionState::getInitialFreeze() const { return m_initialFreeze; }
const SessionStateStructures::OneValue<bool> *SessionState::getSomeChunkWasRemoved() const { return m_someChunksWasRemoved; }
const SessionStateStructures::OneValue<bool> *SessionState::getUploadFinished() const { return m_uploadFinished; }
const SessionStateStructures::OneValue<bool> *SessionState::getNewChunkIsAllowed() const { return m_newChunkIsAllowed; }
const SessionStateStructures::Member *SessionState::getSender() const { return m_sender; }
const SessionStateStructures::FileInfo *SessionState::getFileInfo() const { return m_fileInfo; }
const SessionStateStructures::TransferCounter *SessionState::getTransferCounter() const { return m_transferCounter; }
const SessionStateStructures::OneValue<QMap<QString, SessionStateStructures::Member*>> *SessionState::getReceivers() const { return m_receivers; }
const SessionStateStructures::OneValue<QMap<qint64, SessionStateStructures::Chunk>> *SessionState::getChunks() const { return m_chunks; }

QString SessionState::dump() const
{
    QString string;
    string += QStringLiteral("ID: %1\n").arg(m_sessionId);
    string += QStringLiteral("Limits: Chunks[size=%1;queue=%2] Receivers=%3 Freeze=%4\n")
                  .arg(m_limits.maxChunkSize).arg(m_limits.maxChunkQueue)
                  .arg(m_limits.maxReceiverCount).arg(m_limits.maxInitialFreeze);
    string += QStringLiteral("Last uploaded chunk: %1\n").arg(m_lastUploadedChunk->value);
    string += QStringLiteral("Expire: %1\n").arg(QDateTime::fromSecsSinceEpoch(m_expireTimestamp->value).toString("yyyy-MM-dd hh:mm:ss"));
    string += QStringLiteral("Received bytes: %1\n").arg(m_receivedByMe->value);
    string += QStringLiteral("Initial freeze: %1\n").arg(m_initialFreeze->value ? "yes" : "no");
    string += QStringLiteral("Upload finished: %1\n").arg(m_uploadFinished->value ? "yes" : "no");
    string += QStringLiteral("New chunk allowed: %1\n").arg(m_newChunkIsAllowed->value ? "yes" : "no");
    string += QStringLiteral("Sender: id=%1 online=%2 name=%3\n")
                  .arg(m_sender->id).arg(m_sender->isOnline).arg(m_sender->name);
    string += QStringLiteral("File: size=%1 name=%2\n").arg(m_fileInfo->size).arg(m_fileInfo->name);
    string += QStringLiteral("Transfer: from_sender=%1 to_receivers=%2\n")
                  .arg(m_transferCounter->fromSender).arg(m_transferCounter->toReceivers);

    string += QStringLiteral("Receivers (%1):\n").arg(m_receivers->value.size());
    for (auto it = m_receivers->value.begin(); it != m_receivers->value.end(); ++it) {
        string += QStringLiteral("  id=%1 online=%2 chunk=%3 name=%4\n")
                      .arg(it.value()->id).arg(it.value()->isOnline)
                      .arg(it.value()->currentChunk.index).arg(it.value()->name);
    }

    string += QStringLiteral("Chunks (%1):\n").arg(m_chunks->value.size());
    for (auto it = m_chunks->value.begin(); it != m_chunks->value.end(); ++it) {
        string += QStringLiteral("  index=%1 size=%2\n").arg(it.value().index).arg(it.value().size);
    }

    return string;
}

void SessionState::processEventJson(const QJsonObject &event)
{
    const auto type = event.value("event").toString();
    if (type.isEmpty()) {
        qWarning().noquote() << "SessionState: empty event type in" << QJsonDocument(event).toJson(QJsonDocument::Compact);
        return;
    }

    const auto data = event.value("data").toObject();

    if (type == "start_init") onStartInit(data);
    else if (type == "online") onOnline(data);
    else if (type == "name_changed") onNameChanged(data);
    else if (type == "new_receiver") onNewReceiver(data);
    else if (type == "receiver_removed") onReceiverRemoved(data);
    else if (type == "file_info") onFileInfo(data);
    else if (type == "new_chunk") onNewChunk(data);
    else if (type == "chunk_download") onChunkDownload(data);
    else if (type == "chunk_removed") onChunkRemoved(data);
    else if (type == "bytes_count") onBytesCount(data);
    else if (type == "personal_received") onBytesReceived(data);
    else if (type == "chunks_unfrozen") onChunksUnfrozen();
    else if (type == "upload_finished") onUploadFinished();
    else if (type == "complete") onComplete(data);
    else if (type == "new_chunk_allowed") onNewChunkIsAllowed(data);
    else {
        qWarning().noquote() << "SessionState: unknown event:" << type;
        return;
    }

    emit updated();
}

void SessionState::onStartInit(const QJsonObject &data)
{
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
    qDeleteAll(m_receivers->value);
    m_receivers->value.clear();
    for (const auto &node : receiversArray) {
        const auto obj = node.toObject();
        auto *member = new SessionStateStructures::Member(this);
        member->currentChunk.index = obj.value("current_chunk").toInteger();
        member->id = obj.value("id").toString();
        member->isOnline = obj.value("is_online").toBool();
        member->name = obj.value("name").toString();
        m_receivers->value.insert(member->id, member);
    }

    const auto state = data.value("state").toObject();
    const auto chunks = state.value("chunks").toArray();
    m_chunks->value.clear();
    for (const auto &node : chunks) {
        const auto obj = node.toObject();
        SessionStateStructures::Chunk chunk;
        chunk.index = obj.value("index").toInteger();
        chunk.size = obj.value("size").toInteger();
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

    emit sessionInitialized();
}

void SessionState::onOnline(const QJsonObject &data)
{
    const auto id = data.value("id").toString();
    const bool status = data.value("status").toBool();

    if (m_sender->id == id) {
        if (m_sender->isOnline != status) {
            m_sender->isOnline = status;
            emit m_sender->updated();
        }
    } else {
        auto it = m_receivers->value.find(id);
        if (it != m_receivers->value.end() && it.value()->isOnline != status) {
            it.value()->isOnline = status;
            emit it.value()->updated();
        }
    }

    emit onlineEvent(id, status);
}

void SessionState::onNameChanged(const QJsonObject &data)
{
    const auto id = data.value("id").toString();
    const auto name = data.value("name").toString();

    if (m_sender->id == id) {
        if (m_sender->name != name) {
            m_sender->name = name;
            emit m_sender->updated();
        }
    } else {
        auto it = m_receivers->value.find(id);
        if (it != m_receivers->value.end() && it.value()->name != name) {
            it.value()->name = name;
            emit it.value()->updated();
        }
    }

    emit nameChangedEvent(id, name);
}

void SessionState::onNewReceiver(const QJsonObject &data)
{
    auto *member = new SessionStateStructures::Member(this);
    member->isOnline = true;
    member->name = data.value("name").toString();
    member->id = data.value("id").toString();
    m_receivers->value.insert(member->id, member);

    emit newReceiverEvent(member->id, member->name);
}

void SessionState::onReceiverRemoved(const QJsonObject &data)
{
    const auto id = data.value("id").toString();
    auto it = m_receivers->value.find(id);
    if (it != m_receivers->value.end()) {
        delete it.value();
        m_receivers->value.erase(it);
        emit m_receivers->updated();
    }

    emit receiverRemovedEvent(id);
}

void SessionState::onFileInfo(const QJsonObject &data)
{
    m_fileInfo->name = data.value("name").toString();
    m_fileInfo->size = data.value("size").toInteger();
    emit m_fileInfo->updated();
    emit fileInfoEvent(m_fileInfo->name, m_fileInfo->size);
}

void SessionState::onNewChunk(const QJsonObject &data)
{
    SessionStateStructures::Chunk chunk;
    chunk.index = data.value("index").toInteger();
    chunk.size = data.value("size").toInteger();
    m_chunks->value.insert(chunk.index, chunk);
    emit m_chunks->updated();
    emit newChunkEvent(chunk.index, chunk.size);
}

void SessionState::onChunkDownload(const QJsonObject &data)
{
    const auto id = data.value("id").toString();
    const auto index = data.value("index").toInteger();
    const auto action = data.value("action").toString();

    auto it = m_receivers->value.find(id);
    if (it != m_receivers->value.end()) {
        it.value()->currentChunk.index = index;
        it.value()->currentChunk.inProgress = (action == "started");
        emit it.value()->updated();
    }

    if (action == "finished") {
        emit chunkDownloadFinishedEvent(id, index);
    }
}

void SessionState::onChunkRemoved(const QJsonObject &data)
{
    const auto jArray = data.value("id").toArray();
    bool removed = false;
    for (const auto &val : jArray) {
        removed = m_chunks->value.remove(val.toInteger()) || removed;
    }
    if (removed) {
        emit m_chunks->updated();
        emit chunkRemovedEvent();
    }
}

void SessionState::onBytesCount(const QJsonObject &data)
{
    const auto value = data.value("value").toInteger();
    const auto direction = data.value("direction").toString();

    if (direction == "from_sender") {
        m_transferCounter->fromSender = value;
    } else {
        m_transferCounter->toReceivers = value;
    }

    emit m_transferCounter->updated();
    emit bytesCountEvent(direction, value);
}

void SessionState::onBytesReceived(const QJsonObject &data)
{
    m_receivedByMe->value = data.value("bytes").toInteger();
    emit m_receivedByMe->updated();
    emit personalReceivedEvent(m_receivedByMe->value);
}

void SessionState::onChunksUnfrozen()
{
    m_initialFreeze->value = false;
    emit m_initialFreeze->updated();
    emit freezeDroppedEvent();
}

void SessionState::onUploadFinished()
{
    m_uploadFinished->value = true;
    emit m_uploadFinished->updated();
    emit uploadFinishedEvent();
}

void SessionState::onComplete(const QJsonObject &data)
{
    const auto status = data.value("status").toString();
    emit complete(status);
}

void SessionState::onNewChunkIsAllowed(const QJsonObject &data)
{
    const bool status = data.value("status").toBool();
    if (m_newChunkIsAllowed->value != status) {
        m_newChunkIsAllowed->value = status;
        emit m_newChunkIsAllowed->updated();
    }
    emit newChunkAllowedEvent(status);
}

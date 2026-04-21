// Copyright (C) 2026  Roman Lyubimov
// SPDX-License-Identifier: GPL-3.0-or-later
// For full license text, see <https://www.gnu.org/licenses/gpl-3.0.txt>

#pragma once

#include <QJsonObject>

namespace Action {

struct Serializable
{
    Serializable() = default;
    virtual ~Serializable() = default;
    virtual QJsonObject json() const = 0;
};

// Admin actions (session creator only)

struct SetFileInfo : public Serializable
{
    SetFileInfo(const QString &name, qint64 size) : m_name(name), m_size(size) {}
    QJsonObject json() const override;

private:
    QString m_name;
    qint64 m_size;
};

struct UploadFinished : public Serializable
{
    QJsonObject json() const override;
};

struct KickReceiver : public Serializable
{
    KickReceiver(const QString &id) : m_id(id) {}
    QJsonObject json() const override;

private:
    QString m_id;
};

struct TerminateSession : public Serializable
{
    QJsonObject json() const override;
};

struct DropFreeze : public Serializable
{
    QJsonObject json() const override;
};

// General actions

struct NewName : public Serializable
{
    NewName(const QString &name) : m_name(name) {}
    QJsonObject json() const override;

private:
    QString m_name;
};

struct GetChunk : public Serializable
{
    GetChunk(qint64 index) : m_index(index) {}
    QJsonObject json() const override;

private:
    qint64 m_index;
};

struct ConfirmChunk : public Serializable
{
    ConfirmChunk(qint64 index) : m_index(index) {}
    QJsonObject json() const override;

private:
    qint64 m_index;
};

// Acknowledge a server→client event that carried a top-level "id" field.
// The server waits for this before closing the WebSocket on terminal
// events (complete, kicked). Fallback timer kicks in at ~2 s if omitted.
struct Ack : public Serializable
{
    Ack(qint64 id) : m_id(id) {}
    QJsonObject json() const override;

private:
    qint64 m_id;
};

} // namespace Action

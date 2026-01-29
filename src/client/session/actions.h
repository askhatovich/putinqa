#pragma once

#include <QJsonObject>

namespace Action {
struct Serializable
{
    Serializable() = default;
    virtual ~Serializable() = default;

    virtual QJsonObject json() const = 0;
};

// Admin options for the session creator

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

} // namespace Action

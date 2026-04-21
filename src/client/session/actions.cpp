// Copyright (C) 2026  Roman Lyubimov
// SPDX-License-Identifier: GPL-3.0-or-later
// For full license text, see <https://www.gnu.org/licenses/gpl-3.0.txt>

#include "actions.h"

QJsonObject Action::SetFileInfo::json() const
{
    return {
        {"action", "set_file_info"},
        {"data", QJsonObject{{"name", m_name}, {"size", m_size}}}
    };
}

QJsonObject Action::UploadFinished::json() const
{
    return {{"action", "upload_finished"}, {"data", QJsonObject()}};
}

QJsonObject Action::KickReceiver::json() const
{
    return {
        {"action", "kick_receiver"},
        {"data", QJsonObject{{"id", m_id}}}
    };
}

QJsonObject Action::TerminateSession::json() const
{
    return {{"action", "terminate_session"}, {"data", QJsonObject()}};
}

QJsonObject Action::DropFreeze::json() const
{
    return {{"action", "drop_freeze"}, {"data", QJsonObject()}};
}

QJsonObject Action::NewName::json() const
{
    return {
        {"action", "new_name"},
        {"data", QJsonObject{{"name", m_name}}}
    };
}

QJsonObject Action::GetChunk::json() const
{
    return {
        {"action", "get_chunk"},
        {"data", QJsonObject{{"index", m_index}}}
    };
}

QJsonObject Action::ConfirmChunk::json() const
{
    return {
        {"action", "confirm_chunk"},
        {"data", QJsonObject{{"index", m_index}}}
    };
}

QJsonObject Action::Ack::json() const
{
    return {
        {"action", "ack"},
        {"data", QJsonObject{{"id", m_id}}}
    };
}

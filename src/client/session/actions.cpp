#include "actions.h"

QJsonObject Action::SetFileInfo::json() const
{
    QJsonObject root;
    root["action"] = "set_file_info";

    QJsonObject data;
    data["name"] = m_name;
    data["size"] = m_size;

    root["data"] = data;

    return root;
}

QJsonObject Action::UploadFinished::json() const
{
    QJsonObject root;
    root["action"] = "upload_finished";
    root["data"] = QJsonObject();
    return root;
}

QJsonObject Action::KickReceiver::json() const
{
    QJsonObject root;
    root["action"] = "kick_receiver";

    QJsonObject data;
    data["id"] = m_id;

    root["data"] = data;

    return root;
}

QJsonObject Action::TerminateSession::json() const
{
    QJsonObject root;
    root["action"] = "terminate_session";
    root["data"] = QJsonObject();
    return root;
}

QJsonObject Action::NewName::json() const
{
    QJsonObject root;
    root["action"] = "new_name";

    QJsonObject data;
    data["name"] = m_name;

    root["data"] = data;

    return root;
}

QJsonObject Action::GetChunk::json() const
{
    QJsonObject root;
    root["action"] = "get_chunk";

    QJsonObject data;
    data["index"] = m_index;

    root["data"] = data;

    return root;
}

QJsonObject Action::ConfirmChunk::json() const
{
    QJsonObject root;
    root["action"] = "confirm_chunk";

    QJsonObject data;
    data["index"] = m_index;

    root["data"] = data;

    return root;
}

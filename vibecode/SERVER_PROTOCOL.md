# Server Protocol (put-in-pipe)

Server location: `../put-in-pipe/` (C++20, Crow framework, ASIO).

## HTTP Endpoints

| Method | Path | Auth | Purpose |
|--------|------|------|---------|
| GET | `/api/identity/request?name=<name>` | No | Request client ID. 201=ok, 401=captcha, 503=full |
| POST | `/api/identity/confirmation` | No | Submit captcha answer. 201=ok, 403=wrong |
| GET | `/api/me/info` | Cookie | Current client info |
| POST | `/api/me/leave` | Cookie | Delete client |
| POST | `/api/session/create` | Cookie | Create transfer session (sender) |
| GET | `/api/session/join?id=<id>` | Cookie | Join session (receiver) |
| GET | `/api/session/chunk?id=<index>` | Cookie | Download chunk by index |
| POST | `/api/session/chunk` | Cookie | Upload chunk (HTTP alternative) |
| GET | `/api/statistics/current` | No | Server workload stats |
| WS | `/api/ws` | Cookie | WebSocket for real-time events |

## Authentication

Cookie-based: server sets HttpOnly `putin=<token>` cookie on successful identity request. All subsequent HTTP and WS requests must include this cookie.

**Captcha flow:** When server load exceeds threshold, identity request returns 401 with:
```json
{"captcha_image": "<base64_png>", "captcha_token": "...", "client_id": "...", "captcha_answer_length": 5}
```
Client submits answer via POST to `/api/identity/confirmation`.

## WebSocket Events (Server → Client)

| Event | Data | Description |
|-------|------|-------------|
| `start_init` | Full session state | Sent on WS connect. Contains limits, members, chunks, transfer stats |
| `online` | `{id, status}` | Member online/offline change |
| `name_changed` | `{id, name}` | Member name updated. **NOT echoed to sender of the action** |
| `new_receiver` | `{id, name}` | New receiver joined session |
| `receiver_removed` | `{id}` | Receiver left/kicked |
| `file_info` | `{name, size}` | File metadata set by sender |
| `new_chunk` | `{index, size}` | New chunk available in buffer |
| `chunk_download` | `{id, index, action}` | `action`: "started" or "finished". Per-receiver chunk progress |
| `chunk_removed` | `{id: [indices]}` | Chunks removed from buffer (confirmed by all receivers) |
| `bytes_count` | `{value, direction}` | `direction`: "from_sender" or "to_receivers" |
| `personal_received` | `{bytes}` | Receiver's personal received byte count |
| `chunks_unfrozen` | `{}` | Initial freeze dropped |
| `upload_finished` | `{}` | Sender finished uploading all data |
| `complete` | `{status}` | Session ended. See completion statuses below |
| `new_chunk_allowed` | `{status}` | Sender-only: buffer has space for more chunks |

## WebSocket Actions (Client → Server)

| Action | Role | Data | Description |
|--------|------|------|-------------|
| `set_file_info` | Sender | `{name, size}` | Set file metadata |
| `upload_finished` | Sender | `{}` | Signal all data uploaded |
| `drop_freeze` | Sender | `{}` | Manually drop initial freeze |
| `kick_receiver` | Sender | `{id}` | Remove receiver from session |
| `terminate_session` | Sender | `{}` | Force session termination |
| `new_name` | Any | `{name}` | Change display name (truncated to 20 chars by server) |
| `confirm_chunk` | Receiver | `{index}` | Confirm chunk received |
| Binary frame | Sender | raw bytes | Upload encrypted chunk |

## Session Completion Statuses

| Status | Meaning |
|--------|---------|
| `ok` | All chunks confirmed by all receivers, sender finished |
| `timeout` | Session lifetime exceeded (default 2 hours) |
| `sender_is_gone` | Sender disconnected before upload finished, or manual terminate |
| `no_receivers` | All receivers left (and some activity occurred) |

## Server Limits (defaults)

- Max clients: 500
- Max sessions: 100
- Max receivers per session: 5
- Max chunk size: 5 MB
- Max chunks in buffer: 10
- Session lifetime: 7200s (2 hours)
- Initial freeze: 120s
- Client timeout (no WS): 60s

## Initial Freeze Mechanism

When a session is created, chunks are in "frozen" state. During freeze:
- Sender can upload chunks normally
- Receivers can download chunks normally
- **BUT chunks are NOT removed from buffer after confirmation**

This allows new receivers to join and get existing chunks. Freeze is dropped by:
1. Sender sending `drop_freeze` action
2. Server's auto-timer (120s default)

**After freeze drops:** confirmed chunks are immediately sanitized (removed). If all chunks confirmed and EOF set → session completes.

## Completion Flow

Server determines session complete when: `chunkCount == 0 && eof == true`

1. Session destructor sends `complete` event to all subscribers
2. **1-second delay** (asio timer) before removing clients from ClientList
3. Client destructors close WebSocket connections

The 1-second delay ensures `complete` text frame is delivered before WS close frame.

## Important Server Behaviors

1. **name_changed is NOT echoed** to the client who changed their name. Only other participants receive it.
2. **Sender can't be kicked.** Only receivers can be kicked or removed.
3. **Name truncation:** Server truncates names to 20 characters before broadcasting.
4. **Single WS per client:** Reconnecting drops previous connection.
5. **Chunk download events** (`chunk_download` with action "started"/"finished") are broadcast to ALL participants, including the sender. Sender can track per-receiver download progress from these.
6. **receiver_removed** is NOT sent to the kicked receiver. Their WS is simply closed.

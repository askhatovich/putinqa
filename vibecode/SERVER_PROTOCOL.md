# Server Protocol (put-in-pipe)

Server location: `../put-in-pipe/` (C++20, Crow framework, ASIO).

**Tracked server version: v1.1.0.** The protocol is not backward-compatible with v1.0.0 and earlier — client and server must be upgraded together.

## HTTP Endpoints

| Method | Path | Auth | Purpose |
|--------|------|------|---------|
| GET | `/api/identity/request?name=<name>` | No | Request client ID. 201=ok, 401=captcha, 503=full |
| POST | `/api/identity/confirmation` | No | Submit captcha answer. 201=ok, 403=wrong |
| GET | `/api/me/info` | Cookie | Current client info |
| POST | `/api/me/leave` | Cookie | Delete client |
| POST | `/api/session/create` | Cookie | Create transfer session (sender). Optional JSON body: `{"auto_drop_freeze": bool}` |
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
| `complete` | `{status}` | Session ended. See completion statuses below. **ACK-required** |
| `kicked` | `{}` | Sent to a receiver the sender just removed, before the WS close. **ACK-required** |
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
| `ack` | Any | `{id}` | Acknowledge a server event that carried an `id` field |
| Binary frame | Sender | raw bytes | Upload encrypted chunk |

## Event Acknowledgment (v1.1.0+)

Server→client events that precede a WebSocket close carry a top-level `id` (uint64) field. The client **must** reply with `{"action":"ack","data":{"id":<same id>}}`. The server closes the WebSocket only after the ACK arrives, or after a ~2-second fallback timer for dead clients. This replaces the previous 1-second arbitrary delay that raced WS close frames against the final text frame.

- ACK-required events today: `complete`, `kicked`.
- Other events remain fire-and-forget (no `id` field).
- A client that does not ACK will still see the close — it just happens 2 s later via the fallback.

Example wire exchange:

```json
// server → client
{"event": "complete", "id": 17, "data": {"status": "ok"}}

// client → server
{"action": "ack", "data": {"id": 17}}
```

### Qt client implementation

`Session::onWsText` (src/client/session/session.cpp) detects a top-level `id` on every incoming event and sends the `ack` action via `Action::Ack` immediately, before dispatching the event to `SessionState`. No per-event opt-in is needed.

## Session Create Options (v1.1.0+)

Optional JSON body on `POST /api/session/create`:

| Field | Type | Default | Meaning |
|---|---|---|---|
| `auto_drop_freeze` | bool | `false` | Drop initial freeze automatically on the first confirmed chunk — no manual "start transfer" button needed. **Also** changes session termination: when the last receiver leaves, the session ends with `complete.status = "ok"` regardless of buffer state (fire-and-forget). |

Qt client: `Session::create(bool autoDropFreeze)` (src/client/session/session.cpp). Value is read from `AppController::autoDropFreeze()`, which is persisted in QSettings key `session/auto_drop_freeze` (see SettingsScreen.qml toggle).

## Session Completion Statuses

| Status | Meaning |
|--------|---------|
| `ok` | All chunks confirmed by all receivers, sender finished. In auto-drop mode, also fires when the last receiver leaves (fire-and-forget). |
| `timeout` | Session lifetime exceeded (default 2 hours) |
| `sender_is_gone` | Sender disconnected before upload finished, or manual terminate |
| `no_receivers` | All receivers left without completing the transfer (non-auto-drop only) |

In addition, an individual receiver may be terminated with a `kicked` event (not a `complete` status) — see the ACK section above. The Qt client surfaces this as `completeStatus = "kicked"` via `SessionState::onKicked` → `Session::complete("kicked")`.

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

Server determines session complete in three places:
- `setChunkAsReceived`: last confirm arrives when EOF is already set → `chunkCount == 0 && eof`.
- `setEndOfFile`: EOF arrives when all chunks were already confirmed (handles the race where the last `confirm_chunk` wins over `upload_finished`).
- `removeReceiver`: last receiver leaves after the full transfer; the outcome is reclassified from `no_receivers` to `ok`.

Session destructor:

1. Sends `complete` event (with `id` field) to every subscribed client via ACK-tracked channel.
2. Each client, on ACK (or ~2 s fallback), removes itself from `ClientList`, triggering its own destructor and WebSocket close.

The 1-second arbitrary delay from earlier versions is **gone** — each client closes independently, as soon as its own `complete` event is acknowledged.

## Important Server Behaviors

1. **name_changed is NOT echoed** to the client who changed their name. Only other participants receive it.
2. **Sender can't be kicked.** Only receivers can be kicked or removed.
3. **Name truncation:** Server truncates names to 20 characters before broadcasting.
4. **Single WS per client:** Reconnecting drops previous connection.
5. **Chunk download events** (`chunk_download` with action "started"/"finished") are broadcast to ALL participants, including the sender. Sender can track per-receiver download progress from these.
6. **Kicked receiver gets `kicked` event (ACK-required), not `receiver_removed`.** Other participants still see `receiver_removed` for that id. The kicked receiver's WS closes after ACK or fallback.
7. **Terminal events are delivery-confirmed.** `complete` and `kicked` wait for ACK before the server tears down the WS (2 s fallback).

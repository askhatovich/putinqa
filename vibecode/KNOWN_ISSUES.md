# Known Issues & Edge Cases

## Server `complete` Event Race Condition

**Problem:** Server sends `complete` event from TransferSession destructor, then (after 1s delay) closes WS via Client destructor. In rare cases, the WS close frame may arrive before the `complete` text frame.

**Mitigation (server-side):** 1-second delay between sending `complete` and closing WS connections. Implemented in `TransferSession::~TransferSession()` using `asio::steady_timer`.

**Mitigation (client-side):** For sender, session completion is driven by `upload_finished` echo + freeze drop — not by `complete` event. For receiver, `complete` event is the primary completion signal. If WS closes without `complete`, receiver shows "Error" status.

## Server Does NOT Echo `name_changed` to Self

The server's pub/sub system sends `name_changed` only to OTHER participants. The client who changed their name does not receive the echo. Client-side workaround: update `m_senderName` or `m_receivers` locally after sending `NewName` action.

Server may truncate the name to 20 chars. Client pre-truncates locally (`m_userName.left(20)`) to match.

## Receiver Kicked Without `complete` Event

When a receiver is kicked (`kick_receiver` action), the server removes them from session subscribers BEFORE broadcasting `receiver_removed`. The kicked receiver's WS is closed directly — no `complete` event is sent to them.

Client handles this: `serverClosed && !m_isSender` → `onSessionComplete("error")`.

## Freeze + Small Files

For files that fit entirely in the server's chunk buffer (< maxChunkQueue * maxChunkSize), upload finishes before any receiver connects. Without freeze, server would remove confirmed chunks and complete with "no_receivers".

The freeze mechanism prevents this: chunks are retained during freeze, allowing receivers to connect and download. Sender stays on session screen until freeze drops.

**Important:** Client does NOT auto-drop freeze after upload. Freeze is dropped by:
1. Sender clicking "Stop waiting" button (when receivers are present)
2. Server's auto-timer (120s default)

## Parallel Downloads May Reorder Chunks

Receiver downloads up to 4 chunks concurrently. Chunks may arrive out of order. `m_downloadedChunks` is a `QMap<qint64, QByteArray>` that stores chunks by index. `saveReceivedFile()` assembles them in sorted order.

## WebSocket Cookie Injection

Qt does not automatically forward cookies to WebSocket upgrade requests. `WebSocketConnection` manually extracts cookies from `QNetworkCookieJar` and injects them as a raw "Cookie" header.

## ServerWorkload Polling Target

`ServerWorkload` polls the server specified in:
- During session: `m_activeServer` (server from link or settings)
- Outside session: `m_serverUrl` (server from settings)

This ensures footer stats match the server shown in the header.

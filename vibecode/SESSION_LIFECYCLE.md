# Session Lifecycle

## Sender Flow

```
1. User clicks "Send file" → startSend() → file dialog opens
2. User selects file → selectFile(url) → m_activeServer set → screen="connecting"
3. Authorization: GET /api/identity/request
   ├── 201: authorized → proceedAfterAuth()
   ├── 401: captcha required → screen="captcha" → user solves → screen="connecting"
   └── error: screen="entry" with error message
4. POST /api/session/create (JSON body `{auto_drop_freeze: <QSettings>}` if the toggle is on) → session ID received
5. WebSocket connects to /api/ws with session cookie
6. Server sends start_init → onSessionInitialized():
   - Generate encryption key
   - Build share link
   - Send set_file_info action
   - Open file, start upload loop → screen="sender"
7. Upload loop (uploadNextChunk):
   - Read chunk (maxChunkPayload = maxChunkSize - 40 bytes crypto overhead)
   - Encrypt with XChaCha20-Poly1305
   - Send binary frame via WS
   - Wait for new_chunk event (server accepted chunk)
   - If buffer full: pause, wait for new_chunk_allowed
   - If file exhausted: send upload_finished action
8. Server echoes upload_finished event:
   - If freeze already dropped → onSessionComplete("ok") immediately
   - If freeze still active → wait for freeze to drop
9. Freeze drops (chunks_unfrozen event or manual drop_freeze):
   - If upload already finished → onSessionComplete("ok")
10. Screen="complete" with status
```

## Receiver Flow

```
1. User pastes share link → startReceive(link)
2. Parse link fragment: extract id, key (base64url), encryption algorithm
3. Validate: xchacha20-poly1305 required, key must decode correctly
4. m_activeServer extracted from link URL (not from settings)
5. Authorization (same as sender)
6. GET /api/session/join?id=<sessionId>
7. WebSocket connects
8. Server sends start_init → onSessionInitialized():
   - Store limits, members, file info
   - Enqueue existing chunks for download
   - screen="receiver"
9. Download loop (processDownloadQueue):
   - Up to 4 parallel HTTP GETs to /api/session/chunk?id=<index>
   - Decrypt each chunk
   - Send confirm_chunk action via WS
   - Track m_pendingConfirms (incremented on send, decremented on chunk_download finished echo)
10. checkReceiverDone(): m_uploadFinished && chunksConfirmed >= highestKnownChunk && pendingConfirms == 0
    → m_hasDownloadedFile = true (enables save button on complete screen)
11. Server sends complete event → onSessionComplete(status)
12. Screen="complete", user can save file
```

## Sender Completion Logic (Critical)

The sender completes based on TWO conditions, order-independent:

```
upload_finished (from server echo)  ─┐
                                      ├─→ BOTH required → onSessionComplete("ok")
chunks_unfrozen (freeze dropped)    ─┘
```

- **Small file (fits in buffer):** Upload finishes instantly, but freeze is active. Sender stays on session screen (can copy link, see receivers). Completes when freeze drops.
- **Large file:** Freeze likely drops during upload (either manually or by 120s timer). When upload finishes, freeze is already gone → completes immediately.
- **No receivers:** If freeze times out with no receivers, server sends `complete("no_receivers")`.

This is intentional: sender must have a window to share the link before the session closes.

## Receiver Completion Logic

Receiver relies entirely on server's `complete` event. The `m_hasDownloadedFile` flag is a UI indicator only — it enables the "Save file" button but does NOT trigger session completion.

If the sender kicks this receiver, the server sends an explicit `kicked` event (ACK-required) before the close, and the client surfaces `completeStatus = "kicked"`. A silent close without `complete`/`kicked` still falls through to "Error".

## WebSocket Disconnect Handling

```
onWsConnection(connected=false, serverClosed):
  ├── terminateRequested → complete("terminated_by_you")
  ├── serverClosed && !isSender → complete("error")  [receiver kicked]
  └── otherwise → do nothing (WS may reconnect, or session just hangs)
```

500ms delay before fallback to allow pending `complete` event to arrive.

## Settings During Session

Settings can be opened during an active session. `m_screenBeforeSettings` stores the current screen. Save/Back returns to the previous screen. If name changed during session, `NewName` action is sent to server.

## Auto-drop freeze (opt-in)

`AppController::autoDropFreeze()` (persisted in QSettings as `session/auto_drop_freeze`, toggled from SettingsScreen.qml) is passed to `Session::create(bool)` on sender session start. When the flag is on:

- Server drops the initial freeze automatically on the first confirmed chunk — no need to press "Stop waiting".
- When the last receiver leaves, the server terminates with `status: "ok"` regardless of buffer state (fire-and-forget). Useful for unattended sends.

The flag has no effect for receivers.

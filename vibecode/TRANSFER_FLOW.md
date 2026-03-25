# Transfer Flow (Chunk-Level Detail)

## Upload (Sender)

```
                          ┌─────────────────────┐
                          │   uploadNextChunk()  │
                          └──────────┬──────────┘
                                     │
                          ┌──────────▼──────────┐
                          │  file.atEnd()?       │
                          └──┬──────────────┬───┘
                         yes │              │ no
                    ┌────────▼────────┐  ┌──▼──────────────────┐
                    │ send            │  │ read maxChunkPayload │
                    │ upload_finished │  │ encrypt(data, key)   │
                    │ m_uploadFinished│  │ send binary WS frame │
                    │ = true          │  │ m_waitingForChunk    │
                    └─────────────────┘  │ Accepted = true      │
                                         └──────────┬───────────┘
                                                     │
                                         ┌───────────▼───────────┐
                                         │ wait for new_chunk    │
                                         │ event from server     │
                                         └───────────┬───────────┘
                                                     │
                                         ┌───────────▼───────────┐
                                         │ buffer full?          │
                                         └──┬────────────────┬───┘
                                        yes │                │ no
                              ┌─────────────▼──────┐  ┌──────▼──────────┐
                              │ m_canSendChunk=false│  │ QTimer(0) →     │
                              │ wait for            │  │ uploadNextChunk │
                              │ new_chunk_allowed   │  └─────────────────┘
                              └─────────────────────┘
```

**Key constants:**
- `maxChunkPayload = maxChunkSize - 40` (40 bytes crypto overhead: 24 nonce + 16 tag)
- Default maxChunkSize: 5,242,880 bytes (5 MB)
- Default maxChunkQueue: 10 chunks

## Download (Receiver)

```
                          ┌──────────────────────┐
                          │ processDownloadQueue()│
                          └──────────┬───────────┘
                                     │
                          ┌──────────▼───────────┐
                          │ activeDownloads < 4   │
                          │ && queue not empty?   │
                          └──┬───────────────┬────┘
                         yes │               │ no (wait)
                    ┌────────▼────────┐      │
                    │ dequeue index   │      │
                    │ activeDownloads++│      │
                    │ HTTP GET chunk  │      │
                    └────────┬────────┘      │
                             │               │
                    ┌────────▼────────┐      │
                    │ on response:    │      │
                    │ decrypt(data)   │      │
                    │ store in map    │      │
                    │ send confirm_   │      │
                    │   chunk via WS  │      │
                    │ pendingConfirms++│      │
                    │ activeDownloads--│      │
                    │ → processQueue()│      │
                    └─────────────────┘      │
                                             │
                    ┌────────────────────────┐│
                    │ on chunk_download      ││
                    │   finished (echo):     ││
                    │ pendingConfirms--      ││
                    │ checkReceiverDone()    ││
                    └────────────────────────┘│
```

**Completion condition (checkReceiverDone):**
```
m_uploadFinished == true
  && m_writtenChunks not empty
  && m_chunksConfirmed >= m_highestKnownChunk
  && m_pendingConfirms == 0
```

This sets `m_hasDownloadedFile = true` (UI flag for save button). Actual session completion is driven by server's `complete` event.

## Per-Receiver Progress Tracking (Sender Side)

Sender tracks how many chunks each receiver has confirmed:

```cpp
QMap<QString, int> m_receiverChunksDone;  // receiverId → confirmed chunk count
```

Updated on each `chunk_download finished` event. When:
```
m_uploadFinished && m_highestKnownChunk > 0
  && m_receiverChunksDone[id] >= m_highestKnownChunk
```
→ receiver marked as `done` in QML (green checkmark in member list).

## Retry Logic

- Failed chunk downloads are re-enqueued **unless** HTTP 404 (chunk removed from server buffer)
- No retry limit for individual chunks
- Decrypt failures skip the chunk (logged as warning)

## Disk-Based Chunk Storage (Receiver)

Chunks are written to a temporary file on disk, NOT held in memory. This allows receiving files of any size (hundreds of GB).

**Flow:**
1. On session start, `openDownloadTmpFile()` creates a temp file in system temp directory (`/tmp/putinqa_<random>.tmp`)
2. Chunks are downloaded in parallel (up to 4). They may arrive out of order.
3. `flushChunksToDisk(index, data)`:
   - If `index == m_nextWriteIndex`: write directly to tmp file, then drain any buffered sequential chunks
   - If `index > m_nextWriteIndex`: buffer in memory (`m_chunkBuffer`) until gap is filled
4. Maximum memory usage: ~4 chunks (MAX_PARALLEL_DOWNLOADS) = ~20 MB
5. `m_writtenChunks` (QSet) tracks all received chunk indices (for dedup and completion check)

**Save:**
- `saveReceivedFile(path)` closes the tmp file, then:
  - Tries `QFile::rename()` (instant if same filesystem)
  - Falls back to `QFile::copy()` + `QFile::remove()` (cross-filesystem)

**Cleanup:**
- `cleanupDownloadTmpFile()` closes and deletes the tmp file on session reset
- Called from `resetSessionState()`

# PutinQA — Vibecode Documentation

This directory contains detailed documentation for AI agents and developers working on the PutinQA codebase. Keep these documents up to date when making changes.

## Documents

| File | Contents |
|------|----------|
| [ARCHITECTURE.md](ARCHITECTURE.md) | Project structure, tech stack, ownership model, single-controller pattern |
| [SERVER_PROTOCOL.md](SERVER_PROTOCOL.md) | Full HTTP + WebSocket protocol for put-in-pipe server |
| [SESSION_LIFECYCLE.md](SESSION_LIFECYCLE.md) | Sender/receiver flows, completion logic, freeze mechanism, disconnect handling |
| [ENCRYPTION.md](ENCRYPTION.md) | XChaCha20-Poly1305 scheme, key distribution, chunk wire format |
| [TRANSFER_FLOW.md](TRANSFER_FLOW.md) | Chunk-level upload/download details, buffer flow control, parallel downloads, retry logic |
| [UI_PATTERNS.md](UI_PATTERNS.md) | Theme colors, button patterns, responsive layout, i18n, QSettings, default names |
| [KNOWN_ISSUES.md](KNOWN_ISSUES.md) | Race conditions, server quirks, edge cases, workarounds |

## Build

```bash
cmake -B build && cmake --build build
./build/putinqa
```

Requires: Qt6 (Core, Gui, Quick, QuickControls2, Network, WebSockets, Widgets), libsodium, CMake.

## Server

Server project at `../put-in-pipe/`. See [SERVER_PROTOCOL.md](SERVER_PROTOCOL.md) for protocol details.

```bash
cd ../put-in-pipe
cmake -B build && cmake --build build
./build/put-in-pipe --generate-config /tmp/config.ini
./build/put-in-pipe -c /tmp/config.ini
```

## Maintenance

When modifying the codebase, update the relevant document(s):
- New protocol events/actions → SERVER_PROTOCOL.md
- Session flow changes → SESSION_LIFECYCLE.md
- New UI components/patterns → UI_PATTERNS.md
- New bugs or workarounds → KNOWN_ISSUES.md
- Crypto changes → ENCRYPTION.md
- Architecture changes → ARCHITECTURE.md

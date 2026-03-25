# PutinQA

Desktop client for [Put-In-Pipe](https://github.com/askhatovich/put-in-pipe) — streaming file transfer with end-to-end encryption.

Built with Qt6 QML and libsodium (XChaCha20-Poly1305).

## Features

- Send and receive files with end-to-end encryption
- Real-time transfer progress and participant tracking
- System tray integration
- Bilingual UI (English / Russian)
- Cross-platform (Linux, Windows, macOS)

## Build

**Requirements:** CMake 3.16+, Qt6 (Core, Gui, Quick, QuickControls2, Network, WebSockets, Widgets), libsodium

```bash
cmake -B build
cmake --build build
./build/putinqa
```

## Downloads

Pre-built binaries are available on the [Releases](https://github.com/askhatovich/putinqa/releases) page:

- **Linux** — AppImage
- **Windows** — Portable zip
- **macOS** — DMG

## License

GPL-3.0-or-later

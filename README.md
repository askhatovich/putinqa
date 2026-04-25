# PutinQA

A desktop client for [Put-In-Pipe](https://github.com/askhatovich/put-in-pipe), a streaming file transfer service with end-to-end encryption.

The application is implemented in Qt 6 (QML) and uses libsodium for XChaCha20-Poly1305 encryption.

## Features

- File transmission and reception with end-to-end encryption.
- Real-time display of transfer progress and session participants.
- System tray integration.
- Bilingual user interface (English and Russian).
- Cross-platform support (Linux, Windows, macOS).

## Build

**Requirements:** CMake 3.16 or later; Qt 6 modules Core, Gui, Quick, QuickControls2, Network, WebSockets, and Widgets; libsodium.

```bash
cmake -B build
cmake --build build
./build/putinqa
```

## Downloads

Pre-built binaries are available on the [Releases](https://github.com/askhatovich/putinqa/releases) page:

- **Linux** -- AppImage.
- **Windows** -- portable ZIP archive.
- **macOS** -- DMG image.

## License

GPL-3.0-or-later.

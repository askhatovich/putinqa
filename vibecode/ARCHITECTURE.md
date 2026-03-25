# PutinQA Architecture

PutinQA is a Qt6 QML desktop client for the [put-in-pipe](../put-in-pipe/) streaming file transfer server with end-to-end encryption.

## Technology Stack

- **Language:** C++17
- **UI Framework:** Qt6 QML (Quick, QuickControls2)
- **Build System:** CMake
- **Encryption:** libsodium (XChaCha20-Poly1305 AEAD)
- **Network:** Qt6 Network + WebSockets modules
- **System Tray:** Qt6 Widgets (QSystemTrayIcon)

## Project Structure

```
src/
  main.cpp                          # Entry point, QML engine, system tray
  appcontroller.h/cpp               # Central state machine (all app logic)
  client/
    authorization.h/cpp             # HTTP auth + captcha
    serverworkload.h/cpp            # Periodic server stats polling
    session/
      session.h/cpp                 # HTTP session create/join, chunk download
      sessionstate.h/cpp            # WS event parsing, state structures
      websocketconnection.h/cpp     # WS client with auto-reconnect
      actions.h/cpp                 # JSON action serializers
  crypto/
    crypto.h/cpp                    # libsodium wrapper
  qml/
    main.qml                        # Root window, screen loader, footer
    EntryScreen.qml                 # Send/receive entry point
    SettingsScreen.qml              # Server URL, name, language
    CaptchaScreen.qml               # Captcha verification
    SenderSession.qml               # Sender transfer screen
    ReceiverSession.qml             # Receiver transfer screen
    TransferComplete.qml            # Session completion screen
    ProgressPanel.qml               # Progress bar, file info, buffer
    MemberList.qml                  # Participants with status indicators
    SessionTimer.qml                # Expiration countdown
    NameBadge.qml                   # Header bar with server + settings
  resources.qrc                     # QML resource manifest
```

## Single Controller Pattern

All application state is centralized in `AppController` — a single QObject exposed to QML as `appController` context property. It owns ~50 Q_PROPERTY declarations, 15+ Q_INVOKABLE methods, and manages the entire session lifecycle.

**Rationale:** Avoids fragmented state across multiple controllers. QML bindings react to property change signals automatically. Every UI element reads from and writes to AppController.

## Screen State Machine

```
entry ──> connecting ──> sender ──> complete
  │           │                        ^
  │           └──> captcha ──> connecting
  │           │
  │           └──> receiver ──> complete
  │
  └──> settings ──> (returns to previous screen)
```

Screens are loaded dynamically via QML `Loader` based on `appController.screen` property. Settings preserves the previous screen in `m_screenBeforeSettings` for correct return navigation.

## Ownership & Lifetime

```
AppController
  ├── Authorization*        (created per auth attempt, deleteLater'd on restart)
  ├── Session*              (created per session, deleteLater'd on restart)
  │     ├── SessionState*   (child of Session)
  │     ├── WebSocketConnection* (child of Session)
  │     └── QNetworkAccessManager* (m_downloadManager, child of Session)
  ├── ServerWorkload*       (lives for app lifetime)
  ├── QTimer* freezeTimer   (1s interval countdown)
  └── QTimer* expirationTimer (1s interval countdown)
```

## Server Project

The server (`put-in-pipe`) is a separate C++ project in `../put-in-pipe/`. It uses Crow web framework with ASIO. See [SERVER_PROTOCOL.md](SERVER_PROTOCOL.md) for the full protocol specification.

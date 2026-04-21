# UI Patterns & Conventions

## Theme

Dark theme with consistent color palette:

| Color | Usage |
|-------|-------|
| `#1a1a2e` | Background, input backgrounds |
| `#16213e` | Cards, panels, header bar |
| `#0d1117` | Footer background |
| `#0f3460` | Buttons (normal), borders |
| `#1a4a80` | Buttons (hover) |
| `#0a2a50` | Buttons (pressed) |
| `#e94560` | Accent (errors, terminate, active language indicator) |
| `#4caf50` | Success (green: online indicator, checkmark, active server, progress complete) |
| `#eee` | Primary text |
| `#999` | Secondary text, labels |
| `#666` | Tertiary text, offline indicators |
| `#555` | Placeholder text |
| `#7dcea0` | Own name highlight in member list (green tint) |

## Button Pattern

All clickable elements use `MouseArea` with `hoverEnabled: true` (NOT TapHandler — unreliable in inline components):

```qml
Rectangle {
    color: ma.containsMouse ? (ma.pressed ? pressedColor : hoverColor) : normalColor
    Text { anchors.centerIn: parent; text: "Label"; color: "#eee" }
    MouseArea {
        id: ma; anchors.fill: parent; hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: doSomething()
    }
}
```

## Adaptive Button Width

Buttons that change text (e.g., "Copy" → "Copied!") use `Layout.preferredWidth: textId.implicitWidth + 24` to adapt to content:

```qml
Rectangle {
    Layout.preferredWidth: btnText.implicitWidth + 24; height: 36
    Text { id: btnText; anchors.centerIn: parent; text: dynamicText }
}
```

## Responsive Layout

Session screens (Sender, Receiver) have two layout modes:

```qml
readonly property bool wideMode: width > 620

RowLayout { visible: wideMode; ... }   // Sidebar + content
Flickable { visible: !wideMode; ... }  // Vertical stack
```

## Translation System (i18n)

All UI strings come from `appController.t.<key>` — a QVariantMap property that returns the translation map for the current language.

```qml
text: appController.t.sendFile   // "Send file" or "Отправить файл"
```

Language is persisted in QSettings (`app/language`). Auto-detected from system locale on first run: Russian → "ru", otherwise → "en".

Dynamic strings are composed in QML:
```qml
text: appController.t.startTransfer + " (" + appController.freezeRemaining + appController.t.secondsShort + ")"
```

## Member List Indicators

Receiver status indicators in MemberList:

| State | Indicator |
|-------|-----------|
| Online, downloading | Green circle |
| Offline | Gray circle |
| Download complete (`done`) | Green checkmark |
| Own user (self) | Name in `#7dcea0` green tint |

Sender always shows green/gray circle (no "done" concept for sender).

## Screen Navigation

- **Settings gear** (NameBadge): toggles settings screen, returns to previous screen
- **Back button** (SettingsScreen): same as gear toggle
- **Save button** (SettingsScreen): applies changes, returns to previous screen
- **New transfer** (TransferComplete): calls `restart()`, full state reset

## File Dialogs

Qt6 `FileDialog` components declared in `main.qml`, triggered from other screens:

```qml
FileDialog { id: fileDialog; title: appController.t.selectFileTitle; onAccepted: appController.selectFile(selectedFile) }
FileDialog { id: saveDialog; fileMode: FileDialog.SaveFile; currentFile: appController.suggestedSavePath; ... }
```

## QSettings

Organization: "askhatovich", Application: "putinqa"

| Key | Default | Description |
|-----|---------|-------------|
| `server/url` | `http://127.0.0.1:2233` | Server address for sending |
| `user/name` | Random funny name + emoji | Display name |
| `app/language` | System locale detection | "ru" or "en" |
| `proxy/type` | `none` | `none`, `socks5`, or `http` |
| `proxy/host` | empty | Proxy host |
| `proxy/port` | `0` | Proxy port |
| `session/auto_drop_freeze` | `false` | If true, sender sessions are created with `auto_drop_freeze: true` JSON body — server drops initial freeze on the first confirmed chunk and ends with `ok` when the last receiver leaves (fire-and-forget). Toggled via SettingsScreen.qml. |

**Settings are inviolable:** Only changed explicitly via Settings screen. Runtime data (e.g., server URL from received link) never overwrites QSettings. `m_activeServer` is the temporary session server; `m_serverUrl` is the persistent setting.

## Default Name Generation

On first launch, a random name is generated:
- 50 English names (Wombat, Gecko, Truffle...) or 50 Russian names (Бурундук, Пельмень, Радуга...)
- Selected based on system language
- Random emoji appended from Unicode ranges: smileys (U+1F600-1F636) or animals (U+1F400-1F43E)
- Example: "Пельмень 🦊" or "Nebula 😜"

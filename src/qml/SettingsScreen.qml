// Copyright (C) 2026  Roman Lyubimov
// SPDX-License-Identifier: GPL-3.0-or-later
// For full license text, see <https://www.gnu.org/licenses/gpl-3.0.txt>

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    property string selectedLang: appController.language
    property string selectedProxy: appController.proxyType
    property string proxyHost: appController.proxyHost
    property int proxyPort: appController.proxyPort
    property bool selectedAutoDropFreeze: appController.autoDropFreeze

    ColumnLayout {
        anchors.centerIn: parent
        width: Math.min(parent.width - 32, 400)
        spacing: 20

        Text { text: appController.t.settings; color: "#eee"; font.pixelSize: 22; font.bold: true }

        ColumnLayout {
            Layout.fillWidth: true; spacing: 6
            Text { text: appController.t.serverUrlLabel; color: "#999"; font.pixelSize: 12 }
            Rectangle {
                Layout.fillWidth: true; height: 38; radius: 4
                color: "#16213e"; border.color: "#0f3460"
                TextInput {
                    id: urlInput; anchors.fill: parent; anchors.margins: 8
                    color: "#eee"; font.pixelSize: 13; font.family: "monospace"
                    text: appController.serverUrl; selectByMouse: true
                    verticalAlignment: TextInput.AlignVCenter

                    Text {
                        anchors.fill: parent; verticalAlignment: Text.AlignVCenter
                        visible: !urlInput.text && !urlInput.activeFocus
                        text: "https://example.com"
                        color: "#555"; font.pixelSize: 13; font.family: "monospace"
                    }
                }
            }
        }

        ColumnLayout {
            Layout.fillWidth: true; spacing: 6
            Text { text: appController.t.displayName; color: "#999"; font.pixelSize: 12 }
            Rectangle {
                Layout.fillWidth: true; height: 38; radius: 4
                color: "#16213e"; border.color: "#0f3460"
                TextInput {
                    id: nameInput; anchors.fill: parent; anchors.margins: 8
                    color: "#eee"; font.pixelSize: 13
                    text: appController.userName; selectByMouse: true
                    verticalAlignment: TextInput.AlignVCenter
                }
            }
        }

        ColumnLayout {
            Layout.fillWidth: true; spacing: 6
            Text { text: appController.t.languageLabel; color: "#999"; font.pixelSize: 12 }
            RowLayout {
                spacing: 8

                Rectangle {
                    width: 60; height: 36; radius: 4
                    color: ruMA.containsMouse ? (selectedLang === "ru" ? "#0f3460" : "#1a1a3e") : (selectedLang === "ru" ? "#0f3460" : "#1a1a2e")
                    border.color: selectedLang === "ru" ? "#e94560" : "#0f3460"
                    border.width: selectedLang === "ru" ? 2 : 1
                    Text { anchors.centerIn: parent; text: "RU"; color: selectedLang === "ru" ? "#eee" : "#999"; font.pixelSize: 13; font.bold: selectedLang === "ru" }
                    MouseArea {
                        id: ruMA; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                        onClicked: selectedLang = "ru"
                    }
                }

                Rectangle {
                    width: 60; height: 36; radius: 4
                    color: enMA.containsMouse ? (selectedLang === "en" ? "#0f3460" : "#1a1a3e") : (selectedLang === "en" ? "#0f3460" : "#1a1a2e")
                    border.color: selectedLang === "en" ? "#e94560" : "#0f3460"
                    border.width: selectedLang === "en" ? 2 : 1
                    Text { anchors.centerIn: parent; text: "EN"; color: selectedLang === "en" ? "#eee" : "#999"; font.pixelSize: 13; font.bold: selectedLang === "en" }
                    MouseArea {
                        id: enMA; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                        onClicked: selectedLang = "en"
                    }
                }
            }
        }

        ColumnLayout {
            Layout.fillWidth: true; spacing: 6
            Text { text: appController.t.proxyLabel; color: "#999"; font.pixelSize: 12 }
            RowLayout {
                spacing: 8

                Rectangle {
                    width: 60; height: 36; radius: 4
                    color: noneMA.containsMouse ? (selectedProxy === "none" ? "#0f3460" : "#1a1a3e") : (selectedProxy === "none" ? "#0f3460" : "#1a1a2e")
                    border.color: selectedProxy === "none" ? "#e94560" : "#0f3460"
                    border.width: selectedProxy === "none" ? 2 : 1
                    Text { anchors.centerIn: parent; text: appController.t.proxyNone; color: selectedProxy === "none" ? "#eee" : "#999"; font.pixelSize: 13; font.bold: selectedProxy === "none" }
                    MouseArea {
                        id: noneMA; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                        onClicked: selectedProxy = "none"
                    }
                }

                Rectangle {
                    width: 80; height: 36; radius: 4
                    color: socks5MA.containsMouse ? (selectedProxy === "socks5" ? "#0f3460" : "#1a1a3e") : (selectedProxy === "socks5" ? "#0f3460" : "#1a1a2e")
                    border.color: selectedProxy === "socks5" ? "#e94560" : "#0f3460"
                    border.width: selectedProxy === "socks5" ? 2 : 1
                    Text { anchors.centerIn: parent; text: "SOCKS5"; color: selectedProxy === "socks5" ? "#eee" : "#999"; font.pixelSize: 13; font.bold: selectedProxy === "socks5" }
                    MouseArea {
                        id: socks5MA; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                        onClicked: selectedProxy = "socks5"
                    }
                }

                Rectangle {
                    width: 70; height: 36; radius: 4
                    color: httpMA.containsMouse ? (selectedProxy === "http" ? "#0f3460" : "#1a1a3e") : (selectedProxy === "http" ? "#0f3460" : "#1a1a2e")
                    border.color: selectedProxy === "http" ? "#e94560" : "#0f3460"
                    border.width: selectedProxy === "http" ? 2 : 1
                    Text { anchors.centerIn: parent; text: "HTTP"; color: selectedProxy === "http" ? "#eee" : "#999"; font.pixelSize: 13; font.bold: selectedProxy === "http" }
                    MouseArea {
                        id: httpMA; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                        onClicked: selectedProxy = "http"
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true; spacing: 8
                visible: selectedProxy !== "none"

                Rectangle {
                    Layout.fillWidth: true; height: 38; radius: 4
                    color: "#16213e"; border.color: "#0f3460"
                    TextInput {
                        id: proxyHostInput; anchors.fill: parent; anchors.margins: 8
                        color: "#eee"; font.pixelSize: 13; font.family: "monospace"
                        text: proxyHost; selectByMouse: true
                        verticalAlignment: TextInput.AlignVCenter
                        Text {
                            anchors.fill: parent; verticalAlignment: Text.AlignVCenter
                            visible: !proxyHostInput.text && !proxyHostInput.activeFocus
                            text: "127.0.0.1"; color: "#555"; font.pixelSize: 13; font.family: "monospace"
                        }
                    }
                }

                Text { text: ":"; color: "#999"; font.pixelSize: 16 }

                Rectangle {
                    width: 80; height: 38; radius: 4
                    color: "#16213e"; border.color: "#0f3460"
                    TextInput {
                        id: proxyPortInput; anchors.fill: parent; anchors.margins: 8
                        color: "#eee"; font.pixelSize: 13; font.family: "monospace"
                        text: proxyPort > 0 ? proxyPort.toString() : ""; selectByMouse: true
                        verticalAlignment: TextInput.AlignVCenter
                        validator: IntValidator { bottom: 1; top: 65535 }
                        Text {
                            anchors.fill: parent; verticalAlignment: Text.AlignVCenter
                            visible: !proxyPortInput.text && !proxyPortInput.activeFocus
                            text: selectedProxy === "socks5" ? "9050" : "4444"
                            color: "#555"; font.pixelSize: 13; font.family: "monospace"
                        }
                    }
                }
            }
        }

        // Auto-drop freeze toggle — per-sender-session preference, persisted like the rest.
        RowLayout {
            Layout.fillWidth: true; spacing: 10

            Rectangle {
                id: autoDropBox
                width: 20; height: 20; radius: 3
                border.color: selectedAutoDropFreeze ? "#e94560" : "#0f3460"
                border.width: selectedAutoDropFreeze ? 2 : 1
                color: selectedAutoDropFreeze ? "#e94560" : "#16213e"
                Text {
                    anchors.centerIn: parent
                    text: "✓"
                    color: "#eee"; font.pixelSize: 14; font.bold: true
                    visible: selectedAutoDropFreeze
                }
                MouseArea {
                    anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                    onClicked: selectedAutoDropFreeze = !selectedAutoDropFreeze
                }
            }

            ColumnLayout {
                Layout.fillWidth: true; spacing: 2

                Text {
                    text: appController.t.autoDropFreezeLabel
                    color: "#eee"; font.pixelSize: 13
                    MouseArea {
                        anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                        onClicked: selectedAutoDropFreeze = !selectedAutoDropFreeze
                    }
                }
                Text {
                    text: appController.t.autoDropFreezeHint
                    color: "#777"; font.pixelSize: 11
                    Layout.fillWidth: true
                    wrapMode: Text.WordWrap
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true; spacing: 12

            Rectangle {
                Layout.fillWidth: true; height: 40; radius: 4
                color: saveMA.containsMouse ? (saveMA.pressed ? "#0a2a50" : "#1a4a80") : "#0f3460"
                Text { anchors.centerIn: parent; text: appController.t.save; color: "#eee"; font.pixelSize: 14 }
                MouseArea {
                    id: saveMA; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                    onClicked: appController.saveSettings(urlInput.text.trim(), nameInput.text.trim(), selectedLang,
                                                         selectedProxy, proxyHostInput.text.trim(), parseInt(proxyPortInput.text) || 0,
                                                         selectedAutoDropFreeze)
                }
            }

            Rectangle {
                width: 80; height: 40; radius: 4
                color: "transparent"; border.color: "#333"
                Text { anchors.centerIn: parent; text: appController.t.back; color: backMA.containsMouse ? "#eee" : "#999"; font.pixelSize: 14 }
                MouseArea {
                    id: backMA; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                    onClicked: appController.openSettings()
                }
            }
        }

        Row {
            Layout.alignment: Qt.AlignHCenter
            spacing: 6

            Text {
                text: "PutinQA " + appController.appVersion
                color: "#555"; font.pixelSize: 14
            }

            Text { text: "|"; color: "#444"; font.pixelSize: 14 }

            Text {
                text: "GitHub"
                color: ghMA.containsMouse ? "#e94560" : "#666"
                font.pixelSize: 14; font.underline: true
                MouseArea {
                    id: ghMA; anchors.fill: parent; hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: Qt.openUrlExternally("https://github.com/askhatovich/putinqa")
                }
            }
        }
    }
}

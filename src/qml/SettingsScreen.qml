// Copyright (C) 2026  Roman Lyubimov
// SPDX-License-Identifier: GPL-3.0-or-later
// For full license text, see <https://www.gnu.org/licenses/gpl-3.0.txt>

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    property string selectedLang: appController.language

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

        RowLayout {
            Layout.fillWidth: true; spacing: 12

            Rectangle {
                Layout.fillWidth: true; height: 40; radius: 4
                color: saveMA.containsMouse ? (saveMA.pressed ? "#0a2a50" : "#1a4a80") : "#0f3460"
                Text { anchors.centerIn: parent; text: appController.t.save; color: "#eee"; font.pixelSize: 14 }
                MouseArea {
                    id: saveMA; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                    onClicked: appController.saveSettings(urlInput.text.trim(), nameInput.text.trim(), selectedLang)
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

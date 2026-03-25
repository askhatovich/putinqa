// Copyright (C) 2026  Roman Lyubimov
// SPDX-License-Identifier: GPL-3.0-or-later
// For full license text, see <https://www.gnu.org/licenses/gpl-3.0.txt>

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

ApplicationWindow {
    id: root
    visible: true
    width: 820
    height: 580
    minimumWidth: 480
    minimumHeight: 400
    title: "PutinQA"
    color: "#1a1a2e"

    onClosing: function(close) {
        close.accepted = false
        appController.minimizeToTray()
    }

    FileDialog {
        id: fileDialog
        title: appController.t.selectFileTitle
        onAccepted: appController.selectFile(selectedFile)
    }

    FileDialog {
        id: saveDialog
        fileMode: FileDialog.SaveFile
        title: appController.t.saveFileTitle
        currentFile: appController.suggestedSavePath
        onAccepted: appController.saveReceivedFile(selectedFile)
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // Name badge
        NameBadge {
            Layout.fillWidth: true
        }

        // Main content
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            Loader {
                id: screenLoader
                anchors.fill: parent
                anchors.margins: 16
                source: {
                    switch (appController.screen) {
                    case "entry": return "EntryScreen.qml"
                    case "settings": return "SettingsScreen.qml"
                    case "captcha": return "CaptchaScreen.qml"
                    case "sender": return "SenderSession.qml"
                    case "receiver": return "ReceiverSession.qml"
                    case "complete": return "TransferComplete.qml"
                    default: return ""
                    }
                }
            }

            // Connecting spinner overlay
            Rectangle {
                anchors.fill: parent
                color: "#1a1a2e"
                visible: appController.screen === "connecting"

                Column {
                    anchors.centerIn: parent
                    spacing: 16

                    BusyIndicator {
                        anchors.horizontalCenter: parent.horizontalCenter
                        running: true
                        palette.dark: "#e94560"
                    }

                    Text {
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: appController.t.connecting
                        color: "#999"
                        font.pixelSize: 14
                    }
                }
            }
        }

        // Footer
        Rectangle {
            Layout.fillWidth: true
            height: 32
            color: "#0d1117"

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 12
                anchors.rightMargin: 12

                Text {
                    font.pixelSize: 11
                    color: {
                        var s = appController.stats
                        return (s && s.connected === false) ? "#e94560" : "#666"
                    }
                    text: {
                        var s = appController.stats
                        if (s && s.connected === false)
                            return appController.t.noConnection
                        if (s && s.currentUserCount !== undefined)
                            return appController.t.users + ": " + s.currentUserCount + "/" + s.maxUserCount +
                                   "  " + appController.t.sessions + ": " + s.currentSessionCount + "/" + s.maxSessionCount
                        return appController.t.connectingToServer
                    }
                }

                Item { Layout.fillWidth: true }

                Text {
                    color: "#555"
                    font.pixelSize: 11
                    text: "PutinQA \u2014 Put-in-pipe Qt Application"
                }
            }
        }
    }
}

// Copyright (C) 2026  Roman Lyubimov
// SPDX-License-Identifier: GPL-3.0-or-later
// For full license text, see <https://www.gnu.org/licenses/gpl-3.0.txt>

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    ColumnLayout {
        anchors.centerIn: parent
        width: Math.min(parent.width - 32, 400)
        spacing: 20

        Text {
            Layout.alignment: Qt.AlignHCenter
            text: appController.completeStatus === "ok" ? "\u2713" : "\u2717"
            color: appController.completeStatus === "ok" ? "#4caf50" : "#e94560"
            font.pixelSize: 48; font.bold: true
        }

        Text {
            Layout.alignment: Qt.AlignHCenter; Layout.fillWidth: true
            text: {
                switch (appController.completeStatus) {
                case "ok": return appController.t.transferComplete
                case "timeout": return appController.t.sessionTimedOut
                case "sender_is_gone": return appController.t.senderDisconnected
                case "no_receivers": return appController.t.noReceiversJoined
                case "terminated_by_you": return appController.t.sessionTerminated
                case "error": return appController.t.error
                default: return appController.t.sessionEnded + appController.completeStatus
                }
            }
            color: "#eee"; font.pixelSize: 20; font.bold: true
            wrapMode: Text.Wrap; horizontalAlignment: Text.AlignHCenter
        }

        Text {
            Layout.alignment: Qt.AlignHCenter
            visible: !appController.isSender && appController.hasDownloadedFile && appController.fileName.length > 0
            text: appController.fileName + " (" + appController.formatBytes(appController.fileSize) + ")"
            color: "#999"; font.pixelSize: 13
        }

        // Save button (receiver)
        Rectangle {
            Layout.fillWidth: true; height: 44; radius: 8
            visible: !appController.isSender && appController.hasDownloadedFile
            color: saveMA.containsMouse ? (saveMA.pressed ? "#0a2a50" : "#1a4a80") : "#0f3460"
            Text { anchors.centerIn: parent; text: appController.t.saveFile; color: "#eee"; font.pixelSize: 15; font.bold: true }
            MouseArea {
                id: saveMA; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                onClicked: saveDialog.open()
            }
        }

        // New transfer
        Rectangle {
            Layout.fillWidth: true; height: 40; radius: 8
            color: "transparent"; border.color: "#333"; border.width: 1
            Text { anchors.centerIn: parent; text: appController.t.newTransfer; color: restartMA.containsMouse ? "#eee" : "#999"; font.pixelSize: 14 }
            MouseArea {
                id: restartMA; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                onClicked: appController.restart()
            }
        }
    }
}

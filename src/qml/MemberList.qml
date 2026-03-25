// Copyright (C) 2026  Roman Lyubimov
// SPDX-License-Identifier: GPL-3.0-or-later
// For full license text, see <https://www.gnu.org/licenses/gpl-3.0.txt>

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    radius: 8; color: "#16213e"
    implicitHeight: memberCol.implicitHeight + 16
    implicitWidth: 240

    readonly property string myId: appController.myClientId
    readonly property string senderId: appController.senderName ? (appController.receivers.length >= 0 ? appController.senderName : "") : ""

    ColumnLayout {
        id: memberCol
        anchors.left: parent.left; anchors.right: parent.right; anchors.top: parent.top
        anchors.margins: 8; spacing: 6

        Text {
            text: appController.t.participants; color: "#999"
            font.pixelSize: 11; font.bold: true
            Layout.bottomMargin: 2
        }

        // Sender
        Rectangle {
            property bool isMe: appController.isSender
            Layout.fillWidth: true; height: 32; radius: 4; color: "#1a1a2e"
            border.width: 0

            RowLayout {
                anchors.fill: parent; anchors.leftMargin: 8; anchors.rightMargin: 8; spacing: 6
                Rectangle { width: 8; height: 8; radius: 4; color: appController.senderOnline ? "#4caf50" : "#666" }
                Text { Layout.fillWidth: true; text: appController.senderName || appController.t.senderFallback; color: parent.parent.isMe ? "#7dcea0" : "#eee"; font.pixelSize: 12; elide: Text.ElideRight }
                Text { text: appController.t.senderRole; color: "#0f3460"; font.pixelSize: 10; font.bold: true }
            }
        }

        // Receivers
        Repeater {
            model: appController.receivers

            Rectangle {
                property bool isMe: modelData.id === myId
                Layout.fillWidth: true; height: 32; radius: 4; color: "#1a1a2e"
                border.width: 0
                required property var modelData

                RowLayout {
                    anchors.fill: parent; anchors.leftMargin: 8; anchors.rightMargin: 8; spacing: 6
                    Text {
                        visible: modelData.done === true
                        text: "\u2713"; color: "#4caf50"; font.pixelSize: 14; font.bold: true
                        Layout.preferredWidth: 12
                    }
                    Rectangle {
                        visible: modelData.done !== true
                        width: 8; height: 8; radius: 4
                        color: modelData.isOnline ? "#4caf50" : "#666"
                    }
                    Text { Layout.fillWidth: true; text: modelData.name || appController.t.receiverFallback; color: parent.parent.isMe ? "#7dcea0" : "#eee"; font.pixelSize: 12; elide: Text.ElideRight }

                    Rectangle {
                        width: 20; height: 20; radius: 10
                        visible: appController.isSender
                        color: kickMA.containsMouse ? "#e94560" : "transparent"

                        Text { anchors.centerIn: parent; text: "\u00d7"; color: kickMA.containsMouse ? "#fff" : "#666"; font.pixelSize: 14 }
                        MouseArea {
                            id: kickMA; anchors.fill: parent; hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: appController.kickReceiver(modelData.id)
                        }
                    }
                }
            }
        }

        Text {
            visible: appController.receivers.length === 0
            text: appController.t.waitingForReceivers
            color: "#666"; font.pixelSize: 11; font.italic: true
        }
    }
}

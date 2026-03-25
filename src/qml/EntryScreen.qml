// Copyright (C) 2026  Roman Lyubimov
// SPDX-License-Identifier: GPL-3.0-or-later
// For full license text, see <https://www.gnu.org/licenses/gpl-3.0.txt>

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    ColumnLayout {
        anchors.centerIn: parent
        width: Math.min(parent.width - 32, 420)
        spacing: 24

        Item {
            Layout.alignment: Qt.AlignHCenter
            implicitWidth: titleRow.implicitWidth
            implicitHeight: titleRow.implicitHeight
            Row {
                id: titleRow
                Text {
                    text: "Put-In-Pipe"
                    color: "#eee"; font.pixelSize: 28; font.bold: true
                }
                Text {
                    text: "desktop"
                    color: "#bbb"; font.pixelSize: 11
                    anchors.top: parent.top; anchors.topMargin: 2
                }
            }
        }

        Text {
            Layout.alignment: Qt.AlignHCenter
            Layout.topMargin: -16
            text: appController.t.appSlogan
            color: "#999"; font.pixelSize: 13
        }

        // Send button
        Rectangle {
            Layout.fillWidth: true; height: 52; radius: 8
            color: sendMA.containsMouse ? (sendMA.pressed ? "#0a2a50" : "#1a4a80") : "#0f3460"
            border.color: "#1a4a80"; border.width: 1

            Text {
                anchors.centerIn: parent
                text: appController.t.sendFile
                color: "#eee"; font.pixelSize: 16; font.bold: true
            }

            MouseArea {
                id: sendMA; anchors.fill: parent; hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: { appController.startSend(); fileDialog.open() }
            }
        }

        // Receive section
        Rectangle {
            Layout.fillWidth: true; radius: 8
            color: "#16213e"; border.color: "#0f3460"; border.width: 1
            implicitHeight: receiveCol.implicitHeight + 24

            ColumnLayout {
                id: receiveCol
                anchors.fill: parent; anchors.margins: 12; spacing: 8

                Text { text: appController.t.receiveFile; color: "#eee"; font.pixelSize: 16; font.bold: true }
                Text { text: appController.t.pasteLink; color: "#999"; font.pixelSize: 12 }

                RowLayout {
                    Layout.fillWidth: true; spacing: 8

                    Rectangle {
                        Layout.fillWidth: true; height: 36; radius: 4
                        color: "#1a1a2e"; border.color: "#0f3460"; border.width: 1

                        TextInput {
                            id: linkInput
                            anchors.fill: parent; anchors.margins: 8
                            color: "#eee"; font.pixelSize: 13; font.family: "monospace"
                            clip: true; selectByMouse: true
                            verticalAlignment: TextInput.AlignVCenter
                            Keys.onReturnPressed: doJoin()
                        }
                    }

                    Rectangle {
                        width: joinText.implicitWidth + 24; height: 36; radius: 4
                        color: joinMA.containsMouse ? (joinMA.pressed ? "#0a2a50" : "#1a4a80") : "#0f3460"

                        Text { id: joinText; anchors.centerIn: parent; text: appController.t.join; color: "#eee"; font.pixelSize: 13 }
                        MouseArea {
                            id: joinMA; anchors.fill: parent; hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: doJoin()
                        }
                    }
                }
            }
        }

        // Error message
        Text {
            Layout.alignment: Qt.AlignHCenter; Layout.fillWidth: true
            visible: appController.errorMsg.length > 0
            text: appController.errorMsg
            color: "#e94560"; font.pixelSize: 13
            wrapMode: Text.Wrap; horizontalAlignment: Text.AlignHCenter
        }
    }

    function doJoin() {
        var link = linkInput.text.trim()
        if (link.length > 0) appController.startReceive(link)
    }
}

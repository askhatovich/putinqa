// Copyright (C) 2026  Roman Lyubimov
// SPDX-License-Identifier: GPL-3.0-or-later
// For full license text, see <https://www.gnu.org/licenses/gpl-3.0.txt>

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: progressRoot
    radius: 8
    color: "#16213e"
    implicitHeight: progressCol.implicitHeight + 20
    implicitWidth: 300

    ColumnLayout {
        id: progressCol
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: 10
        spacing: 10

        // File info
        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Text {
                Layout.fillWidth: true
                text: appController.fileName || appController.t.noFileSelected
                color: "#eee"
                font.pixelSize: 14
                elide: Text.ElideMiddle
            }

            Text {
                text: appController.fileSize > 0 ? appController.formatBytes(appController.fileSize) : ""
                color: "#999"
                font.pixelSize: 12
            }
        }

        // Progress bar
        Rectangle {
            Layout.fillWidth: true
            height: 8
            radius: 4
            color: "#1a1a2e"

            Rectangle {
                width: parent.width * appController.progress
                height: parent.height
                radius: 4
                color: appController.uploadFinished ? "#4caf50" : "#e94560"

                Behavior on width {
                    NumberAnimation { duration: 300; easing.type: Easing.OutQuad }
                }
            }
        }

        // Stats row
        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            Text {
                text: appController.formatBytes(appController.bytesTransferred) + " / " +
                      appController.formatBytes(appController.fileSize)
                color: "#999"
                font.pixelSize: 12
            }

            Text {
                text: Math.round(appController.progress * 100) + "%"
                color: "#eee"
                font.pixelSize: 12
                font.bold: true
            }

            Item { Layout.fillWidth: true }

            // Buffer status (sender only)
            Text {
                visible: appController.isSender
                text: appController.t.buffer + ": " + appController.bufferUsed + "/" + appController.bufferMax
                color: "#999"
                font.pixelSize: 11
            }
        }

        // Upload finished indicator
        Text {
            visible: appController.uploadFinished
            text: appController.isSender ? appController.t.uploadComplete : appController.t.allDataReceived
            color: "#4caf50"
            font.pixelSize: 12
        }
    }
}

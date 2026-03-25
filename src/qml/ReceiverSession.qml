// Copyright (C) 2026  Roman Lyubimov
// SPDX-License-Identifier: GPL-3.0-or-later
// For full license text, see <https://www.gnu.org/licenses/gpl-3.0.txt>

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: receiverRoot

    readonly property bool wideMode: width > 620

    RowLayout {
        anchors.fill: parent
        spacing: 12
        visible: receiverRoot.wideMode

        // Sidebar
        ColumnLayout {
            Layout.preferredWidth: 260
            Layout.maximumWidth: 260
            Layout.minimumWidth: 200
            Layout.fillHeight: true
            Layout.alignment: Qt.AlignTop
            spacing: 8

            MemberList {
                Layout.fillWidth: true
            }

            SessionTimer {
                Layout.fillWidth: true
            }

            // Leave button
            Rectangle {
                Layout.fillWidth: true
                height: 36; radius: 4
                color: leaveMA1.containsMouse ? (leaveMA1.pressed ? Qt.rgba(0.91,0.27,0.38,0.5) : Qt.rgba(0.91,0.27,0.38,0.3)) : Qt.rgba(0.91,0.27,0.38,0.15)
                border.color: "#e94560"

                Text {
                    anchors.centerIn: parent
                    text: appController.t.leaveSession
                    color: "#e94560"; font.pixelSize: 13
                }

                MouseArea {
                    id: leaveMA1; anchors.fill: parent; hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: appController.leaveSession()
                }
            }

            Item { Layout.fillHeight: true }
        }

        // Main content
        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.alignment: Qt.AlignTop
            spacing: 12

            ProgressPanel {
                Layout.fillWidth: true
            }

            Text {
                Layout.alignment: Qt.AlignHCenter
                text: appController.t.chunks + ": " + appController.chunksConfirmed + " / " +
                      (appController.highestKnownChunk > 0 ? appController.highestKnownChunk : "?")
                color: "#999"
                font.pixelSize: 12
            }

            Text {
                Layout.fillWidth: true
                visible: appController.errorMsg.length > 0
                text: appController.errorMsg
                color: "#e94560"
                font.pixelSize: 13
                wrapMode: Text.Wrap
            }

            Item { Layout.fillHeight: true }
        }
    }

    // Narrow layout
    Flickable {
        anchors.fill: parent
        visible: !receiverRoot.wideMode
        contentHeight: narrowCol.implicitHeight
        clip: true
        boundsBehavior: Flickable.StopAtBounds

        ColumnLayout {
            id: narrowCol
            width: parent.width
            spacing: 12

            ProgressPanel {
                Layout.fillWidth: true
            }

            Text {
                Layout.alignment: Qt.AlignHCenter
                text: appController.t.chunks + ": " + appController.chunksConfirmed + " / " +
                      (appController.highestKnownChunk > 0 ? appController.highestKnownChunk : "?")
                color: "#999"
                font.pixelSize: 12
            }

            MemberList {
                Layout.fillWidth: true
            }

            SessionTimer {
                Layout.fillWidth: true
            }

            // Leave button
            Rectangle {
                Layout.fillWidth: true; height: 36; radius: 4
                color: leaveMA2.containsMouse ? Qt.rgba(0.91,0.27,0.38,0.3) : Qt.rgba(0.91,0.27,0.38,0.15)
                border.color: "#e94560"
                Text { anchors.centerIn: parent; text: appController.t.leaveSession; color: "#e94560"; font.pixelSize: 13 }
                MouseArea { id: leaveMA2; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: appController.leaveSession() }
            }

            Text {
                Layout.fillWidth: true
                visible: appController.errorMsg.length > 0
                text: appController.errorMsg
                color: "#e94560"
                font.pixelSize: 13
                wrapMode: Text.Wrap
            }
        }
    }
}

// Copyright (C) 2026  Roman Lyubimov
// SPDX-License-Identifier: GPL-3.0-or-later
// For full license text, see <https://www.gnu.org/licenses/gpl-3.0.txt>

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: senderRoot

    readonly property bool wideMode: width > 620

    RowLayout {
        anchors.fill: parent
        spacing: 12
        visible: senderRoot.wideMode

        // Sidebar
        ColumnLayout {
            Layout.preferredWidth: 260
            Layout.maximumWidth: 260
            Layout.minimumWidth: 200
            Layout.fillHeight: true
            Layout.alignment: Qt.AlignTop
            spacing: 8

            MemberList { Layout.fillWidth: true }

            // Freeze button
            Rectangle {
                Layout.fillWidth: true
                height: 40
                radius: 4
                visible: appController.frozen && appController.receiversPresent
                color: freezeMA1.containsMouse ? (freezeMA1.pressed ? "#0a2a50" : "#1a4a80") : "#0f3460"
                border.color: "#1a4a80"

                Rectangle {
                    anchors.left: parent.left; anchors.top: parent.top; anchors.bottom: parent.bottom
                    width: parent.width * Math.min(1, appController.freezeRemaining / 120)
                    radius: 4; color: Qt.rgba(0.91, 0.27, 0.38, 0.25)
                    Behavior on width { NumberAnimation { duration: 1000 } }
                }

                Text {
                    anchors.centerIn: parent; z: 1
                    text: appController.t.startTransfer + " (" + appController.freezeRemaining + appController.t.secondsShort + ")"
                    color: "#eee"; font.pixelSize: 13
                }

                MouseArea {
                    id: freezeMA1; anchors.fill: parent; hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: appController.dropFreeze()
                }
            }

            // Freeze waiting text
            Text {
                Layout.fillWidth: true
                visible: appController.frozen && !appController.receiversPresent
                text: appController.t.waitingForReceivers + " (" + appController.freezeRemaining + appController.t.secondsShort + ")"
                color: "#999"; font.pixelSize: 12
                horizontalAlignment: Text.AlignHCenter
            }

            SessionTimer {
                Layout.fillWidth: true
                visible: !appController.frozen
            }

            // Terminate button
            Rectangle {
                Layout.fillWidth: true
                height: 36; radius: 4
                color: termMA1.containsMouse ? (termMA1.pressed ? Qt.rgba(0.91,0.27,0.38,0.5) : Qt.rgba(0.91,0.27,0.38,0.3)) : Qt.rgba(0.91,0.27,0.38,0.15)
                border.color: "#e94560"

                Text {
                    anchors.centerIn: parent
                    text: appController.t.terminateSession
                    color: "#e94560"; font.pixelSize: 13
                }

                MouseArea {
                    id: termMA1; anchors.fill: parent; hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: appController.terminateSession()
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

            // Share link
            Rectangle {
                Layout.fillWidth: true
                radius: 8; color: "#16213e"
                implicitHeight: shareLinkCol1.implicitHeight + 20

                ColumnLayout {
                    id: shareLinkCol1
                    anchors.left: parent.left; anchors.right: parent.right; anchors.top: parent.top
                    anchors.margins: 10; spacing: 6

                    Text { text: appController.t.shareLinkLabel; color: "#999"; font.pixelSize: 12 }

                    RowLayout {
                        Layout.fillWidth: true; spacing: 6

                        Rectangle {
                            Layout.fillWidth: true; height: 34; radius: 4
                            color: "#1a1a2e"; border.color: "#0f3460"
                            TextInput {
                                anchors.fill: parent; anchors.margins: 6
                                color: "#eee"; font.pixelSize: 11; font.family: "monospace"
                                text: appController.shareLink; readOnly: true
                                selectByMouse: true; verticalAlignment: TextInput.AlignVCenter; clip: true
                            }
                        }

                        Rectangle {
                            id: copyBtn1; Layout.preferredWidth: copyText1.implicitWidth + 24; height: 34; radius: 4
                            property bool copied: false
                            color: copyMA1.containsMouse ? (copyMA1.pressed ? "#0a2a50" : "#1a4a80") : "#0f3460"

                            Text {
                                id: copyText1; anchors.centerIn: parent
                                text: copyBtn1.copied ? appController.t.copied : appController.t.copy
                                color: "#eee"; font.pixelSize: 12
                            }
                            Timer { id: copyTimer1; interval: 2000; onTriggered: copyBtn1.copied = false }
                            MouseArea {
                                id: copyMA1; anchors.fill: parent; hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: { appController.copyShareLink(); copyBtn1.copied = true; copyTimer1.restart() }
                            }
                        }
                    }
                }
            }

            ProgressPanel { Layout.fillWidth: true }

            Text {
                Layout.fillWidth: true
                visible: appController.errorMsg.length > 0
                text: appController.errorMsg
                color: "#e94560"; font.pixelSize: 13; wrapMode: Text.Wrap
            }

            Item { Layout.fillHeight: true }
        }
    }

    // Narrow layout
    Flickable {
        anchors.fill: parent
        visible: !senderRoot.wideMode
        contentHeight: narrowCol.implicitHeight
        clip: true; boundsBehavior: Flickable.StopAtBounds

        ColumnLayout {
            id: narrowCol
            width: parent.width; spacing: 12

            // Share link
            Rectangle {
                Layout.fillWidth: true
                radius: 8; color: "#16213e"
                implicitHeight: shareLinkCol2.implicitHeight + 20

                ColumnLayout {
                    id: shareLinkCol2
                    anchors.left: parent.left; anchors.right: parent.right; anchors.top: parent.top
                    anchors.margins: 10; spacing: 6

                    Text { text: appController.t.shareLinkLabel; color: "#999"; font.pixelSize: 12 }

                    RowLayout {
                        Layout.fillWidth: true; spacing: 6

                        Rectangle {
                            Layout.fillWidth: true; height: 34; radius: 4
                            color: "#1a1a2e"; border.color: "#0f3460"
                            TextInput {
                                anchors.fill: parent; anchors.margins: 6
                                color: "#eee"; font.pixelSize: 11; font.family: "monospace"
                                text: appController.shareLink; readOnly: true
                                selectByMouse: true; verticalAlignment: TextInput.AlignVCenter; clip: true
                            }
                        }

                        Rectangle {
                            id: copyBtn2; Layout.preferredWidth: copyText2.implicitWidth + 24; height: 34; radius: 4
                            property bool copied: false
                            color: copyMA2.containsMouse ? "#1a4a80" : "#0f3460"

                            Text { id: copyText2; anchors.centerIn: parent; text: copyBtn2.copied ? appController.t.copied : appController.t.copy; color: "#eee"; font.pixelSize: 12 }
                            Timer { id: copyTimer2; interval: 2000; onTriggered: copyBtn2.copied = false }
                            MouseArea {
                                id: copyMA2; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                                onClicked: { appController.copyShareLink(); copyBtn2.copied = true; copyTimer2.restart() }
                            }
                        }
                    }
                }
            }

            ProgressPanel { Layout.fillWidth: true }

            MemberList { Layout.fillWidth: true }

            // Freeze button
            Rectangle {
                Layout.fillWidth: true; height: 40; radius: 4
                visible: appController.frozen && appController.receiversPresent
                color: freezeMA2.containsMouse ? "#1a4a80" : "#0f3460"
                border.color: "#1a4a80"

                Text { anchors.centerIn: parent; text: "Start transfer (" + appController.freezeRemaining + "s)"; color: "#eee"; font.pixelSize: 13 }
                MouseArea { id: freezeMA2; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: appController.dropFreeze() }
            }

            Text {
                Layout.fillWidth: true
                visible: appController.frozen && !appController.receiversPresent
                text: appController.t.waitingForReceivers + " (" + appController.freezeRemaining + appController.t.secondsShort + ")"
                color: "#999"; font.pixelSize: 12; horizontalAlignment: Text.AlignHCenter
            }

            SessionTimer { Layout.fillWidth: true; visible: !appController.frozen }

            // Terminate button
            Rectangle {
                Layout.fillWidth: true; height: 36; radius: 4
                color: termMA2.containsMouse ? Qt.rgba(0.91,0.27,0.38,0.3) : Qt.rgba(0.91,0.27,0.38,0.15)
                border.color: "#e94560"
                Text { anchors.centerIn: parent; text: "Terminate session"; color: "#e94560"; font.pixelSize: 13 }
                MouseArea { id: termMA2; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: appController.terminateSession() }
            }

            Text {
                Layout.fillWidth: true; visible: appController.errorMsg.length > 0
                text: appController.errorMsg; color: "#e94560"; font.pixelSize: 13; wrapMode: Text.Wrap
            }
        }
    }
}

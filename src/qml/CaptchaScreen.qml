// Copyright (C) 2026  Roman Lyubimov
// SPDX-License-Identifier: GPL-3.0-or-later
// For full license text, see <https://www.gnu.org/licenses/gpl-3.0.txt>

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    ColumnLayout {
        anchors.centerIn: parent
        width: Math.min(parent.width - 32, 380)
        spacing: 20

        Text {
            Layout.alignment: Qt.AlignHCenter
            text: appController.t.captchaTitle
            color: "#eee"; font.pixelSize: 20; font.bold: true
        }

        Rectangle {
            Layout.alignment: Qt.AlignHCenter
            width: 280; height: 100; radius: 8
            color: "#16213e"; border.color: "#0f3460"
            Image {
                anchors.centerIn: parent
                source: appController.captchaImage.length > 0
                        ? "data:image/png;base64," + appController.captchaImage : ""
                fillMode: Image.PreserveAspectFit
                width: parent.width - 16; height: parent.height - 16
            }
        }

        RowLayout {
            Layout.fillWidth: true; spacing: 8

            Rectangle {
                Layout.fillWidth: true; height: 40; radius: 4
                color: "#16213e"; border.color: "#0f3460"
                TextInput {
                    id: captchaInput; anchors.fill: parent; anchors.margins: 8
                    color: "#eee"; font.pixelSize: 16; font.family: "monospace"; font.letterSpacing: 4
                    horizontalAlignment: TextInput.AlignHCenter; verticalAlignment: TextInput.AlignVCenter
                    maximumLength: appController.captchaAnswerLength > 0 ? appController.captchaAnswerLength : 10
                    selectByMouse: true
                    Keys.onReturnPressed: submitCaptcha()
                }
            }

            Rectangle {
                width: 80; height: 40; radius: 4
                color: submitMA.containsMouse ? (submitMA.pressed ? "#0a2a50" : "#1a4a80") : "#0f3460"
                Text { anchors.centerIn: parent; text: appController.t.submit; color: "#eee"; font.pixelSize: 14 }
                MouseArea {
                    id: submitMA; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                    onClicked: submitCaptcha()
                }
            }
        }

        Text {
            Layout.alignment: Qt.AlignHCenter
            visible: appController.errorMsg.length > 0
            text: appController.errorMsg; color: "#e94560"; font.pixelSize: 13
        }
    }

    function submitCaptcha() {
        if (captchaInput.text.trim().length > 0)
            appController.solveCaptcha(captchaInput.text.trim())
    }

    Component.onCompleted: captchaInput.forceActiveFocus()
}

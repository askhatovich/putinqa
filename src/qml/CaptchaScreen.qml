// Copyright (C) 2026  Roman Lyubimov
// SPDX-License-Identifier: GPL-3.0-or-later
// For full license text, see <https://www.gnu.org/licenses/gpl-3.0.txt>

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    ColumnLayout {
        id: col
        anchors.centerIn: parent
        width: Math.min(parent.width - 32, 400)
        spacing: 18

        Text {
            Layout.alignment: Qt.AlignHCenter
            text: appController.t.captchaTitle
            color: "#eee"; font.pixelSize: 20; font.bold: true
        }

        // Captcha image: square card that scales with the ColumnLayout
        // width. Pixelated rendering (smooth: false) matches the web UI's
        // `image-rendering: pixelated` so scaled captcha stays legible.
        Rectangle {
            id: imageCard
            Layout.alignment: Qt.AlignHCenter
            Layout.fillWidth: true
            Layout.preferredHeight: width
            radius: 10
            color: "#1a1a2e"
            border.color: "#0f3460"
            border.width: 1

            Image {
                anchors.fill: parent
                anchors.margins: 10
                source: appController.captchaImage.length > 0
                        ? "data:image/png;base64," + appController.captchaImage : ""
                fillMode: Image.Stretch
                smooth: false
                mipmap: false
                cache: false
            }
        }

        // Input with focus-aware border and a subtle placeholder that shows
        // the expected answer length as a dotted guide.
        Rectangle {
            Layout.alignment: Qt.AlignHCenter
            Layout.fillWidth: true
            height: 48
            radius: 6
            color: "#16213e"
            border.color: captchaInput.activeFocus ? "#e94560" : "#0f3460"
            border.width: captchaInput.activeFocus ? 2 : 1

            TextInput {
                id: captchaInput
                anchors.fill: parent
                anchors.margins: 8
                color: "#eee"
                font.pixelSize: 20
                font.family: "monospace"
                font.letterSpacing: 6
                font.bold: true
                horizontalAlignment: TextInput.AlignHCenter
                verticalAlignment: TextInput.AlignVCenter
                maximumLength: appController.captchaAnswerLength > 0 ? appController.captchaAnswerLength : 10
                selectByMouse: true
                Keys.onReturnPressed: submitCaptcha()

                Text {
                    anchors.fill: parent
                    verticalAlignment: Text.AlignVCenter
                    horizontalAlignment: Text.AlignHCenter
                    visible: !captchaInput.text
                    text: appController.captchaAnswerLength > 0
                          ? "•".repeat(appController.captchaAnswerLength)
                          : ""
                    color: "#444"
                    font.pixelSize: 20
                    font.family: "monospace"
                    font.letterSpacing: 6
                }
            }
        }

        // Primary submit button, full-width
        Rectangle {
            Layout.alignment: Qt.AlignHCenter
            Layout.fillWidth: true
            height: 44
            radius: 6
            color: submitMA.containsMouse ? (submitMA.pressed ? "#c73850" : "#d63450") : "#e94560"
            Text {
                anchors.centerIn: parent
                text: appController.t.submit
                color: "#fff"; font.pixelSize: 15; font.bold: true
            }
            MouseArea {
                id: submitMA
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: submitCaptcha()
            }
        }

        Text {
            Layout.alignment: Qt.AlignHCenter
            Layout.fillWidth: true
            visible: appController.errorMsg.length > 0
            text: appController.errorMsg
            color: "#e94560"; font.pixelSize: 13
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
        }
    }

    function submitCaptcha() {
        if (captchaInput.text.trim().length > 0)
            appController.solveCaptcha(captchaInput.text.trim())
    }

    Component.onCompleted: captchaInput.forceActiveFocus()
}

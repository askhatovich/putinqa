// Copyright (C) 2026  Roman Lyubimov
// SPDX-License-Identifier: GPL-3.0-or-later
// For full license text, see <https://www.gnu.org/licenses/gpl-3.0.txt>

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    height: 40
    color: "#16213e"

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 12; anchors.rightMargin: 12
        spacing: 8

        // Proxy indicator
        Text {
            visible: appController.proxyType !== "none" && appController.proxyHost.length > 0
            text: appController.proxyType.toUpperCase() + " " + appController.proxyHost + ":" + appController.proxyPort
            color: "#e9a040"
            font.pixelSize: 10
            font.family: "monospace"
            elide: Text.ElideRight
            Layout.maximumWidth: 160
        }

        Text {
            visible: appController.proxyType !== "none" && appController.proxyHost.length > 0
                     && (appController.inSession ? appController.activeServer.length > 0 : appController.serverUrl.length > 0)
            text: "\u2192"
            color: "#666"
            font.pixelSize: 11
        }

        // Server indicator
        Text {
            visible: appController.inSession ? appController.activeServer.length > 0 : appController.serverUrl.length > 0
            text: appController.inSession ? appController.activeServer : appController.serverUrl
            color: appController.inSession ? "#4caf50" : "#666"
            font.pixelSize: 11
            font.family: "monospace"
            elide: Text.ElideMiddle
            Layout.maximumWidth: 220
        }

        Item { Layout.fillWidth: true }

        // Settings button
        Rectangle {
            width: 28; height: 28; radius: 4
            color: settingsMA.containsMouse ? "#1a4a80" : "transparent"
            Text { anchors.centerIn: parent; text: "\u2699"; color: "#999"; font.pixelSize: 16 }
            MouseArea {
                id: settingsMA; anchors.fill: parent; hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: appController.openSettings()
            }
        }
    }
}

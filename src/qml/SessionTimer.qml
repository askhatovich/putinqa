// Copyright (C) 2026  Roman Lyubimov
// SPDX-License-Identifier: GPL-3.0-or-later
// For full license text, see <https://www.gnu.org/licenses/gpl-3.0.txt>

import QtQuick
import QtQuick.Layouts

Item {
    Layout.fillWidth: true
    height: 24

    Text {
        anchors.centerIn: parent
        text: {
            var secs = appController.sessionExpirationIn
            var m = Math.floor(secs / 60)
            var s = secs % 60
            return appController.t.sessionExpiresIn + " " + m + ":" + (s < 10 ? "0" : "") + s
        }
        color: appController.sessionExpirationIn < 300 ? "#e94560" : "#666"
        font.pixelSize: 11
    }
}

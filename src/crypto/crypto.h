// Copyright (C) 2026  Roman Lyubimov
// SPDX-License-Identifier: GPL-3.0-or-later
// For full license text, see <https://www.gnu.org/licenses/gpl-3.0.txt>

#pragma once

#include <QByteArray>
#include <QString>

namespace Crypto {

bool init();
QByteArray generateKey();
QByteArray encrypt(const QByteArray &plaintext, const QByteArray &key);
QByteArray decrypt(const QByteArray &data, const QByteArray &key);
QString keyToBase64Url(const QByteArray &key);
QByteArray base64UrlToKey(const QString &str);

} // namespace Crypto

// Copyright (C) 2026  Roman Lyubimov
// SPDX-License-Identifier: GPL-3.0-or-later
// For full license text, see <https://www.gnu.org/licenses/gpl-3.0.txt>

#include "crypto.h"
#include <sodium.h>

bool Crypto::init()
{
    return sodium_init() >= 0;
}

QByteArray Crypto::generateKey()
{
    QByteArray key(crypto_aead_xchacha20poly1305_ietf_KEYBYTES, Qt::Uninitialized);
    crypto_aead_xchacha20poly1305_ietf_keygen(
        reinterpret_cast<unsigned char *>(key.data()));
    return key;
}

QByteArray Crypto::encrypt(const QByteArray &plaintext, const QByteArray &key)
{
    constexpr int nonceLen = crypto_aead_xchacha20poly1305_ietf_NPUBBYTES;

    QByteArray nonce(nonceLen, Qt::Uninitialized);
    randombytes_buf(reinterpret_cast<unsigned char *>(nonce.data()), nonceLen);

    QByteArray ciphertext(plaintext.size() + crypto_aead_xchacha20poly1305_ietf_ABYTES,
                          Qt::Uninitialized);
    unsigned long long ciphertextLen = 0;

    crypto_aead_xchacha20poly1305_ietf_encrypt(
        reinterpret_cast<unsigned char *>(ciphertext.data()),
        &ciphertextLen,
        reinterpret_cast<const unsigned char *>(plaintext.constData()),
        plaintext.size(),
        nullptr, 0,
        nullptr,
        reinterpret_cast<const unsigned char *>(nonce.constData()),
        reinterpret_cast<const unsigned char *>(key.constData()));

    ciphertext.resize(static_cast<qsizetype>(ciphertextLen));

    QByteArray result;
    result.reserve(nonceLen + ciphertext.size());
    result.append(nonce);
    result.append(ciphertext);
    return result;
}

QByteArray Crypto::decrypt(const QByteArray &data, const QByteArray &key)
{
    constexpr int nonceLen = crypto_aead_xchacha20poly1305_ietf_NPUBBYTES;

    if (data.size() <= nonceLen) {
        return {};
    }

    const QByteArray nonce = data.left(nonceLen);
    const QByteArray ciphertext = data.mid(nonceLen);

    QByteArray plaintext(ciphertext.size(), Qt::Uninitialized);
    unsigned long long plaintextLen = 0;

    const int ret = crypto_aead_xchacha20poly1305_ietf_decrypt(
        reinterpret_cast<unsigned char *>(plaintext.data()),
        &plaintextLen,
        nullptr,
        reinterpret_cast<const unsigned char *>(ciphertext.constData()),
        ciphertext.size(),
        nullptr, 0,
        reinterpret_cast<const unsigned char *>(nonce.constData()),
        reinterpret_cast<const unsigned char *>(key.constData()));

    if (ret != 0) {
        return {};
    }

    plaintext.resize(static_cast<qsizetype>(plaintextLen));
    return plaintext;
}

QString Crypto::keyToBase64Url(const QByteArray &key)
{
    return QString::fromLatin1(
        key.toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals));
}

QByteArray Crypto::base64UrlToKey(const QString &str)
{
    return QByteArray::fromBase64(
        str.toLatin1(),
        QByteArray::Base64UrlEncoding | QByteArray::AbortOnBase64DecodingErrors);
}

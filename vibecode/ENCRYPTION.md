# Encryption

## Algorithm

XChaCha20-Poly1305 AEAD (Authenticated Encryption with Associated Data) via libsodium.

- Key: 32 bytes (256-bit), randomly generated per session
- Nonce: 24 bytes (192-bit), randomly generated per chunk
- Auth tag: 16 bytes (128-bit), appended to ciphertext

## Chunk Wire Format

```
[nonce: 24 bytes][ciphertext: plaintext_size bytes][tag: 16 bytes]
```

Total overhead per chunk: 40 bytes (24 nonce + 16 tag).

Server's `maxChunkSize` includes this overhead, so actual payload per chunk is `maxChunkSize - 40`.

## Key Distribution

Encryption key is embedded in the share link URL fragment:

```
http://server/#id=SESSION_ID&encryption=xchacha20-poly1305&key=BASE64URL_KEY
```

The fragment (`#...`) is never sent to the server — the key stays client-side only. This provides true end-to-end encryption: the server stores and relays encrypted chunks without access to the plaintext.

## Key Encoding

- `Crypto::keyToBase64Url()` — RFC 4648 base64url encoding, no padding
- `Crypto::base64UrlToKey()` — decodes back to 32-byte key, returns empty on error

## Encryption Flow (Sender)

1. `Crypto::generateKey()` — random 32-byte key via `crypto_aead_xchacha20poly1305_ietf_keygen`
2. Read file chunk (up to `maxChunkPayload` bytes)
3. `Crypto::encrypt(plaintext, key)` — random nonce, encrypt, prepend nonce
4. Send encrypted bytes as WS binary frame

## Decryption Flow (Receiver)

1. Download encrypted chunk via HTTP
2. `Crypto::decrypt(data, key)` — extract nonce (first 24 bytes), decrypt remainder
3. Returns empty QByteArray on authentication failure (tampered data)
4. Store decrypted chunk in `m_downloadedChunks` map by index

## Security Properties

- Each chunk has a unique random nonce (not sequential)
- Authentication tag prevents tampering
- Server never sees plaintext
- Key compromise requires access to the share link

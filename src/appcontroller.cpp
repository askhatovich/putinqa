// Copyright (C) 2026  Roman Lyubimov
// SPDX-License-Identifier: GPL-3.0-or-later
// For full license text, see <https://www.gnu.org/licenses/gpl-3.0.txt>

#include "appcontroller.h"
#include "crypto/crypto.h"
#include "client/session/actions.h"

#include <QClipboard>
#include <QGuiApplication>
#include <QFileInfo>
#include <QDir>
#include <QStandardPaths>
#include <QRandomGenerator>
#include <QUrlQuery>
#include <QDateTime>
#include <QDebug>
#include <QImage>
#include <QBuffer>
#include <qrencode.h>

AppController::AppController(QObject *parent)
    : QObject(parent)
    , m_settings("askhatovich", "putinqa")
    , m_serverWorkload(new ServerWorkload(this))
    , m_freezeTimer(new QTimer(this))
    , m_expirationTimer(new QTimer(this))
{
    loadSettings();

    if (!Crypto::init()) {
        qFatal("Failed to initialize libsodium");
    }

    if (!m_serverUrl.isEmpty()) {
        m_serverWorkload->onServerHostUpdated(QUrl(m_serverUrl));
    }
    QObject::connect(m_serverWorkload, &ServerWorkload::updated,
                     this, &AppController::onServerWorkloadUpdated);
    QObject::connect(m_serverWorkload, &ServerWorkload::connectionFailed, this, [this]() {
        if (m_stats.value("connected", true).toBool()) {
            m_stats["connected"] = false;
            emit statsChanged();
        }
    });

    m_freezeTimer->setInterval(1000);
    QObject::connect(m_freezeTimer, &QTimer::timeout, this, [this]() {
        if (m_freezeRemaining > 0) {
            m_freezeRemaining--;
            emit freezeRemainingChanged();
        }
        if (m_freezeRemaining <= 0) m_freezeTimer->stop();
    });

    m_expirationTimer->setInterval(1000);
    QObject::connect(m_expirationTimer, &QTimer::timeout, this, [this]() {
        if (m_sessionExpirationIn > 0) {
            m_sessionExpirationIn--;
            emit sessionExpirationInChanged();
        }
    });
}

void AppController::loadSettings()
{
    m_serverUrl = m_settings.value("server/url", "https://pip.dotcpp.ru").toString();
    m_userName = m_settings.value("user/name", "").toString();

    m_language = m_settings.value("app/language", "").toString();
    if (m_language.isEmpty()) {
        m_language = (QLocale::system().language() == QLocale::Russian) ? "ru" : "en";
        m_settings.setValue("app/language", m_language);
    }

    if (m_userName.isEmpty()) {
        static const QStringList en = {
            "Wombat","Gecko","Parrot","Jellyfish","Flamingo","Coyote","Badger","Lemur","Toucan","Chameleon",
            "Squirrel","Dolphin","Lobster","Pelican","Mantis","Salmon","Puffin","Beetle","Sparrow","Marmot",
            "Truffle","Pretzel","Crouton","Popcorn","Cupcake","Brownie","Pancake","Gumball","Cinnamon","Coconut",
            "Nimbus","Ripple","Flicker","Twinkle","Drizzle","Rumble","Nebula","Quartz","Thunder","Willow",
            "Riddle","Rascal","Gadget","Bandit","Rocket","Nimble","Frosty","Sketch","Tempo","Wobble",
        };
        static const QStringList ru = {
            QString::fromUtf8("Бурундук"), QString::fromUtf8("Фламинго"), QString::fromUtf8("Дельфин"),
            QString::fromUtf8("Попугай"), QString::fromUtf8("Медуза"), QString::fromUtf8("Хамелеон"),
            QString::fromUtf8("Белка"), QString::fromUtf8("Барсук"), QString::fromUtf8("Лемур"),
            QString::fromUtf8("Тукан"), QString::fromUtf8("Пельмень"), QString::fromUtf8("Бублик"),
            QString::fromUtf8("Ириска"), QString::fromUtf8("Крендель"), QString::fromUtf8("Пончик"),
            QString::fromUtf8("Сырник"), QString::fromUtf8("Блинчик"), QString::fromUtf8("Зефирка"),
            QString::fromUtf8("Карамелька"), QString::fromUtf8("Кокос"), QString::fromUtf8("Гром"),
            QString::fromUtf8("Вихрь"), QString::fromUtf8("Радуга"), QString::fromUtf8("Туман"),
            QString::fromUtf8("Иней"), QString::fromUtf8("Рябина"), QString::fromUtf8("Капелька"),
            QString::fromUtf8("Метеор"), QString::fromUtf8("Кварц"), QString::fromUtf8("Магнит"),
            QString::fromUtf8("Шкода"), QString::fromUtf8("Фантик"), QString::fromUtf8("Кубик"),
            QString::fromUtf8("Ракета"), QString::fromUtf8("Пружинка"), QString::fromUtf8("Штучка"),
            QString::fromUtf8("Моторчик"), QString::fromUtf8("Чудик"), QString::fromUtf8("Топтыжка"),
            QString::fromUtf8("Смельчак"), QString::fromUtf8("Бубенчик"), QString::fromUtf8("Барабан"),
            QString::fromUtf8("Молния"), QString::fromUtf8("Юла"), QString::fromUtf8("Непоседа"),
            QString::fromUtf8("Живчик"), QString::fromUtf8("Шустрик"), QString::fromUtf8("Торопыжка"),
            QString::fromUtf8("Задира"), QString::fromUtf8("Ветрило"),
        };
        const auto &names = (m_language == "ru") ? ru : en;

        // Random emoji from dense Unicode emoji blocks
        struct Range { char32_t start; char32_t end; };
        static const Range emojiRanges[] = {
            {0x1F600, 0x1F636}, // smileys
            {0x1F400, 0x1F43E}, // animals
        };
        const auto &r = emojiRanges[QRandomGenerator::global()->bounded(2)];
        char32_t cp = r.start + QRandomGenerator::global()->bounded(r.end - r.start + 1);

        m_userName = names[QRandomGenerator::global()->bounded(names.size())]
                     + " " + QString::fromUcs4(&cp, 1);
        m_settings.setValue("user/name", m_userName);
    }

    m_proxyType = m_settings.value("proxy/type", "none").toString();
    m_proxyHost = m_settings.value("proxy/host", "").toString();
    m_proxyPort = static_cast<quint16>(m_settings.value("proxy/port", 0).toUInt());
    applyProxy();

    emit userNameChanged();
    emit serverUrlChanged();
    emit languageChanged();
}

void AppController::applyProxy()
{
    QNetworkProxy proxy;
    if (m_proxyType == "socks5") {
        proxy.setType(QNetworkProxy::Socks5Proxy);
        proxy.setHostName(m_proxyHost);
        proxy.setPort(m_proxyPort);
    } else if (m_proxyType == "http") {
        proxy.setType(QNetworkProxy::HttpProxy);
        proxy.setHostName(m_proxyHost);
        proxy.setPort(m_proxyPort);
    } else {
        proxy.setType(QNetworkProxy::NoProxy);
    }
    QNetworkProxy::setApplicationProxy(proxy);
}

void AppController::setScreen(const QString &screen)
{
    if (m_screen == screen) return;
    bool wasInSession = inSession();
    m_screen = screen;
    emit screenChanged();
    if (inSession() != wasInSession) emit inSessionChanged();
}

void AppController::setError(const QString &msg)
{
    m_errorMsg = msg;
    emit errorMsgChanged();
}

// --- QML Actions ---

void AppController::startSend()
{
    m_isSender = true;
    m_pendingRole = "sender";
    emit isSenderChanged();
}

void AppController::selectFile(const QUrl &fileUrl)
{
    m_filePath = fileUrl.toLocalFile();
    QFileInfo fi(m_filePath);
    if (!fi.exists() || !fi.isFile()) {
        setError("File not found: " + m_filePath);
        return;
    }

    m_fileName = fi.fileName();
    m_fileSize = fi.size();
    emit fileNameChanged();
    emit fileSizeChanged();

    m_activeServer = m_serverUrl;
    emit activeServerChanged();
    m_serverWorkload->onServerHostUpdated(QUrl(m_activeServer));

    setScreen("connecting");
    authorize();
}

void AppController::startReceive(const QString &link)
{
    m_isSender = false;
    m_pendingRole = "receiver";
    emit isSenderChanged();

    QUrl url(link);
    QString fragment = url.fragment();
    QUrlQuery query(fragment);

    m_pendingSessionId = query.queryItemValue("id");
    QString keyStr = query.queryItemValue("key");
    QString encryption = query.queryItemValue("encryption");

    if (m_pendingSessionId.isEmpty() || keyStr.isEmpty()) {
        setError("Invalid link: missing session ID or key");
        return;
    }

    if (encryption != "xchacha20-poly1305") {
        setError("Unsupported encryption: " + encryption);
        return;
    }

    m_encryptionKey = Crypto::base64UrlToKey(keyStr);
    if (m_encryptionKey.isEmpty()) {
        setError("Invalid encryption key");
        return;
    }

    // Extract server from link (don't overwrite settings)
    QUrl serverUrl;
    serverUrl.setScheme(url.scheme());
    serverUrl.setHost(url.host());
    if (url.port() != -1) serverUrl.setPort(url.port());
    QString linkServer = serverUrl.toString();

    m_activeServer = linkServer.isEmpty() ? m_serverUrl : linkServer;
    emit activeServerChanged();
    m_serverWorkload->onServerHostUpdated(QUrl(m_activeServer));

    setScreen("connecting");
    authorize();
}

void AppController::authorize()
{
    if (m_auth) m_auth->deleteLater();
    m_auth = new Authorization(this);
    m_auth->setUrl(QUrl(m_activeServer));
    m_auth->setName(m_userName);

    QObject::connect(m_auth, &Authorization::authorized, this, &AppController::onAuthorized);
    QObject::connect(m_auth, &Authorization::captchaRequired, this, &AppController::onCaptchaRequired);
    QObject::connect(m_auth, &Authorization::error, this, &AppController::onAuthError);

    m_auth->connect();
}

void AppController::solveCaptcha(const QString &answer)
{
    if (!m_auth) return;
    setScreen("connecting");
    m_auth->confirmCaptcha(answer);
}

void AppController::copyShareLink()
{
    auto *clipboard = QGuiApplication::clipboard();
    if (clipboard) clipboard->setText(m_shareLink);
}

void AppController::openSettings()
{
    if (m_screen == "settings") {
        setScreen(m_screenBeforeSettings.isEmpty() ? "entry" : m_screenBeforeSettings);
    } else {
        m_screenBeforeSettings = m_screen;
        setScreen("settings");
    }
}

void AppController::saveSettings(const QString &url, const QString &name, const QString &language,
                                  const QString &proxyType, const QString &proxyHost, quint16 proxyPort)
{
    m_serverUrl = url;
    bool nameChanged = (m_userName != name);
    m_userName = name;
    m_settings.setValue("server/url", m_serverUrl);
    m_settings.setValue("user/name", m_userName);
    emit serverUrlChanged();
    emit userNameChanged();

    QString effectiveProxyType = proxyType;
    if (effectiveProxyType != "none" && (proxyHost.isEmpty() || proxyPort == 0))
        effectiveProxyType = "none";

    if (m_proxyType != effectiveProxyType || m_proxyHost != proxyHost || m_proxyPort != proxyPort) {
        m_proxyType = effectiveProxyType;
        m_proxyHost = proxyHost;
        m_proxyPort = proxyPort;
        m_settings.setValue("proxy/type", m_proxyType);
        m_settings.setValue("proxy/host", m_proxyHost);
        m_settings.setValue("proxy/port", m_proxyPort);
        applyProxy();
        emit proxyChanged();
    }

    if (nameChanged && m_session) {
        // Server truncates to 20 chars — match locally
        QString serverName = m_userName.left(20);
        m_userName = serverName;
        m_settings.setValue("user/name", m_userName);
        emit userNameChanged();

        m_session->sendJsonMessage(Action::NewName(m_userName).json());

        if (m_isSender) {
            m_senderName = m_userName;
            emit senderNameChanged();
        } else {
            QString myId = m_auth ? m_auth->getId() : QString();
            for (int i = 0; i < m_receivers.size(); ++i) {
                auto map = m_receivers[i].toMap();
                if (map["id"].toString() == myId) {
                    map["name"] = m_userName;
                    m_receivers[i] = map;
                    emit receiversChanged();
                    break;
                }
            }
        }
    }

    if (m_language != language) {
        m_language = language;
        m_settings.setValue("app/language", m_language);
        emit languageChanged();
    }

    m_serverWorkload->onServerHostUpdated(QUrl(m_serverUrl));
    setScreen(m_screenBeforeSettings.isEmpty() ? "entry" : m_screenBeforeSettings);
}

void AppController::dropFreeze()
{
    if (m_session) {
        m_session->sendJsonMessage(Action::DropFreeze().json());
    }
}

void AppController::kickReceiver(const QString &id)
{
    if (m_session) {
        m_session->sendJsonMessage(Action::KickReceiver(id).json());
    }
}

void AppController::terminateSession()
{
    if (m_session) {
        m_terminateRequested = true;
        m_waitingForChunkAccepted = false;
        m_canSendChunk = false;
        if (m_uploadFile) {
            m_uploadFile->close();
            delete m_uploadFile;
            m_uploadFile = nullptr;
        }
        m_session->sendJsonMessage(Action::TerminateSession().json());
    }
}

void AppController::leaveSession()
{
    if (!m_session) { restart(); return; }


    QUrl url(m_activeServer);
    url.setPath("/api/me/leave");

    auto *manager = new QNetworkAccessManager(this);
    manager->setTransferTimeout(10000);
    manager->setCookieJar(m_session->getCookieJar().data());
    m_session->getCookieJar()->setParent(nullptr);

    auto *reply = manager->post(QNetworkRequest(url), QByteArray());
    QObject::connect(reply, &QNetworkReply::finished, this, [this, manager]() {
        manager->deleteLater();
        restart();
    });
}

void AppController::changeName(const QString &name)
{
    m_userName = name;
    m_settings.setValue("user/name", m_userName);
    emit userNameChanged();

    if (m_session) {
        QString serverName = m_userName.left(20);
        m_userName = serverName;
        m_settings.setValue("user/name", m_userName);
        emit userNameChanged();

        m_session->sendJsonMessage(Action::NewName(m_userName).json());

        if (m_isSender) {
            m_senderName = m_userName;
            emit senderNameChanged();
        } else {
            // Server doesn't echo name_changed to self — update locally
            QString myId = m_auth ? m_auth->getId() : QString();
            for (int i = 0; i < m_receivers.size(); ++i) {
                auto map = m_receivers[i].toMap();
                if (map["id"].toString() == myId) {
                    map["name"] = m_userName;
                    m_receivers[i] = map;
                    emit receiversChanged();
                    break;
                }
            }
        }
    }
}

void AppController::minimizeToTray()
{
    emit trayRequested();
}

void AppController::restart()
{
    if (m_session) {
        m_session->deleteLater();
        m_session = nullptr;
    }
    if (m_auth) {
        m_auth->deleteLater();
        m_auth = nullptr;
    }
    resetSessionState();
    setScreen("entry");
}

void AppController::resetSessionState()
{
    m_screenBeforeSettings.clear();
    m_encryptionKey.clear();
    m_shareLink.clear(); emit shareLinkChanged();
    m_filePath.clear();
    m_fileName.clear(); emit fileNameChanged();
    m_fileSize = 0; emit fileSizeChanged();
    m_progress = 0; emit progressChanged();
    m_bytesTransferred = 0; emit bytesTransferredChanged();
    m_uploadFinished = false; emit uploadFinishedChanged();
    m_bufferUsed = 0; emit bufferUsedChanged();
    m_bufferMax = 10; emit bufferMaxChanged();
    m_frozen = true; emit frozenChanged();
    m_freezeRemaining = 0; emit freezeRemainingChanged();
    m_sessionExpirationIn = 0; emit sessionExpirationInChanged();
    m_senderName.clear(); emit senderNameChanged();
    m_senderOnline = false; emit senderOnlineChanged();
    m_receivers.clear(); emit receiversChanged();
    m_completeStatus.clear(); emit completeStatusChanged();
    m_hasDownloadedFile = false; emit hasDownloadedFileChanged();
    m_captchaImage.clear(); emit captchaImageChanged();
    m_captchaAnswerLength = 0; emit captchaAnswerLengthChanged();
    m_chunksConfirmed = 0; emit chunksConfirmedChanged();
    m_highestKnownChunk = 0; emit highestKnownChunkChanged();
    m_pendingConfirms = 0;
    setError("");

    if (m_uploadFile) {
        m_uploadFile->close();
        delete m_uploadFile;
        m_uploadFile = nullptr;
    }
    m_waitingForChunkAccepted = false;
    m_canSendChunk = true;
    cleanupDownloadTmpFile();
    m_downloadQueue.clear();
    m_activeDownloads = 0;
    m_receiverChunksDone.clear();
    m_pendingSessionId.clear();
    m_pendingRole.clear();

    m_activeServer.clear(); emit activeServerChanged();
    m_serverWorkload->onServerHostUpdated(QUrl(m_serverUrl));
    m_terminateRequested = false;
    m_freezeTimer->stop();
    m_expirationTimer->stop();
}

QString AppController::formatBytes(qint64 bytes) const
{
    if (bytes < 1024) return QStringLiteral("%1 B").arg(bytes);
    if (bytes < 1024 * 1024) return QStringLiteral("%1 KB").arg(bytes / 1024.0, 0, 'f', 1);
    if (bytes < 1024LL * 1024 * 1024) return QStringLiteral("%1 MB").arg(bytes / (1024.0 * 1024), 0, 'f', 1);
    return QStringLiteral("%1 GB").arg(bytes / (1024.0 * 1024 * 1024), 0, 'f', 2);
}

QString AppController::qrDataUrl() const
{
    if (m_shareLink.isEmpty())
        return {};

    QRcode *qr = QRcode_encodeString(m_shareLink.toUtf8().constData(),
                                     0, QR_ECLEVEL_M, QR_MODE_8, 1);
    if (!qr)
        return {};

    const int modules = qr->width;
    const int margin = 2;
    const int scale = 8;
    const int size = (modules + margin * 2) * scale;

    QImage img(size, size, QImage::Format_RGB32);
    img.fill(QColor(0x1a, 0x1a, 0x2e));  // dark background

    const QColor fg(0xee, 0xee, 0xee);
    for (int y = 0; y < modules; ++y) {
        for (int x = 0; x < modules; ++x) {
            if (qr->data[y * modules + x] & 1) {
                const int px = (x + margin) * scale;
                const int py = (y + margin) * scale;
                for (int dy = 0; dy < scale; ++dy)
                    for (int dx = 0; dx < scale; ++dx)
                        img.setPixel(px + dx, py + dy, fg.rgb());
            }
        }
    }

    QRcode_free(qr);

    QByteArray pngData;
    QBuffer buf(&pngData);
    buf.open(QIODevice::WriteOnly);
    img.save(&buf, "PNG");

    return QStringLiteral("data:image/png;base64,") + QString::fromLatin1(pngData.toBase64());
}

QVariantMap AppController::translations() const
{
    static const QVariantMap en = {
        {"appSlogan", "Streaming file transfer with end-to-end encryption"},
        {"sendFile", "Send file"},
        {"receiveFile", "Receive file"},
        {"pasteLink", "Paste the share link:"},
        {"join", "Join"},
        {"settings", "Settings"},
        {"serverUrlLabel", "Server for sending"},
        {"displayName", "Display name"},
        {"languageLabel", "Language"},
        {"save", "Save"},
        {"back", "Back"},
        {"captchaTitle", "Verify you're human"},
        {"submit", "Submit"},
        {"shareLinkLabel", "Share this link with the receiver:"},
        {"copy", "Copy"},
        {"copied", "Copied!"},
        {"startTransfer", "Stop waiting"},
        {"waitingForReceivers", "Waiting for receivers..."},
        {"terminateSession", "Terminate session"},
        {"leaveSession", "Leave session"},
        {"noFileSelected", "No file selected"},
        {"buffer", "Buffer"},
        {"uploadComplete", "Upload complete"},
        {"allDataReceived", "All data received from sender"},
        {"participants", "Participants"},
        {"senderRole", "sender"},
        {"senderFallback", "Sender"},
        {"receiverFallback", "Receiver"},
        {"sessionExpiresIn", "Session expires in"},
        {"transferComplete", "Transfer complete!"},
        {"sessionTimedOut", "Session timed out"},
        {"senderDisconnected", "Sender disconnected"},
        {"noReceiversJoined", "No receivers joined"},
        {"sessionTerminated", "Session terminated"},
        {"error", "Error"},
        {"sessionEnded", "Session ended: "},
        {"saveFile", "Save file"},
        {"newTransfer", "Home"},
        {"connecting", "Connecting..."},
        {"users", "Users"},
        {"sessions", "Sessions"},
        {"connectingToServer", "Connecting to server..."},
        {"selectFileTitle", "Select a file to send"},
        {"saveFileTitle", "Save received file"},
        {"chunks", "Chunks"},
        {"secondsShort", "s"},
        {"noConnection", "No connection"},
        {"proxyLabel", "Proxy"},
        {"proxyNone", "Off"},
        {"proxyHost", "Host"},
        {"proxyPort", "Port"},
    };
    static const QVariantMap ru = {
        {"appSlogan", QString::fromUtf8("Потоковая передача файлов со сквозным шифрованием")},
        {"sendFile", QString::fromUtf8("Отправить файл")},
        {"receiveFile", QString::fromUtf8("Получить файл")},
        {"pasteLink", QString::fromUtf8("Вставьте ссылку:")},
        {"join", QString::fromUtf8("Подключиться")},
        {"settings", QString::fromUtf8("Настройки")},
        {"serverUrlLabel", QString::fromUtf8("Сервер для отправки")},
        {"displayName", QString::fromUtf8("Отображаемое имя")},
        {"languageLabel", QString::fromUtf8("Язык")},
        {"save", QString::fromUtf8("Сохранить")},
        {"back", QString::fromUtf8("Назад")},
        {"captchaTitle", QString::fromUtf8("Подтвердите, что вы человек")},
        {"submit", QString::fromUtf8("Отправить")},
        {"shareLinkLabel", QString::fromUtf8("Отправьте ссылку получателям:")},
        {"copy", QString::fromUtf8("Копировать")},
        {"copied", QString::fromUtf8("Скопировано!")},
        {"startTransfer", QString::fromUtf8("Закончить ожидание")},
        {"waitingForReceivers", QString::fromUtf8("Ожидание получателей...")},
        {"terminateSession", QString::fromUtf8("Завершить сессию")},
        {"leaveSession", QString::fromUtf8("Покинуть сессию")},
        {"noFileSelected", QString::fromUtf8("Файл не выбран")},
        {"buffer", QString::fromUtf8("Буфер")},
        {"uploadComplete", QString::fromUtf8("Загрузка завершена")},
        {"allDataReceived", QString::fromUtf8("Все данные получены от отправителя")},
        {"participants", QString::fromUtf8("Участники")},
        {"senderRole", QString::fromUtf8("отправитель")},
        {"senderFallback", QString::fromUtf8("Отправитель")},
        {"receiverFallback", QString::fromUtf8("Получатель")},
        {"sessionExpiresIn", QString::fromUtf8("Сессия истекает через")},
        {"transferComplete", QString::fromUtf8("Передача завершена!")},
        {"sessionTimedOut", QString::fromUtf8("Время сессии истекло")},
        {"senderDisconnected", QString::fromUtf8("Отправитель отключился")},
        {"noReceiversJoined", QString::fromUtf8("Получатели не подключились")},
        {"sessionTerminated", QString::fromUtf8("Сессия завершена")},
        {"error", QString::fromUtf8("Ошибка")},
        {"sessionEnded", QString::fromUtf8("Сессия завершена: ")},
        {"saveFile", QString::fromUtf8("Сохранить файл")},
        {"newTransfer", QString::fromUtf8("Домой")},
        {"connecting", QString::fromUtf8("Подключение...")},
        {"users", QString::fromUtf8("Пользователи")},
        {"sessions", QString::fromUtf8("Сессии")},
        {"connectingToServer", QString::fromUtf8("Подключение к серверу...")},
        {"selectFileTitle", QString::fromUtf8("Выберите файл для отправки")},
        {"saveFileTitle", QString::fromUtf8("Сохранить полученный файл")},
        {"chunks", QString::fromUtf8("Чанки")},
        {"secondsShort", QString::fromUtf8("с")},
        {"noConnection", QString::fromUtf8("Нет соединения")},
        {"proxyLabel", QString::fromUtf8("Прокси")},
        {"proxyNone", QString::fromUtf8("Выкл")},
        {"proxyHost", QString::fromUtf8("Хост")},
        {"proxyPort", QString::fromUtf8("Порт")},
    };
    return m_language == "ru" ? ru : en;
}

void AppController::openDownloadTmpFile()
{
    QString tmpDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    m_downloadTmpPath = QDir(tmpDir).filePath(
        QStringLiteral("putinqa_%1.tmp").arg(QRandomGenerator::global()->generate64(), 0, 16));
    m_downloadTmpFile = new QFile(m_downloadTmpPath, this);
    if (!m_downloadTmpFile->open(QIODevice::WriteOnly)) {
        qWarning() << "Cannot create tmp file:" << m_downloadTmpPath;
        setError("Cannot create temporary file");
    }
}

void AppController::flushChunksToDisk(qint64 index, const QByteArray &data)
{
    if (!m_downloadTmpFile || !m_downloadTmpFile->isOpen()) return;

    // If this chunk is the next sequential one, write it and drain buffer
    if (index == m_nextWriteIndex) {
        m_downloadTmpFile->write(data);
        m_nextWriteIndex++;

        // Flush any buffered chunks that are now sequential
        while (m_chunkBuffer.contains(m_nextWriteIndex)) {
            m_downloadTmpFile->write(m_chunkBuffer.take(m_nextWriteIndex));
            m_nextWriteIndex++;
        }
    } else {
        // Out of order — buffer in memory
        m_chunkBuffer.insert(index, data);
    }
}

void AppController::cleanupDownloadTmpFile()
{
    if (m_downloadTmpFile) {
        m_downloadTmpFile->close();
        QFile::remove(m_downloadTmpPath);
        delete m_downloadTmpFile;
        m_downloadTmpFile = nullptr;
    }
    m_downloadTmpPath.clear();
    m_chunkBuffer.clear();
    m_writtenChunks.clear();
    m_nextWriteIndex = 1;
}

QUrl AppController::suggestedSavePath() const
{
    QString dir = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
    if (dir.isEmpty()) dir = QDir::homePath();
    return QUrl::fromLocalFile(QDir(dir).filePath(m_fileName.isEmpty() ? "download" : m_fileName));
}

void AppController::saveReceivedFile(const QUrl &path)
{
    QString filePath = path.toLocalFile();

    if (m_downloadTmpFile) {
        m_downloadTmpFile->close();
    }

    // Try rename (instant if same filesystem), fall back to copy
    if (QFile::rename(m_downloadTmpPath, filePath)) {
        qInfo() << "File moved to" << filePath;
    } else {
        // Cross-filesystem: copy + remove
        if (QFile::copy(m_downloadTmpPath, filePath)) {
            QFile::remove(m_downloadTmpPath);
            qInfo() << "File copied to" << filePath;
        } else {
            setError("Cannot save to: " + filePath);
            // Reopen tmp file in case user retries with different path
            if (m_downloadTmpFile)
                m_downloadTmpFile->open(QIODevice::Append);
            return;
        }
    }

    // Tmp file moved/copied — clear reference without deleting
    if (m_downloadTmpFile) {
        delete m_downloadTmpFile;
        m_downloadTmpFile = nullptr;
    }
    m_downloadTmpPath.clear();
}

// --- Auth callbacks ---

void AppController::onAuthorized()
{
    emit myClientIdChanged();
    proceedAfterAuth();
}

void AppController::onCaptchaRequired(const QString &imageBase64, int answerLength)
{
    m_captchaImage = imageBase64;
    m_captchaAnswerLength = answerLength;
    emit captchaImageChanged();
    emit captchaAnswerLengthChanged();
    setScreen("captcha");
}

void AppController::onAuthError(const QString &reason)
{
    setError(reason);
    setScreen("entry");
}

void AppController::proceedAfterAuth()
{
    if (m_pendingRole == "sender") {
        startSenderSession();
    } else {
        startReceiverSession();
    }
}

void AppController::startSenderSession()
{
    if (m_session) m_session->deleteLater();
    m_session = new Session(m_auth->getUrl(), m_auth->getCookieJar(), this);
    connectSessionSignals();

    m_encryptionKey = Crypto::generateKey();
    m_session->create();
}

void AppController::startReceiverSession()
{
    if (m_session) m_session->deleteLater();
    m_session = new Session(m_auth->getUrl(), m_auth->getCookieJar(), this);
    connectSessionSignals();

    m_session->join(m_pendingSessionId);
}

void AppController::connectSessionSignals()
{
    QObject::connect(m_session, &Session::complete, this, &AppController::onSessionComplete);
    QObject::connect(m_session, &Session::webSocketConnection, this, &AppController::onWsConnection);
    QObject::connect(m_session, &Session::chunkDataReceived, this, &AppController::onChunkDataReceived);
    QObject::connect(m_session, &Session::chunkDownloadFailed, this, &AppController::onChunkDownloadFailed);

    auto *state = m_session->state();
    QObject::connect(state, &SessionState::sessionInitialized, this, &AppController::onSessionInitialized);
    QObject::connect(state, &SessionState::newChunkEvent, this, &AppController::onNewChunkEvent);
    QObject::connect(state, &SessionState::chunkRemovedEvent, this, &AppController::onChunkRemovedEvent);
    QObject::connect(state, &SessionState::newChunkAllowedEvent, this, &AppController::onNewChunkAllowed);
    QObject::connect(state, &SessionState::freezeDroppedEvent, this, &AppController::onFreezeDropped);
    QObject::connect(state, &SessionState::uploadFinishedEvent, this, &AppController::onUploadFinishedEvent);
    QObject::connect(state, &SessionState::fileInfoEvent, this, &AppController::onFileInfoEvent);
    QObject::connect(state, &SessionState::bytesCountEvent, this, &AppController::onBytesCountEvent);
    QObject::connect(state, &SessionState::personalReceivedEvent, this, &AppController::onPersonalReceivedEvent);
    QObject::connect(state, &SessionState::newReceiverEvent, this, &AppController::onNewReceiverEvent);
    QObject::connect(state, &SessionState::receiverRemovedEvent, this, &AppController::onReceiverRemovedEvent);
    QObject::connect(state, &SessionState::onlineEvent, this, &AppController::onOnlineEvent);
    QObject::connect(state, &SessionState::nameChangedEvent, this, &AppController::onNameChangedEvent);
    QObject::connect(state, &SessionState::chunkDownloadFinishedEvent, this, &AppController::onChunkDownloadFinished);
}

// --- Session callbacks ---

void AppController::onSessionComplete(const QString &status)
{
    if (m_screen == "complete") return; // already handled

    m_completeStatus = status;
    emit completeStatusChanged();

    if (!m_isSender && status == "ok" && !m_writtenChunks.isEmpty()) {
        m_hasDownloadedFile = true;
        emit hasDownloadedFileChanged();
    }

    m_expirationTimer->stop();
    m_freezeTimer->stop();

    // Receiver: delay leave so sender sees the checkmark for a few seconds
    if (!m_isSender && m_session) {
        auto cookieJar = m_session->getCookieJar();
        QString serverUrl = m_activeServer;
        int delayMs = (status == "ok") ? 5000 : 0;
        QTimer::singleShot(delayMs, this, [cookieJar, serverUrl]() {
            QUrl url(serverUrl);
            url.setPath("/api/me/leave");
            auto *manager = new QNetworkAccessManager();
            manager->setTransferTimeout(10000);
            manager->setCookieJar(cookieJar.data());
            cookieJar->setParent(nullptr);
            auto *reply = manager->post(QNetworkRequest(url), QByteArray());
            QObject::connect(reply, &QNetworkReply::finished, manager, &QObject::deleteLater);
        });
    }

    // Disconnect from server — session is done from our side
    if (m_session) {
        m_session->deleteLater();
        m_session = nullptr;
    }

    setScreen("complete");
}

void AppController::onWsConnection(bool connected, bool serverClosed)
{

    if (!connected && m_screen != "complete" && m_screen != "entry") {
        // WS disconnected during active session.
        // Short delay to allow any pending "complete" event to arrive first.
        QTimer::singleShot(500, this, [this, serverClosed]() {
            if (m_screen == "complete" || m_screen == "entry") return;

            if (m_terminateRequested) {
                onSessionComplete("terminated_by_you");
            } else if (serverClosed && !m_isSender) {
                // Receiver: server closed WS without "complete" — kicked/removed.
                onSessionComplete("error");
            }
        });
    }
}

void AppController::onSessionInitialized()
{
    const auto &state = m_session->getState();

    m_bufferMax = state.getLimits().maxChunkQueue;
    emit bufferMaxChanged();

    constexpr int CRYPTO_OVERHEAD = 40;
    m_maxChunkPayload = state.getLimits().maxChunkSize - CRYPTO_OVERHEAD;

    m_frozen = state.getInitialFreeze()->value;
    emit frozenChanged();

    m_sessionExpirationIn = static_cast<int>(
        state.getExpireTimestamp()->value - QDateTime::currentSecsSinceEpoch());
    emit sessionExpirationInChanged();
    m_expirationTimer->start();

    m_freezeRemaining = static_cast<int>(state.getLimits().maxInitialFreeze);
    emit freezeRemainingChanged();
    if (m_frozen) m_freezeTimer->start();

    // Sender info
    m_senderName = state.getSender()->name;
    m_senderOnline = state.getSender()->isOnline;
    emit senderNameChanged();
    emit senderOnlineChanged();

    // File info from state (receiver may already have it)
    const auto *fi = state.getFileInfo();
    if (!fi->name.isEmpty()) {
        m_fileName = fi->name;
        m_fileSize = fi->size;
        emit fileNameChanged();
        emit fileSizeChanged();
    }

    // Upload finished from state (small files: sender finished before receiver joined)
    if (!m_isSender && state.getUploadFinished()->value) {
        m_uploadFinished = true;
        emit uploadFinishedChanged();
    }

    updateReceiversList();

    if (m_isSender) {
        buildShareLink();

        m_session->sendJsonMessage(
            Action::SetFileInfo(m_fileName, m_fileSize).json());

        m_uploadFile = new QFile(m_filePath, this);
        if (!m_uploadFile->open(QIODevice::ReadOnly)) {
            setError("Cannot open file");
            return;
        }

        setScreen("sender");
        uploadNextChunk();
    } else {
        setScreen("receiver");
        openDownloadTmpFile();

        const auto &chunks = state.getChunks()->value;
        for (auto it = chunks.begin(); it != chunks.end(); ++it) {
            if (it.value().index > m_highestKnownChunk)
                m_highestKnownChunk = it.value().index;
            m_downloadQueue.enqueue(it.value().index);
        }
        emit highestKnownChunkChanged();
        processDownloadQueue();
    }
}

// --- Upload logic ---

void AppController::uploadNextChunk()
{
    if (!m_uploadFile || !m_session || m_waitingForChunkAccepted) return;

    if (m_uploadFile->atEnd()) {
        m_session->sendJsonMessage(Action::UploadFinished().json());
        m_uploadFinished = true;
        emit uploadFinishedChanged();
        m_uploadFile->close();
        delete m_uploadFile;
        m_uploadFile = nullptr;
        return;
    }

    QByteArray raw = m_uploadFile->read(m_maxChunkPayload);
    QByteArray encrypted = Crypto::encrypt(raw, m_encryptionKey);
    m_session->sendBinaryMessage(encrypted);
    m_waitingForChunkAccepted = true;
}

void AppController::onNewChunkEvent(qint64 index, qint64 size)
{
    Q_UNUSED(size)

    if (m_isSender) {
        if (index > m_highestKnownChunk) {
            m_highestKnownChunk = index;
            emit highestKnownChunkChanged();
        }

        if (!m_session) return;

        m_bufferUsed = m_session->getState().getChunks()->value.size();
        emit bufferUsedChanged();

        if (m_waitingForChunkAccepted) {
            m_waitingForChunkAccepted = false;

            if (m_bufferUsed >= m_bufferMax) {
                m_canSendChunk = false;
            } else {
                QTimer::singleShot(0, this, &AppController::uploadNextChunk);
            }
        }
    } else {
        if (index > m_highestKnownChunk) {
            m_highestKnownChunk = index;
            emit highestKnownChunkChanged();
        }
        m_downloadQueue.enqueue(index);
        processDownloadQueue();
    }
}

void AppController::onChunkRemovedEvent()
{
    if (m_isSender && m_session) {
        m_bufferUsed = m_session->getState().getChunks()->value.size();
        emit bufferUsedChanged();
    }
}

void AppController::onNewChunkAllowed(bool status)
{
    if (m_isSender && status && !m_canSendChunk && m_session) {
        m_canSendChunk = true;
        QTimer::singleShot(0, this, &AppController::uploadNextChunk);
    }
}

// --- Download logic ---

void AppController::processDownloadQueue()
{
    while (m_activeDownloads < MAX_PARALLEL_DOWNLOADS && !m_downloadQueue.isEmpty() && m_session) {
        qint64 index = m_downloadQueue.dequeue();
        if (m_writtenChunks.contains(index)) continue;

        m_activeDownloads++;
        m_session->downloadChunkHttp(index);
    }
}

void AppController::onChunkDataReceived(qint64 index, const QByteArray &data)
{
    QByteArray decrypted = Crypto::decrypt(data, m_encryptionKey);
    if (decrypted.isEmpty()) {
        qWarning() << "Failed to decrypt chunk" << index;
        m_activeDownloads--;
        processDownloadQueue();
        return;
    }

    m_writtenChunks.insert(index);
    flushChunksToDisk(index, decrypted);

    m_session->sendJsonMessage(Action::ConfirmChunk(index).json());
    m_chunksConfirmed++;
    m_pendingConfirms++;
    emit chunksConfirmedChanged();

    m_activeDownloads--;
    processDownloadQueue();
}

void AppController::onChunkDownloadFailed(qint64 index, const QString &error)
{
    qWarning() << "Chunk" << index << "download failed:" << error;
    m_activeDownloads--;
    // Re-enqueue for retry (unless 404 = removed)
    if (!error.contains("404")) {
        m_downloadQueue.enqueue(index);
    }
    processDownloadQueue();
}

void AppController::onChunkDownloadFinished(const QString &receiverId, qint64 index)
{
    Q_UNUSED(index)
    // Server acknowledged our confirm_chunk
    if (m_auth && receiverId == m_auth->getId() && m_pendingConfirms > 0) {
        m_pendingConfirms--;
        checkReceiverDone();
    }

    // Track per-receiver download progress (for sender's member list)
    if (m_isSender) {
        m_receiverChunksDone[receiverId]++;
        updateReceiversList();
    }
}

void AppController::checkReceiverDone()
{
    if (m_uploadFinished && !m_writtenChunks.isEmpty() &&
        m_chunksConfirmed > 0 && m_chunksConfirmed >= m_highestKnownChunk &&
        m_pendingConfirms == 0) {
        m_hasDownloadedFile = true;
        emit hasDownloadedFileChanged();
        // All chunks downloaded, decrypted, confirmed, and acknowledged.
        // Receiver's job is done — complete immediately.
        onSessionComplete("ok");
    }
}

// --- State event handlers ---

void AppController::onFreezeDropped()
{
    m_frozen = false;
    emit frozenChanged();
    m_freezeTimer->stop();

    // Sender: if upload already finished, freeze was the only thing holding us.
    if (m_isSender && m_uploadFinished) {
        onSessionComplete("ok");
    }
}

void AppController::onUploadFinishedEvent()
{
    if (m_isSender) {
        // Server confirmed all data received.
        // If freeze already dropped — complete immediately.
        // If freeze still active — wait, complete when freeze drops.
        if (!m_frozen) {
            onSessionComplete("ok");
        }
        // else: onFreezeDropped() will complete the session
    } else {
        m_uploadFinished = true;
        emit uploadFinishedChanged();
        checkReceiverDone();
    }
}

void AppController::onFileInfoEvent(const QString &name, qint64 size)
{
    m_fileName = name;
    m_fileSize = size;
    emit fileNameChanged();
    emit fileSizeChanged();
}

void AppController::onBytesCountEvent(const QString &direction, qint64 value)
{
    if (m_isSender && direction == "from_sender") {
        m_bytesTransferred = qMin(value, m_fileSize);
        emit bytesTransferredChanged();
        if (m_fileSize > 0) {
            m_progress = qMin(1.0, static_cast<double>(m_bytesTransferred) / m_fileSize);
            emit progressChanged();
        }
    }
}

void AppController::onPersonalReceivedEvent(qint64 bytes)
{
    if (!m_isSender) {
        m_bytesTransferred = qMin(bytes, m_fileSize);
        emit bytesTransferredChanged();
        if (m_fileSize > 0) {
            m_progress = qMin(1.0, static_cast<double>(m_bytesTransferred) / m_fileSize);
            emit progressChanged();
        }
    }
}

void AppController::onNewReceiverEvent(const QString &id, const QString &name)
{
    Q_UNUSED(id) Q_UNUSED(name)
    updateReceiversList();
}

void AppController::onReceiverRemovedEvent(const QString &id)
{
    Q_UNUSED(id)
    updateReceiversList();
}

void AppController::onOnlineEvent(const QString &id, bool online)
{
    // Update sender
    if (m_session && m_session->getState().getSender()->id == id) {
        m_senderOnline = online;
        emit senderOnlineChanged();
        return;
    }
    // Update in receivers list
    for (int i = 0; i < m_receivers.size(); ++i) {
        auto map = m_receivers[i].toMap();
        if (map["id"].toString() == id) {
            map["isOnline"] = online;
            // Update done status when going offline
            if (!online && m_isSender && m_uploadFinished && m_highestKnownChunk > 0 &&
                m_receiverChunksDone.value(id, 0) >= m_highestKnownChunk) {
                map["done"] = true;
            }
            m_receivers[i] = map;
            emit receiversChanged();
            return;
        }
    }
}

void AppController::onNameChangedEvent(const QString &id, const QString &name)
{
    if (m_session && m_session->getState().getSender()->id == id) {
        m_senderName = name;
        emit senderNameChanged();
    }
    updateReceiversList();
}

void AppController::updateReceiversList()
{
    if (!m_session) return;

    m_receivers.clear();
    const auto &map = m_session->getState().getReceivers()->value;
    for (auto it = map.begin(); it != map.end(); ++it) {
        QVariantMap r;
        r["id"] = it.value()->id;
        r["name"] = it.value()->name;
        r["isOnline"] = it.value()->isOnline;
        r["currentChunk"] = it.value()->currentChunk.index;
        bool done = m_uploadFinished && m_highestKnownChunk > 0 &&
                    m_receiverChunksDone.value(it.value()->id, 0) >= m_highestKnownChunk;
        r["done"] = done;
        m_receivers.append(r);
    }
    emit receiversChanged();
}

void AppController::buildShareLink()
{
    if (!m_session || m_session->getId().isEmpty() || m_encryptionKey.isEmpty()) return;

    m_shareLink = QStringLiteral("%1/#id=%2&encryption=xchacha20-poly1305&key=%3")
                      .arg(m_serverUrl, m_session->getId(),
                           Crypto::keyToBase64Url(m_encryptionKey));
    emit shareLinkChanged();
}

void AppController::onServerWorkloadUpdated(const ServerWorkloadInfo &info)
{
    m_stats["connected"] = true;
    m_stats["currentUserCount"] = info.currentUserCount;
    m_stats["currentSessionCount"] = info.currentSessionCount;
    m_stats["maxUserCount"] = info.maxUserCount;
    m_stats["maxSessionCount"] = info.maxSessionCount;
    emit statsChanged();
}

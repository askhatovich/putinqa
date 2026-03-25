// Copyright (C) 2026  Roman Lyubimov
// SPDX-License-Identifier: GPL-3.0-or-later
// For full license text, see <https://www.gnu.org/licenses/gpl-3.0.txt>

#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickWindow>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QIcon>
#include <QLocalServer>
#include <QLocalSocket>

#include "appcontroller.h"

static const QString SERVER_NAME = QStringLiteral("putinqa");

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setOrganizationName("askhatovich");
    app.setApplicationName("putinqa");
    app.setDesktopFileName("putinqa");
    app.setQuitOnLastWindowClosed(false);
    app.setWindowIcon(QIcon(":/icons/appicon.png"));

    // Single instance check
    {
        QLocalSocket socket;
        socket.connectToServer(SERVER_NAME);
        if (socket.waitForConnected(500)) {
            // Another instance is running — ask it to show, then quit
            socket.write("show");
            socket.waitForBytesWritten(500);
            socket.disconnectFromServer();
            return 0;
        }
    }

    // Remove stale socket (e.g. after crash on Linux)
    QLocalServer::removeServer(SERVER_NAME);

    QLocalServer localServer;
    localServer.listen(SERVER_NAME);

    AppController controller;

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("appController", &controller);
    engine.load(QUrl("qrc:/qml/main.qml"));

    if (engine.rootObjects().isEmpty())
        return -1;

    auto *window = qobject_cast<QQuickWindow *>(engine.rootObjects().first());

    // When another instance connects, show this window
    QObject::connect(&localServer, &QLocalServer::newConnection, window, [&localServer, window]() {
        auto *client = localServer.nextPendingConnection();
        if (client) {
            client->waitForReadyRead(500);
            client->deleteLater();
        }
        window->show();
        window->raise();
        window->requestActivate();
    });

    // System tray
    QSystemTrayIcon trayIcon;
    trayIcon.setIcon(app.windowIcon());
    trayIcon.setToolTip("PutinQA - File Transfer");

    QMenu trayMenu;
    QAction *showAction = trayMenu.addAction("Show");
    QAction *quitAction = trayMenu.addAction("Quit");
    trayIcon.setContextMenu(&trayMenu);

    QObject::connect(showAction, &QAction::triggered, window, [window]() {
        window->show();
        window->raise();
        window->requestActivate();
    });

    QObject::connect(quitAction, &QAction::triggered, &app, &QApplication::quit);

    QObject::connect(&trayIcon, &QSystemTrayIcon::activated,
                     window, [window](QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::Trigger ||
            reason == QSystemTrayIcon::DoubleClick) {
            window->show();
            window->raise();
            window->requestActivate();
        }
    });

    QObject::connect(&controller, &AppController::trayRequested, window, [window, &trayIcon]() {
        window->hide();
        trayIcon.show();
    });

    QObject::connect(&controller, &AppController::showWindowRequested, window, [window]() {
        window->show();
        window->raise();
        window->requestActivate();
    });

    trayIcon.show();

    return app.exec();
}

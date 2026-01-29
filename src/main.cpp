#include "mainwindow.h"

#include <QApplication>
#include <QThread>

#include "client/client.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    Client sender("sender");

    QObject::connect(&sender, &Client::authorized, [&sender](){
        sender.createSession("test.txt");
    });

    // QObject::connect(&sender, &Client::complete, [](bool error, const QString &text){
    //     qDebug() << "S Complete" << error << text;
    // });

    Client receiver1("receiver1");
    QString sessionId;

    QObject::connect(&sender, &Client::joinedToSession, [&sender, &sessionId, &receiver1](){
        QThread::sleep(1);
        sessionId = sender.getSessionId();
        receiver1.authorize();
    });

    QObject::connect(&receiver1, &Client::authorized, [&receiver1, &sessionId](){
        receiver1.joinToSession(sessionId);
    });

    Client receiver2("receiver2");
    QObject::connect(&receiver1, &Client::joinedToSession, [&receiver2](){
        receiver2.authorize();
    });

    QObject::connect(&receiver2, &Client::authorized, [&receiver2, &sessionId](){
        receiver2.joinToSession(sessionId);
    });

    QObject::connect(&receiver2, &Client::webSocketConnection, [&receiver1](bool connected){
        if (connected) {
            receiver1.forceQuit();
        }
    });

    // QObject::connect(&receiver, &Client::complete, [](bool error, const QString &text){
    //     qDebug() << "R Complete" << error << text;
    // });

    sender.authorize();

    return a.exec();
}

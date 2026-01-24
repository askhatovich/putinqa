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

    QObject::connect(&sender, &Client::complete, [](bool error, const QString &text){
        qDebug() << "S Complete" << error << text;
    });

    Client receiver("receiver");
    QString sessionId;

    QObject::connect(&sender, &Client::joinedToSession, [&sender, &sessionId, &receiver](){
        QThread::sleep(1);
        sessionId = sender.getSessionId();
        receiver.authorize();
    });

    QObject::connect(&receiver, &Client::authorized, [&receiver, &sessionId](){
        receiver.joinToSession(sessionId);
    });

    QObject::connect(&receiver, &Client::complete, [](bool error, const QString &text){
        qDebug() << "R Complete" << error << text;
    });

    sender.authorize();

    return a.exec();
}

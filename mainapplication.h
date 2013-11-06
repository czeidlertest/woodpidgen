#ifndef MAINAPPLICATION_H
#define MAINAPPLICATION_H

#include <QApplication>
#include <QtNetwork/QNetworkAccessManager>

#include <cryptointerface.h>
#include <mainwindow.h>
#include <profile.h>


class DatabaseInterface;

#include <serverconnection.h>
#include <QDebug>

class PingRCCommand : public QObject
{
Q_OBJECT
public:
    PingRCCommand(RemoteConnection *connection, QObject *parent = NULL) :
        QObject(parent),
        fNetworkConnection(connection)
    {
    }

public slots:
    void connectionAttemptFinished(QNetworkReply::NetworkError code)
    {
        if (code != QNetworkReply::NoError)
            return;

        QByteArray data("ping");
        fReply = fNetworkConnection->send(data);
        connect(fReply, SIGNAL(finished()), this, SLOT(received()));
    }

    virtual void received()
    {
        QByteArray data = fReply->readAll();
        qDebug() << data << endl;
    }

private:
    RemoteConnection* fNetworkConnection;
    RemoteConnectionReply *fReply;
};


class MainApplication : public QApplication
{
Q_OBJECT
public:
    MainApplication(int &argc, char *argv[]);
    ~MainApplication();

    QNetworkAccessManager* getNetworkAccessManager();

private:
    WP::err createNewProfile();

    MainWindow *fMainWindow;

    Profile* fProfile;
    QNetworkAccessManager *fNetworkAccessManager;
};

#endif // MAINAPPLICATION_H

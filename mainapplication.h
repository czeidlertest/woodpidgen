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
class PingRCReply : public RemoteConnectionReply
{
Q_OBJECT
public:
    PingRCReply(NetworkConnection *connection, QObject *parent = NULL) :
        RemoteConnectionReply(parent),
        fNetworkConnection(connection)
    {
    }

public slots:
    void connectionAttemptFinished(QNetworkReply::NetworkError code)
    {
        if (code != QNetworkReply::NoError)
            return;

        QByteArray data("ping");
        fNetworkConnection->send(this, data);
    }

    virtual void received(const QByteArray &data)
    {
        qDebug() << data << endl;
    }

private:
    NetworkConnection* fNetworkConnection;
};


class MainApplication : public QApplication
{
Q_OBJECT
public:
    MainApplication(int &argc, char *argv[]);
    ~MainApplication();

    QNetworkAccessManager* getNetworkAccessManager();

public slots:
    void connectionAttemptFinished(QNetworkReply::NetworkError);

private:
    WP::err createNewProfile();

    MainWindow *fMainWindow;

    Profile* fProfile;
    QNetworkAccessManager *fNetworkAccessManager;
};

#endif // MAINAPPLICATION_H

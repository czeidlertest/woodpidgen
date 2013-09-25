#ifndef MAINAPPLICATION_H
#define MAINAPPLICATION_H

#include <QApplication>
#include <QtNetwork/QNetworkAccessManager>

#include <cryptointerface.h>
#include <mainwindow.h>
#include <profile.h>


class DatabaseInterface;


class MainApplication : public QApplication
{
public:
    MainApplication(int &argc, char *argv[]);
    ~MainApplication();

    QNetworkAccessManager* getNetworkAccessManager();

private:
    MainWindow *fMainWindow;

    DatabaseInterface *fDatabase;
    CryptoInterface *fCrypto;
    Profile* fProfile;

    QNetworkAccessManager *fNetworkAccessManager;
};

#endif // MAINAPPLICATION_H

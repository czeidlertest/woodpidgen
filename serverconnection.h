#ifndef SERVERCONNECTION_H
#define SERVERCONNECTION_H

#include <QBuffer>
#include <QByteArray>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>

#include <cryptointerface.h>


class RemoteConnectionReply : public QObject
{
Q_OBJECT
public:
    RemoteConnectionReply(QIODevice *device, QObject *parent = NULL);
    virtual ~RemoteConnectionReply() {}

    QIODevice *device();

    QByteArray readAll();

signals:
    void finished();
    void error(WP::err error);

protected:
    QIODevice *fDevice;
};


class RemoteConnection : public QObject
{
Q_OBJECT
public:
    RemoteConnection(QObject *parent = NULL);
    virtual ~RemoteConnection();

    virtual WP::err connectToServer() = 0;
    virtual WP::err disconnect() = 0;
    virtual RemoteConnectionReply *send(const QByteArray& data) = 0;

    bool isConnected();
    bool isConnecting();

protected:
    void setConnectionStarted();
    void setConnected();
    void setDisconnected();

    bool fConnected;
    bool fConnecting;
};


class HTTPConnectionReply : public RemoteConnectionReply {
Q_OBJECT
public:
    HTTPConnectionReply(QIODevice *device, QNetworkReply *reply, QObject *parent = NULL);

private slots:
    void finishedSlot();
    void errorSlot(QNetworkReply::NetworkError code);
};


class HTTPConnection : public RemoteConnection
{
Q_OBJECT
public:
    HTTPConnection(const QUrl &url, QObject *parent = NULL);
    virtual ~HTTPConnection();

    QUrl getUrl();

    static QNetworkAccessManager *getNetworkAccessManager();

    WP::err connectToServer();
    WP::err disconnect();
    virtual RemoteConnectionReply *send(const QByteArray& data);

signals:
    void connectionAttemptFinished(QNetworkReply::NetworkError code);

protected slots:
    void replyFinished(QNetworkReply *reply);

protected:
    QUrl fUrl;
    QMap<QNetworkReply*, RemoteConnectionReply*> fReplyMap;
};


class PHPEncryptionFilter;

class PHPEncryptedDevice : public QBuffer {
Q_OBJECT
public:
    PHPEncryptedDevice(PHPEncryptionFilter *encryption, QNetworkReply *source);

protected:
    qint64 readData(char *data, qint64 maxSize);

private:
    PHPEncryptionFilter *fEncryption;
    QNetworkReply *fSource;
    bool fHasBeenEncrypted;
};


class EncryptedPHPConnectionReply : public HTTPConnectionReply {
public:
    EncryptedPHPConnectionReply(PHPEncryptionFilter *encryption, QNetworkReply *reply,
                                QObject *parent = NULL);
    virtual ~EncryptedPHPConnectionReply();
};

class EncryptedPHPConnection : public HTTPConnection
{
Q_OBJECT
public:
    EncryptedPHPConnection(QUrl url, QObject *parent = NULL);

    WP::err connectToServer();
    WP::err disconnect();

    virtual RemoteConnectionReply *send(const QByteArray& data);

private slots:
    void handleConnectionAttemptReply();
    void networkRequestError(QNetworkReply::NetworkError code);

private:
    CryptoInterface *fCrypto;
    QNetworkReply *fNetworkReply;

    QString fSecretNumber;
    QByteArray fInitVector;
    PHPEncryptionFilter *fEncryption;
};


#endif // SERVERCONNECTION_H

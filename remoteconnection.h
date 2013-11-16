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
    void finished(WP::err error);

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

signals:
    void connectionAttemptFinished(WP::err);

protected:
    void setConnectionStarted();
    void setConnected();
    void setDisconnected();

    bool fConnected;
    bool fConnecting;
};


class Profile;

class RemoteAuthentication : public QObject
{
Q_OBJECT
public:
    RemoteAuthentication(RemoteConnection *connection, const QString &userName,
                         const QString &keyStoreId, const QString &keyId);
    virtual ~RemoteAuthentication() {}

    RemoteConnection *getConnection();

    //! Establish a connection and login afterwards.
    WP::err login(Profile *profile);
    void logout();
    bool verified();

protected slots:
    void handleConnectionAttempt(WP::err code);
    void handleAuthenticationRequest(WP::err code);
    void handleAuthenticationAttempt(WP::err code);

signals:
    void authenticationAttemptFinished(WP::err code);

protected:
    virtual void getLoginRequestData(QByteArray &data) = 0;
    virtual void getLoginData(QByteArray &data, const QByteArray &serverRequest) = 0;
    virtual void getLogoutData(QByteArray &data) = 0;

    RemoteConnection *fConnection;
    QString fUserName;
    QString fKeyStoreId;
    QString fKeyId;
    Profile *fProfile;

    RemoteConnectionReply *fAuthenticationReply;

    bool fAuthenticationInProgress;
    bool fVerified;
};


class SignatureAuthentication : public RemoteAuthentication {
public:
    SignatureAuthentication(RemoteConnection *connection, const QString &userName,
                            const QString &keyStoreId, const QString &keyId);

protected:
    virtual void getLoginRequestData(QByteArray &data);
    virtual void getLoginData(QByteArray &data, const QByteArray &serverRequest);
    virtual void getLogoutData(QByteArray &data);
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

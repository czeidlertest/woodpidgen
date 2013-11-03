#ifndef SERVERCONNECTION_H
#define SERVERCONNECTION_H

#include <QByteArray>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>

#include <cryptointerface.h>


class RemoteConnectionReply : public QObject
{
Q_OBJECT
public:
    RemoteConnectionReply(QObject *parent = NULL);
    virtual ~RemoteConnectionReply() {}

public slots:
    virtual void received(const QByteArray &data) = 0;
};


/*! filter incomming or outgoing data, e.g., to decrypt encrypt it. */
class RemoteConnectionFilter {
public:
    virtual ~RemoteConnectionFilter() {}

    //! called before send data
    virtual void sendFilter(const QByteArray &in, QByteArray &out) = 0;
    //! called when receive data
    virtual void receiveFilter(const QByteArray &in, QByteArray &out) = 0;
};


class RemoteConnection : public QObject
{
Q_OBJECT
public:
    RemoteConnection(QObject *parent = NULL);
    virtual ~RemoteConnection();

    virtual WP::err connectToServer() = 0;
    virtual WP::err disconnect() = 0;
    virtual WP::err send(RemoteConnectionReply* request, const QByteArray& data) = 0;

    bool isConnected();
    bool isConnecting();

    //! filter can be NULL
    void setFilter(RemoteConnectionFilter *filter);

signals:
    void connectionAttemptFinished(QNetworkReply::NetworkError code);

protected:
    void setConnectionStarted();
    void setConnected();
    void setDisconnected();

    bool fConnected;
    bool fConnecting;
    RemoteConnectionFilter *fFilter;
};


class NetworkConnection : public RemoteConnection
{
Q_OBJECT
public:
    NetworkConnection(const QUrl &url, QObject *parent = NULL);
    virtual ~NetworkConnection();

    QUrl getUrl();

    static QNetworkAccessManager* getNetworkAccessManager();

    virtual WP::err send(RemoteConnectionReply* remoteConnectionReply, const QByteArray& data);

private slots:
    virtual void replyFinished(QNetworkReply *reply);
    virtual void replyError(QNetworkReply::NetworkError code);
protected:
    QUrl fUrl;

private:
    QMap<QNetworkReply*, RemoteConnectionReply*> fReplyMap;
};


class EncryptedPHPConnection : public NetworkConnection
{
Q_OBJECT
public:
    EncryptedPHPConnection(QUrl url, QObject *parent = NULL);

    WP::err connectToServer();
    WP::err disconnect();

private slots:
    void replyFinished();
    void networkRequestError(QNetworkReply::NetworkError code);

private:
    class PHPEncryptionFilter : public RemoteConnectionFilter {
    public:
        PHPEncryptionFilter(CryptoInterface *crypto, const SecureArray &cipherKey,
                            const QByteArray &iv);
        //! called before send data
        virtual void sendFilter(const QByteArray &in, QByteArray &out);
        //! called when receive data
        virtual void receiveFilter(const QByteArray &in, QByteArray &out);
    private:
        CryptoInterface *fCrypto;
        SecureArray fCipherKey;
        QByteArray fIV;
    };

private:
    CryptoInterface *fCrypto;
    QNetworkReply *fNetworkReply;

    QString fSecretNumber;
    QByteArray fInitVector;
};


#endif // SERVERCONNECTION_H

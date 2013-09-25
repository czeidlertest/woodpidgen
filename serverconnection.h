#ifndef SERVERCONNECTION_H
#define SERVERCONNECTION_H


#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>


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
    virtual sendFilter(const QByteArray &in, QByteArray &out) = 0;
    //! called when receive data
    virtual receiveFilter(const QByteArray &in, QByteArray &out) = 0;
};


class RemoteConnection
{
public:
    RemoteConnection();
    virtual ~RemoteConnection() {}

    virtual int send(RemoteConnectionReply* request, const QByteArray& data) = 0;

    //! filter can be NULL
    void setFilter(RemoteConnectionFilter *filter);
protected:
    RemoteConnectionFilter *fFilter;
};


class NetworkConnection : public RemoteConnection
{
public:
    NetworkConnection(const QUrl &url);
    virtual ~NetworkConnection();

    static QNetworkAccessManager* getNetworkAccessManager();

    virtual int send(RemoteConnectionReply* request, const QByteArray& data);

private slots:
    virtual void replyFinished(QNetworkReply *reply);

private:
    QUrl fUrl;
    QMap<QNetworkReply*, ServerConnectionRequest*> fReplyMap;
};




#endif // SERVERCONNECTION_H

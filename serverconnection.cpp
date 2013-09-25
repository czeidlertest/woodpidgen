#include "serverconnection.h"

#include <QCoreApplication>

#include <mainapplication.h>


RemoteConnection::RemoteConnection() :
    fFilter(NULL)
{
}


NetworkConnection::NetworkConnection(const QUrl &url) :
    fUrl(url)
{
}

NetworkConnection::~NetworkConnection()
{
}

QNetworkAccessManager* NetworkConnection::getNetworkAccessManager()
{
    return ((MainApplication*)qApp)->getNetworkAccessManager();
}

int NetworkConnection::send(RemoteConnectionReply *request, const QByteArray &data)
{
    QNetworkRequest request;
    request.setUrl(fUrl);
    request.setRawHeader("User-Agent", "MyOwnBrowser 1.0");

    QNetworkAccessManager *manager = getNetworkAccessManager();
    QNetworkReply *reply = NULL;
    if (fFilter != NULL) {
        QByteArray outgoing;
        fFilter->sendFilter(data, outgoing);
        reply = manager->post(request, outgoing);
    } else {
        reply = manager->post(request, data);
    }
    if (reply == NULL)
        return -1;

    connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(replyFinished(QNetworkReply*)));

    fReplyMap.insert(reply, request);
    return 0;
}

void NetworkConnection::replyFinished(QNetworkReply *reply)
{
    QMap::iterator it = fReplyMap.find(reply);
    RemoteConnectionReply *receiver = it.value();
    receiver->received(reply->readAll());

    fReplyMap.remove(reply);
}


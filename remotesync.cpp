#include "remotesync.h"

#include "protocolparser.h"


class SyncOutStanza : public OutStanza {
public:
    SyncOutStanza(const QString &branch, const QString &tipCommit) :
        OutStanza("sync")
    {
        addAttribute("branch", branch);
        addAttribute("tip", tipCommit);
    }
};


RemoteSync::RemoteSync(DatabaseInterface *database, RemoteConnection *connection, QObject *parent) :
    QObject(parent),
    fDatabase(database),
    fRemoteConnection(connection)
{
}

RemoteSync::~RemoteSync()
{
}

WP::err RemoteSync::sync()
{
    if (fRemoteConnection->isConnected())
        syncConnected(QNetworkReply::NoError);
    else {
        fRemoteConnection->connectToServer();
        connect(fRemoteConnection, SIGNAL(connectionAttemptFinished(QNetworkReply::NetworkError)),
                this, SLOT(syncConnected(QNetworkReply::NetworkError)));
    }
    return WP::kOk;
}

void RemoteSync::syncConnected(QNetworkReply::NetworkError code)
{
    QString branch = fDatabase->branch();
    QString tipCommit = fDatabase->getTip();

    QByteArray outData;
    ProtocolOutStream outStream(&outData);

    IqOutStanza *iqStanza = new IqOutStanza(kGet);
    outStream.pushStanza(iqStanza);

    SyncOutStanza *syncStanza = new SyncOutStanza(branch, tipCommit);
    outStream.pushChildStanza(syncStanza);

    outStream.flush();

    // TODO!!! connect
    //fRemoteConnection->send(outData);
}

void RemoteSync::syncReply(QNetworkReply::NetworkError code)
{

}

void RemoteSync::syncResponse(const QByteArray &data)
{

}

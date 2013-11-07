#include "remotesync.h"

#include "protocolparser.h"


class SyncOutStanza : public OutStanza {
public:
    SyncOutStanza(const QString &branch, const QString &tipCommit) :
        OutStanza("sync_pull")
    {
        addAttribute("branch", branch);
        addAttribute("tip", tipCommit);
    }
};


RemoteSync::RemoteSync(DatabaseInterface *database, RemoteConnection *connection, QObject *parent) :
    QObject(parent),
    fDatabase(database),
    fRemoteConnection(connection),
    fServerReply(NULL)
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
        connect(fRemoteConnection, SIGNAL(connectionAttemptFinished(QNetworkReply::NetworkError)),
                this, SLOT(syncConnected(QNetworkReply::NetworkError)));
        fRemoteConnection->connectToServer();
    }
    return WP::kOk;
}

void RemoteSync::syncConnected(QNetworkReply::NetworkError code)
{
    if (code != QNetworkReply::NoError)
        return;

    QString branch = fDatabase->branch();
    QString tipCommit = fDatabase->getTip();

    QByteArray outData;
    ProtocolOutStream outStream(&outData);

    IqOutStanza *iqStanza = new IqOutStanza(kGet);
    outStream.pushStanza(iqStanza);

    SyncOutStanza *syncStanza = new SyncOutStanza(branch, tipCommit);
    outStream.pushChildStanza(syncStanza);

    outStream.flush();

    if (fServerReply != NULL)
        fServerReply->disconnect();
    fServerReply = fRemoteConnection->send(outData);
    connect(fServerReply, SIGNAL(finished()), this, SLOT(syncReply()));
}

void RemoteSync::syncReply()
{
    qDebug(fServerReply->readAll());
    fServerReply = NULL;
}

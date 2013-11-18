#include "remotesync.h"

#include "protocolparser.h"
#include "remoteauthentication.h"


class SyncOutStanza : public OutStanza {
public:
    SyncOutStanza(const QString &branch, const QString &tipCommit) :
        OutStanza("sync_pull")
    {
        addAttribute("branch", branch);
        addAttribute("tip", tipCommit);
    }
};


RemoteSync::RemoteSync(DatabaseInterface *database, RemoteAuthentication *remoteAuth,
                       Profile *profile, QObject *parent) :
    QObject(parent),
    fDatabase(database),
    fAuthentication(remoteAuth),
    fRemoteConnection(remoteAuth->getConnection()),
    fServerReply(NULL),
    fProfile(profile)
{
}

RemoteSync::~RemoteSync()
{
}

WP::err RemoteSync::sync()
{
    if (fAuthentication->verified())
        syncConnected(WP::kOk);
    else {
        connect(fAuthentication, SIGNAL(authenticationAttemptFinished(WP::err)),
                this, SLOT(syncConnected(WP::err)));
        fAuthentication->login(fProfile);
    }
    return WP::kOk;
}

void RemoteSync::syncConnected(WP::err code)
{
    if (code != WP::kOk)
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
    connect(fServerReply, SIGNAL(finished(WP::err)), this, SLOT(syncReply(WP::err)));
}

class SyncPullHandler : public InStanzaHandler {
public:
    SyncPullHandler() :
        InStanzaHandler("sync_pull")
    {
    }

    void handleStanza(const QXmlStreamAttributes &attributes)
    {
        if (attributes.hasAttribute("branch"))
            branch = attributes.value("branch").toString();
        if (attributes.hasAttribute("tip"))
            tip = attributes.value("tip").toString();
    }

public:
    QString branch;
    QString tip;
};


class SyncPullPackHandler : public InStanzaHandler {
public:
    SyncPullPackHandler() :
        InStanzaHandler("pack", true)
    {
    }

    void handleText(const QStringRef &text)
    {
        pack = QByteArray::fromBase64(text.toLatin1());
    }

public:
    QByteArray pack;
};


void RemoteSync::syncReply(WP::err code)
{
    if (code != WP::kOk)
        return;

    QByteArray data = fServerReply->readAll();
    fServerReply = NULL;
 qDebug(data);
    IqInStanzaHandler iqHandler;
    SyncPullHandler *syncPullHandler = new SyncPullHandler();
    SyncPullPackHandler *syncPullPackHandler = new SyncPullPackHandler();
    iqHandler.addChildHandler(syncPullHandler);
    syncPullHandler->addChildHandler(syncPullPackHandler);

    ProtocolInStream inStream(data);
    inStream.addHandler(&iqHandler);

    inStream.parse();

    QString localBranch = fDatabase->branch();
    QString localTipCommit = fDatabase->getTip();

    if (iqHandler.type() != kResult || syncPullHandler->branch != localBranch) {
        // error occured, the server should at least send back the branch name
        // TODO better error message
        emit syncFinished(WP::kEntryNotFound);
        return;
    }
    if (syncPullHandler->tip == localTipCommit) {
        // done
        emit syncFinished(WP::kOk);
        return;
    }
    // see if the server is ahead by checking if we got packages
    if (syncPullPackHandler->pack.size() != 0) {
        WP::err error = fDatabase->importPack(syncPullPackHandler->pack, localTipCommit,
                                              syncPullHandler->tip);
        emit syncFinished(error);
        return;
    }
    // we are ahead of the server: push changes to the server
    QByteArray pack;
    WP::err error = fDatabase->exportPack(pack, syncPullHandler->tip, localTipCommit);
    if (error != WP::kOk) {
        emit syncFinished(error);
        return;
    }

    QByteArray outData;
    ProtocolOutStream outStream(&outData);

    IqOutStanza *iqStanza = new IqOutStanza(kSet);
    outStream.pushStanza(iqStanza);
    OutStanza *pushStanza = new OutStanza("sync_push");
    pushStanza->addAttribute("branch", localBranch);
    pushStanza->addAttribute("start_commit", syncPullHandler->tip);
    pushStanza->addAttribute("last_commit", localTipCommit);
    outStream.pushChildStanza(pushStanza);
    OutStanza *pushPackStanza = new OutStanza("pack");
    pushPackStanza->setText(pack.toBase64());
    outStream.pushChildStanza(pushPackStanza);

    outStream.flush();

    fServerReply = fRemoteConnection->send(outData);
    connect(fServerReply, SIGNAL(finished(WP::err)), this, SLOT(syncPushReply(WP::err)));
}

void RemoteSync::syncPushReply(WP::err code)
{
    if (code != WP::kOk)
        return;

    QByteArray data = fServerReply->readAll();
    qDebug(data);
    emit syncFinished(WP::kOk);
}
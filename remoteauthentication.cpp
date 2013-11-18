#include "remoteauthentication.h"

#include "protocolparser.h"


RemoteAuthentication::RemoteAuthentication(RemoteConnection *connection, const QString &userName, const QString &keyStoreId,
                                           const QString &keyId) :
    fConnection(connection),
    fUserName(userName),
    fKeyStoreId(keyStoreId),
    fKeyId(keyId),
    fProfile(NULL),
    fAuthenticationReply(NULL),
    fAuthenticationInProgress(false),
    fVerified(false)
{
}

RemoteConnection *RemoteAuthentication::getConnection()
{
    return fConnection;
}

WP::err RemoteAuthentication::login(Profile *profile)
{
    fProfile = profile;
    if (fAuthenticationInProgress)
        return WP::kOk;
    if (fVerified) {
        emit authenticationAttemptFinished(WP::kOk);
    }

    fAuthenticationInProgress = true;
    if (fConnection->isConnected())
        handleConnectionAttempt(WP::kOk);
    else {
        connect(fConnection, SIGNAL(connectionAttemptFinished(WP::err)), this, SLOT(handleConnectionAttempt(WP::err)));
        fConnection->connectToServer();
    }
    return WP::kOk;
}

void RemoteAuthentication::logout()
{
    if (fAuthenticationInProgress)
        return;
    if (!fVerified)
        return;

    fVerified = false;
    fAuthenticationInProgress = false;

    QByteArray data;
    getLogoutData(data);
    fConnection->send(data);
}

bool RemoteAuthentication::verified()
{
    return fVerified;
}

void RemoteAuthentication::handleConnectionAttempt(WP::err code)
{
    if (code != WP::kOk) {
        fAuthenticationInProgress = false;
        emit authenticationAttemptFinished(code);
        return;
    }
    QByteArray data;
    getLoginRequestData(data);
    fAuthenticationReply = fConnection->send(data);
    connect(fAuthenticationReply, SIGNAL(finished(WP::err)), this, SLOT(handleAuthenticationRequest(WP::err)));
}

void RemoteAuthentication::handleAuthenticationRequest(WP::err code)
{
    if (code != WP::kOk) {
        fAuthenticationInProgress = false;
        emit authenticationAttemptFinished(code);
        return;
    }
    QByteArray data;
    getLoginData(data, fAuthenticationReply->readAll());
    fAuthenticationReply = fConnection->send(data);
    connect(fAuthenticationReply, SIGNAL(finished(WP::err)), this, SLOT(handleAuthenticationAttempt(WP::err)));
}

void RemoteAuthentication::handleAuthenticationAttempt(WP::err code)
{
    if (code != WP::kOk) {
        fAuthenticationInProgress = false;
        fAuthenticationReply = NULL;
        emit authenticationAttemptFinished(code);
        return;
    }
    fAuthenticationInProgress = false;
    fVerified = true;
    fAuthenticationReply = NULL;
    emit authenticationAttemptFinished(WP::kOk);
}


SignatureAuthentication::SignatureAuthentication(RemoteConnection *connection, const QString &userName, const QString &keyStoreId, const QString &keyId) :
    RemoteAuthentication(connection, userName, keyStoreId, keyId)
{
}

void SignatureAuthentication::getLoginRequestData(QByteArray &data)
{
    ProtocolOutStream outStream(&data);

    IqOutStanza *iqStanza = new IqOutStanza(kGet);
    outStream.pushStanza(iqStanza);

    OutStanza *authStanza =  new OutStanza("user_auth");
    authStanza->addAttribute("type", "signature");
    authStanza->addAttribute("user", fUserName);
    outStream.pushChildStanza(authStanza);

    outStream.flush();
}

void SignatureAuthentication::getLoginData(QByteArray &data, const QByteArray &serverRequest)
{
    qDebug(serverRequest);
    /*IqInStanzaHandler iqHandler;
    SyncPullHandler *syncPullHandler = new SyncPullHandler();
    SyncPullPackHandler *syncPullPackHandler = new SyncPullPackHandler();
    iqHandler.addChildHandler(syncPullHandler);
    syncPullHandler->addChildHandler(syncPullPackHandler);

    ProtocolInStream inStream(data);
    inStream.addHandler(&iqHandler);

    inStream.parse();*/
}

void SignatureAuthentication::getLogoutData(QByteArray &data)
{

}

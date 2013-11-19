#include "remoteauthentication.h"

#include "databaseutil.h"
#include "profile.h"
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
    WP::err error = getLoginData(data, fAuthenticationReply->readAll());
    if (error != WP::kOk) {
        fAuthenticationInProgress = false;
        emit authenticationAttemptFinished(error);
        return;
    }

    fAuthenticationReply = fConnection->send(data);
    connect(fAuthenticationReply, SIGNAL(finished(WP::err)), this,
            SLOT(handleAuthenticationAttempt(WP::err)));
}

void RemoteAuthentication::handleAuthenticationAttempt(WP::err code)
{
    WP::err error = code;
    if (error == WP::kOk) {
        QByteArray data = fAuthenticationReply->readAll();
        error = wasLoginSuccessful(data);
    }
    if (error == WP::kOk)
        fVerified = true;

    fAuthenticationInProgress = false;
    fAuthenticationReply = NULL;
    emit authenticationAttemptFinished(error);
}


SignatureAuthentication::SignatureAuthentication(RemoteConnection *connection,
                                                 const QString &userName,
                                                 const QString &keyStoreId,
                                                 const QString &keyId) :
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

class UserAuthHandler : public InStanzaHandler {
public:
    UserAuthHandler() :
        InStanzaHandler("user_auth")
    {
    }

    bool handleStanza(const QXmlStreamAttributes &attributes)
    {
        if (!attributes.hasAttribute("sign_token"))
            return false;

        signToken = attributes.value("sign_token").toString();
        return true;
    }

public:
    QString signToken;
};

WP::err SignatureAuthentication::getLoginData(QByteArray &data, const QByteArray &serverRequest)
{
qDebug(serverRequest);
    IqInStanzaHandler iqHandler(kResult);
    UserAuthHandler *userAuthHandler = new UserAuthHandler();
    iqHandler.addChildHandler(userAuthHandler);

    ProtocolInStream inStream(serverRequest);
    inStream.addHandler(&iqHandler);
    inStream.parse();

    if (!userAuthHandler->hasBeenHandled())
        return WP::kError;

    KeyStore *keyStore = fProfile->findKeyStore(fKeyStoreId);
    if (keyStore == NULL)
        return WP::kEntryNotFound;

    QString certificate;
    QString publicKey;
    QString privateKey;
    WP::err error = keyStore->readAsymmetricKey(fKeyId, certificate, publicKey, privateKey);
    if (error != WP::kOk)
        return error;

    CryptoInterface *crypto = CryptoInterfaceSingleton::getCryptoInterface();
    QByteArray signature;
    SecureArray password = "";

    error = crypto->sign(userAuthHandler->signToken.toLatin1(), signature, privateKey, password);
    if (error != WP::kOk)
        return error;
    signature = signature.toBase64();

    ProtocolOutStream outStream(&data);
    IqOutStanza *iqStanza = new IqOutStanza(kSet);
    outStream.pushStanza(iqStanza);
    OutStanza *authStanza =  new OutStanza("user_auth_signed");
    authStanza->addAttribute("signature", signature);
    outStream.pushChildStanza(authStanza);
    outStream.flush();

    return WP::kOk;
}


class UserAuthResultHandler : public InStanzaHandler {
public:
    UserAuthResultHandler() :
        InStanzaHandler("user_auth_signed")
    {
    }

    bool handleStanza(const QXmlStreamAttributes &attributes)
    {
        if (!attributes.hasAttribute("verification"))
            return false;

        result = attributes.value("verification").toString();
        return true;
    }

public:
    QString result;
};


WP::err SignatureAuthentication::wasLoginSuccessful(QByteArray &data)
{
qDebug(data);
    IqInStanzaHandler iqHandler(kResult);
    UserAuthResultHandler *userAuthResutlHandler = new UserAuthResultHandler();
    iqHandler.addChildHandler(userAuthResutlHandler);

    ProtocolInStream inStream(data);
    inStream.addHandler(&iqHandler);
    inStream.parse();

    if (!userAuthResutlHandler->hasBeenHandled())
        return WP::kError;

    if (userAuthResutlHandler->result != "ok")
        return WP::kError;

    return WP::kOk;
}

void SignatureAuthentication::getLogoutData(QByteArray &data)
{
    ProtocolOutStream outStream(&data);
    IqOutStanza *iqStanza = new IqOutStanza(kSet);
    outStream.pushStanza(iqStanza);
    OutStanza *authStanza =  new OutStanza("logout");
    outStream.pushChildStanza(authStanza);
    outStream.flush();
}

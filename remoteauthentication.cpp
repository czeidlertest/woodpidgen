#include "remoteauthentication.h"

#include "databaseutil.h"
#include "profile.h"
#include "protocolparser.h"


RemoteAuthentication::RemoteAuthentication(RemoteConnection *connection) :
    fConnection(connection),
    fAuthenticationReply(NULL),
    fAuthenticationInProgress(false),
    fVerified(false)
{
}

RemoteConnection *RemoteAuthentication::getConnection()
{
    return fConnection;
}

WP::err RemoteAuthentication::login()
{
    if (fAuthenticationInProgress)
        return WP::kOk;
    if (fVerified) {
        emit authenticationAttemptFinished(WP::kOk);
        return WP::kOk;
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

void RemoteAuthentication::setAuthenticationCanceled(WP::err code)
{
    fAuthenticationInProgress = false;
    //fAuthenticationReply = NULL;
    emit authenticationAttemptFinished(code);
}

void RemoteAuthentication::setAuthenticationSucceeded()
{
    fVerified = true;
    fAuthenticationInProgress = false;
    fAuthenticationReply = NULL;
    emit authenticationAttemptFinished(WP::kOk);
}


const char *kAuthStanza = "auth";
const char *kAuthSignedStanza = "auth_signed";

SignatureAuthentication::SignatureAuthentication(RemoteConnection *connection,
                                                 Profile *profile,
                                                 const QString &userName,
                                                 const QString &keyStoreId,
                                                 const QString &keyId, const QString &serverUser) :
    RemoteAuthentication(connection),
    fProfile(profile),
    fUserName(userName),
    fServerUser(serverUser),
    fKeyStoreId(keyStoreId),
    fKeyId(keyId)
{
}

void SignatureAuthentication::getLoginRequestData(QByteArray &data)
{
    ProtocolOutStream outStream(&data);

    IqOutStanza *iqStanza = new IqOutStanza(kGet);
    outStream.pushStanza(iqStanza);

    OutStanza *authStanza =  new OutStanza(kAuthStanza);
    authStanza->addAttribute("type", "signature");
    authStanza->addAttribute("user", fUserName);
    authStanza->addAttribute("server_user", fServerUser);
    outStream.pushChildStanza(authStanza);

    outStream.flush();
}

class UserAuthHandler : public InStanzaHandler {
public:
    UserAuthHandler() :
        InStanzaHandler(kAuthStanza)
    {
    }

    bool handleStanza(const QXmlStreamAttributes &attributes)
    {
        if (!attributes.hasAttribute("status"))
            return false;
        status = attributes.value("status").toString();

        if (attributes.hasAttribute("sign_token"))
            signToken = attributes.value("sign_token").toString();
        return true;
    }

public:
    QString status;
    QString signToken;
};

WP::err SignatureAuthentication::getLoginData(QByteArray &data, const QByteArray &serverRequest)
{
    IqInStanzaHandler iqHandler(kResult);
    UserAuthHandler *userAuthHandler = new UserAuthHandler();
    iqHandler.addChildHandler(userAuthHandler);

    ProtocolInStream inStream(serverRequest);
    inStream.addHandler(&iqHandler);
    inStream.parse();

    if (!userAuthHandler->hasBeenHandled())
        return WP::kError;

    if (userAuthHandler->status == "i_dont_know_you")
        return WP::kContactNeeded;
    if (userAuthHandler->status != "sign_this_token")
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
    OutStanza *authStanza =  new OutStanza(kAuthSignedStanza);
    authStanza->addAttribute("signature", signature);
    outStream.pushChildStanza(authStanza);
    outStream.flush();

    return WP::kOk;
}


class UserAuthResultHandler : public InStanzaHandler {
public:
    UserAuthResultHandler() :
        InStanzaHandler(kAuthSignedStanza)
    {
    }

    bool handleStanza(const QXmlStreamAttributes &attributes)
    {
        return true;
    }
};

class AuthResultRoleHandler : public InStanzaHandler {
public:
    AuthResultRoleHandler() :
        InStanzaHandler("role", true)
    {
    }

    bool handleStanza(const QXmlStreamAttributes &attributes)
    {
        return true;
    }

    bool handleText(const QStringRef &text) {
        roles.append(text.toString());
        return true;
    }

public:
    QStringList roles;
};

WP::err SignatureAuthentication::wasLoginSuccessful(QByteArray &data)
{
    IqInStanzaHandler iqHandler(kResult);
    UserAuthResultHandler *userAuthResutlHandler = new UserAuthResultHandler();
    AuthResultRoleHandler *roleHander = new AuthResultRoleHandler();
    userAuthResutlHandler->addChildHandler(roleHander);
    iqHandler.addChildHandler(userAuthResutlHandler);

    ProtocolInStream inStream(data);
    inStream.addHandler(&iqHandler);
    inStream.parse();

    if (!userAuthResutlHandler->hasBeenHandled())
        return WP::kError;

    fRoles = roleHander->roles;
    if (fRoles.count() == 0)
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


void SignatureAuthentication::handleConnectionAttempt(WP::err code)
{
    if (code != WP::kOk) {
        setAuthenticationCanceled(code);
        return;
    }
    QByteArray data;
    getLoginRequestData(data);
    fAuthenticationReply = fConnection->send(data);
    connect(fAuthenticationReply, SIGNAL(finished(WP::err)), this, SLOT(handleAuthenticationRequest(WP::err)));
}

void SignatureAuthentication::handleAuthenticationRequest(WP::err code)
{
    if (code != WP::kOk) {
        setAuthenticationCanceled(code);
        return;
    }

    QByteArray data;
    WP::err error = getLoginData(data, fAuthenticationReply->readAll());
    if (error != WP::kOk) {
        setAuthenticationCanceled(error);
        return;
    }

    fAuthenticationReply = fConnection->send(data);
    connect(fAuthenticationReply, SIGNAL(finished(WP::err)), this,
            SLOT(handleAuthenticationAttempt(WP::err)));
}

void SignatureAuthentication::handleAuthenticationAttempt(WP::err code)
{
    WP::err error = code;
    if (error == WP::kOk) {
        QByteArray data = fAuthenticationReply->readAll();
        error = wasLoginSuccessful(data);
    }
    if (error != WP::kOk) {
        setAuthenticationCanceled(error);
        return;
    }
    setAuthenticationSucceeded();
}

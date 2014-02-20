#include "remoteauthentication.h"

#include "databaseutil.h"
#include "profile.h"
#include "protocolparser.h"


RemoteAuthentication::RemoteAuthentication(RemoteConnection *_connection) :
    connection(_connection),
    authenticationReply(NULL),
    authenticationInProgress(false),
    verified(false)
{
}

RemoteConnection *RemoteAuthentication::getConnection()
{
    return connection;
}

WP::err RemoteAuthentication::login()
{
    if (authenticationInProgress)
        return WP::kOk;
    if (verified) {
        emit authenticationAttemptFinished(WP::kOk);
        return WP::kOk;
    }

    authenticationInProgress = true;
    if (connection->isConnected())
        handleConnectionAttempt(WP::kOk);
    else {
        connect(connection, SIGNAL(connectionAttemptFinished(WP::err)), this, SLOT(handleConnectionAttempt(WP::err)));
        connection->connectToServer();
    }
    return WP::kOk;
}

void RemoteAuthentication::logout()
{
    if (authenticationInProgress)
        return;
    if (!verified)
        return;

    verified = false;
    authenticationInProgress = false;

    QByteArray data;
    getLogoutData(data);
    connection->send(data);
}

bool RemoteAuthentication::isVerified()
{
    return verified;
}

void RemoteAuthentication::setAuthenticationCanceled(WP::err code)
{
    authenticationInProgress = false;
    //fAuthenticationReply = NULL;
    emit authenticationAttemptFinished(code);
}

void RemoteAuthentication::setAuthenticationSucceeded()
{
    verified = true;
    authenticationInProgress = false;
    authenticationReply = NULL;
    emit authenticationAttemptFinished(WP::kOk);
}


const char *kAuthStanza = "auth";
const char *kAuthSignedStanza = "auth_signed";

SignatureAuthentication::SignatureAuthentication(RemoteConnection *connection,
                                                 Profile *_profile,
                                                 const QString &_userName,
                                                 const QString &_keyStoreId,
                                                 const QString &_keyId, const QString &_serverUser) :
    RemoteAuthentication(connection),
    profile(_profile),
    userName(_userName),
    serverUser(_serverUser),
    keyStoreId(_keyStoreId),
    keyId(_keyId)
{
}

void SignatureAuthentication::getLoginRequestData(QByteArray &data)
{
    ProtocolOutStream outStream(&data);

    IqOutStanza *iqStanza = new IqOutStanza(kGet);
    outStream.pushStanza(iqStanza);

    OutStanza *authStanza =  new OutStanza(kAuthStanza);
    authStanza->addAttribute("type", "signature");
    authStanza->addAttribute("user", userName);
    authStanza->addAttribute("server_user", serverUser);
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

    KeyStore *keyStore = profile->findKeyStore(keyStoreId);
    if (keyStore == NULL)
        return WP::kEntryNotFound;

    QString certificate;
    QString publicKey;
    QString privateKey;
    WP::err error = keyStore->readAsymmetricKey(keyId, certificate, publicKey, privateKey);
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

    roles = roleHander->roles;
    if (roles.count() == 0)
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
    authenticationReply = connection->send(data);
    connect(authenticationReply, SIGNAL(finished(WP::err)), this, SLOT(handleAuthenticationRequest(WP::err)));
}

void SignatureAuthentication::handleAuthenticationRequest(WP::err code)
{
    if (code != WP::kOk) {
        setAuthenticationCanceled(code);
        return;
    }

    QByteArray data;
    WP::err error = getLoginData(data, authenticationReply->readAll());
    if (error != WP::kOk) {
        setAuthenticationCanceled(error);
        return;
    }

    authenticationReply = connection->send(data);
    connect(authenticationReply, SIGNAL(finished(WP::err)), this,
            SLOT(handleAuthenticationAttempt(WP::err)));
}

void SignatureAuthentication::handleAuthenticationAttempt(WP::err code)
{
    WP::err error = code;
    if (error == WP::kOk) {
        QByteArray data = authenticationReply->readAll();
        error = wasLoginSuccessful(data);
    }
    if (error != WP::kOk) {
        setAuthenticationCanceled(error);
        return;
    }
    setAuthenticationSucceeded();
}

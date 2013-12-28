#include "contactrequest.h"

#include <QDebug>

#include "protocolparser.h"


const char *kContactRequestStanza = "conctact_request";


class ContactRequestHandler : public InStanzaHandler {
public:
    ContactRequestHandler() :
        InStanzaHandler(kContactRequestStanza)
    {
    }

    bool handleStanza(const QXmlStreamAttributes &attributes)
    {
        if (!attributes.hasAttribute("status"))
            return false;
        status = attributes.value("status").toString();
        if (status != "accepted")
            return true;

        if (!attributes.hasAttribute("uid"))
            return false;
        if (!attributes.hasAttribute("name"))
            return false;
        if (!attributes.hasAttribute("certificateSignature"))
            return false;
        if (!attributes.hasAttribute("keyId"))
            return false;

        uid = attributes.value("uid").toString();
        nickname = attributes.value("name").toString();
        certificateSignature = attributes.value("certificateSignature").toString();
        keyId = attributes.value("keyId").toString();
        return true;
    }

public:
    QString status;
    QString uid;
    QString nickname;
    QString certificateSignature;
    QString keyId;
};

class CertificateHandler : public InStanzaHandler {
public:
    CertificateHandler() :
        InStanzaHandler("certificate", true)
    {
    }

    bool handleStanza(const QXmlStreamAttributes &attributes)
    {
        return true;
    }

    bool handleText(const QStringRef &text) {
        certificate = text.toString();
        return true;
    }

public:
    QString certificate;
};

class PublicKeyHandler : public InStanzaHandler {
public:
    PublicKeyHandler() :
        InStanzaHandler("publicKey", true)
    {
    }

    bool handleStanza(const QXmlStreamAttributes &attributes)
    {
        return true;
    }

    bool handleText(const QStringRef &text) {
        publicKey = text.toString();
        return true;
    }

public:
    QString publicKey;
};


ContactRequest::ContactRequest(RemoteConnection *connection, const QString &serverUser,
                               UserIdentity *identity, QObject *parent) :
    QObject(parent),
    fConnection(connection),
    fServerUser(serverUser),
    fUserIdentity(identity),
    fServerReply(NULL)
{
}

WP::err ContactRequest::postRequest()
{
    if (fConnection->isConnected())
        onConnectionRelpy(WP::kOk);
    else {
        connect(fConnection, SIGNAL(connectionAttemptFinished(WP::err)), this,
                SLOT(onConnectionRelpy(WP::err)));
        fConnection->connectToServer();
    }
    return WP::kOk;
}

void ContactRequest::onConnectionRelpy(WP::err code)
{
    if (code != WP::kOk)
        return;

    QByteArray data;
    makeRequest(data, fUserIdentity->getMyself());

    fServerReply = fConnection->send(data);
    connect(fServerReply, SIGNAL(finished(WP::err)), this,
            SLOT(onRequestReply(WP::err)));
}

void ContactRequest::onRequestReply(WP::err code)
{
    if (code != WP::kOk)
        return;

    QByteArray data = fServerReply->readAll();
    fServerReply = NULL;
qDebug() << data;
    IqInStanzaHandler iqHandler(kResult);
    ContactRequestHandler *requestHandler = new ContactRequestHandler();
    CertificateHandler *certificateHandler = new CertificateHandler();
    PublicKeyHandler *publicKeyHandler = new PublicKeyHandler();
    iqHandler.addChildHandler(requestHandler);
    requestHandler->addChildHandler(certificateHandler);
    requestHandler->addChildHandler(publicKeyHandler);

    ProtocolInStream inStream(data);
    inStream.addHandler(&iqHandler);

    inStream.parse();

    if (!requestHandler->hasBeenHandled()) {
        emit contactRequestFinished(WP::kBadValue);
        return;
    }

    if (requestHandler->status != "accepted") {
        emit contactRequestFinished(WP::kNotAllowed);
        return;
    }

    if (!certificateHandler->hasBeenHandled() || !publicKeyHandler->hasBeenHandled()) {
        emit contactRequestFinished(WP::kBadValue);
        return;
    }

    CryptoInterface *crypto = CryptoInterfaceSingleton::getCryptoInterface();
    if (crypto->verifySignatur(certificateHandler->certificate.toLatin1(),
                               requestHandler->certificateSignature.toLatin1(),
                               publicKeyHandler->publicKey)) {
        emit contactRequestFinished(WP::kBadValue);
        return;
    }
    Contact *contact = new Contact(requestHandler->uid, requestHandler->nickname,
                                   requestHandler->keyId, certificateHandler->certificate,
                                   publicKeyHandler->publicKey);
    WP::err error = fUserIdentity->addContact(contact);
    if (error != WP::kOk) {
        emit contactRequestFinished(WP::kError);
        return;
    }

    emit contactRequestFinished(WP::kOk);
    return;
}

void ContactRequest::makeRequest(QByteArray &data, Contact *myself)
{
    ProtocolOutStream outStream(&data);
    IqOutStanza *iqStanza = new IqOutStanza(kGet);
    outStream.pushStanza(iqStanza);

    QString keyId = myself->getKeys()->getMainKeyId();
    QString certificate, publicKey, privateKey;
    fUserIdentity->getKeyStore()->readAsymmetricKey(keyId, certificate, publicKey, privateKey);
    CryptoInterface *crypto = CryptoInterfaceSingleton::getCryptoInterface();
    QByteArray certificateSignature;
    crypto->sign(certificate.toLatin1(), certificateSignature, privateKey, "");

    OutStanza *requestStanza =  new OutStanza(kContactRequestStanza);
    requestStanza->addAttribute("uid", myself->getUid());
    requestStanza->addAttribute("name", myself->getNickname());
    requestStanza->addAttribute("certificateSignature", certificateSignature);
    requestStanza->addAttribute("keyId", keyId);
    outStream.pushChildStanza(requestStanza);

    OutStanza *certificateStanza = new OutStanza("certificate");
    certificateStanza->setText(certificate);
    outStream.pushChildStanza(requestStanza);
    outStream.cdDotDot();

    OutStanza *publicKeyStanza = new OutStanza("publicKey");
    publicKeyStanza->setText(publicKey);
    outStream.pushChildStanza(publicKeyStanza);
    outStream.cdDotDot();

    outStream.flush();
}

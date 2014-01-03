#include "mailmessenger.h"

#include "protocolparser.h"
#include "remoteauthentication.h"
#include "useridentity.h"


MailMessenger::MailMessenger(Mailbox *mailbox, const QString &targetAddress, Profile *profile, UserIdentity *identity) :
    fMailbox(mailbox),
    fIdentity(identity),
    fAddress(targetAddress),
    fContactRequest(NULL),
    fMessage(NULL),
    fRemoteConnection(NULL),
    fMessageChannel(NULL)
{
    parseAddress(targetAddress);
    //fRemoteConnection = sPHPConnectionManager.connectionFor(QUrl(fTargetServer));
    fRemoteConnection = ConnectionManager::connectionHTTPFor(QUrl(fTargetServer));
    if (fRemoteConnection == NULL)
        return;
    fAuthentication = new SignatureAuthentication(fRemoteConnection, profile, identity->getMyself()->getUid(),
                                                  identity->getKeyStore()->getUid(),
                                                  identity->getMyself()->getKeys()->getMainKeyId(), fTargetUser);
}

MailMessenger::~MailMessenger()
{
    delete fAuthentication;
    delete fRemoteConnection;
    delete fMessage;
}

WP::err MailMessenger::postMessage(const RawMailMessage *message, const QString channelId)
{
    delete fMessage;
    fMessage = message;

    fChannelId = channelId;

    if (fTargetServer == "")
        return WP::kNotInit;
    if (fRemoteConnection == NULL)
        return WP::kNotInit;
    if (fAuthentication == NULL)
        return WP::kNotInit;

    Contact *targetContact = fIdentity->findContact(fAddress);
    if (targetContact != NULL) {
        onContactFound(WP::kOk);
        return WP::kOk;
    }

    return startContactRequest();
}


void MailMessenger::authConnected(WP::err error)
{
    if (error == WP::kContactNeeded) {
        startContactRequest();
        return;
    } else if (error != WP::kOk)
        return;

    // channel info
    if (fChannelId == "") {
        //channel = new
        fMessageChannel = new MessageChannel();
        error = fMessageChannel->createNew(fIdentity->getMyself());
        if (error != WP::kOk) {
            emit sendResult(error);
            return;
        }

    } else {
        fMessageChannel = fMailbox->findMessageChannel(fChannelId);
        if (fMessageChannel == NULL) {
            emit sendResult(WP::kChannelNotFound);
            return;
        }
    }

    QByteArray data;
    ProtocolOutStream outStream(&data);
    IqOutStanza *iqStanza = new IqOutStanza(kSet);
    outStream.pushStanza(iqStanza);

    Contact *myself = fIdentity->getMyself();
    OutStanza *messageStanza =  new OutStanza("message");
    messageStanza->addAttribute("uid", fMessage->getUid());
    messageStanza->addAttribute("channel_uid", fMessageChannel->getUid());
    messageStanza->addAttribute("from", myself->getUid());
    outStream.pushChildStanza(messageStanza);

    // header
    error = envelopMessage(&outStream, fMessage->getBody());
    if (error != WP::kOk) {
        emit sendResult(error);
        return;
    }

    if (fChannelId == "") {
        // add new channel
        Contact *receiver = fIdentity->findContact(fAddress);
        Q_ASSERT(receiver != NULL);
        error = XMLSecureChannel::toPublicXML(&outStream, fMessageChannel, receiver);
        if (error != WP::kOk) {
            emit sendResult(error);
            return;
        }
    }

    outStream.flush();

    fServerReply = fRemoteConnection->send(data);
    connect(fServerReply, SIGNAL(finished(WP::err)), this, SLOT(handleReply(WP::err)));
}

void MailMessenger::onContactFound(WP::err error)
{
    delete fContactRequest;
    fContactRequest = NULL;

    if (error != WP::kOk)
        return;

    if (fAuthentication->verified())
        authConnected(WP::kOk);
    else {
        connect(fAuthentication, SIGNAL(authenticationAttemptFinished(WP::err)),
                this, SLOT(authConnected(WP::err)));
        fAuthentication->login();
    }
}

WP::err MailMessenger::envelopMessage(ProtocolOutStream *outStream, const QByteArray &org)
{
    Contact *myself = fIdentity->getMyself();

    SecureParcel *parcel = fMessageChannel->getSecureParcel();
    QByteArray encryptedData;
    WP::err error = parcel->cloakData(org, encryptedData);
    if (error != WP::kOk)
        return error;

    const QString &myKeyId = myself->getKeys()->getMainKeyId();
    QByteArray signature;
    error = myself->sign(myKeyId, encryptedData, signature);
    if (error != WP::kOk)
        return error;

    encryptedData = encryptedData.toBase64();
    signature = signature.toBase64();

    OutStanza *dataStanza = new OutStanza("primary_data");
    dataStanza->addAttribute("signature_key", myKeyId);
    dataStanza->addAttribute("signature", signature);
    dataStanza->setText(encryptedData);
    outStream->pushChildStanza(dataStanza);

//   outStream->cdDotDot();
    return WP::kOk;
}

void MailMessenger::handleReply(WP::err error)
{
    QByteArray data = fServerReply->readAll();
}

void MailMessenger::parseAddress(const QString &targetAddress)
{
    QString address = targetAddress.trimmed();
    QStringList parts = address.split("@");
    if (parts.count() != 2)
        return;
    fTargetUser = parts[0];
    fTargetServer = "http://";
    fTargetServer += parts[1];
    fTargetServer += "/php_server/portal.php";
}

WP::err MailMessenger::startContactRequest()
{
    fContactRequest = new ContactRequest(fRemoteConnection, fTargetUser, fIdentity, this);
    connect(fContactRequest, SIGNAL(contactRequestFinished(WP::err)), this, SLOT(onContactFound(WP::err)));
    return fContactRequest->postRequest();
}

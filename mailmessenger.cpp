#include "mailmessenger.h"

#include "protocolparser.h"
#include "remoteauthentication.h"
#include "useridentity.h"


MailMessenger::MailMessenger(Mailbox *mailbox, const QString &targetAddress, Profile *profile, UserIdentity *identity) :
    mailbox(mailbox),
    userIdentity(identity),
    address(targetAddress),
    targetContact(NULL),
    contactRequest(NULL),
    rawMessage(NULL),
    remoteConnection(NULL),
    messageChannel(NULL)
{
    parseAddress(targetAddress);
    //fRemoteConnection = sPHPConnectionManager.connectionFor(QUrl(fTargetServer));
    remoteConnection = ConnectionManager::connectionHTTPFor(QUrl(targetServer));
    if (remoteConnection == NULL)
        return;
    authentication = new SignatureAuthentication(remoteConnection, profile, identity->getMyself()->getUid(),
                                                  identity->getKeyStore()->getUid(),
                                                  identity->getMyself()->getKeys()->getMainKeyId(), targetUser);
}

MailMessenger::~MailMessenger()
{
    delete authentication;
    delete remoteConnection;
    delete rawMessage;
}

WP::err MailMessenger::postMessage(const RawMessage *message, const QString _channelId)
{
    delete message;
    message = message;

    channelId = _channelId;

    if (targetServer == "")
        return WP::kNotInit;
    if (remoteConnection == NULL)
        return WP::kNotInit;
    if (authentication == NULL)
        return WP::kNotInit;

    targetContact = userIdentity->findContact(address);
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
    if (channelId == "") {
        messageChannel = new MessageChannel(targetContact, targetContact->getKeys()->getMainKeyId());
    } else {
        messageChannel = mailbox->findMessageThread(channelId)->getMessageChannel();
        if (messageChannel == NULL) {
            emit sendResult(WP::kChannelNotFound);
            return;
        }
    }

    Message *message = new Message(messageChannel);
    message->setBody(rawMessage->body);

    QByteArray data;
    ProtocolOutStream outStream(&data);
    IqOutStanza *iqStanza = new IqOutStanza(kSet);
    outStream.pushStanza(iqStanza);

    Contact *myself = userIdentity->getMyself();
    OutStanza *messageStanza =  new OutStanza("put_message");
    messageStanza->addAttribute("uid", message->getUid());
    messageStanza->addAttribute("from", myself->getUid());
    outStream.pushChildStanza(messageStanza);

    QString signatureKeyId = myself->getKeys()->getMainKeyId();

    error = XMLSecureParcel::write(&outStream, myself, signatureKeyId, message, "message");
    if (error != WP::kOk) {
        emit sendResult(error);
        return;
    }

    if (channelId == "") {
        // add new channel
        error = XMLSecureParcel::write(&outStream, myself, signatureKeyId, messageChannel, "channel");
        if (error != WP::kOk) {
            emit sendResult(error);
            return;
        }
    }

    outStream.flush();

    serverReply = remoteConnection->send(data);
    connect(serverReply, SIGNAL(finished(WP::err)), this, SLOT(handleReply(WP::err)));
}

void MailMessenger::onContactFound(WP::err error)
{
    delete contactRequest;
    contactRequest = NULL;

    if (error != WP::kOk)
        return;

    targetContact = userIdentity->findContact(address);

    if (authentication->verified())
        authConnected(WP::kOk);
    else {
        connect(authentication, SIGNAL(authenticationAttemptFinished(WP::err)),
                this, SLOT(authConnected(WP::err)));
        authentication->login();
    }
}

void MailMessenger::handleReply(WP::err error)
{
    QByteArray data = serverReply->readAll();
}

void MailMessenger::parseAddress(const QString &targetAddress)
{
    QString address = targetAddress.trimmed();
    QStringList parts = address.split("@");
    if (parts.count() != 2)
        return;
    targetUser = parts[0];
    targetServer = "http://";
    targetServer += parts[1];
    targetServer += "/php_server/portal.php";
}

WP::err MailMessenger::startContactRequest()
{
    contactRequest = new ContactRequest(remoteConnection, targetUser, userIdentity, this);
    connect(contactRequest, SIGNAL(contactRequestFinished(WP::err)), this, SLOT(onContactFound(WP::err)));
    return contactRequest->postRequest();
}

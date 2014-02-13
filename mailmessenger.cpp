#include "mailmessenger.h"

#include "protocolparser.h"
#include "remoteauthentication.h"
#include "useridentity.h"


MailMessenger::MailMessenger(Mailbox *mailbox, const MessageChannelInfo::Participant *_receiver, Profile *profile) :
    mailbox(mailbox),
    userIdentity(mailbox->getOwner()),
    receiver(_receiver),
    targetContact(NULL),
    contactRequest(NULL),
    message(NULL),
    remoteConnection(NULL)
{
    parseAddress(receiver->address);
    remoteConnection = ConnectionManager::defaultConnectionFor(QUrl(targetServer));
    if (remoteConnection == NULL)
        return;
    authentication = new SignatureAuthentication(remoteConnection, profile, userIdentity->getMyself()->getUid(),
                                                  userIdentity->getKeyStore()->getUid(),
                                                  userIdentity->getMyself()->getKeys()->getMainKeyId(), targetUser);
}

MailMessenger::~MailMessenger()
{
    delete authentication;
    delete remoteConnection;
}

WP::err MailMessenger::postMessage(Message *message)
{
    this->message = message;

    if (targetServer == "")
        return WP::kNotInit;
    if (remoteConnection == NULL)
        return WP::kNotInit;
    if (authentication == NULL)
        return WP::kNotInit;

    Contact *contact = userIdentity->findContact(receiver->address);
    if (contact != NULL) {
        onContactFound(WP::kOk);
        return WP::kOk;
    } else if (receiver->uid != "") {
        contact = userIdentity->findContactByUid(receiver->uid);
        if (contact != NULL) {
            onContactFound(WP::kOk);
            return WP::kOk;
        }
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

    MessageChannel *newMessageChannel = NULL;
    // channel info
    if (message->getChannel() == NULL) {
        newMessageChannel = new MessageChannel(targetContact, targetContact->getKeys()->getMainKeyId());
        message->setChannel(newMessageChannel);
        message->getChannelInfo()->setChannel(newMessageChannel);
    }

    QByteArray data;
    ProtocolOutStream outStream(&data);
    IqOutStanza *iqStanza = new IqOutStanza(kSet);
    outStream.pushStanza(iqStanza);

    Contact *myself = userIdentity->getMyself();
    OutStanza *messageStanza =  new OutStanza("put_message");
    messageStanza->addAttribute("channel", message->getChannel()->getUid());
    outStream.pushChildStanza(messageStanza);

    QString signatureKeyId = myself->getKeys()->getMainKeyId();

    // write new channel
    if (newMessageChannel != NULL) {
        error = XMLSecureParcel::write(&outStream, myself, signatureKeyId, message->getChannel(), "channel");
        if (error != WP::kOk) {
            emit sendResult(error);
            return;
        }
    }

    // write new channel info
    MessageChannelInfo *info = message->getChannelInfo();
    if (info->isNewLocale()) {
        info->setParticipantUid(targetContact->getAddress(), targetContact->getUid());
        error = XMLSecureParcel::write(&outStream, myself, signatureKeyId, info, "channel_info");
        if (error != WP::kOk) {
            emit sendResult(error);
            return;
        }
    }

    // write message
    error = XMLSecureParcel::write(&outStream, myself, signatureKeyId, message, "message");
    if (error != WP::kOk) {
        emit sendResult(error);
        return;
    }

    outStream.flush();

    delete message;
    message = NULL;
    delete newMessageChannel;

    serverReply = remoteConnection->send(data);
    connect(serverReply, SIGNAL(finished(WP::err)), this, SLOT(handleReply(WP::err)));
}

void MailMessenger::onContactFound(WP::err error)
{
    delete contactRequest;
    contactRequest = NULL;

    if (error != WP::kOk)
        return;

    targetContact = userIdentity->findContact(receiver->address);
    if (targetContact == NULL)
        return;

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


MultiMailMessenger::MultiMailMessenger(Mailbox *_mailbox, Profile *_profile) :
    mailMessenger(NULL),
    message(NULL),
    mailbox(_mailbox),
    messageChannelInfo(NULL),
    profile(_profile),
    lastParticipantIndex(-1)
{

}

MultiMailMessenger::~MultiMailMessenger()
{
    delete mailMessenger;
    delete message;
}

WP::err MultiMailMessenger::postMessage(Message *message)
{
    delete this->message;
    this->message = message;

    messageChannelInfo = message->getChannelInfo();

    onSendResult(WP::kOk);

    return WP::kOk;
}

void MultiMailMessenger::onSendResult(WP::err error)
{
    if (error != WP::kOk) {
        // todo
    }

    lastParticipantIndex++;
    if (lastParticipantIndex == messageChannelInfo->getParticipants().size()) {
        emit messagesSent();
        return;
    }
    const MessageChannelInfo::Participant *participant = &messageChannelInfo->getParticipants().at(lastParticipantIndex);

    delete mailMessenger;
    mailMessenger = new MailMessenger(mailbox, participant, profile);
    mailMessenger->postMessage(message);

    connect(mailMessenger, SIGNAL(sendResult(WP::err)), this, SLOT(onSendResult(WP::err)));
}

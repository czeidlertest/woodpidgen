#include "mailmessenger.h"

#include "protocolparser.h"
#include "remoteauthentication.h"
#include "useridentity.h"


MailMessenger::MailMessenger(const QString &targetAddress, Profile *profile, UserIdentity *identity) :
    fIdentity(identity),
    fAddress(targetAddress),
    fContactRequest(NULL),
    fMessage(NULL),
    fRemoteConnection(NULL)
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

WP::err MailMessenger::postMessage(const RawMailMessage *message)
{
    delete fMessage;
    fMessage = message;

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

    QByteArray data;
    ProtocolOutStream outStream(&data);
    IqOutStanza *iqStanza = new IqOutStanza(kSet);
    outStream.pushStanza(iqStanza);
    OutStanza *messageStanza =  new OutStanza("message");
    outStream.pushChildStanza(messageStanza);

    OutStanza *headerStanza =  new OutStanza("header");
    headerStanza->setText(fMessage->getHeader());
    outStream.pushChildStanza(headerStanza);

    OutStanza *bodyStanza =  new OutStanza("body");
    bodyStanza->setText(fMessage->getBody());
    outStream.pushStanza(bodyStanza);

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

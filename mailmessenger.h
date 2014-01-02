#ifndef MAILMESSENGER_H
#define MAILMESSENGER_H

#include "contactrequest.h"
#include "mail.h"
#include "mailbox.h"
#include "remoteauthentication.h"

class Profile;
class UserIdentity;

class MailMessenger : QObject {
Q_OBJECT
public:
    MailMessenger(Mailbox *mailbox, const QString &targetAddress, Profile *profile, UserIdentity *identity);
    ~MailMessenger();

    WP::err postMessage(const RawMailMessage *message, const QString channelId = "");

signals:
    void sendResult(WP::err error);

private slots:
    void handleReply(WP::err error);
    void authConnected(WP::err error);
    void onContactFound(WP::err error);

private:
    WP::err envelopHeader(ProtocolOutStream *outStream, const QByteArray &org);
    WP::err envelopBody(ProtocolOutStream *outStream, const QByteArray &org);

    void parseAddress(const QString &targetAddress);
    WP::err startContactRequest();

    Mailbox *fMailbox;

    UserIdentity *fIdentity;
    QString fAddress;
    QString fTargetServer;
    QString fTargetUser;
    QString fChannelId;

    MessageChannel *fMessageChannel;
    ContactRequest* fContactRequest;

    const RawMailMessage *fMessage;
    RemoteConnection *fRemoteConnection;
    RemoteConnectionReply *fServerReply;
    RemoteAuthentication *fAuthentication;
};


#endif // MAILMESSENGER_H

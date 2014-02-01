#ifndef MAILMESSENGER_H
#define MAILMESSENGER_H

#include "contactrequest.h"
#include "mail.h"
#include "mailbox.h"
#include "remoteauthentication.h"

class Profile;
class UserIdentity;

class RawMessage {
public:
    QByteArray body;
};

class MailMessenger : QObject {
Q_OBJECT
public:
    MailMessenger(Mailbox *mailbox, const QString &targetAddress, Profile *profile, UserIdentity *identity);
    ~MailMessenger();

    WP::err postMessage(const RawMessage *rawMessage, const QString channelId = "");

signals:
    void sendResult(WP::err error);

private slots:
    void handleReply(WP::err error);
    void authConnected(WP::err error);
    void onContactFound(WP::err error);

private:
    void parseAddress(const QString &targetAddress);
    WP::err startContactRequest();

    Mailbox *mailbox;

    UserIdentity *userIdentity;
    QString address;
    QString targetServer;
    QString targetUser;
    QString channelId;

    Contact *targetContact;

    MessageChannel *messageChannel;
    ContactRequest* contactRequest;

    const RawMessage *rawMessage;

    RemoteConnection *remoteConnection;
    RemoteConnectionReply *serverReply;
    RemoteAuthentication *authentication;
};


#endif // MAILMESSENGER_H

#ifndef MAILMESSENGER_H
#define MAILMESSENGER_H

#include "contactrequest.h"
#include "mail.h"
#include "remoteauthentication.h"

class Profile;
class UserIdentity;

class MailMessenger : QObject {
Q_OBJECT
public:
    MailMessenger(const QString &targetAddress, Profile *profile, UserIdentity *identity);
    ~MailMessenger();

    WP::err postMessage(const RawMailMessage *message);

private slots:
    void handleReply(WP::err error);
    void authConnected(WP::err error);
    void onContactFound(WP::err error);

private:
    void parseAddress(const QString &targetAddress);
    WP::err startContactRequest();

    UserIdentity *fIdentity;
    QString fAddress;
    QString fTargetServer;
    QString fTargetUser;
    ContactRequest* fContactRequest;

    const RawMailMessage *fMessage;
    RemoteConnection *fRemoteConnection;
    RemoteConnectionReply *fServerReply;
    RemoteAuthentication *fAuthentication;
};


#endif // MAILMESSENGER_H

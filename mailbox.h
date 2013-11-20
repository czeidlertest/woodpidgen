#ifndef MAILBOX_H
#define MAILBOX_H

#include "databaseutil.h"

class Mailbox : public EncryptedUserData
{
public:
    Mailbox(const DatabaseBranch *branch, const QString &baseDir = "");
    ~Mailbox();

    WP::err createMailbox(KeyStore *keyStore, const QString &defaultKeyId,
                          bool addUidToBaseDir = true);
    WP::err open(KeyStoreFinder *keyStoreFinder);


    WP::err addMail(const QByteArray &header, const QByteArray &body);
};


#endif // MAILBOX_H

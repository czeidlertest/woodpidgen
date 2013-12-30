#ifndef USERIDENTITY_H
#define USERIDENTITY_H


#include <QMap>
#include <QString>
#include <QTextStream>

#include "contact.h"
#include "cryptointerface.h"
#include "databaseinterface.h"
#include "databaseutil.h"
#include "mailbox.h"


/*! UserIdentity database structure:
branch identities:
\id (sha1 hash of the certificate)
- master_key: encrypted master key
- master_key_info: master password encryption info: salt, encryption algo, iteration algo, number of iterations
- data_encryption_algo: algo to encrypt all identity data with the master_key
- signature_name
- identity_name
--\private_keys
--\public_keys
--\certificates
--\details
  - first_name
  - family_name
  - icon
--\contacts
}
 */
class UserIdentity : public EncryptedUserData
{
public:
    UserIdentity(const DatabaseBranch *branch, const QString &baseDir = "");
    ~UserIdentity();

    WP::err createNewIdentity(KeyStore *keyStore, const QString &defaultKeyId, Mailbox *mailbox,
                              bool addUidToBaseDir = true);
    WP::err open(KeyStoreFinder *keyStoreFinder, MailboxFinder *mailboxFinder);

    Mailbox *getMailbox() const;
    Contact *getMyself() const;
    const QList<Contact *> &getContacts();

    WP::err addContact(Contact *contact);
    Contact *findContact(const QString &address);

private:
    WP::err writePublicSignature(const QString &filename, const QString &publicKey);

    Mailbox *fMailbox;

    Contact *fMyselfContact;
    QList<Contact *> fContacts;
};

#endif // USERIDENTITY_H

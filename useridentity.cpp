#include "useridentity.h"

#include <QFile>
#include <QStringList>

#include "cryptointerface.h"

const char* kPathMailboxId = "mailbox_id";
const char* kPathIdentityKeyId = "identity_key_id";


UserIdentity::UserIdentity(const DatabaseBranch *branch, const QString &baseDir) :
    fMailbox(NULL),
    fMyselfContact(NULL)
{
    setToDatabase(branch, baseDir);
}

UserIdentity::~UserIdentity()
{
    delete fMyselfContact;
    foreach (Contact *contact, fContacts)
        delete contact;
}


WP::err UserIdentity::createNewIdentity(KeyStore *keyStore, const QString &defaultKeyId,
                                        const QString &nickname, Mailbox *mailbox,
                                        bool addUidToBaseDir)
{
    // derive uid
    QString certificate;
    QString publicKey;
    QString privateKey;
    WP::err error = fCrypto->generateKeyPair(certificate, publicKey, privateKey, "");
    if (error != WP::kOk)
        return error;
    QByteArray hashResult = fCrypto->sha1Hash(certificate.toLatin1());
    QString uidHex = fCrypto->toHex(hashResult);

    // start creating the identity
    error = EncryptedUserData::create(uidHex, keyStore, defaultKeyId, addUidToBaseDir);
    if (error != WP::kOk)
        return error;
    fMailbox = mailbox;

    QString keyId;
    error = fKeyStore->writeAsymmetricKey(certificate, publicKey, privateKey, keyId);
    if (error != WP::kOk)
        return error;

    fMyselfContact = new Contact(this, "myself");
    error = fMyselfContact->createUserIdentityContact(keyStore, keyId, nickname);
    if (error != WP::kOk)
        return error;

    error = write(kPathMailboxId, fMailbox->getUid());
    if (error != WP::kOk)
        return error;

    QString outPut("signature.pup");
    writePublicSignature(outPut, publicKey);

    return error;
}

WP::err UserIdentity::open(KeyStoreFinder *keyStoreFinder, MailboxFinder *mailboxFinder)
{
    WP::err error = EncryptedUserData::open(keyStoreFinder);
    if (error != WP::kOk)
        return error;

    QString mailboxId;
    error = read(kPathMailboxId, mailboxId);
    if (error != WP::kOk)
        return error;
    fMailbox = mailboxFinder->find(mailboxId);
    if (fMailbox == NULL)
        return WP::kEntryNotFound;

    fMyselfContact = new Contact(this, "myself");
    error = fMyselfContact->open(keyStoreFinder);
    if (error != WP::kOk)
        return error;

    QStringList contactNames = listDirectories("contacts");
    foreach (const QString &contactName, contactNames) {
        QString path = getDatabaseBaseDir() + "/contacts/" + contactName;
        Contact *contact = new Contact(this, path);
        WP::err error = contact->open(keyStoreFinder);
        if (error != WP::kOk) {
            delete contact;
            continue;
        }
        fContacts.append(contact);
    }

    return error;
}

Mailbox *UserIdentity::getMailbox() const
{
    return fMailbox;
}

Contact *UserIdentity::getMyself() const
{
    return fMyselfContact;
}

const QList<Contact *> &UserIdentity::getContacts()
{
    return fContacts;
}

WP::err UserIdentity::addContact(Contact *contact)
{
    QString contactUid = contact->getUid();
    QString path = getDatabaseBaseDir() + "/contacts/" + contactUid;
    contact->setDirectory(path);
    WP::err error = contact->writeConfig();
    if (error != WP::kOk)
        return error;
    fContacts.append(contact);
    return WP::kOk;
}

Contact *UserIdentity::findContact(const QString &address)
{
    foreach (Contact *contact, fContacts) {
        if (contact->getAddress() == address)
            return contact;
    }
    return NULL;
}

WP::err UserIdentity::writePublicSignature(const QString &filename, const QString &publicKey)
{
    QFile file(filename);
    file.open(QIODevice::WriteOnly | QIODevice::Truncate);
    QByteArray data = publicKey.toLatin1();
    int count = data.count();
    if (file.write(data, count) != count)
        return WP::kError;
    return WP::kOk;
}

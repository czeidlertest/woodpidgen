#include "mailbox.h"


Mailbox::Mailbox(const DatabaseBranch *branch, const QString &baseDir)
{
    setToDatabase(branch, baseDir);
}

Mailbox::~Mailbox()
{

}

WP::err Mailbox::createMailbox(KeyStore *keyStore, const QString &defaultKeyId, bool addUidToBaseDir)
{
    // derive uid
    QByteArray uid = fCrypto->generateInitalizationVector(512);
    setUid(fCrypto->toHex(fCrypto->sha1Hash(uid)));

    // start creating the identity
    WP::err error = EncryptedUserData::create(uid, keyStore, defaultKeyId, addUidToBaseDir);
    if (error != WP::kOk)
        return error;

    return error;
}

WP::err Mailbox::open(KeyStoreFinder *keyStoreFinder)
{
    WP::err error = EncryptedUserData::open(keyStoreFinder);
    if (error != WP::kOk)
        return error;

    return error;
}

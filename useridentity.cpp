#include "useridentity.h"

#include <QFile>
#include <QStringList>

#include "cryptointerface.h"

const char* kPathMailboxId = "mailbox_id";
const char* kPathIdentityKeyId = "identity_key_id";


UserIdentity::UserIdentity(const DatabaseBranch *branch, const QString &baseDir) :
    fMailbox(NULL)
{
    setToDatabase(branch, baseDir);
}

UserIdentity::~UserIdentity()
{
}


WP::err UserIdentity::createNewIdentity(KeyStore *keyStore, const QString &defaultKeyId, Mailbox *mailbox, bool addUidToBaseDir)
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

    error = fKeyStore->writeAsymmetricKey(certificate, publicKey, privateKey, fIdentityKeyId);
    if (error != WP::kOk)
        return error;
    error = write(kPathIdentityKeyId, fIdentityKeyId);
    if (error != WP::kOk)
        return error;
    error = write(kPathMailboxId, fMailbox->getUid());
    if (error != WP::kOk)
        return error;

    QString outPut("signature.pup");
    writePublicSignature(outPut, publicKey);

    // test data
    QByteArray testData("Hello id");
    QByteArray encyptedTestData;
    error = fCrypto->encyrptAsymmetric(testData, encyptedTestData, certificate.toLatin1());
    if (error != WP::kOk)
        return error;

    write("test_data", encyptedTestData);

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

    QByteArray identityKeyArray;
    read(kPathIdentityKeyId, identityKeyArray);
    fIdentityKeyId = identityKeyArray;

    error = readSafe("userName", fUserName);
    if (error != WP::kOk)
        return error;

    // test
    QString publicKey;
    QString privateKey;
    QString certificate;
    fKeyStore->readAsymmetricKey(fIdentityKeyId, certificate, publicKey, privateKey);

    // test data
    QByteArray encyptedTestData;
    read("test_data", encyptedTestData);
    QByteArray testData;
    error = fCrypto->decryptAsymmetric(encyptedTestData, testData, privateKey, "", certificate);

    printf("test %s\n", testData.data());

    return error;
}

const QString &UserIdentity::getIdentityKeyId() const
{
    return fIdentityKeyId;
}

const QString &UserIdentity::getUserName() const
{
    return fUserName;
}

const WP::err UserIdentity::setUserName(const QString &userName)
{
    WP::err error = writeSafe("userName", userName);
    if (error != WP::kOk)
        return error;
    fUserName = userName;
    return error;
}

Mailbox *UserIdentity::getMailbox() const
{
    return fMailbox;
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

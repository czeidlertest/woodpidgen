#include "useridentity.h"

#include <QStringList>

#include "cryptointerface.h"


UserIdentity::UserIdentity(const QString &path, const QString &branch, const QString &baseDir) :
    EncryptedUserData(path, branch, baseDir)
{
}

UserIdentity::UserIdentity(const QString &path, const QString &branch, const QString &id, const QString &baseDir) :
    EncryptedUserData(path, branch, baseDir),
    fIdentityName(id)
{
}

UserIdentity::~UserIdentity()
{
}


int UserIdentity::createNewIdentity(KeyStore *keyStore)
{
    int error = EncryptedUserData::setTo(keyStore);
    if (error != 0)
        return -1;

    QString certificate;
    QString publicKey;
    QString privateKey;
    error = fCrypto->generateKeyPair(certificate, publicKey, privateKey, "");
    if (error != 0)
        return -1;

    QString keyId;
    error = fKeyStore->addAsymmetricKey(certificate, publicKey, privateKey, keyId);
    if (error != 0)
        return -1;

    QByteArray hashResult = fCrypto->sha1Hash(certificate.toAscii());
    fIdentityName = fCrypto->toHex(hashResult);

    QString path = fIdentityName + "/key_store_id";
    fDatabase->add(path, fKeyStore->getKeyStoreId().toAscii());
    path = fIdentityName + "/identity_key";
    fDatabase->add(path, keyId.toAscii());

    // test data
    QByteArray testData("Hello id");
    QByteArray encyptedTestData;
    error = fCrypto->encyrptAsymmetric(testData, encyptedTestData, certificate.toAscii());
    if (error != 0)
        return -1;

    path = fIdentityName + "/" + "test_data";
    fDatabase->add(path, encyptedTestData);

    return error;
}

int UserIdentity::setTo(KeyStore *keyStore)
{
    int error = EncryptedUserData::setTo(keyStore);
    if (error != 0)
        return -1;

    QByteArray identityKeyArray;
    QString path = fIdentityName + "/identity_key";
    fDatabase->get(path, identityKeyArray);
    fIdentityKey = identityKeyArray;

    // test
    QString publicKey;
    QString privateKey;
    QString certificate;
    fKeyStore->getAsymmetricKey(fIdentityKey, certificate, publicKey, privateKey);

    // test data
    QByteArray encyptedTestData;
    path = fIdentityName + "/" + "test_data";
    fDatabase->get(path, encyptedTestData);
    QByteArray testData;
    error = fCrypto->decryptAsymmetric(encyptedTestData, testData, privateKey, "", certificate);

    printf("test %s\n", testData.data());

    return error;
}

QStringList UserIdentity::getIdenties(DatabaseInterface *database, const QString branch)
{
    database->setBranch(branch);
    QStringList list = database->listDirectories("");
    return list;
}

QString UserIdentity::getId()
{
    return fIdentityName;
}


#include "useridentity.h"

#include <QStringList>

#include "cryptointerface.h"


UserIdentity::UserIdentity(const QString &path, const QString &branch, const QString &baseDir)
{
    setToDatabase(path, branch, baseDir);
}

UserIdentity::~UserIdentity()
{
}


WP::err UserIdentity::createNewIdentity(bool addUidToBaseDir)
{
    QString certificate;
    QString publicKey;
    QString privateKey;
    WP::err error = fCrypto->generateKeyPair(certificate, publicKey, privateKey, "");
    if (error != WP::kOk)
        return error;

    QString keyId;
    error = fKeyStore->writeAsymmetricKey(certificate, publicKey, privateKey, keyId);
    if (error != WP::kOk)
        return error;

    QByteArray hashResult = fCrypto->sha1Hash(certificate.toAscii());
    QString uid = fCrypto->toHex(hashResult);
    if (addUidToBaseDir) {
        QString newBaseDir;
        (fDatabaseBaseDir == "") ? newBaseDir = uid : newBaseDir = fDatabaseBaseDir + "/" + uid;
        setBaseDir(newBaseDir);
    }
    // write uid
    setUid(uid);

    error = setUid(uid);
    if (error != WP::kOk)
        return error;

    QString path = prependBaseDir("key_store_id");
    write(path, fKeyStore->getUid().toAscii());
    path = prependBaseDir("identity_key");
    write(path, keyId.toAscii());

    // test data
    QByteArray testData("Hello id");
    QByteArray encyptedTestData;
    error = fCrypto->encyrptAsymmetric(testData, encyptedTestData, certificate.toAscii());
    if (error != WP::kOk)
        return error;

    path = prependBaseDir("test_data");
    write(path, encyptedTestData);

    return error;
}

WP::err UserIdentity::open()
{
    QByteArray identityKeyArray;
    QString path = prependBaseDir("identity_key");
    read(path, identityKeyArray);
    fIdentityKey = identityKeyArray;

    // test
    QString publicKey;
    QString privateKey;
    QString certificate;
    fKeyStore->readAsymmetricKey(fIdentityKey, certificate, publicKey, privateKey);

    // test data
    QByteArray encyptedTestData;
    path = prependBaseDir("test_data");
    read(path, encyptedTestData);
    QByteArray testData;
    WP::err error = fCrypto->decryptAsymmetric(encyptedTestData, testData, privateKey, "", certificate);

    printf("test %s\n", testData.data());

    return error;
}

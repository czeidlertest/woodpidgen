#include "useridentity.h"

#include <QFile>
#include <QStringList>

#include "cryptointerface.h"


UserIdentity::UserIdentity(const DatabaseBranch *branch, const QString &baseDir)
{
    setToDatabase(branch, baseDir);
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

    QByteArray hashResult = fCrypto->sha1Hash(certificate.toLatin1());
    QString uid = fCrypto->toHex(hashResult);
    if (addUidToBaseDir) {
        QString newBaseDir;
        (fDatabaseBaseDir == "") ? newBaseDir = uid : newBaseDir = fDatabaseBaseDir + "/" + uid;
        setBaseDir(newBaseDir);
    }
    setUid(uid);

    error = EncryptedUserData::writeConfig();
    if (error != WP::kOk)
        return error;
    error = write("identity_key", keyId);
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

WP::err UserIdentity::open(KeyStoreFinder *keyStoreFinder)
{
    WP::err error = EncryptedUserData::readKeyStore(keyStoreFinder);
    if (error != WP::kOk)
        return error;

    QByteArray identityKeyArray;
    read("identity_key", identityKeyArray);
    fIdentityKey = identityKeyArray;

    // test
    QString publicKey;
    QString privateKey;
    QString certificate;
    fKeyStore->readAsymmetricKey(fIdentityKey, certificate, publicKey, privateKey);

    // test data
    QByteArray encyptedTestData;
    read("test_data", encyptedTestData);
    QByteArray testData;
    error = fCrypto->decryptAsymmetric(encyptedTestData, testData, privateKey, "", certificate);

    printf("test %s\n", testData.data());

    return error;
}

const QString &UserIdentity::getIdentityKey()
{
    return fIdentityKey;
}

WP::err UserIdentity::writePublicSignature(const QString &filename, const QString &publicKey)
{
    QFile file(filename);
    file.open(QIODevice::WriteOnly | QIODevice::Truncate);
    QByteArray data = publicKey.toLatin1();
    int count = data.count();
    if (file.write(data.data(), count) != count)
        return WP::kError;
    return WP::kOk;
}

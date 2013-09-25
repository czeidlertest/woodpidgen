#include "useridentity.h"

#include <QStringList>

#include "cryptointerface.h"


UserIdentity::UserIdentity(DatabaseInterface *database, CryptoInterface *crypto,
                           const QString branch)
    :
    DatabaseEncryption(database, crypto, branch)
{
}


int UserIdentity::createNewIdentity(const SecureArray &password)
{
    fDatabase->setBranch(fBranch);

    QString certificate;
    QString publicKey;
    QString privateKey;
    int error = fCrypto->generateKeyPair(certificate, publicKey, privateKey, password);
    if (error != 0)
        return -1;

    QByteArray hashResult = fCrypto->sha1Hash(certificate.toAscii());
    const QString baseDir = fCrypto->toHex(hashResult);

    error = createNewMasterKey(password, fMasterKey, fMasterKeyIV, baseDir);
    if (error != 0)
        return -1;

    QByteArray encryptedPrivateKey;
    error = fCrypto->encryptSymmetric(privateKey.toAscii(), encryptedPrivateKey, fMasterKey,
                                      fMasterKeyIV);
    if (error != 0)
        return -1;

    QString path = baseDir + "/keys/" + baseDir + "/" + "public_key";
    fDatabase->add(path, publicKey.toAscii());
    path = baseDir + "/keys/" + baseDir + "/" + "private_key";
    fDatabase->add(path, encryptedPrivateKey);
    path = baseDir + "/keys/" + baseDir + "/" + "certificate";
    fDatabase->add(path, certificate.toAscii());

    // test data
    QByteArray testData("Hello id");
    QByteArray encyptedTestData;
    error = fCrypto->encyrptData(testData, encyptedTestData, certificate.toAscii());
    if (error != 0)
        return -1;

    path = baseDir + "/" + "test_data";
    fDatabase->add(path, encyptedTestData);

    return error;
}

int UserIdentity::open(const SecureArray &password, const QString id)
{
    fIdentityName = id;
    fDatabase->setBranch(fBranch);

    int error = readMasterKey(password, fMasterKey, fMasterKeyIV, fIdentityName);
    if (error != 0)
        return -1;

    // test
    QByteArray publicKey;
    QString path = fIdentityName + "/keys/" + fIdentityName + "/" + "public_key";
    fDatabase->get(path, publicKey);
    QByteArray encryptedPrivateKey;
    path = fIdentityName + "/keys/" + fIdentityName + "/"+ "private_key";
    fDatabase->get(path, encryptedPrivateKey);
    QByteArray certificate;
    path = fIdentityName + "/keys/" + fIdentityName + "/"+ "certificate";
    fDatabase->get(path, certificate);

    SecureArray privateKey;
    error = fCrypto->decryptSymmetric(encryptedPrivateKey, privateKey, fMasterKey, fMasterKeyIV);
    if (error != 0)
        return -1;

    // test data
    QByteArray encyptedTestData;
    path = fIdentityName + "/" + "test_data";
    fDatabase->get(path, encyptedTestData);
    QByteArray testData;
    error = fCrypto->decryptData(encyptedTestData, testData, privateKey, password, certificate);

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


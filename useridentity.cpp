#include "useridentity.h"

#include <QStringList>
#include <QTextStream>
#include <QUuid>

#include "cryptointerface.h"


const char* kPathMasterKey = "master_key";
const char* kPathMasterKeyIV = "master_key_iv";
const char* kPathMasterPasswordKDF = "master_password_kdf";
const char* kPathMasterPasswordAlgo = "master_password_algo";
const char* kPathMasterPasswordSalt = "master_password_salt";
const char* kPathMasterPasswordSize = "master_password_size";
const char* kPathMasterPasswordIterations = "master_password_iterations";

const int kMasterPasswordIterations = 20000;


int UserIdentity::createNewIdentity(DatabaseInterface *database, const SecureArray &password, const QString branch)
{
    database->setBranch(branch);
    CryptoInterface crypto;

    QByteArray salt = crypto.generateSalt(QUuid::createUuid().toString());
    const QString kdfName = "pbkdf2";
    const QString algoName = "sha1";
    int keyLength = 256;

    QString certificate;
    QString publicKey;
    QString privateKey;
    int error = crypto.generateKeyPair(certificate, publicKey, privateKey, password);
    if (error != 0)
        return -1;

    QByteArray hashResult = crypto.sha1Hash(certificate.toAscii());

    const QString baseDir = crypto.toHex(hashResult);

    // key to encrypte the master key
    SecureArray passwordKey = crypto.deriveKey(password, kdfName, algoName, salt, keyLength, kMasterPasswordIterations);
    // key to encrypte all other data

    QByteArray iv = crypto.generateInitalizationVector(keyLength);
    SecureArray masterKey = crypto.generateSymetricKey(keyLength);
    QByteArray encryptedMasterKey;
    error = crypto.encryptSymmetric(masterKey, encryptedMasterKey, passwordKey, iv);

    // write master password (master password is encrypted
    QString path = baseDir + "/" + kPathMasterKey;
    database->add(path, encryptedMasterKey);
    path = baseDir + "/" + kPathMasterKeyIV;
    database->add(path, iv);
    path = baseDir + "/" + kPathMasterPasswordKDF;
    database->add(path, kdfName.toAscii());
    path = baseDir + "/" + kPathMasterPasswordAlgo;
    database->add(path, algoName.toAscii());
    path = baseDir + "/" + kPathMasterPasswordSalt;
    database->add(path, salt);
    path = baseDir + "/" + kPathMasterPasswordSize;
    QString keyLengthString;
    QTextStream(&keyLengthString) << keyLength;
    database->add(path, keyLengthString.toAscii());
    path = baseDir + "/" + kPathMasterPasswordIterations;
    QString iterationsString;
    QTextStream(&iterationsString) << kMasterPasswordIterations;
    database->add(path, iterationsString.toAscii());

     // write encrypted public key , private key and certificate
    QByteArray encryptedPrivateKey;
    error = crypto.encryptSymmetric(SecureArray(privateKey.toAscii()), encryptedPrivateKey, passwordKey, iv);
    if (error != 0)
        return -1;

    path = baseDir + "/" + "public_key";
    database->add(path, publicKey.toAscii());
    path = baseDir + "/" + "private_key";
    database->add(path, encryptedPrivateKey);
    path = baseDir + "/" + "certificate";
    database->add(path, certificate.toAscii());

    // test data
    QByteArray testData("Hello id");
    QByteArray encyptedTestData;
    error = crypto.encyrptData(testData, encyptedTestData, certificate.toAscii());
    if (error != 0)
        return -1;

    path = baseDir + "/" + "test_data";
    database->add(path, encyptedTestData);

    error = database->commit();

    return error;
}

QStringList UserIdentity::getIdenties(DatabaseInterface *database, const QString branch)
{
    database->setBranch(branch);
    QStringList list = database->listDirectories("");
    return list;
}

void UserIdentity::printToStream(QTextStream &stream)
{
    fDatabase->setBranch(fBranch);
}

UserIdentity::UserIdentity()
{
}

UserIdentity::UserIdentity(DatabaseInterface *database, const QString id,
                           const SecureArray &password, const QString branch)
    :
    fDatabase(database),
    fBranch(branch),
    fIdentityName(id)
{
    fDatabase->setBranch(fBranch);
    CryptoInterface crypto;

    // write master password (master password is encrypted
    QString path = fIdentityName + "/" + kPathMasterKey;
    QByteArray encryptedMasterKey;
    database->get(path, encryptedMasterKey);
    QByteArray iv;
    path = fIdentityName + "/" + kPathMasterKeyIV;
    database->get(path, iv);
    path = fIdentityName + "/" + kPathMasterPasswordKDF;
    QByteArray kdfName;
    database->get(path, kdfName);
    path = fIdentityName + "/" + kPathMasterPasswordAlgo;
    QByteArray algoName;
    database->get(path, algoName);
    path = fIdentityName + "/" + kPathMasterPasswordSalt;
    QByteArray salt;
    database->get(path, salt);
    path = fIdentityName + "/" + kPathMasterPasswordSize;
    QByteArray masterPasswordSize;
    database->get(path, masterPasswordSize);
    path = fIdentityName + "/" + kPathMasterPasswordIterations;
    QByteArray masterPasswordIterations;
    database->get(path, masterPasswordIterations);

    QTextStream sizeStream(masterPasswordSize);
    unsigned int keyLength;
    sizeStream >> keyLength;
    QTextStream iterationsStream(masterPasswordIterations);
    unsigned int iterations;
    iterationsStream >> iterations;
    // key to encrypte the master key
    SecureArray passwordKey = crypto.deriveKey(password, kdfName, algoName, salt, keyLength, iterations);
    // key to encrypte all other data

    crypto.decryptSymmetric(encryptedMasterKey, fMasterKey, passwordKey, iv);


    // test
    QByteArray publicKey;
    path = fIdentityName + "/" + "public_key";
    database->get(path, publicKey);
    QByteArray encryptedPrivateKey;
    path = fIdentityName + "/" + "private_key";
    database->get(path, encryptedPrivateKey);
    QByteArray certificate;
    path = fIdentityName + "/" + "certificate";
    database->get(path, certificate);

    QByteArray privateKey;
    crypto.decryptSymmetric(encryptedPrivateKey, privateKey, passwordKey, iv);

    // test data
    QByteArray encyptedTestData;
    path = fIdentityName + "/" + "test_data";
    database->get(path, encyptedTestData);
    QByteArray testData;
    crypto.decryptData(encyptedTestData, testData, privateKey, password, certificate);

    printf("test %s\n", testData.data());
}

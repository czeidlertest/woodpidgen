#include "databaseutil.h"

#include <QTextStream>
#include <QUuid>


const char* kPathMasterKey = "master_key";
const char* kPathMasterKeyIV = "master_key_iv";
const char* kPathMasterPasswordKDF = "master_password_kdf";
const char* kPathMasterPasswordAlgo = "master_password_algo";
const char* kPathMasterPasswordSalt = "master_password_salt";
const char* kPathMasterPasswordSize = "master_password_size";
const char* kPathMasterPasswordIterations = "master_password_iterations";

const int kMasterPasswordIterations = 20000;
const int kMasterPasswordLength = 256;

DatabaseEncryption::DatabaseEncryption(DatabaseInterface *database, CryptoInterface *crypto, const QString &branch)
    :
    fDatabase(database),
    fBranch(branch),
    fCrypto(crypto)
{
}

void DatabaseEncryption::setBranch(const QString &branch)
{
    fBranch = branch;
}

int DatabaseEncryption::createNewMasterKey(const SecureArray &password,
                                           SecureArray &masterKey, QByteArray &iv,
                                           const QString &baseDir)
{
    fDatabase->setBranch(fBranch);

    QByteArray salt = fCrypto->generateSalt(QUuid::createUuid().toString());
    const QString kdfName = "pbkdf2";
    const QString algoName = "sha1";

    SecureArray passwordKey = fCrypto->deriveKey(password, kdfName, algoName, salt,
                                               kMasterPasswordLength, kMasterPasswordIterations);

    iv = fCrypto->generateInitalizationVector(kMasterPasswordLength);
    masterKey = fCrypto->generateSymetricKey(kMasterPasswordLength);

    QByteArray encryptedMasterKey;
    int error = fCrypto->encryptSymmetric(masterKey, encryptedMasterKey, passwordKey, iv);
    if (error != 0)
        return error;

    // write master password (master password is encrypted
    QString path = baseDir + "/" + kPathMasterKey;
    fDatabase->add(path, encryptedMasterKey);
    path = baseDir + "/" + kPathMasterKeyIV;
    fDatabase->add(path, iv);
    path = baseDir + "/" + kPathMasterPasswordKDF;
    fDatabase->add(path, kdfName.toAscii());
    path = baseDir + "/" + kPathMasterPasswordAlgo;
    fDatabase->add(path, algoName.toAscii());
    path = baseDir + "/" + kPathMasterPasswordSalt;
    fDatabase->add(path, salt);
    path = baseDir + "/" + kPathMasterPasswordSize;
    QString keyLengthString;
    QTextStream(&keyLengthString) << kMasterPasswordLength;
    fDatabase->add(path, keyLengthString.toAscii());
    path = baseDir + "/" + kPathMasterPasswordIterations;
    QString iterationsString;
    QTextStream(&iterationsString) << kMasterPasswordIterations;
    fDatabase->add(path, iterationsString.toAscii());

    return 0;
}

int DatabaseEncryption::readMasterKey(const SecureArray &password,
                                      SecureArray &masterKey, QByteArray &iv,
                                      const QString &baseDir)
{
    fDatabase->setBranch(fBranch);

    // write master password (master password is encrypted
    QString path = baseDir + "/" + kPathMasterKey;
    QByteArray encryptedMasterKey;
    fDatabase->get(path, encryptedMasterKey);
    path = baseDir + "/" + kPathMasterKeyIV;
    fDatabase->get(path, iv);
    path = baseDir + "/" + kPathMasterPasswordKDF;
    QByteArray kdfName;
    fDatabase->get(path, kdfName);
    path = baseDir + "/" + kPathMasterPasswordAlgo;
    QByteArray algoName;
    fDatabase->get(path, algoName);
    path = baseDir + "/" + kPathMasterPasswordSalt;
    QByteArray salt;
    fDatabase->get(path, salt);
    path = baseDir + "/" + kPathMasterPasswordSize;
    QByteArray masterPasswordSize;
    fDatabase->get(path, masterPasswordSize);
    path = baseDir + "/" + kPathMasterPasswordIterations;
    QByteArray masterPasswordIterations;
    fDatabase->get(path, masterPasswordIterations);

    QTextStream sizeStream(masterPasswordSize);
    unsigned int keyLength;
    sizeStream >> keyLength;
    QTextStream iterationsStream(masterPasswordIterations);
    unsigned int iterations;
    iterationsStream >> iterations;
    // key to encrypte the master key
    SecureArray passwordKey = fCrypto->deriveKey(password, kdfName, algoName, salt, keyLength, iterations);
    // key to encrypte all other data

    return fCrypto->decryptSymmetric(encryptedMasterKey, masterKey, passwordKey, iv);
}

int DatabaseEncryption::commit()
{
    return fDatabase->commit();
}

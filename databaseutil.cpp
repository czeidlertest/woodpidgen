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

const char *kPathSymmetricKey = "symmetric_key";
const char *kPathSymmetricIV = "symmetric_iv";
const char *kPathPrivateKey = "private_key";
const char *kPathPublicKey = "public_key";
const char *kPathCertificate = "certificate";

KeyStore::KeyStore(DatabaseInterface *database, CryptoInterface *crypto, const QString &branch, const QString &baseDir) :
    fDatabase(database),
    fCrypto(crypto),
    fBranch(branch),
    fBaseDir(baseDir)
{
}


int KeyStore::open(const SecureArray &password)
{
    return readMasterKey(password, fMasterKey, fMasterKeyIV, fBaseDir);
}

int KeyStore::create(const SecureArray &password)
{
    return createNewMasterKey(password, fMasterKey, fMasterKeyIV, fBaseDir);
}

QString KeyStore::getKeyStoreId()
{
    fDatabase->setBranch(fBranch);
    // write master password (master password is encrypted
    QString path = fBaseDir + "/" + kPathMasterKey;
    QByteArray encryptedKey;
    int error = fDatabase->get(path, encryptedKey);
    return fCrypto->toHex(fCrypto->sha1Hash(encryptedKey));
}

int KeyStore::createNewMasterKey(const SecureArray &password,
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

int KeyStore::readMasterKey(const SecureArray &password,
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

int KeyStore::addSymmetricKey(const SecureArray &key, const QByteArray &iv, QString &keyId)
{
    QByteArray encryptedKey;
    int error = fCrypto->encryptSymmetric(key, encryptedKey, fMasterKey, fMasterKeyIV);
    if (error != 0)
        return error;
    keyId = fCrypto->toHex(fCrypto->sha1Hash(encryptedKey));

    const QString dir = getDirectory(keyId);
    QString path = dir + "/" + kPathSymmetricKey;
    error = fDatabase->add(path, encryptedKey);
    if (error != 0)
        return error;
    path = dir + "/" + kPathSymmetricIV;
    error = fDatabase->add(path, iv);
    if (error != 0) {
        fDatabase->remove(dir);
        return error;
    }
    return 0;
}

int KeyStore::getSymmetricKey(const QString &keyId, SecureArray &key, QByteArray &iv)
{
    const QString dir = getDirectory(keyId);

    QByteArray encryptedKey;
    QString path = dir + "/" + kPathSymmetricKey;
    int error = fDatabase->get(dir, encryptedKey);
    if (error != 0)
        return error;
    path = dir + "/" + kPathSymmetricIV;
    error = fDatabase->get(dir, iv);
    if (error != 0)
        return error;
    return fCrypto->decryptSymmetric(encryptedKey, key, fMasterKey, fMasterKeyIV);
}

int KeyStore::addAsymmetricKey(const QString &certificate, const QString &publicKey,
                               const QString &privateKey, QString &keyId)
{
    QByteArray encryptedPrivate;
    int error = fCrypto->encryptSymmetric(privateKey.toAscii(), encryptedPrivate, fMasterKey,
                                          fMasterKeyIV);
    if (error != 0)
        return error;
    QByteArray encryptedPublic;
    error = fCrypto->encryptSymmetric(publicKey.toAscii(), encryptedPublic, fMasterKey,
                                      fMasterKeyIV);
    if (error != 0)
        return error;
    QByteArray encryptedCertificate;
    error = fCrypto->encryptSymmetric(certificate.toAscii(), encryptedCertificate, fMasterKey,
                                      fMasterKeyIV);
    if (error != 0)
        return error;

    keyId = fCrypto->toHex(fCrypto->sha1Hash(encryptedPublic));
    const QString dir = getDirectory(keyId);
    QString path = dir + "/" + kPathPrivateKey;
    error = fDatabase->add(path, encryptedPrivate);
    if (error != 0)
        return error;
    path = dir + "/" + kPathPublicKey;
    error = fDatabase->add(path, encryptedPublic);
    if (error != 0) {
        fDatabase->remove(dir);
        return error;
    }
    path = dir + "/" + kPathCertificate;
    error = fDatabase->add(path, encryptedCertificate);
    if (error != 0) {
        fDatabase->remove(dir);
        return error;
    }
    return 0;

}

int KeyStore::getAsymmetricKey(const QString &keyId, QString &certificate, QString &publicKey, QString &privateKey)
{
    const QString dir = getDirectory(keyId);

    QString path = dir + "/" + kPathPrivateKey;
    QByteArray encryptedPrivate;
    int error = fDatabase->get(path, encryptedPrivate);
    if (error != 0)
        return error;
    path = dir + "/" + kPathPublicKey;
    QByteArray encryptedPublic;
    error = fDatabase->get(path, encryptedPublic);
    if (error != 0)
        return error;
    path = dir + "/" + kPathCertificate;
    QByteArray encryptedCertificate;
    error = fDatabase->get(path, encryptedCertificate);
    if (error != 0)
        return error;

    SecureArray decryptedPrivate;
    SecureArray decryptedPublic;
    SecureArray decryptedCertificate;

    error = fCrypto->decryptSymmetric(encryptedPrivate, decryptedPrivate, fMasterKey, fMasterKeyIV);
    if (error != 0)
        return error;
    error = fCrypto->decryptSymmetric(encryptedPublic, decryptedPublic, fMasterKey, fMasterKeyIV);
    if (error != 0)
        return error;
    error = fCrypto->decryptSymmetric(encryptedCertificate, decryptedCertificate, fMasterKey, fMasterKeyIV);
    if (error != 0)
        return error;

    privateKey = decryptedPrivate;
    publicKey = decryptedPublic;
    certificate = decryptedCertificate;
    return 0;
}

int KeyStore::commit()
{
    return fDatabase->commit();
}


CryptoInterface *KeyStore::getCryptoInterface()
{
    return fCrypto;
}

DatabaseInterface *KeyStore::getDatabaseInterface()
{
    return fDatabase;
}

QString KeyStore::getDirectory(const QString &keyId)
{
    QString dir = "";
    if (fBaseDir != "")
        dir += fBaseDir + "/";
    dir += keyId;
    return dir;
}


const char *kPathKeyStoreID = "key_store_id";
const char *kPathKeyStorePath = "key_store_path";
const char *kPathKeyStoreBranch = "key_store_branch";


EncryptedUserData::EncryptedUserData(const QString &path, const QString &branch,
                                     const QString &baseDir) :
    fDatabase(NULL),
    fDatabasePath(path),
    fDatabaseBranch(branch),
    fBaseDir(baseDir)
{
    fCrypto = CryptoInterfaceSingleton::getCryptoInterface();
    DatabaseFactory::open(fDatabasePath, fDatabaseBranch, &fDatabase);
}

EncryptedUserData::~EncryptedUserData()
{
    delete fDatabase;
}

QString EncryptedUserData::getKeyStoreId()
{
    if (fDatabase == NULL)
        return "";

    QString baseDir = "";
    if (fBaseDir != "")
        baseDir += fBaseDir + "/";
    QString path = baseDir + kPathKeyStoreID;
    QByteArray keyStoreIdArray;
    fDatabase->get(path, keyStoreIdArray);

    return QString(keyStoreIdArray);
}

int EncryptedUserData::add(const QString &path, const QByteArray &data)
{
    return fDatabase->add(path, data);
}

int EncryptedUserData::get(const QString &path, QByteArray &data) const
{
    return fDatabase->get(path, data);
}

int EncryptedUserData::commit()
{
    return fDatabase->commit();
}

int EncryptedUserData::addSafe(const QString &path, const QByteArray &data, const QString &keyId)
{
    SecureArray key;
    QByteArray iv;
    int error = fKeyStore->getSymmetricKey(keyId, key, iv);
    if (error != 0)
        return error;

    QByteArray encrypted;
    error = fCrypto->encryptSymmetric(data, encrypted, key, iv);
    if (error != 0)
        return error;
    return add(path, encrypted);
}

int EncryptedUserData::getSafe(const QString &path, QByteArray &data, const QString &keyId) const
{
    SecureArray key;
    QByteArray iv;
    int error = fKeyStore->getSymmetricKey(keyId, key, iv);
    if (error != 0)
        return error;

    QByteArray encrypted;
    error = get(path, encrypted);
    if (error != 0)
        return error;
    return fCrypto->decryptSymmetric(encrypted, data, key, iv);
}


int EncryptedUserData::setTo(KeyStore *keyStore)
{
    fKeyStore = keyStore;
    return 0;
}

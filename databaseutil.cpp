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

const char *kPathUniqueId = "uid";

UserData::UserData() :
    fDatabase(NULL),
    fInitStatus(WP::kUninit)
{
    fCrypto = CryptoInterfaceSingleton::getCryptoInterface();
}

UserData::~UserData()
{
    delete fDatabase;
}

WP::err UserData::initCheck() const
{
    return fInitStatus;
}

WP::err UserData::commit()
{
    return fDatabase->commit();
}

QString UserData::getUid() const
{
    return fUid;
}

WP::err UserData::write(const QString &path, const QByteArray &data)
{
    return fDatabase->write(path, data);
}

WP::err UserData::write(const QString &path, const QString &data)
{
    return fDatabase->write(path, data);
}

WP::err UserData::read(const QString &path, QByteArray &data) const
{
    return fDatabase->read(path, data);
}

WP::err UserData::read(const QString &path, QString &data) const
{
    return fDatabase->read(path, data);
}

WP::err UserData::setUid(const QString &uid)
{
    if (fInitStatus != WP::kOk)
        return fInitStatus;
    WP::err status = fDatabase->write(prependBaseDir(kPathUniqueId), uid);
    if (status != WP::kOk)
        return status;
    fUid = uid;
    return WP::kOk;
}

WP::err UserData::setToDatabase(const QString &path, const QString &branch, const QString &baseDir, bool createDatabase)
{
    fDatabasePath = path;
    fDatabaseBranch = branch;
    fDatabaseBaseDir = baseDir;
    fInitStatus = DatabaseFactory::open(fDatabasePath, fDatabaseBranch, &fDatabase);
    if (fInitStatus != WP::kOk)
        return fInitStatus;

    fDatabase->read(prependBaseDir(kPathUniqueId), fUid);

    return WP::kOk;
}

void UserData::setBaseDir(const QString &baseDir)
{
    fDatabaseBaseDir = baseDir;
}

QString UserData::prependBaseDir(const QString &path) const
{
    if (fDatabaseBaseDir == "")
        return path;
    QString completePath = path;
    completePath.prepend(fDatabaseBaseDir + "/");
    return completePath;
}


KeyStore::KeyStore(const QString &path, const QString &branch, const QString &baseDir)
{
    setToDatabase(path, branch, baseDir);
}


WP::err KeyStore::open(const SecureArray &password)
{
    return readMasterKey(password, fMasterKey, fMasterKeyIV, fDatabaseBaseDir);
}

WP::err KeyStore::create(const SecureArray &password, bool addUidToBaseDir)
{
    QByteArray salt = fCrypto->generateSalt(QUuid::createUuid().toString());
    const QString kdfName = "pbkdf2";
    const QString algoName = "sha1";

    SecureArray passwordKey = fCrypto->deriveKey(password, kdfName, algoName, salt,
                                               kMasterPasswordLength, kMasterPasswordIterations);

    fMasterKeyIV = fCrypto->generateInitalizationVector(kMasterPasswordLength);
    fMasterKey = fCrypto->generateSymetricKey(kMasterPasswordLength);

    QByteArray encryptedMasterKey;
    WP::err error = fCrypto->encryptSymmetric(fMasterKeyIV, encryptedMasterKey, passwordKey, fMasterKeyIV);
    if (error != WP::kOk)
        return error;

    QString uid = fCrypto->toHex(fCrypto->sha1Hash(encryptedMasterKey));
    if (addUidToBaseDir)
        setBaseDir(fDatabaseBaseDir + "/" + uid);
    // write uid
    setUid(uid);

    // write master password (master password is encrypted
    QString path = fDatabaseBaseDir + "/" + kPathMasterKey;
    fDatabase->write(path, encryptedMasterKey);
    path = fDatabaseBaseDir + "/" + kPathMasterKeyIV;
    fDatabase->write(path, fMasterKeyIV);
    path = fDatabaseBaseDir + "/" + kPathMasterPasswordKDF;
    fDatabase->write(path, kdfName.toAscii());
    path = fDatabaseBaseDir + "/" + kPathMasterPasswordAlgo;
    fDatabase->write(path, algoName.toAscii());
    path = fDatabaseBaseDir + "/" + kPathMasterPasswordSalt;
    fDatabase->write(path, salt);
    path = fDatabaseBaseDir + "/" + kPathMasterPasswordSize;
    QString keyLengthString;
    QTextStream(&keyLengthString) << kMasterPasswordLength;
    fDatabase->write(path, keyLengthString.toAscii());
    path = fDatabaseBaseDir + "/" + kPathMasterPasswordIterations;
    QString iterationsString;
    QTextStream(&iterationsString) << kMasterPasswordIterations;
    fDatabase->write(path, iterationsString.toAscii());

    return WP::kOk;
}

WP::err KeyStore::readMasterKey(const SecureArray &password,
                                SecureArray &masterKey, QByteArray &iv,
                                const QString &baseDir)
{
    // write master password (master password is encrypted
    QString path = baseDir + "/" + kPathMasterKey;
    QByteArray encryptedMasterKey;
    fDatabase->read(path, encryptedMasterKey);
    path = baseDir + "/" + kPathMasterKeyIV;
    fDatabase->read(path, iv);
    path = baseDir + "/" + kPathMasterPasswordKDF;
    QByteArray kdfName;
    fDatabase->read(path, kdfName);
    path = baseDir + "/" + kPathMasterPasswordAlgo;
    QByteArray algoName;
    fDatabase->read(path, algoName);
    path = baseDir + "/" + kPathMasterPasswordSalt;
    QByteArray salt;
    fDatabase->read(path, salt);
    path = baseDir + "/" + kPathMasterPasswordSize;
    QByteArray masterPasswordSize;
    fDatabase->read(path, masterPasswordSize);
    path = baseDir + "/" + kPathMasterPasswordIterations;
    QByteArray masterPasswordIterations;
    fDatabase->read(path, masterPasswordIterations);

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

WP::err KeyStore::writeSymmetricKey(const SecureArray &key, const QByteArray &iv, QString &keyId)
{
    QByteArray encryptedKey;
    WP::err error = fCrypto->encryptSymmetric(key, encryptedKey, fMasterKey, fMasterKeyIV);
    if (error != WP::kOk)
        return error;
    keyId = fCrypto->toHex(fCrypto->sha1Hash(encryptedKey));

    const QString dir = getDirectory(keyId);
    QString path = dir + "/" + kPathSymmetricKey;
    error = fDatabase->write(path, encryptedKey);
    if (error != WP::kOk)
        return error;
    path = dir + "/" + kPathSymmetricIV;
    error = fDatabase->write(path, iv);
    if (error != WP::kOk) {
        fDatabase->remove(dir);
        return error;
    }
    return WP::kOk;
}

WP::err KeyStore::readSymmetricKey(const QString &keyId, SecureArray &key, QByteArray &iv)
{
    const QString dir = getDirectory(keyId);

    QByteArray encryptedKey;
    QString path = dir + "/" + kPathSymmetricKey;
    WP::err error = fDatabase->read(dir, encryptedKey);
    if (error != WP::kOk)
        return error;
    path = dir + "/" + kPathSymmetricIV;
    error = fDatabase->read(dir, iv);
    if (error != WP::kOk)
        return error;
    return fCrypto->decryptSymmetric(encryptedKey, key, fMasterKey, fMasterKeyIV);
}

WP::err KeyStore::writeAsymmetricKey(const QString &certificate, const QString &publicKey,
                               const QString &privateKey, QString &keyId)
{
    QByteArray encryptedPrivate;
    WP::err error = fCrypto->encryptSymmetric(privateKey.toAscii(), encryptedPrivate, fMasterKey,
                                          fMasterKeyIV);
    if (error != WP::kOk)
        return error;
    QByteArray encryptedPublic;
    error = fCrypto->encryptSymmetric(publicKey.toAscii(), encryptedPublic, fMasterKey,
                                      fMasterKeyIV);
    if (error != WP::kOk)
        return error;
    QByteArray encryptedCertificate;
    error = fCrypto->encryptSymmetric(certificate.toAscii(), encryptedCertificate, fMasterKey,
                                      fMasterKeyIV);
    if (error != WP::kOk)
        return error;

    keyId = fCrypto->toHex(fCrypto->sha1Hash(encryptedPublic));
    const QString dir = getDirectory(keyId);
    QString path = dir + "/" + kPathPrivateKey;
    error = fDatabase->write(path, encryptedPrivate);
    if (error != WP::kOk)
        return error;
    path = dir + "/" + kPathPublicKey;
    error = fDatabase->write(path, encryptedPublic);
    if (error != WP::kOk) {
        fDatabase->remove(dir);
        return error;
    }
    path = dir + "/" + kPathCertificate;
    error = fDatabase->write(path, encryptedCertificate);
    if (error != WP::kOk) {
        fDatabase->remove(dir);
        return error;
    }
    return WP::kOk;

}

WP::err KeyStore::readAsymmetricKey(const QString &keyId, QString &certificate, QString &publicKey, QString &privateKey)
{
    const QString dir = getDirectory(keyId);

    QString path = dir + "/" + kPathPrivateKey;
    QByteArray encryptedPrivate;
    WP::err error = fDatabase->read(path, encryptedPrivate);
    if (error != WP::kOk)
        return error;
    path = dir + "/" + kPathPublicKey;
    QByteArray encryptedPublic;
    error = fDatabase->read(path, encryptedPublic);
    if (error != WP::kOk)
        return error;
    path = dir + "/" + kPathCertificate;
    QByteArray encryptedCertificate;
    error = fDatabase->read(path, encryptedCertificate);
    if (error != WP::kOk)
        return error;

    SecureArray decryptedPrivate;
    SecureArray decryptedPublic;
    SecureArray decryptedCertificate;

    error = fCrypto->decryptSymmetric(encryptedPrivate, decryptedPrivate, fMasterKey, fMasterKeyIV);
    if (error != WP::kOk)
        return error;
    error = fCrypto->decryptSymmetric(encryptedPublic, decryptedPublic, fMasterKey, fMasterKeyIV);
    if (error != WP::kOk)
        return error;
    error = fCrypto->decryptSymmetric(encryptedCertificate, decryptedCertificate, fMasterKey, fMasterKeyIV);
    if (error != WP::kOk)
        return error;

    privateKey = decryptedPrivate;
    publicKey = decryptedPublic;
    certificate = decryptedCertificate;
    return WP::kOk;
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
    if (fDatabaseBaseDir != "")
        dir += fDatabaseBaseDir + "/";
    dir += keyId;
    return dir;
}


const char *kPathKeyStoreID = "key_store_id";
const char *kPathDefaultKeyID = "default_key_id";
const char *kPathKeyStorePath = "key_store_path";
const char *kPathKeyStoreBranch = "key_store_branch";


EncryptedUserData::EncryptedUserData() :
    fKeyStore(NULL)
{
}

EncryptedUserData::~EncryptedUserData()
{
}

WP::err EncryptedUserData::initCheck()
{
    WP::err status = UserData::initCheck();
    if (status != WP::kOk)
        return status;
    if (fKeyStore == NULL)
        return WP::kNoKeyStore;
    return WP::kOk;
}

WP::err EncryptedUserData::readKeyStoreId(QString &keyStoreId) const
{
    if (fDatabase == NULL)
        return WP::kUninit;

    QString path = prependBaseDir(kPathKeyStoreID);
    fDatabase->read(path, keyStoreId);

    return WP::kOk;
}

WP::err EncryptedUserData::writeSafe(const QString &path, const QByteArray &data)
{
    return writeSafe(path, data, fDefaultKeyId);
}

WP::err EncryptedUserData::readSafe(const QString &path, QByteArray &data) const
{
    return readSafe(path, data, fDefaultKeyId);
}

WP::err EncryptedUserData::writeSafe(const QString &path, const QByteArray &data, const QString &keyId)
{
    SecureArray key;
    QByteArray iv;
    WP::err error = fKeyStore->readSymmetricKey(keyId, key, iv);
    if (error != WP::kOk)
        return error;

    QByteArray encrypted;
    error = fCrypto->encryptSymmetric(data, encrypted, key, iv);
    if (error != WP::kOk)
        return error;
    return write(path, encrypted);
}

WP::err EncryptedUserData::readSafe(const QString &path, QByteArray &data, const QString &keyId) const
{
    SecureArray key;
    QByteArray iv;
    WP::err error = fKeyStore->readSymmetricKey(keyId, key, iv);
    if (error != WP::kOk)
        return error;

    QByteArray encrypted;
    error = read(path, encrypted);
    if (error != WP::kOk)
        return error;
    return fCrypto->decryptSymmetric(encrypted, data, key, iv);
}

WP::err EncryptedUserData::writeKeyStoreId()
{
    QString path = prependBaseDir(kPathKeyStoreID);
    return write(path, fKeyStore->getUid());
}


WP::err EncryptedUserData::setKeyStore(KeyStore *keyStore)
{
    fKeyStore = keyStore;
    return WP::kOk;
}

QString EncryptedUserData::getDefaultKeyId() const
{
    return fDefaultKeyId;
}

WP::err EncryptedUserData::setDefaultKeyId(const QString &keyId)
{
    WP::err error = write(prependBaseDir(kPathDefaultKeyID), keyId);
    if (error != WP::kOk)
        return error;
    fDefaultKeyId = keyId;
    return WP::kOk;
}

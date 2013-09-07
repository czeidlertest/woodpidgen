#include "useridentity.h"

#include <QUuid>

#include "cryptointerface.h"


const char* kPathMasterKey = "master_key";
const char* kPathMasterKeyKDF = "master_key_kdf";
const char* kPathMasterKeyAlgo = "master_key_algo";
const char* kPathMasterKeySalt = "master_key_salt";
const char* kPathMasterKeySize = "master_key_size";

const int kMasterKeyIterations = 20000;


int UserIdentity::createNewIdentity(UserIdentity &id, DatabaseInterface *database, const QCA::SecureArray &password, const QString branch)
{
    database->setBranch(branch);

    CryptoInterface crypto;

    QByteArray salt = crypto.generateSalt(QUuid::createUuid().toString());
    QString kdfName = "pbkdf2";
    QString algoName = "sha1";
    int keyLength = 256;

    QString certificate;
    QString publicKey;
    QString privateKey;
    int error = crypto.generateKeyPair(certificate, publicKey, privateKey, password);
    if (error != 0)
        return -1;

    QCA::Hash shaHash("sha1");
    shaHash.update(certificate.toStdString().c_str());
    QByteArray hashResult = shaHash.final().toByteArray();

    const QString baseDir = hashResult;

    // key to encrypte the master key
    QCA::SymmetricKey passwordKey = crypto.deriveKey(password, kdfName, algoName, salt, keyLength, kMasterKeyIterations);

    // key to encrypte all other data

    QByteArray iv = crypto.generateInitalizationVector(keyLength);
    QCA::SecureArray masterKey = crypto.generateSymetricKey(keyLength);

    QByteArray encryptedMasterKey;
    error = crypto.encryptSymmetric(masterKey, encryptedMasterKey, passwordKey, iv);

    QString path = baseDir + "/" + kPathMasterKey;
    database->add(path, masterKey.toByteArray());

     // write master password

    return 0;
}

UserIdentity::UserIdentity(DatabaseInterface *database, const QCA::SecureArray& password, const QString branch)
    :
    fDatabase(database),
    fBranch(branch)
{
}

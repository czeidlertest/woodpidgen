#ifndef DATABASEUTIL_H
#define DATABASEUTIL_H

#include <cryptointerface.h>
#include <databaseinterface.h>
#include <error_codes.h>


class UserData {
public:
    UserData();
    virtual ~UserData();

    virtual WP::err initCheck() const;

    WP::err commit();

    QString getUid() const;

    // convenient functions to DatabaseInterface
    WP::err write(const QString& path, const QByteArray& data);
    WP::err write(const QString& path, const QString& data);
    WP::err read(const QString& path, QByteArray& data) const;
    WP::err read(const QString& path, QString& data) const;

protected:
    WP::err setUid(const QString &uid);
    WP::err setToDatabase(const QString &path, const QString &branch, const QString &baseDir = "",
                          bool createDatabase = true);
    void setBaseDir(const QString &baseDir);

    QString prependBaseDir(const QString &path) const;

protected:
    QString fDatabasePath;
    QString fDatabaseBranch;
    QString fDatabaseBaseDir;

    CryptoInterface *fCrypto;
    DatabaseInterface *fDatabase;

private:
    WP::err fInitStatus;
    QString fUid;
};

class KeyStore : public UserData {
public:
    KeyStore(const QString &path, const QString &branch = "keystore", const QString &baseDir = "");

    WP::err open(const SecureArray &password);
    WP::err create(const SecureArray &password, bool addUidToBaseDir = true);

    WP::err writeSymmetricKey(const SecureArray &key, const QByteArray &iv, QString &keyId);
    WP::err readSymmetricKey(const QString &keyId, SecureArray &key, QByteArray &iv);

    WP::err writeAsymmetricKey(const QString &certificate, const QString &publicKey,
                         const QString &privateKey, QString &keyId);
    WP::err readAsymmetricKey(const QString &keyId, QString &certificate, QString &publicKey,
                         QString &privateKey);

    WP::err removeKey(const QString &id);

    CryptoInterface* getCryptoInterface();
    DatabaseInterface* getDatabaseInterface();

private:
    QString getDirectory(const QString &keyId);

    WP::err readMasterKey(const SecureArray &password, SecureArray &masterKey, QByteArray& iv,
                      const QString &baseDir = "");

protected:
    SecureArray fMasterKey;
    QByteArray fMasterKeyIV;
};


class EncryptedUserData : public UserData {
public:
    EncryptedUserData();
    virtual ~EncryptedUserData();

    virtual WP::err initCheck();

    /*! To initialize a EncryptedUserData the returned key store id should be used to load the
     *  keyStore.*/
    virtual WP::err readKeyStoreId(QString &keyStoreId) const;
    virtual WP::err setKeyStore(KeyStore *keyStore);

    QString getDefaultKeyId() const;
    WP::err setDefaultKeyId(const QString &keyId);

    // add and get encrypted data using the default key
    WP::err writeSafe(const QString& path, const QByteArray& data);
    WP::err readSafe(const QString& path, QByteArray& data) const;
    // add and get encrypted data
    WP::err writeSafe(const QString& path, const QByteArray& data, const QString &keyId);
    WP::err readSafe(const QString& path, QByteArray& data, const QString &keyId) const;

protected:
    WP::err writeKeyStoreId();

    KeyStore *fKeyStore;
    QString fDefaultKeyId;
};


#endif // DATABASEUTIL_H

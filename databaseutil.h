#ifndef DATABASEUTIL_H
#define DATABASEUTIL_H

#include <qobject.h>

#include "cryptointerface.h"
#include "databaseinterface.h"
#include "error_codes.h"

/*
class EncryptedUserData;

class StorageDirectory {
public:
    StorageDirectory(EncryptedUserData *database, const QString &directory);

    WP::err write(const QString& path, const QByteArray& data);
    WP::err write(const QString& path, const QString& data);
    WP::err read(const QString& path, QByteArray& data) const;
    WP::err read(const QString& path, QString& data) const;
    WP::err writeSafe(const QString& path, const QString& data);
    WP::err writeSafe(const QString& path, const QByteArray& data);
    WP::err writeSafe(const QString& path, const QByteArray& data, const QString &keyId);
    WP::err readSafe(const QString& path, QString& data) const;
    WP::err readSafe(const QString& path, QByteArray& data) const;
    WP::err readSafe(const QString& path, QByteArray& data, const QString &keyId) const;

    WP::err remove(const QString& path);

    const QString& directory();

protected:
    EncryptedUserData *fDatabase;
    QString fDirectory;
};
*/

class RemoteDataStorage;
class RemoteConnection;

class DatabaseBranch {
public:
    DatabaseBranch(const QString &getDatabasePath, const QString &getBranch);
    ~DatabaseBranch();

    void setTo(const QString &getDatabasePath, const QString &getBranch);
    const QString &getDatabasePath() const;
    const QString &getBranch() const;
    QString databaseHash() const;
    DatabaseInterface *getDatabase() const;

    WP::err commit();
    int countRemotes() const;
    RemoteDataStorage *getRemoteAt(int i) const;
    RemoteConnection *getRemoteConnectionAt(int i) const;
    WP::err addRemote(RemoteDataStorage* data);
private:
    QString fDatabasePath;
    QString fBranch;
    QList<RemoteDataStorage*> fRemotes;
    DatabaseInterface *fDatabase;
};


class UserData : public QObject {
Q_OBJECT
public:
    UserData();
    virtual ~UserData();

    virtual WP::err writeConfig();

    QString getUid();

    // convenient functions to DatabaseInterface
    WP::err write(const QString& path, const QByteArray& data);
    WP::err write(const QString& path, const QString& data);
    WP::err read(const QString& path, QByteArray& data) const;
    WP::err read(const QString& path, QString& data) const;
    WP::err remove(const QString& path);

    QStringList listDirectories(const QString &path) const;

    const DatabaseBranch *getDatabaseBranch() const;
    QString getDatabaseBaseDir() const;
    DatabaseInterface *getDatabase() const;
    void setBaseDir(const QString &baseDir);

protected:
    void setUid(const QString &uid);
    WP::err setToDatabase(const DatabaseBranch *branch, const QString &baseDir = "");

    QString prependBaseDir(const QString &path) const;

protected:
    const DatabaseBranch *fDatabaseBranch;
    QString fDatabaseBaseDir;

    CryptoInterface *fCrypto;
    DatabaseInterface *fDatabase;

private:
    QString fUid;
};

class KeyStore : public UserData {
public:
    KeyStore(const DatabaseBranch *branch, const QString &baseDir = "");

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

protected:
    SecureArray fMasterKey;
    QByteArray fMasterKeyIV;
};

class KeyStoreFinder {
public:
    virtual ~KeyStoreFinder() {}
    virtual KeyStore *find(const QString &keyStoreId) = 0;
};

class EncryptedUserData : public UserData {
public:
    EncryptedUserData(const EncryptedUserData& data);
    EncryptedUserData();
    virtual ~EncryptedUserData();

    void setTo(EncryptedUserData *database, const QString &baseDir);

    virtual WP::err writeConfig();
    virtual WP::err readKeyStore(KeyStoreFinder *keyStoreFinder);

    KeyStore *getKeyStore() const;
    void setKeyStore(KeyStore *keyStore);

    QString getDefaultKeyId() const;
    void setDefaultKeyId(const QString &keyId);

    // add and get encrypted data using the default key
    WP::err writeSafe(const QString& path, const QByteArray& data);
    WP::err readSafe(const QString& path, QByteArray& data) const;
    WP::err writeSafe(const QString& path, const QString& data);
    WP::err readSafe(const QString& path, QString& data) const;
    // add and get encrypted data
    WP::err writeSafe(const QString& path, const QString& data, const QString &keyId);
    WP::err writeSafe(const QString& path, const QByteArray& data, const QString &keyId);
    WP::err readSafe(const QString& path, QString& data, const QString &keyId) const;
    WP::err readSafe(const QString& path, QByteArray& data, const QString &keyId) const;

protected:
    KeyStore *fKeyStore;
    QString fDefaultKeyId;
};


class RemoteDataStorage : public EncryptedUserData {
public:
    RemoteDataStorage();
    RemoteDataStorage(const DatabaseBranch *database, const QString &baseDir);
    virtual ~RemoteDataStorage() {}

    RemoteConnection *getRemoteConnection();

    virtual QString type() const = 0;
    virtual WP::err open(KeyStoreFinder *keyStoreFinder) = 0;
protected:
    RemoteConnection *fConnection;
};

#endif // DATABASEUTIL_H

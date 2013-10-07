#ifndef DATABASEUTIL_H
#define DATABASEUTIL_H

#include <cryptointerface.h>
#include <databaseinterface.h>


class KeyStore {
public:
    KeyStore(DatabaseInterface *database, CryptoInterface *crypto,
             const QString &branch = "keystore", const QString &baseDir = "");

    int open(const SecureArray &password);
    int create(const SecureArray &password);

    QString getKeyStoreId();

    int addSymmetricKey(const SecureArray &key, const QByteArray &iv, QString &keyId);
    int getSymmetricKey(const QString &keyId, SecureArray &key, QByteArray &iv);

    int addAsymmetricKey(const QString &certificate, const QString &publicKey,
                         const QString &privateKey, QString &keyId);
    int getAsymmetricKey(const QString &keyId, QString &certificate, QString &publicKey,
                         QString &privateKey);

    int removeKey(const QString &id);

    int commit();

    CryptoInterface* getCryptoInterface();
    DatabaseInterface* getDatabaseInterface();

private:
    QString getDirectory(const QString &keyId);

    int createNewMasterKey(const SecureArray &password, SecureArray& masterKey, QByteArray& iv,
                           const QString &baseDir = "");
    int readMasterKey(const SecureArray &password, SecureArray &masterKey, QByteArray& iv,
                      const QString &baseDir = "");

protected:
    DatabaseInterface *fDatabase;
    CryptoInterface *fCrypto;
    QString fBranch;
    QString fBaseDir;

    SecureArray fMasterKey;
    QByteArray fMasterKeyIV;
};


class EncryptedUserData {
public:
    EncryptedUserData(const QString &path, const QString &branch, const QString &baseDir = "");
    virtual ~EncryptedUserData();

    /*! To initialize a EncryptedUserData the returned key store id should be used to load the
     *  keyStore.*/
    virtual QString getKeyStoreId();
    virtual int setTo(KeyStore *keyStore);

    // convenient functions to DatabaseInterface
    int add(const QString& path, const QByteArray& data);
    int get(const QString& path, QByteArray& data) const;
    int commit();

    // add and get encrypted data
    int addSafe(const QString& path, const QByteArray& data, const QString &keyId);
    int getSafe(const QString& path, QByteArray& data, const QString &keyId) const;

protected:
    QString fDatabasePath;
    QString fDatabaseBranch;
    QString fBaseDir;

    CryptoInterface *fCrypto;
    DatabaseInterface *fDatabase;
    KeyStore *fKeyStore;
};


#endif // DATABASEUTIL_H

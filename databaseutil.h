#ifndef DATABASEUTIL_H
#define DATABASEUTIL_H

#include <cryptointerface.h>
#include <databaseinterface.h>


class DatabaseEncryption
{
public:
    DatabaseEncryption(DatabaseInterface *database, CryptoInterface *crypto, const QString &branch);

    void setBranch(const QString &branch);

    int createNewMasterKey(const SecureArray &password, SecureArray& masterKey, QByteArray& iv,
                           const QString &baseDir ="");
    int readMasterKey(const SecureArray &password, SecureArray &masterKey, QByteArray& iv,
                      const QString &baseDir = "");

    int commit();

protected:
    DatabaseInterface *fDatabase;
    QString fBranch;
    CryptoInterface *fCrypto;
};

#endif // DATABASEUTIL_H

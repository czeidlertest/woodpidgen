#ifndef PHPREMOTESTORAGE_H
#define PHPREMOTESTORAGE_H

#include "databaseutil.h"
#include "serverconnection.h"


class PHPConnectionManager
{
public:
    EncryptedPHPConnection *connectionFor(const QString &url);

private:
    QMap<QString, EncryptedPHPConnection*> fConnections;
};


class PHPRemoteStorage : public RemoteDataStorage
{
public:
    PHPRemoteStorage(const QString &url);
    PHPRemoteStorage(const DatabaseBranch *database, const QString &baseDir);

    virtual QString type() const;
    virtual WP::err writeConfig();
    virtual WP::err open(KeyStoreFinder *keyStoreFinder);

private:
    QString hash();

    static PHPConnectionManager sPHPConnectionManager;

    QString fUrl;
};

#endif // PHPREMOTESTORAGE_H

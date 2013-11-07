#ifndef PHPREMOTESTORAGE_H
#define PHPREMOTESTORAGE_H

#include "databaseutil.h"
#include "serverconnection.h"


template<class Type>
class ConnectionManager
{
public:
    Type *connectionFor(const QString &url) {
        Type *connection = NULL;
        typename QMap<QString, Type*>::iterator it = fConnections.find(url);
        if (it != fConnections.end()) {
            connection = it.value();
        } else {
            connection = new Type(QUrl(url));
        if (connection == NULL)
            return NULL;
            fConnections[url] = connection;
        }
        return connection;
    }

private:
    QMap<QString, Type*> fConnections;
};

class URLRemoteStorage : public RemoteDataStorage
{
public:
    URLRemoteStorage(const QString &url);
    URLRemoteStorage(const DatabaseBranch *database, const QString &baseDir);

    virtual WP::err writeConfig();
    virtual WP::err open(KeyStoreFinder *keyStoreFinder);

protected:
    virtual QString hash();
    QString fUrl;
};


class HTTPRemoteStorage : public URLRemoteStorage
{
public:
    HTTPRemoteStorage(const QString &url);
    HTTPRemoteStorage(const DatabaseBranch *database, const QString &baseDir);

    virtual QString type() const;
    virtual WP::err open(KeyStoreFinder *keyStoreFinder);

private:
    static ConnectionManager<HTTPConnection> sHTTPConnectionManager;
};


class PHPEncryptedRemoteStorage : public URLRemoteStorage
{
public:
    PHPEncryptedRemoteStorage(const QString &url);
    PHPEncryptedRemoteStorage(const DatabaseBranch *database, const QString &baseDir);

    virtual QString type() const;
    virtual WP::err open(KeyStoreFinder *keyStoreFinder);

private:
    static ConnectionManager<EncryptedPHPConnection> sPHPConnectionManager;
};

#endif // PHPREMOTESTORAGE_H

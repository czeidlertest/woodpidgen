#include "phpremotestorage.h"


PHPConnectionManager PHPRemoteStorage::sPHPConnectionManager;

PHPRemoteStorage::PHPRemoteStorage(const QString &url) :
    fUrl(url)
{
    setUid(hash());
}

PHPRemoteStorage::PHPRemoteStorage(const DatabaseBranch *database, const QString &baseDir) :
    RemoteDataStorage(database, baseDir)
{
}


QString PHPRemoteStorage::type() const
{
    return "PHPRemoteStorage";
}

WP::err PHPRemoteStorage::open(KeyStoreFinder *keyStoreFinder)
{
    WP::err error = RemoteDataStorage::readKeyStore(keyStoreFinder);
    if (error != WP::kOk)
        return error;

    error = readSafe("url", fUrl);
    if (error != WP::kOk)
        return error;

    fConnection = sPHPConnectionManager.connectionFor(fUrl);
    if (fConnection == NULL)
        return WP::kError;

    setUid(hash());

    return WP::kOk;
}

WP::err PHPRemoteStorage::writeConfig()
{
    WP::err error = RemoteDataStorage::writeConfig();
    if (error != WP::kOk)
        return error;

    error = writeSafe("url", fUrl);
    if (error != WP::kOk)
        return error;

    return error;
}

QString PHPRemoteStorage::hash()
{
    QByteArray hashResult = fCrypto->sha1Hash(fUrl.toLatin1());
    QString uid = fCrypto->toHex(hashResult);
    return uid;
}

EncryptedPHPConnection *PHPConnectionManager::connectionFor(const QString &url)
{
    EncryptedPHPConnection *connection = NULL;
    QMap<QString, EncryptedPHPConnection*>::iterator it = fConnections.find(url);
    if (it != fConnections.end()) {
        connection = it.value();
    } else {
        connection = new EncryptedPHPConnection(QUrl(url));
        if (connection == NULL)
            return NULL;
        fConnections[url] = connection;
    }
    return connection;
}

#include "phpremotestorage.h"


URLRemoteStorage::URLRemoteStorage(const QString &url) :
    fUrl(url)
{
    setUid(hash());
}

URLRemoteStorage::URLRemoteStorage(const DatabaseBranch *database, const QString &baseDir) :
    RemoteDataStorage(database, baseDir)
{
}

QString URLRemoteStorage::hash()
{
    QByteArray hashResult = fCrypto->sha1Hash(fUrl.toLatin1());
    QString uid = fCrypto->toHex(hashResult);
    return uid;
}

WP::err URLRemoteStorage::writeConfig()
{
    WP::err error = RemoteDataStorage::writeConfig();
    if (error != WP::kOk)
        return error;

    error = writeSafe("url", fUrl);
    if (error != WP::kOk)
        return error;

    return error;
}

WP::err URLRemoteStorage::open(KeyStoreFinder *keyStoreFinder)
{
    WP::err error = RemoteDataStorage::readKeyStore(keyStoreFinder);
    if (error != WP::kOk)
        return error;

    error = readSafe("url", fUrl);
    if (error != WP::kOk)
        return error;
    return error;
}


ConnectionManager<HTTPConnection> HTTPRemoteStorage::sHTTPConnectionManager;

HTTPRemoteStorage::HTTPRemoteStorage(const QString &url) :
    URLRemoteStorage(url)
{
    fConnection = sHTTPConnectionManager.connectionFor(fUrl);
}

HTTPRemoteStorage::HTTPRemoteStorage(const DatabaseBranch *database, const QString &baseDir) :
    URLRemoteStorage(database, baseDir)
{

}

QString HTTPRemoteStorage::type() const
{
    return "HTTPRemoteStorage";
}

WP::err HTTPRemoteStorage::open(KeyStoreFinder *keyStoreFinder)
{
    WP::err error = URLRemoteStorage::open(keyStoreFinder);
    if (error != WP::kOk)
        return error;

    fConnection = sHTTPConnectionManager.connectionFor(fUrl);
    if (fConnection == NULL)
        return WP::kError;

    setUid(hash());

    return WP::kOk;
}

ConnectionManager<EncryptedPHPConnection> PHPEncryptedRemoteStorage::sPHPConnectionManager;

PHPEncryptedRemoteStorage::PHPEncryptedRemoteStorage(const QString &url) :
    URLRemoteStorage(url)
{
    fConnection = sPHPConnectionManager.connectionFor(fUrl);
}

PHPEncryptedRemoteStorage::PHPEncryptedRemoteStorage(const DatabaseBranch *database, const QString &baseDir) :
    URLRemoteStorage(database, baseDir)
{
}


QString PHPEncryptedRemoteStorage::type() const
{
    return "PHPEncryptedRemoteStorage";
}

WP::err PHPEncryptedRemoteStorage::open(KeyStoreFinder *keyStoreFinder)
{
    WP::err error = URLRemoteStorage::open(keyStoreFinder);
    if (error != WP::kOk)
        return error;

    fConnection = sPHPConnectionManager.connectionFor(fUrl);
    if (fConnection == NULL)
        return WP::kError;

    setUid(hash());

    return WP::kOk;
}

#ifndef REMOTESTORAGE_H
#define REMOTESTORAGE_H

#include "databaseutil.h"
#include "remoteconnection.h"


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


class Profile;
class RemoteAuthentication;

class RemoteDataStorage {
public:
    RemoteDataStorage(Profile *profile);
    virtual ~RemoteDataStorage();

    WP::err write(StorageDirectory &dir);
    WP::err load(StorageDirectory &dir);

    QString getUid();

    const QString &getConnectionType();
    const QString &getUrl();

    const QString &getAuthType();
    const QString &getAuthUserName();
    const QString &getAuthKeyStoreId();
    const QString &getAuthKeyId();

    RemoteConnection *getRemoteConnection();
    RemoteAuthentication *getRemoteAuthentication();

private:
    friend class Profile;

    void setPHPEncryptedRemoteConnection(const QString &url);
    void setHTTPRemoteConnection(const QString &url);

    void setSignatureAuth(const QString &userName, const QString &keyStoreId, const QString &keyId);

    QString hash();

    static ConnectionManager<HTTPConnection> sHTTPConnectionManager;
    static ConnectionManager<EncryptedPHPConnection> sPHPConnectionManager;

    Profile *fProfile;

    RemoteConnection *fConnection;
    RemoteAuthentication *fAuthentication;

    QString fConnectionType;
    QString fUrl;
    QString fUid;

    QString fAuthType;
    QString fAuthUserName;
    QString fAuthKeyStoreId;
    QString fAuthKeyId;
};

#endif // REMOTESTORAGE_H

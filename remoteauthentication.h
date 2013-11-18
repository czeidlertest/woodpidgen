#ifndef REMOTEAUTHENTICATION_H
#define REMOTEAUTHENTICATION_H

#include "remoteconnection.h"

class Profile;

class RemoteAuthentication : public QObject
{
Q_OBJECT
public:
    RemoteAuthentication(RemoteConnection *connection, const QString &userName,
                         const QString &keyStoreId, const QString &keyId);
    virtual ~RemoteAuthentication() {}

    RemoteConnection *getConnection();

    //! Establish a connection and login afterwards.
    WP::err login(Profile *profile);
    void logout();
    bool verified();

protected slots:
    void handleConnectionAttempt(WP::err code);
    void handleAuthenticationRequest(WP::err code);
    void handleAuthenticationAttempt(WP::err code);

signals:
    void authenticationAttemptFinished(WP::err code);

protected:
    virtual void getLoginRequestData(QByteArray &data) = 0;
    virtual WP::err getLoginData(QByteArray &data, const QByteArray &serverRequest) = 0;
    virtual void getLogoutData(QByteArray &data) = 0;

    RemoteConnection *fConnection;
    QString fUserName;
    QString fKeyStoreId;
    QString fKeyId;
    Profile *fProfile;

    RemoteConnectionReply *fAuthenticationReply;

    bool fAuthenticationInProgress;
    bool fVerified;
};


class SignatureAuthentication : public RemoteAuthentication {
public:
    SignatureAuthentication(RemoteConnection *connection, const QString &userName,
                            const QString &keyStoreId, const QString &keyId);

protected:
    virtual void getLoginRequestData(QByteArray &data);
    virtual WP::err getLoginData(QByteArray &data, const QByteArray &serverRequest);
    virtual void getLogoutData(QByteArray &data);
};


#endif // REMOTEAUTHENTICATION_H

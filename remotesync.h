#ifndef REMOTESYNC_H
#define REMOTESYNC_H

#include <QObject>

#include "databaseinterface.h"
#include "error_codes.h"
#include "profile.h"
#include "remoteconnection.h"


class RemoteSync : public QObject
{
    Q_OBJECT
public:
    explicit RemoteSync(DatabaseInterface *database, RemoteAuthentication* remoteAuth,
                        Profile *profile, QObject *parent = 0);
    ~RemoteSync();

    WP::err sync();

signals:
    void syncFinished(WP::err status);

private slots:
    void syncConnected(WP::err code);
    void syncReply();
    void syncPushReply();

private:
    DatabaseInterface *fDatabase;
    RemoteAuthentication *fAuthentication;
    RemoteConnection *fRemoteConnection;
    RemoteConnectionReply *fServerReply;
    Profile *fProfile;
};

#endif // REMOTESYNC_H

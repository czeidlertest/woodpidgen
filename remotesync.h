#ifndef REMOTESYNC_H
#define REMOTESYNC_H

#include <QObject>

#include "databaseinterface.h"
#include "error_codes.h"
#include "serverconnection.h"


class RemoteSync : public QObject
{
    Q_OBJECT
public:
    explicit RemoteSync(DatabaseInterface *database, RemoteConnection* connection, QObject *parent = 0);
    ~RemoteSync();

    WP::err sync();

signals:
    void syncFinished(WP::err status);

private slots:
    void syncConnected(QNetworkReply::NetworkError code);
    void syncReply();

private:
    DatabaseInterface *fDatabase;
    RemoteConnection *fRemoteConnection;
    RemoteConnectionReply *fServerReply;
};

#endif // REMOTESYNC_H

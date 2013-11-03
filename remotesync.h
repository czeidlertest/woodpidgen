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

private:
    DatabaseInterface *fDatabase;
    RemoteConnection *fRemoteConnection;
    RemoteConnectionReply *fSyncReply;
signals:
    void syncFinished(WP::err status);

private slots:
    void syncConnected(QNetworkReply::NetworkError code);
    void syncReply(QNetworkReply::NetworkError code);

private slots:
    void syncResponse(const QByteArray &data);
};

#endif // REMOTESYNC_H

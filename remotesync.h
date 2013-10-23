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
    
    WP::err sync();

private:
    DatabaseInterface *fDatabase;
    RemoteConnection *fRemoteConnection;

signals:
    void syncFinished(WP::err status);

private slots:
    void syncResponse(const QByteArray &data);
};

#endif // REMOTESYNC_H

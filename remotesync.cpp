#include "remotesync.h"

#include "protocolparser.h"


RemoteSync::RemoteSync(DatabaseInterface *database, RemoteConnection *connection, QObject *parent) :
    QObject(parent),
    fDatabase(database),
    fRemoteConnection(connection)
{
}


WP::err RemoteSync::sync()
{
    QString branch = fDatabase->branch();
    QString tipCommit = fDatabase->getTip();

    IqStanza iqStanza(IqStanza::kGet);
    iqStanza.addAttribute("sync", "repository", "test");

    return WP::kOk;
}

void RemoteSync::syncResponse(const QByteArray &data)
{
    Stanza* root = ProtocolParser::parse(data);
    IqStanza *iqStanza = IqStanza::fromStanza(root);
    if (iqStanza == NULL)
        return;

    delete iqStanza;
    delete root;
}

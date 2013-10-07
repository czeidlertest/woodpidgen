#include "databaseinterface.h"

#include "gitinterface.h"


int DatabaseFactory::open(const QString &databasePath, const QString &branch, DatabaseInterface **database)
{
    *database = NULL;
    DatabaseInterface *gitDatabase = new GitInterface;
    int error = gitDatabase->setTo(databasePath);
    if (error != 0)
        return error;
    error = gitDatabase->setBranch(branch);
    if (error != 0)
        return error;
    *database = gitDatabase;
    return 0;
}

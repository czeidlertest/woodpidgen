#include "gitinterface.h"


GitInterface::GitInterface()
    :
    fRepository(NULL),
    fObjectDatabase(NULL)
{
}


GitInterface::~GitInterface()
{
    unSet();
}

int GitInterface::setTo(const QString &path, bool create)
{
    unSet();
    fRepositoryPath = path;

    int error = git_repository_open(&fRepository, fRepositoryPath.toStdString().c_str());

    if (error != 0 && create)
        error = git_repository_init(&fRepository, fRepositoryPath.toStdString().c_str(), true);

    if (error != 0)
        return error;

    return git_repository_odb(&fObjectDatabase, fRepository);
}

void GitInterface::unSet()
{
    git_repository_free(fRepository);
    git_odb_free(fObjectDatabase);
}

int GitInterface::AddMessage(const char *data)
{
    git_oid oid;
    int error = git_odb_write(&oid, fObjectDatabase, data, strlen(data), GIT_OBJ_BLOB);
    return error;
}

#include "gitinterface.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFile>


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

int GitInterface::WriteObject(const char *data, int size)
{
    QCryptographicHash hash(QCryptographicHash::Sha1);
    hash.addData(data, size);
    QByteArray hashBin =  hash.result();
    std::string hashHex =  hashBin.toHex().data();
    printf("hash %s\n", hashHex.c_str());

    QString path;
    path.sprintf("%s/objects", fRepositoryPath.toStdString().c_str());
    QDir dir(path);
    dir.mkdir(hashHex.substr(0, 2).c_str());
    path += "/";
    path += hashHex.substr(0, 2).c_str();
    path += "/";
    path += hashHex.substr(2).c_str();
    QFile file(path);
    if (file.exists())
        return -1;

    file.open(QIODevice::WriteOnly | QIODevice::Truncate);
    QByteArray arrayData;
    arrayData = QByteArray::fromRawData(data, size);
    QByteArray compressedData = qCompress(arrayData);
    file.write(compressedData.data(), compressedData.size());
    return 0;
}

int GitInterface::WriteFile(const QString &hash, const char *data, int size)
{
    std::string hashHex = hash.toStdString();
    QString path;
    path.sprintf("%s/objects", fRepositoryPath.toStdString().c_str());
    QDir dir(path);
    dir.mkdir(hashHex.substr(0, 2).c_str());
    path += "/";
    path += hashHex.substr(0, 2).c_str();
    path += "/";
    path += hashHex.substr(2).c_str();
    QFile file(path);
    if (file.exists())
        return -1;

    file.open(QIODevice::WriteOnly | QIODevice::Truncate);
    file.write(data, size);
    return 0;
}

#ifndef GITINTERFACE_H
#define GITINTERFACE_H


#include <QString>
#include <git2.h>


class GitInterface
{
public:
    GitInterface();
    ~GitInterface();

    int setTo(const QString &path, bool create = true);
    void    unSet();

    int     AddMessage(const char* data);
private:
    QString         fRepositoryPath;
    git_repository* fRepository;
    git_odb*        fObjectDatabase;
};

#endif // GITINTERFACE_H

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

    int     addMessage(const char* data);
    int     writeObject(const char *data, int size);
    int     writeFile(const QString& hash, const char *data, int size);

    QString getTip(const QString& branch);
    bool    updateTip(const QString& branch, const QString& last);
private:
    QString         fRepositoryPath;
    git_repository* fRepository;
    git_odb*        fObjectDatabase;
};

#endif // GITINTERFACE_H

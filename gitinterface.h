#ifndef GITINTERFACE_H
#define GITINTERFACE_H


#include <QString>
#include <git2.h>

#include "databaseinterface.h"


class GitInterface : public DatabaseInterface
{
public:
                                GitInterface();
                                ~GitInterface();

            int                 setBranch(const QString &branch, bool createBranch = true);
            int                 add(const QString &path, const QByteArray& data);
            int                 commit();
            int                 get(const QString& path, QByteArray& data);

    int setTo(const QString &path, bool create = true);
    void    unSet();

    int     addMessage(const char* data);
    int     writeObject(const char *data, int size);
    int     writeFile(const QString& hash, const char *data, int size);

            QString             getTip(const QString& branch);
            bool                updateTip(const QString& branch, const QString& last);

private:
    git_commit*     getTipCommit(const QString& branch);
    git_tree*       getTipTree(const QString& branch);
    QString         removeFilename(QString& path);

    QString         fRepositoryPath;
    git_repository* fRepository;
    git_odb*        fObjectDatabase;
    QString         fCurrentBranch;

    git_oid         fNewRootTreeOid;
};

#endif // GITINTERFACE_H

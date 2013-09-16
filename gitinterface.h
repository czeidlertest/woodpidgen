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

    int setBranch(const QString &branch, bool createBranch = true);
    int add(const QString &path, const QByteArray &data);
    int commit();
    int get(const QString &path, QByteArray &data) const;

    int setTo(const QString &path, bool create = true);
    void unSet();

    int writeObject(const char *data, int size);
    int writeFile(const QString& hash, const char *data, int size);

    QString getTip(const QString &branch) const;
    bool updateTip(const QString &branch, const QString &last);

    QStringList listFiles(const QString &path) const;
    QStringList listDirectories(const QString &path) const;

private:
    QStringList listDirectoryContent(const QString &path, int type = -1) const;

    git_commit *getTipCommit(const QString &branch) const;
    git_tree *getTipTree(const QString &branch) const;
    //! returns the tree for the given directory
    git_tree *getDirectoryTree(const QString &dirPath, const QString &branch) const;

    QString removeFilename(QString &path) const;

private:
    QString fRepositoryPath;
    git_repository *fRepository;
    git_odb *fObjectDatabase;
    QString fCurrentBranch;

    git_oid fNewRootTreeOid;
};

#endif // GITINTERFACE_H

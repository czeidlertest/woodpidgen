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

    WP::err setBranch(const QString &branch, bool createBranch = true);
    WP::err write(const QString &path, const QByteArray &data);
    WP::err remove(const QString& path);
    WP::err commit();
    WP::err read(const QString &path, QByteArray &data) const;

    WP::err setTo(const QString &path, bool create = true);
    void unSet();
    QString path();

    WP::err writeObject(const char *data, int size);
    WP::err writeFile(const QString& hash, const char *data, int size);

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

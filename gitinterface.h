#ifndef GITINTERFACE_H
#define GITINTERFACE_H


#include <QString>
#include <git2.h>


class DatabaseInterface {
public:
    virtual                     ~DatabaseInterface() {}

    virtual int                 setBranch(const char* branch,
                                    bool createBranch = true) = 0;
    virtual int                 add(const QString& path, const QByteArray& data) = 0;
    virtual int                 commit() = 0;

    enum errors {
        kNotInit = -1
    };
};

class GitInterface : public DatabaseInterface
{
public:
                                GitInterface();
                                ~GitInterface();

    int                         setBranch(const char* branch, bool createBranch = true);
    int                         add(const QString &path, const QByteArray& data);
    int                         commit();

    int setTo(const QString &path, bool create = true);
    void    unSet();

    int     addMessage(const char* data);
    int     writeObject(const char *data, int size);
    int     writeFile(const QString& hash, const char *data, int size);

    QString getTip(const QString& branch);
    bool    updateTip(const QString& branch, const QString& last);

private:
    git_commit*     getTipCommit(const QString& branch);
    QString         removeFilename(QString& path);

    QString         fRepositoryPath;
    git_repository* fRepository;
    git_odb*        fObjectDatabase;
    QString         fCurrentBranch;

    git_oid         fNewRootTreeOid;
};

#endif // GITINTERFACE_H

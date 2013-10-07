#ifndef DATABASEINTERFACE_H
#define DATABASEINTERFACE_H

#include <QByteArray>
#include <QString>


class DatabaseInterface {
public:
    virtual ~DatabaseInterface() {}

    virtual int setBranch(const QString &branch,
                          bool createBranch = true) = 0;
    virtual int add(const QString& path, const QByteArray& data) = 0;
    virtual int remove(const QString& path) = 0;
    virtual int commit() = 0;

    virtual int get(const QString& path, QByteArray& data) const = 0;

    virtual int setTo(const QString &path, bool create = true) = 0;
    virtual void unSet() = 0;
    virtual QString path() = 0;

    virtual QStringList listFiles(const QString &path) const = 0;
    virtual QStringList listDirectories(const QString &path) const = 0;

    enum errors {
        kNotInit = -1
    };
};


class DatabaseFactory {
public:
    static int open(const QString &databasePath, const QString &branch, DatabaseInterface **database);
};

#endif // DATABASEINTERFACE_H

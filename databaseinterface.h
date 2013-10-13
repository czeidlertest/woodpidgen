#ifndef DATABASEINTERFACE_H
#define DATABASEINTERFACE_H

#include <QByteArray>
#include <QString>

#include "error_codes.h"


class DatabaseInterface {
public:
    virtual ~DatabaseInterface() {}

    virtual WP::err setBranch(const QString &branch,
                              bool createBranch = true) = 0;
    virtual WP::err write(const QString& path, const QByteArray& data) = 0;
    virtual WP::err write(const QString& path, const QString& data);
    virtual WP::err remove(const QString& path) = 0;
    virtual WP::err commit() = 0;

    virtual WP::err read(const QString& path, QByteArray& data) const = 0;
    virtual WP::err read(const QString& path, QString& data) const;

    virtual WP::err setTo(const QString &path, bool create = true) = 0;
    virtual void unSet() = 0;
    virtual QString path() = 0;

    virtual QStringList listFiles(const QString &path) const = 0;
    virtual QStringList listDirectories(const QString &path) const = 0;
};


class DatabaseFactory {
public:
    static WP::err open(const QString &databasePath, const QString &branch, DatabaseInterface **database);
};

#endif // DATABASEINTERFACE_H

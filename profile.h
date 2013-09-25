#ifndef PROFILE_H
#define PROFILE_H

#include <QAbstractListModel>
#include <QList>

#include <cryptointerface.h>
#include <databaseinterface.h>
#include <databaseutil.h>

const QString kStorageTypeProfile = "profile";
const QString kStorageTypeIdentity = "identity";
const QString kStorageTypeMessages = "messages";


class ProfileEntry {
public:
    ProfileEntry(const QString &location, const QString &branch);
    virtual ~ProfileEntry() {}

    virtual int write(DatabaseInterface *database, CryptoInterface *crypto) const;
    virtual int load(DatabaseInterface *database, CryptoInterface *crypto);
    virtual QString hash(CryptoInterface *crypto) const;

protected:
    QString fLocation;
    QString fBranch;
};


class UserIdentity;


class ProfileEntryIdentity : public ProfileEntry {
public:
    ProfileEntryIdentity(UserIdentity* identity, const QString &location, const QString &branch);

    UserIdentity* identity();

protected:
    UserIdentity *fIdentity;
};


class IdentityListModel : public QAbstractListModel {
public:
    IdentityListModel(QObject * parent = 0);

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    int rowCount(const QModelIndex & parent = QModelIndex()) const;

    void addIdentity(ProfileEntryIdentity* identity);
    void removeIdentity(ProfileEntryIdentity* identity);
private:
    QList<ProfileEntryIdentity*> fIdentities;
};


class Profile : public DatabaseEncryption
{
public:
    Profile(DatabaseInterface *database, CryptoInterface *crypto,
            const QString branch = "profile");

    int createNewProfile(const SecureArray& password);
    int open(const SecureArray &password);

    ProfileEntryIdentity* addIdentity(UserIdentity *identity);
    IdentityListModel* getIdentityList();

private:
    int writeEntry(const ProfileEntry* entry);


private:
    SecureArray fMasterKey;
    QByteArray fMasterKeyIV;

    IdentityListModel fIdentities;
};

#endif // PROFILE_H

#ifndef PROFILE_H
#define PROFILE_H

#include <QAbstractListModel>
#include <QList>

#include <cryptointerface.h>
#include <databaseinterface.h>
#include <databaseutil.h>
#include <error_codes.h>

const QString kStorageTypeProfile = "profile";
const QString kStorageTypeIdentity = "identity";
const QString kStorageTypeMessages = "messages";


template<class Type>
class ProfileEntry {
public:
    ProfileEntry(DatabaseInterface *database, const QString &path) :
        fProfileDatabase(database),
        fProfileEntryPath(path),
        fUserData(NULL)
    {
    }

    virtual ~ProfileEntry() {
        delete fUserData;
    }

    virtual WP::err write() const
    {
        QString path = fProfileEntryPath + "/database_path";
        WP::err error = fProfileDatabase->write(path, fDatabasePath);
        if (error != WP::kOk)
            return error;
        path = fProfileEntryPath + "/database_branch";
        error = fProfileDatabase->write(path, fDatabaseBranch);
        if (error != WP::kOk) {
            fProfileDatabase->remove(fDatabasePath);
            return error;
        }
        path = fDatabasePath + "/database_base_dir";
        error = fProfileDatabase->write(path, fDatabaseBaseDir);
        if (error != WP::kOk) {
            fProfileDatabase->remove(fDatabasePath);
            return error;
        }
        return WP::kOk;
    }

    virtual WP::err load() {
        QString path = fProfileEntryPath + "/database_path";
        WP::err error = fProfileDatabase->read(path, fDatabasePath);
        if (error != WP::kOk)
            return error;
        path = fProfileEntryPath + "/database_branch";
        error = fProfileDatabase->read(path, fDatabaseBranch);
        if (error != WP::kOk)
            return error;
        path = fDatabasePath + "/database_base_dir";
        error = fProfileDatabase->read(path, fDatabaseBaseDir);
        if (error != WP::kOk)
            return error;

        fUserData = instanciate();
        return WP::kOk;
    }

    void setUserData(Type *data) {
        delete fUserData;
        fUserData = data;
    }

    Type *getUserData() const {
        return fUserData;
    }

    Type *writeEntry() const {
        return fUserData;
    }

protected:
    virtual Type *instanciate() {
        return new Type(fDatabasePath, fDatabaseBranch, fDatabaseBaseDir);
    }

protected:
    DatabaseInterface *fProfileDatabase;
    QString fProfileEntryPath;

    QString fDatabasePath;
    QString fDatabaseBranch;
    QString fDatabaseBaseDir;

    Type *fUserData;
};


class KeyStore;

class ProfileEntryKeyStore : public ProfileEntry<KeyStore> {
public:
    ProfileEntryKeyStore(DatabaseInterface *database, const QString &path);
};


class UserIdentity;

class ProfileEntryIdentity : public ProfileEntry<UserIdentity> {
public:
    ProfileEntryIdentity(DatabaseInterface *database, const QString &path);
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


class Profile : public EncryptedUserData
{
public:
    Profile(const QString &path, const QString &branch);
    ~Profile();

    /*! create a new profile and adds a new KeyStore with \par password. */
    WP::err createNewProfile(const SecureArray &password);

    //! adds an external key store to the profile
    WP::err addKeyStore(KeyStore *keyStore);
    WP::err removeKeyStore(KeyStore *keyStore);
    KeyStore *findKeyStore(const QString &keyStoreId);

    WP::err addUserIdentity(UserIdentity *useIdentity);
    WP::err removeUserIdentity(UserIdentity *useIdentity);

    WP::err open(const SecureArray &password);

    ProfileEntryIdentity* addIdentity(UserIdentity *identity);
    IdentityListModel* getIdentityList();

private:
    WP::err loadKeyStores();
    WP::err loadUserIdentities();

    WP::err createNewKeyStore(const SecureArray &password, KeyStore **keyStoreOut);
    WP::err createNewUserIdentity(KeyStore *keyStore, UserIdentity **userIdentityOut);

private:    
    IdentityListModel fIdentities;

    QString fMasterKeyStoreId;
    QString fMasterKeyId;
    QMap<QString, KeyStore*> fMapOfKeyStores;
    QMap<QString, UserIdentity*> fMapOfUserIdenties;
};


#endif // PROFILE_H

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
    ProfileEntry(DatabaseInterface *database, const QString &path, Type *userData) :
        fEntryDatabase(database),
        fUserData(userData)
    {
        if (fUserData != NULL) {
            fDatabasePath = fUserData->getDatabasePath();
            fDatabaseBranch = fUserData->getDatabaseBranch();
            fDatabaseBaseDir = fUserData->getDatabaseBaseDir();
        }

        if (path != "") {
            fEntryDatabasePath = path;
            if (fDatabaseBaseDir != "")
                fEntryDatabasePath += "/" + fDatabaseBaseDir;
        } else
            fEntryDatabasePath += fDatabaseBaseDir;
    }

    virtual ~ProfileEntry() {
        delete fUserData;
    }

    virtual WP::err write() const
    {
        QString path = fEntryDatabasePath + "/database_path";
        WP::err error = fEntryDatabase->write(path, fDatabasePath);
        if (error != WP::kOk)
            return error;
        path = fEntryDatabasePath + "/database_branch";
        error = fEntryDatabase->write(path, fDatabaseBranch);
        if (error != WP::kOk) {
            fEntryDatabase->remove(fDatabasePath);
            return error;
        }
        path = fEntryDatabasePath + "/database_base_dir";
        error = fEntryDatabase->write(path, fDatabaseBaseDir);
        if (error != WP::kOk) {
            fEntryDatabase->remove(fDatabasePath);
            return error;
        }
        return WP::kOk;
    }

    virtual WP::err load() {
        QString path = fEntryDatabasePath + "/database_path";
        WP::err error = fEntryDatabase->read(path, fDatabasePath);
        if (error != WP::kOk)
            return error;
        path = fEntryDatabasePath + "/database_branch";
        error = fEntryDatabase->read(path, fDatabaseBranch);
        if (error != WP::kOk)
            return error;
        path = fEntryDatabasePath + "/database_base_dir";
        error = fEntryDatabase->read(path, fDatabaseBaseDir);
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

protected:
    virtual Type *instanciate() {
        return new Type(fDatabasePath, fDatabaseBranch, fDatabaseBaseDir);
    }

protected:
    DatabaseInterface *fEntryDatabase;
    QString fEntryDatabasePath;

    QString fDatabasePath;
    QString fDatabaseBranch;
    QString fDatabaseBaseDir;

    Type *fUserData;
};


class KeyStore;

class ProfileEntryKeyStore : public ProfileEntry<KeyStore> {
public:
    ProfileEntryKeyStore(DatabaseInterface *database, const QString &path,
                         KeyStore *keyStore);
};


class UserIdentity;

class ProfileEntryIdentity : public ProfileEntry<UserIdentity> {
public:
    ProfileEntryIdentity(DatabaseInterface *database, const QString &path, UserIdentity *identity);
};


class IdentityListModel : public QAbstractListModel {
public:
    IdentityListModel(QObject * parent = 0);

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    int rowCount(const QModelIndex & parent = QModelIndex()) const;

    void addIdentity(ProfileEntryIdentity *identity);
    void removeIdentity(ProfileEntryIdentity *identity);
    ProfileEntryIdentity *removeIdentityAt(int index);
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

    WP::err addUserIdentity(UserIdentity *identity);
    WP::err removeUserIdentity(UserIdentity *useIdentity);

    WP::err open(const SecureArray &password);

    IdentityListModel* getIdentityList();

private:
    void clear();

    WP::err loadKeyStores();
    void addKeyStore(ProfileEntryKeyStore *entry);
    WP::err loadUserIdentities();
    void addUserIdentity(ProfileEntryIdentity *entry);

    WP::err createNewKeyStore(const SecureArray &password, KeyStore **keyStoreOut);
    WP::err createNewUserIdentity(KeyStore *keyStore, UserIdentity **userIdentityOut);

private:    
    IdentityListModel fIdentities;

    QString fMasterKeyStoreId;
    QString fMasterKeyId;
    QMap<QString, ProfileEntryKeyStore*> fMapOfKeyStores;
};


#endif // PROFILE_H

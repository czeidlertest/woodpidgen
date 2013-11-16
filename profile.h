#ifndef PROFILE_H
#define PROFILE_H

#include <QAbstractListModel>
#include <QList>

#include "cryptointerface.h"
#include "databaseinterface.h"
#include "databaseutil.h"
#include "error_codes.h"

class ProfileEntryKeyStore;
class ProfileEntryRemote;
class ProfileEntryIdentity;
class RemoteConnection;
class UserIdentity;


class IdentityListModel : public QAbstractListModel {
public:
    IdentityListModel(QObject * parent = 0);

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    int rowCount(const QModelIndex & parent = QModelIndex()) const;

    void addIdentity(ProfileEntryIdentity *identity);
    void removeIdentity(ProfileEntryIdentity *identity);
    ProfileEntryIdentity *removeIdentityAt(int index);
    UserIdentity *identityAt(int index);
private:
    QList<ProfileEntryIdentity*> fIdentities;
};


class Profile : public EncryptedUserData
{
public:
    Profile(const QString &path, const QString &branchName);
    ~Profile();

    /*! create a new profile and adds a new KeyStore with \par password. */
    WP::err createNewProfile(const SecureArray &password);

    WP::err commit();

    //! adds an external key store to the profile
    WP::err addKeyStore(KeyStore *keyStore);
    WP::err removeKeyStore(KeyStore *keyStore);
    KeyStore *findKeyStore(const QString &keyStoreId);

    WP::err addUserIdentity(UserIdentity *identity);
    WP::err removeUserIdentity(UserIdentity *useIdentity);

    DatabaseBranch* databaseBranchFor(const QString &database, const QString &branch);

    RemoteDataStorage *addPHPRemote(const QString &url);
    RemoteDataStorage *addHTTPRemote(const QString &url);
    WP::err setSignatureAuth(RemoteDataStorage *remote, const QString &userName, const QString &keyStoreId, const QString &keyId);
    WP::err connectRemote(DatabaseBranch *branch, RemoteDataStorage *remote);
    RemoteDataStorage *findRemote(const QString &remoteId);
    WP::err connectFreeBranches(RemoteDataStorage *remote);

    WP::err open(const SecureArray &password);

    IdentityListModel* getIdentityList();

    EncryptedUserData* findStorageDirectory(UserData *userData);

    QList<DatabaseBranch*> &getBranches();
private:
    void clear();

    WP::err loadKeyStores();
    void addKeyStore(ProfileEntryKeyStore *entry);
    WP::err loadUserIdentities();
    void addUserIdentity(ProfileEntryIdentity *entry);
    WP::err loadRemotes();
    WP::err addRemoteDataStorage(RemoteDataStorage *remote);
    void addRemote(RemoteDataStorage *remote);
    RemoteDataStorage *findRemoteDataStorage(const QString &id);

    WP::err createNewKeyStore(const SecureArray &password, DatabaseBranch *branch, KeyStore **keyStoreOut);
    WP::err createNewUserIdentity(KeyStore *keyStore, DatabaseBranch *branch, UserIdentity **userIdentityOut);

    WP::err writeDatabaseBranch(DatabaseBranch *databaseBranch);
    WP::err loadDatabaseBranches();
private:
    class ProfileKeyStoreFinder : public KeyStoreFinder {
    public:
        ProfileKeyStoreFinder(QMap<QString, ProfileEntryKeyStore*> &map);
        virtual KeyStore *find(const QString &keyStoreId);
    private:
        QMap<QString, ProfileEntryKeyStore*> &fMapOfKeyStores;
    };

    IdentityListModel fIdentities;

    QMap<QString, ProfileEntryKeyStore*> fMapOfKeyStores;
    QMap<QString, RemoteDataStorage*> fMapOfRemotes;
    
    QList<DatabaseBranch*> fBranches;
};


template<class Type>
class ProfileEntry : public EncryptedUserData {
public:
    ProfileEntry(EncryptedUserData *database, const QString &path, Type *userData) :
        EncryptedUserData(*database),
        fUserData(userData)
    {
        setBaseDir(path);

        if (fUserData != NULL) {
            fDatabaseBranch = fUserData->getDatabaseBranch();
            fDatabaseBaseDir = fUserData->getDatabaseBaseDir();
        }
    }

    virtual ~ProfileEntry() {
        delete fUserData;
    }

    virtual WP::err writeEntry()
    {
        WP::err error = write("database_path", fDatabaseBranch->getDatabasePath());
        if (error != WP::kOk)
            return error;
        error = write("database_branch", fDatabaseBranch->getBranch());
        if (error != WP::kOk)
            return error;
        error = write("database_base_dir", fDatabaseBaseDir);
        if (error != WP::kOk)
            return error;
        return WP::kOk;
    }

    virtual WP::err load(Profile *profile) {
        QString databasePath;
        WP::err error = read("database_path", databasePath);
        if (error != WP::kOk)
            return error;
        QString databaseBranch;
        error = read("database_branch", databaseBranch);
        if (error != WP::kOk)
            return error;
        fDatabaseBranch = profile->databaseBranchFor(databasePath, databaseBranch);
        error = read("database_base_dir", fDatabaseBaseDir);
        if (error != WP::kOk)
            return error;

        fUserData = instanciate();
        if (fUserData == NULL)
            return WP::kError;
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
    virtual Type *instanciate() = 0;

protected:
    const DatabaseBranch *fDatabaseBranch;
    QString fDatabaseBaseDir;

    Type *fUserData;
};


class KeyStore;

class ProfileEntryKeyStore : public ProfileEntry<KeyStore> {
public:
    ProfileEntryKeyStore(EncryptedUserData *database, const QString &path,
                         KeyStore *keyStore);
protected:
    KeyStore *instanciate();
};


class UserIdentity;

class ProfileEntryIdentity : public ProfileEntry<UserIdentity> {
public:
    ProfileEntryIdentity(EncryptedUserData *database, const QString &path, UserIdentity *identity);
protected:
    UserIdentity *instanciate();
};


#endif // PROFILE_H

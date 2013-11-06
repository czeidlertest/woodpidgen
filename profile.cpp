#include "profile.h"

#include <QStringList>

#include "useridentity.h"
#include "phpremotestorage.h"

IdentityListModel::IdentityListModel(QObject * parent)
    :
      QAbstractListModel(parent)
{
}

QVariant IdentityListModel::data(const QModelIndex &index, int role) const
{
    if (role == Qt::DisplayRole)
        return fIdentities.at(index.row())->getUserData()->getUid();

    return QVariant();
}

int IdentityListModel::rowCount(const QModelIndex &/*parent*/) const
{
    return fIdentities.count();
}

void IdentityListModel::addIdentity(ProfileEntryIdentity *identity)
{
    int count = fIdentities.count();
    beginInsertRows(QModelIndex(), count, count);
    fIdentities.append(identity);
    endInsertRows();
}

void IdentityListModel::removeIdentity(ProfileEntryIdentity *identity)
{
    int index = fIdentities.indexOf(identity);
    beginRemoveRows(QModelIndex(), index, index);
    fIdentities.removeAt(index);
    endRemoveRows();
}

ProfileEntryIdentity *IdentityListModel::removeIdentityAt(int index)
{
    ProfileEntryIdentity *entry = fIdentities.at(index);
    beginRemoveRows(QModelIndex(), index, index);
    fIdentities.removeAt(index);
    endRemoveRows();
    return entry;
}

Profile::Profile(const QString &path, const QString &branchName)
{
    DatabaseBranch *databaseBranch = databaseBranchFor(path, branchName);
    setToDatabase(databaseBranch, "");
}

Profile::~Profile()
{
    clear();
}

WP::err Profile::createNewProfile(const SecureArray &password, RemoteDataStorage *remote)
{
    QByteArray uid = fCrypto->generateInitalizationVector(512);
    setUid(fCrypto->toHex(fCrypto->sha1Hash(uid)));

    // init key store and master key
    DatabaseBranch *keyStoreBranch = databaseBranchFor(fDatabaseBranch->getDatabasePath(), "key_stores");
    KeyStore* keyStore = NULL;
    WP::err error = createNewKeyStore(password, keyStoreBranch, &keyStore);
    if (error != WP::kOk)
        return error;
    setKeyStore(keyStore);

    SecureArray masterKey = fCrypto->generateSymetricKey(256);
    QString masterKeyId;
    error = keyStore->writeSymmetricKey(masterKey, fCrypto->generateInitalizationVector(256),
                                        masterKeyId);
    if (error != WP::kOk)
        return error;

    setDefaultKeyId(masterKeyId);
    error = writeConfig();
    if (error != WP::kOk)
        return error;

    // init user identity
    DatabaseBranch *identitiesBranch = databaseBranchFor(fDatabaseBranch->getDatabasePath(), "identities");
    UserIdentity *identity = NULL;
    error = createNewUserIdentity(keyStore, identitiesBranch, &identity);
    if (error != WP::kOk)
        return error;

    // connect all branches to the remote
    if (remote != NULL) {
        QString path = "remotes/";
        path += remote->getUid();
        remote->setTo(this, path);
        error = remote->writeConfig();
        if (error != WP::kOk)
            return error;
        addRemoteDataStorage(remote);
        for (int i = 0; i < fBranches.count(); i++) {
            DatabaseBranch *branch = fBranches.at(i);
            connectRemote(branch, remote);
        }
    }

    // commit all branches
    for (int i = 0; i < fBranches.count(); i++) {
        DatabaseBranch *branch = fBranches.at(i);
        branch->commit();
    }

    return WP::kOk;
}

WP::err Profile::open(const SecureArray &password)
{
    if (fDatabase == NULL)
        return WP::kNotInit;

    WP::err error = loadKeyStores();
    if (error != WP::kOk)
        return error;

    Profile::ProfileKeyStoreFinder keyStoreFinder(fMapOfKeyStores);
    error = readKeyStore(&keyStoreFinder);
    if (error != WP::kOk)
        return error;
    error = fKeyStore->open(password);
    if (error != WP::kOk)
        return error;

    // remotes
    error = loadRemotes();
    if (error != WP::kOk)
        return error;

    // load database branches
    error = loadDatabaseBranches();
    if (error != WP::kOk)
        return error;

    error = loadUserIdentities();
    if (error != WP::kOk)
        return error;

    return WP::kOk;
}

WP::err Profile::loadKeyStores()
{
    QStringList keyStores = listDirectories("key_stores");

    for (int i = 0; i < keyStores.count(); i++) {
        ProfileEntryKeyStore *entry
            = new ProfileEntryKeyStore(this, prependBaseDir("key_stores/" + keyStores.at(i)), NULL);
        if (entry->load(this) != WP::kOk)
            continue;
        addKeyStore(entry);
    }
    return WP::kOk;
}

WP::err Profile::loadUserIdentities()
{
    QStringList identityList = listDirectories("user_identities");

    for (int i = 0; i < identityList.count(); i++) {
        ProfileEntryIdentity *entry
            = new ProfileEntryIdentity(this, prependBaseDir("user_identities/" + identityList.at(i)), NULL);
        if (entry->load(this) != WP::kOk)
            continue;
        UserIdentity *id = entry->getUserData();
        Profile::ProfileKeyStoreFinder keyStoreFinder(fMapOfKeyStores);
        WP::err error = id->readKeyStore(&keyStoreFinder);
        if (error != WP::kOk){
            delete entry;
            continue;
        }

        addUserIdentity(entry);
    }
    return WP::kOk;
}

void Profile::addUserIdentity(ProfileEntryIdentity *entry)
{
    fIdentities.addIdentity(entry);
    addUserData(entry->getUserData(), entry);
}

WP::err Profile::loadRemotes()
{
    QStringList identityList = listDirectories("remotes");

    for (int i = 0; i < identityList.count(); i++) {
        ProfileEntryRemote *entry
            = new ProfileEntryRemote(this, prependBaseDir("remotes/" + identityList.at(i)), NULL);
        entry->setTo(this, entry->getDatabaseBaseDir());
        if (entry->load(this) != WP::kOk)
            continue;
        RemoteDataStorage *remote = entry->getUserData();
        Profile::ProfileKeyStoreFinder keyStoreFinder(fMapOfKeyStores);
        WP::err error = remote->open(&keyStoreFinder);
        if (error != WP::kOk){
            delete entry;
            continue;
        }
        addRemote(entry);
    }
    return WP::kOk;
}

void Profile::addRemote(ProfileEntryRemote *entry)
{
    fMapOfRemotes[entry->getUserData()->getUid()] = entry;
    addUserData(entry->getUserData(), entry);
}

WP::err Profile::createNewKeyStore(const SecureArray &password, DatabaseBranch *branch, KeyStore **keyStoreOut)
{
    // init key store and master key
    KeyStore* keyStore = new KeyStore(branch, "");
    WP::err error = keyStore->create(password);
    if (error != WP::kOk)
        return error;
    error = addKeyStore(keyStore);
    if (error != WP::kOk)
        return error;
    *keyStoreOut = keyStore;
    return WP::kOk;
}

WP::err Profile::createNewUserIdentity(KeyStore *keyStore, DatabaseBranch *branch, UserIdentity **userIdentityOut)
{
    UserIdentity *identity = new UserIdentity(branch, "");
    identity->setKeyStore(keyStore);
    WP::err error = identity->createNewIdentity();
    if (error != WP::kOk)
        return error;
    error = addUserIdentity(identity);
    if (error != WP::kOk)
        return error;
    *userIdentityOut = identity;
    return WP::kOk;
}

void Profile::addUserData(UserData *data, EncryptedUserData *configDir)
{
    fConfigIndex[data] = configDir;
}

WP::err Profile::writeDatabaseBranch(DatabaseBranch *databaseBranch)
{
    QString path = "remote_branches/";
    path += databaseBranch->databaseHash();

    WP::err error = writeSafe(path + "/path", databaseBranch->getDatabasePath());
    if (error != WP::kOk)
        return error;

    path += "/";
    path += databaseBranch->getBranch();
    path += "/remotes/";
    for (int i = 0; i < databaseBranch->countRemotes(); i++) {
        RemoteDataStorage *remote = databaseBranch->getRemoteAt(i);
        QString remotePath = path;
        remotePath += remote->getUid();
        remotePath += "/id";
        error = writeSafe(remotePath, remote->getUid());
        if (error != WP::kOk)
            return error;
    }
    return WP::kOk;
}

WP::err Profile::loadDatabaseBranches()
{
    QString path = "remote_branches";
    QStringList databases = listDirectories(path);
    for (int i = 0; i < databases.count(); i++) {
        QString subPath = path + "/" + databases.at(i);
        QString databasePath;
        WP::err error = readSafe(subPath + "/path", databasePath);
        if (error != WP::kOk)
            continue;
        QStringList branches = listDirectories(subPath);
        for (int a = 0; a < branches.count(); a++) {
            DatabaseBranch *databaseBranch = databaseBranchFor(databasePath, branches.at(a));

            QString remotePath = subPath + "/" + branches.at(a) + "/remotes";
            QStringList remotes = listDirectories(remotePath);
            for (int r = 0; r < remotes.count(); r++) {
                QString remoteId;
                error = readSafe(remotePath + "/" + remotes.at(r) + "/id", remoteId);
                if (error != WP::kOk)
                    continue;
                RemoteDataStorage *remote = findRemote(remoteId);
                error = connectRemote(databaseBranch, remote);
                if (error != WP::kOk)
                    continue;
            }
        }
    }
    return WP::kOk;
}

IdentityListModel *Profile::getIdentityList()
{
    return &fIdentities;
}

EncryptedUserData *Profile::findStorageDirectory(UserData *userData)
{
    QMap<UserData*, EncryptedUserData*>::iterator it = fConfigIndex.find(userData);
    if (it == fConfigIndex.end())
        return NULL;
    return it.value();
}

QList<DatabaseBranch *> &Profile::getBranches()
{
    return fBranches;
}

void Profile::clear()
{
    QMap<QString, ProfileEntryKeyStore*>::const_iterator it;
    for (it = fMapOfKeyStores.begin(); it != fMapOfKeyStores.end(); it++)
        delete it.value();
    fMapOfKeyStores.clear();

    while (fIdentities.rowCount() > 0)
        delete fIdentities.removeIdentityAt(0);

    for (int i = 0; i < fBranches.count(); i++)
        delete fBranches.at(i);
    fBranches.clear();
}

KeyStore *Profile::findKeyStore(const QString &keyStoreId)
{
    QMap<QString, ProfileEntryKeyStore*>::const_iterator it;
    it = fMapOfKeyStores.find(keyStoreId);
    if (it == fMapOfKeyStores.end())
        return NULL;
    return it.value()->getUserData();
}

WP::err Profile::addUserIdentity(UserIdentity *identity)
{
    QString dir = "user_identities/";
    dir += identity->getUid();

    ProfileEntryIdentity *entry = new ProfileEntryIdentity(this, dir, identity);
    WP::err error = entry->writeEntry();
    if (error != WP::kOk)
        return error;
    addUserIdentity(entry);
    return WP::kOk;
}

DatabaseBranch *Profile::databaseBranchFor(const QString &database, const QString &branch)
{
    for (int i = 0; i < fBranches.count(); i++) {
        DatabaseBranch *databaseBranch = fBranches.at(i);
        if (databaseBranch->getDatabasePath() == database && databaseBranch->getBranch() == branch)
            return databaseBranch;
    }
    // not found create one
    DatabaseBranch *databaseBranch = new DatabaseBranch(database, branch);
    if (databaseBranch == NULL)
        return NULL;
    fBranches.append(databaseBranch);
    return databaseBranch;
}

WP::err Profile::addRemoteDataStorage(RemoteDataStorage *remote)
{
    QString dir = "remotes/";
    dir += remote->getUid();

    ProfileEntryRemote *entry = new ProfileEntryRemote(this, dir, remote);
    entry->setTo(this, entry->getDatabaseBaseDir());
    WP::err error = entry->writeEntry();
    if (error != WP::kOk)
        return error;
    addRemote(entry);
    return WP::kOk;
}

WP::err Profile::connectRemote(DatabaseBranch *branch, RemoteDataStorage *remote)
{
    branch->addRemote(remote);
    return writeDatabaseBranch(branch);
}

RemoteDataStorage *Profile::findRemote(const QString &remoteId)
{
    QMap<QString, ProfileEntryRemote*>::iterator it;
    it = fMapOfRemotes.find(remoteId);
    if (it == fMapOfRemotes.end())
        return NULL;
    return it.value()->getUserData();
}

WP::err Profile::addKeyStore(KeyStore *keyStore)
{
    QString dir = "key_stores/";
    dir += keyStore->getUid();

    ProfileEntryKeyStore *entry = new ProfileEntryKeyStore(this, dir, keyStore);
    WP::err error = entry->writeEntry();
    if (error != WP::kOk)
        return error;
    addKeyStore(entry);
    return WP::kOk;
}

void Profile::addKeyStore(ProfileEntryKeyStore *entry)
{
    fMapOfKeyStores[entry->getUserData()->getUid()] = entry;
    addUserData(entry->getUserData(), entry);
}

ProfileEntryIdentity::ProfileEntryIdentity(EncryptedUserData *database, const QString &path,
                                           UserIdentity *identity) :
    ProfileEntry<UserIdentity>(database, path, identity)
{
}

UserIdentity *ProfileEntryIdentity::instanciate()
{
    return new UserIdentity(fDatabaseBranch, fDatabaseBaseDir);
}

ProfileEntryKeyStore::ProfileEntryKeyStore(EncryptedUserData *database, const QString &path,
                                           KeyStore *keyStore) :
    ProfileEntry<KeyStore>(database, path, keyStore)
{
}

KeyStore *ProfileEntryKeyStore::instanciate()
{
    return new KeyStore(fDatabaseBranch, fDatabaseBaseDir);
}


ProfileEntryRemote::ProfileEntryRemote(EncryptedUserData *database, const QString &path, RemoteDataStorage *remote) :
    ProfileEntry<RemoteDataStorage>(database, path, remote)
{

}

WP::err ProfileEntryRemote::writeEntry()
{
    WP::err error = ProfileEntry<RemoteDataStorage>::writeEntry();
    if (error != WP::kOk)
        return error;

    error = writeSafe("type", fUserData->type());
    return error;
}

RemoteDataStorage *ProfileEntryRemote::instanciate()
{
    WP::err error = WP::kError;
    QString type;
    error = readSafe("type", type);
    if (error != WP::kOk)
        return NULL;
    if (type == "PHPRemoteStorage")
        return new PHPRemoteStorage(fDatabaseBranch, fDatabaseBaseDir);
    return NULL;
}


Profile::ProfileKeyStoreFinder::ProfileKeyStoreFinder(QMap<QString, ProfileEntryKeyStore *> &map) :
    fMapOfKeyStores(map)
{
}

KeyStore *Profile::ProfileKeyStoreFinder::find(const QString &keyStoreId)
{
    QMap<QString, ProfileEntryKeyStore*>::const_iterator it;
    it = fMapOfKeyStores.find(keyStoreId);
    if (it == fMapOfKeyStores.end())
        return NULL;
    return it.value()->getUserData();
}

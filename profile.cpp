#include "profile.h"

#include <QStringList>

#include <useridentity.h>


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

Profile::Profile(const QString &path, const QString &branch)
{
    setToDatabase(path, branch);

    if (UserData::initCheck() !=  WP::kOk)
        return;
}

Profile::~Profile()
{
    clear();
}

WP::err Profile::createNewProfile(const SecureArray &password)
{
    WP::err error = UserData::initCheck();
    if (error !=  WP::kOk)
        return error;

    QByteArray uid = fCrypto->generateInitalizationVector(512);
    setUid(fCrypto->toHex(fCrypto->sha1Hash(uid)));

    // init key store and master key
    KeyStore* keyStore = NULL;
    error = createNewKeyStore(password, &keyStore);
    if (error != WP::kOk)
        return error;
    error = setKeyStore(keyStore);
    if (error != WP::kOk)
        return error;
    error = writeKeyStoreId();
    if (error != WP::kOk)
        return error;

    SecureArray masterKey = fCrypto->generateSymetricKey(256);
    error = keyStore->writeSymmetricKey(masterKey, fCrypto->generateInitalizationVector(256),
                                        fMasterKeyId);
    if (error != WP::kOk)
        return error;
    error = write("master_key_id", fMasterKeyId);
    if (error != WP::kOk)
        return error;

    // init user identity
    UserIdentity *identity = NULL;
    error = createNewUserIdentity(keyStore, &identity);
    if (error != WP::kOk)
        return error;

    // store everything
    error = keyStore->commit();
    if (error != WP::kOk)
        return error;
    error = identity->commit();
    if (error != WP::kOk)
        return error;
    error = commit();
    if (error != WP::kOk)
        return error;
    return WP::kOk;
}

WP::err Profile::open(const SecureArray &password)
{
    clear();

    WP::err error = DatabaseFactory::open(fDatabasePath, fDatabaseBranch, &fDatabase);
    if (error != WP::kOk)
        return error;

    QString keyStoreId;
    error = readKeyStoreId(keyStoreId);
    if (error != WP::kOk)
        return error;
    error = read("master_key_id", fMasterKeyId);
    if (error != WP::kOk)
        return error;
    setDefaultKeyId(fMasterKeyId);

    loadKeyStores();
    KeyStore *keyStore = findKeyStore(keyStoreId);
    if (keyStore == NULL) {
        clear();
        return WP::kUninit;
    }
    setKeyStore(keyStore);
    fKeyStore->open(password);

    loadUserIdentities();

    return WP::kOk;
}

WP::err Profile::loadKeyStores()
{
    QStringList keyStores = fDatabase->listDirectories(prependBaseDir("key_stores"));

    for (int i = 0; i < keyStores.count(); i++) {
        ProfileEntryKeyStore *entry
            = new ProfileEntryKeyStore(fDatabase, prependBaseDir("key_stores/" + keyStores.at(i)), NULL);
        if (entry->load() != WP::kOk)
            continue;
        addKeyStore(entry);
    }
    return WP::kOk;
}

WP::err Profile::loadUserIdentities()
{
    QStringList identityList = fDatabase->listDirectories(prependBaseDir("user_identities"));

    for (int i = 0; i < identityList.count(); i++) {
        ProfileEntryIdentity *entry
            = new ProfileEntryIdentity(fDatabase, prependBaseDir("user_identities/" + identityList.at(i)), NULL);
        if (entry->load() != WP::kOk)
            continue;
        UserIdentity *id = entry->getUserData();
        QString keyStoreId;
        WP::err error = id->readKeyStoreId(keyStoreId);
        if (error != WP::kOk)
            continue;
        KeyStore *keyStore = findKeyStore(keyStoreId);
        if (keyStore == NULL) {
            delete entry;
            continue;
        }
        id->setKeyStore(keyStore);
        addUserIdentity(entry);
    }
    return WP::kOk;
}

void Profile::addUserIdentity(ProfileEntryIdentity *entry)
{
    fIdentities.addIdentity(entry);
}

WP::err Profile::createNewKeyStore(const SecureArray &password, KeyStore **keyStoreOut)
{
    // init key store and master key
    KeyStore* keyStore = new KeyStore(fDatabasePath, "key_stores", "");
    WP::err error = keyStore->create(password);
    if (error != WP::kOk)
        return error;
    error = addKeyStore(keyStore);
    if (error != WP::kOk)
        return error;
    *keyStoreOut = keyStore;
    return WP::kOk;
}

WP::err Profile::createNewUserIdentity(KeyStore *keyStore, UserIdentity **userIdentityOut)
{
    UserIdentity *identity = new UserIdentity(fDatabasePath, "user_identities", "");
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

IdentityListModel *Profile::getIdentityList()
{
    return &fIdentities;
}

void Profile::clear()
{
    QMap<QString, ProfileEntryKeyStore*>::const_iterator it;
    for (it = fMapOfKeyStores.begin(); it != fMapOfKeyStores.end(); it++)
        delete it.value();
    fMapOfKeyStores.clear();

    while (fIdentities.rowCount() > 0)
        delete fIdentities.removeIdentityAt(0);
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
    ProfileEntryIdentity *entry = new ProfileEntryIdentity(fDatabase, "user_identities", identity);
    WP::err error = entry->write();
    if (error != WP::kOk)
        return error;
    addUserIdentity(entry);
    return WP::kOk;
}

WP::err Profile::addKeyStore(KeyStore *keyStore)
{
    ProfileEntryKeyStore *entry = new ProfileEntryKeyStore(fDatabase, "key_stores", keyStore);
    WP::err error = entry->write();
    if (error != WP::kOk)
        return error;
    addKeyStore(entry);
    return WP::kOk;
}

void Profile::addKeyStore(ProfileEntryKeyStore *entry)
{
    fMapOfKeyStores[entry->getUserData()->getUid()] = entry;
}

ProfileEntryIdentity::ProfileEntryIdentity(DatabaseInterface *database, const QString &path,
                                           UserIdentity *identity) :
    ProfileEntry<UserIdentity>(database, path, identity)
{
}

ProfileEntryKeyStore::ProfileEntryKeyStore(DatabaseInterface *database, const QString &path,
                                           KeyStore *keyStore) :
    ProfileEntry<KeyStore>(database, path, keyStore)
{
}

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


Profile::Profile(const QString &path, const QString &branch)
{
    setToDatabase(path, branch);

    if (UserData::initCheck() !=  WP::kOk)
        return;

    // load list of key stores
    loadKeyStores();

    QString keyStoreId = getUid();
    KeyStore *KeyStore = findKeyStore(keyStoreId);
    setKeyStore(KeyStore);
}

Profile::~Profile()
{
    QMap<QString, KeyStore*>::const_iterator it;
    for (it = fMapOfKeyStores.begin(); it != fMapOfKeyStores.end(); it++)
        delete it.value();
}

WP::err Profile::createNewProfile(const SecureArray &password)
{
    WP::err error = UserData::initCheck();
    if (error !=  WP::kOk)
        return error;

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

    error = keyStore->writeSymmetricKey(password, fCrypto->generateInitalizationVector(256), fMasterKeyId);
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
    fKeyStore->open(password);

    if (fDatabase != NULL)
        return WP::kError;
    WP::err error = DatabaseFactory::open(fDatabasePath, fDatabaseBranch, &fDatabase);
    if (error != WP::kOk)
        return error;

    QStringList ids = UserIdentity::getIdenties(fDatabase);
    for (int i = 0; i < ids.count(); i++) {

        UserIdentity *id = new UserIdentity(fDatabasePath, fDatabaseBranch);
        QString keyStoreId = id->getUid();
        KeyStore *keyStore = findKeyStore(keyStoreId);
        if (keyStore == NULL) {
            delete id;
            continue;
        }
        id->setKeyStore(keyStore);
        addIdentity(id);
    }
    loadKeyStores();
    loadUserIdentities();

    return WP::kOk;
}

ProfileEntryIdentity *Profile::addIdentity(UserIdentity *identity)
{
    // TODO FIX HACK
    ProfileEntryIdentity *entry = new ProfileEntryIdentity(fDatabase, "identities");
    entry->setUserData(identity);
    fIdentities.addIdentity(entry);
    return entry;
}

WP::err Profile::loadKeyStores()
{
    QString path = "KeyStores";
    QStringList keyStores = fDatabase->listDirectories(prependBaseDir(path));

    for (int i = 0; i < keyStores.count(); i++) {
        path = "KeyStores/" + keyStores.at(i);

    }
    return WP::kOk;
}

WP::err Profile::loadUserIdentities()
{
    return WP::kOk;
}

WP::err Profile::createNewKeyStore(const SecureArray &password, KeyStore **keyStoreOut)
{
    // init key store and master key
    KeyStore* keyStore = new KeyStore(fDatabasePath, "keystore", "");
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

KeyStore *Profile::findKeyStore(const QString &keyStoreId)
{
    QMap<QString, KeyStore*>::const_iterator it;
    it = fMapOfKeyStores.find(keyStoreId);
    if (it == fMapOfKeyStores.end())
        return NULL;
    return it.value();
}

WP::err Profile::addUserIdentity(UserIdentity *useIdentity)
{
    fMapOfUserIdenties[useIdentity->getUid()] = useIdentity;
    return WP::kOk;
}

WP::err Profile::addKeyStore(KeyStore *keyStore)
{
    fMapOfKeyStores[keyStore->getUid()] = keyStore;
    return WP::kOk;
}

ProfileEntryIdentity::ProfileEntryIdentity(DatabaseInterface *database, const QString &path) :
    ProfileEntry(database, path)
{
}

ProfileEntryKeyStore::ProfileEntryKeyStore(DatabaseInterface *database, const QString &path) :
    ProfileEntry(database, path)
{
}





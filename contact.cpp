#include "contact.h"


Contact::Contact(const QString &uid, const QString &nickname, const QString &keyId,
                 const QString &certificate, const QString &publicKey) :
    StorageDirectory(NULL, ""),
    fPrivateKeyStore(false)
{
    fUid = uid;
    fNickname = nickname;

    fKeys = new ContactKeysBuddies(NULL, "");
    fKeys->addKeySet(keyId, certificate, publicKey);
    fKeys->setMainKeyId(keyId);
}

Contact::Contact(EncryptedUserData *database, const QString &directory) :
    StorageDirectory(database, directory),
    fPrivateKeyStore(false),
    fKeys(NULL)
{

}

Contact::~Contact()
{
    delete fKeys;
}

WP::err Contact::createUserIdentityContact(KeyStore *keyStore, const QString &keyId,
                                           const QString &userNickname)
{
    fUid = keyId;

    fPrivateKeyStore = true;

    fKeys = new ContactKeysKeyStore(fDatabase, fDirectory + "/keys", keyStore);
    fKeys->addKeySet(keyId, true);

    fNickname = userNickname;
    return writeConfig();
}


WP::err Contact::open(KeyStoreFinder *keyStoreFinder)
{
    QString type;
    WP::err error = read("keystore_type", type);
    if (error == WP::kOk && type == "private") {
        fPrivateKeyStore = true;
        QString keyStoreId;
        error = readSafe("keyStoreId", keyStoreId);
        if (error != WP::kOk)
            return error;
        KeyStore *keyStore = keyStoreFinder->find(keyStoreId);
        if (keyStore == NULL)
            return WP::kEntryNotFound;
        fKeys = new ContactKeysKeyStore(fDatabase, getKeysDirectory(), keyStore);
    } else
        fKeys = new ContactKeysBuddies(fDatabase, getKeysDirectory());

    error = fKeys->open();
    if (error != WP::kOk)
        return error;

    error = readSafe("uid", fUid);
    if (error != WP::kOk)
        return error;

    error = readSafe("nickname", fNickname);
    if (error != WP::kOk)
        return error;

    error = readSafe("serverUser", fServerUser);
    if (error != WP::kOk)
        return error;

    error = readSafe("server", fServer);
    if (error != WP::kOk)
        return error;

    return error;
}

QString Contact::getUid() const
{
    return fUid;
}

ContactKeys *Contact::getKeys()
{
    return fKeys;
}

const QString &Contact::getNickname() const
{
    return fNickname;
}

const QString Contact::getAddress() const
{
    return fServerUser + "@" + fServer;
}

const QString &Contact::getServerUser() const
{
    return fServerUser;
}

const QString &Contact::getServer() const
{
    return fServer;
}

void Contact::setServerUser(const QString &serverUser)
{
    fServerUser = serverUser;
}

void Contact::setServer(const QString &server)
{
    fServer = server;
}

WP::err Contact::writeConfig()
{
    WP::err error = WP::kOk;
    if (fPrivateKeyStore) {
        fPrivateKeyStore = true;
        error = write("keystore_type", QString("private"));
        if (error != WP::kOk)
            return error;
        error = writeSafe("keyStoreId", fKeys->getKeyStore()->getUid());
        if (error != WP::kOk)
            return error;
    }
    fKeys->setTo(fDatabase, getKeysDirectory());
    error = fKeys->writeConfig();
    if (error != WP::kOk)
        return error;

    error = writeSafe("uid", fUid);
    if (error != WP::kOk)
        return error;

    error = writeSafe("nickname", fNickname);
    if (error != WP::kOk)
        return error;

    error = writeSafe("serverUser", fServerUser);
    if (error != WP::kOk)
        return error;

    error = writeSafe("server", fServer);
    if (error != WP::kOk)
        return error;

    return error;
}

QString Contact::getKeysDirectory() const
{
    return fDirectory + "/keys";
}

ContactKeys::ContactKeys(EncryptedUserData *database, const QString &directory) :
    StorageDirectory(database, directory)
{

}

const QString ContactKeys::getMainKeyId() const
{
    return fMainKeyId;
}

WP::err ContactKeys::setMainKeyId(const QString &keyId)
{
    fMainKeyId = keyId;
}

WP::err ContactKeys::open()
{
    return readSafe("mainKeyId", fMainKeyId);
}

WP::err ContactKeys::writeConfig()
{
    return writeSafe("mainKeyId", fMainKeyId);
}

KeyStore *ContactKeys::getKeyStore()
{
    return NULL;
}


ContactKeysKeyStore::ContactKeysKeyStore(EncryptedUserData *database, const QString &directory,
                                         KeyStore *keyStore) :
    ContactKeys(database, directory),
    fKeyStore(keyStore)
{

}

WP::err ContactKeysKeyStore::writeConfig()
{
    QString keyStoreId = fDatabase->getKeyStore()->getUid();
    WP::err error = WP::kOk;
    foreach (const QString &keyId, fKeyIdList) {
        error = write(keyId + "/keystore", keyStoreId);
        if (error != WP::kOk)
            return error;
    }

    return ContactKeys::writeConfig();
}

WP::err ContactKeysKeyStore::open()
{
    WP::err error = ContactKeys::open();
    if (error != WP::kOk)
        return error;

    fKeyIdList = fDatabase->listDirectories("");
    return error;
}

WP::err ContactKeysKeyStore::getKeySet(const QString &keyId, QString &certificate,
                                       QString &publicKey, QString &privateKey) const
{
    if (!fKeyIdList.contains(keyId))
        return WP::kEntryNotFound;
    return fKeyStore->readAsymmetricKey(keyId, certificate, publicKey, privateKey);
}

WP::err ContactKeysKeyStore::addKeySet(const QString &keyId, bool setAsMainKey)
{
    // first test if the keyId is valid
    QString certificate;
    QString publicKey;
    QString privateKey;
    WP::err error = fKeyStore->readAsymmetricKey(keyId, certificate, publicKey,
                                                                privateKey);
    if (error != WP::kOk)
        return error;
    fKeyIdList.append(keyId);
    if (setAsMainKey)
        setMainKeyId(keyId);
    return WP::kOk;
}

WP::err ContactKeysKeyStore::getKeySet(const QString &keyId, QString &certificate,
                                       QString &publicKey) const
{
    QString privateKey;
    return getKeySet(keyId, certificate, publicKey, privateKey);
}

WP::err ContactKeysKeyStore::addKeySet(const QString &keyId, const QString &certificate,
                                       const QString &publicKey)
{
    return WP::kError;
}

KeyStore *ContactKeysKeyStore::getKeyStore()
{
    return fKeyStore;
}


ContactKeysBuddies::ContactKeysBuddies(EncryptedUserData *database, const QString &directory) :
    ContactKeys(database, directory)
{
}

WP::err ContactKeysBuddies::writeConfig()
{
    WP::err error = WP::kOk;
    QMap<QString, PublicKeySet>::iterator it;
    for (it = fKeyMap.begin(); it != fKeyMap.end(); it++) {
        QString keyId = it.key();
        error = writeSafe(keyId + "/certificate", it.value().certificate);
        if (error != WP::kOk)
            return error;
        error = write(keyId + "/publicKey", it.value().publicKey);
        if (error != WP::kOk)
            return error;
    }

    return ContactKeys::writeConfig();
}

WP::err ContactKeysBuddies::open()
{
    WP::err error = WP::kOk;

    QStringList keyIds = fDatabase->listDirectories("");
    foreach (const QString &keyId, keyIds) {
        PublicKeySet keySet;
        error = readSafe(keyId + "/certificate", keySet.certificate);
        if (error != WP::kOk)
            return error;
        // the public key is needed by the server to verify incoming data so it can't be stored encrypted
        error = read(keyId + "/publicKey", keySet.publicKey);
        if (error != WP::kOk)
            return error;
        fKeyMap[keyId] = keySet;
    }

    return error;
}

WP::err ContactKeysBuddies::getKeySet(const QString &keyId, QString &certificate,
                                      QString &publicKey, QString &privateKey) const
{
    return WP::kError;
}

WP::err ContactKeysBuddies::addKeySet(const QString &keyId, bool setAsMainKey)
{
    return WP::kError;
}

WP::err ContactKeysBuddies::getKeySet(const QString &keyId, QString &certificate,
                                      QString &publicKey) const
{
    QMap<QString, PublicKeySet>::const_iterator it = fKeyMap.find(keyId);
    if (it == fKeyMap.end())
        return WP::kEntryNotFound;

    certificate = it.value().certificate;
    publicKey = it.value().publicKey;
    return WP::kOk;
}

WP::err ContactKeysBuddies::addKeySet(const QString &keyId, const QString &certificate,
                                      const QString &publicKey)
{
    PublicKeySet keySet;
    keySet.certificate = certificate;
    keySet.publicKey = publicKey;

    fKeyMap[keyId] = keySet;
    return WP::kOk;
}

#ifndef CONTACT_H
#define CONTACT_H

#include "databaseutil.h"

#include <QMap>
#include <QStringList>


class ContactKeys : public StorageDirectory {
public:
    ContactKeys(EncryptedUserData *database, const QString &directory);
    virtual ~ContactKeys() {}

    const QString getMainKeyId() const;
    WP::err setMainKeyId(const QString &keyId);

    virtual WP::err open();
    virtual WP::err writeConfig();

    // methods for keystore version
    virtual WP::err getKeySet(const QString &keyId, QString &certificate, QString &publicKey,
                            QString &privateKey) const = 0;
    virtual WP::err addKeySet(const QString &keyId, bool setAsMainKey = false) = 0;

    // methods for remote contacts where we don't have the private key
    virtual WP::err getKeySet(const QString &keyId, QString &certificate,
                            QString &publicKey) const = 0;
    virtual WP::err addKeySet(const QString &keyId, const QString &certificate,
                              const QString &publicKey) = 0;

    virtual KeyStore *getKeyStore();

protected:
    QString fMainKeyId;
};


class ContactKeysKeyStore : public ContactKeys {
public:
    ContactKeysKeyStore(EncryptedUserData *database, const QString &directory, KeyStore *keyStore);

    virtual WP::err writeConfig();
    virtual WP::err open();

    // methods for keystore version
    virtual WP::err getKeySet(const QString &keyId, QString &certificate, QString &publicKey,
                            QString &privateKey) const;
    virtual WP::err addKeySet(const QString &keyId, bool setAsMainKey = false);

    // methods for remote contacts where we don't have the private key
    virtual WP::err getKeySet(const QString &keyId, QString &certificate,
                            QString &publicKey) const;
    virtual WP::err addKeySet(const QString &keyId, const QString &certificate,
                              const QString &publicKey);

    virtual KeyStore *getKeyStore();
private:
    KeyStore *fKeyStore;
    QStringList fKeyIdList;
};


class ContactKeysBuddies : public ContactKeys {
public:
    ContactKeysBuddies(EncryptedUserData *database, const QString &directory);

    virtual WP::err writeConfig();
    virtual WP::err open();

    // methods for keystore version
    virtual WP::err getKeySet(const QString &keyId, QString &certificate, QString &publicKey,
                            QString &privateKey) const;
    virtual WP::err addKeySet(const QString &keyId, bool setAsMainKey = false);

    // methods for remote contacts where we don't have the private key
    virtual WP::err getKeySet(const QString &keyId, QString &certificate,
                            QString &publicKey) const;
    virtual WP::err addKeySet(const QString &keyId, const QString &certificate,
                              const QString &publicKey);

private:
    class PublicKeySet {
    public:
        QString certificate;
        QString publicKey;
    };

    QMap<QString, PublicKeySet> fKeyMap;
};


class Contact : public StorageDirectory {
public:
    Contact(const QString &uid, const QString &nickname, const QString &keyId,
            const QString &certificate, const QString &publicKey);
    Contact(EncryptedUserData *database, const QString &directory);
    virtual ~Contact();

    virtual WP::err createUserIdentityContact(KeyStore *keyStore, const QString &keyId,
                                              const QString &userNickname);
    virtual WP::err open(KeyStoreFinder *keyStoreFinder);

    QString getUid() const;
    ContactKeys* getKeys();
    virtual const QString &getNickname() const;

    // build the mail address
    virtual const QString getAddress() const;
    const QString &getServerUser() const;
    const QString &getServer() const;

    void setServerUser(const QString &serverUser);
    void setServer(const QString &server);

    virtual WP::err writeConfig();

private:
    QString getKeysDirectory() const;

    bool fPrivateKeyStore;
    QString fUid;
    ContactKeys *fKeys;
    QString fNickname;

    QString fServerUser;
    QString fServer;
};


#endif // CONTACT_H

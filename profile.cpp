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
        return fIdentities.at(index.row())->identity()->getId();

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


Profile::Profile(const QString &path, const QString &branch) :
    fDatabase(NULL),
    fKeyStore(NULL),
    fDatabasePath(path),
    fDatabaseBranch(branch)
{
    fCrypto = CryptoInterfaceSingleton::getCryptoInterface();
}

Profile::~Profile()
{
    delete fDatabase;
}

int Profile::createNewProfile(const SecureArray &password)
{
    int error = DatabaseFactory::open(fDatabasePath, fDatabaseBranch, &fDatabase);
    if (error != 0)
        return error;

    return 0;
}

int Profile::open(const SecureArray &password)
{
    if (fDatabase != NULL)
        return -1;
    int error = DatabaseFactory::open(fDatabasePath, fDatabaseBranch, &fDatabase);
    if (error != 0)
        return error;

    QStringList ids = UserIdentity::getIdenties(fDatabase);
    for (int i = 0; i < ids.count(); i++) {
        UserIdentity *id = new UserIdentity(fDatabasePath, fDatabaseBranch);
        QString keyStoreId = id->getKeyStoreId();
        KeyStore *keyStore = findKeyStore(keyStoreId);
        if (keyStore == NULL) {
            delete id;
            continue;
        }
        id->setTo(keyStore);
        addIdentity(id);
    }
    return 0;
}

int Profile::commit()
{
    return fDatabase->commit();
}

ProfileEntryIdentity *Profile::addIdentity(UserIdentity *identity)
{
    // TODO FIX HACK
    ProfileEntryIdentity *entry = new ProfileEntryIdentity(identity, "local?", "identities");
    fIdentities.addIdentity(entry);
    return entry;
}

int Profile::writeEntry(const ProfileEntry *entry)
{
    return entry->write(fDatabase, fCrypto);
}


IdentityListModel *Profile::getIdentityList()
{
    return &fIdentities;
}

KeyStore *Profile::findKeyStore(const QString &keyStoreId)
{
    TODO
    return NULL;
}


ProfileEntry::ProfileEntry(const QString &location, const QString &branch)
    : fLocation(location),
      fBranch(branch)
{
}

int ProfileEntry::write(DatabaseInterface *database, CryptoInterface *crypto) const
{
    database->setBranch(fBranch);

    const QString baseDir = fLocation + "/" + hash(crypto);

    QString path = baseDir + "/location";
    database->add(path, fLocation.toAscii());
    path = baseDir + "/branch";
    database->add(path, fBranch.toAscii());
    path = baseDir + "/type";
    database->add(path, fBranch.toAscii());
    return 0;
}

int ProfileEntry::load(DatabaseInterface *database, CryptoInterface *crypto)
{
    database->setBranch(fBranch);

    const QString baseDir = fLocation + "/" + hash(crypto);

    QString path = baseDir + "/location";
    QByteArray locationArray;
    database->get(path, locationArray);
    fLocation = locationArray;
    path = baseDir + "/branch";
    QByteArray branchArray;
    database->get(path, branchArray);
    fBranch = branchArray;

    return 0;
}

QString ProfileEntry::hash(CryptoInterface *crypto) const
{
    QByteArray hashData = QString(fLocation + fBranch).toAscii();
    QByteArray hashResult = crypto->sha1Hash(hashData);
    return crypto->toHex(hashResult);
}

ProfileEntryIdentity::ProfileEntryIdentity(UserIdentity *identity, const QString &location,
                                   const QString &branch)
    : ProfileEntry(location, branch),
      fIdentity(identity)
{
}

UserIdentity *ProfileEntryIdentity::identity()
{
    return fIdentity;
}

#ifndef USERIDENTITY_H
#define USERIDENTITY_H


#include <QMap>
#include <QString>
#include <QTextStream>

#include <cryptointerface.h>
#include <databaseinterface.h>
#include <databaseutil.h>


/*! UserIdentity database structure:
branch identities:
\id (sha1 hash of the certificate)
- master_key: encrypted master key
- master_key_info: master password encryption info: salt, encryption algo, iteration algo, number of iterations
- data_encryption_algo: algo to encrypt all identity data with the master_key
- signature_name
- identity_name
--\private_keys
--\public_keys
--\certificates
--\details
  - first_name
  - family_name
  - icon
--\contacts
}
 */
class UserIdentity : public EncryptedUserData
{
public:
    UserIdentity(const QString &path, const QString &branch, const QString &baseDir = "");
    ~UserIdentity();

    WP::err createNewIdentity(bool addUidToBaseDir = true);
    WP::err open();

    static QStringList getIdenties(DatabaseInterface *database, const QString branch = "identities");

            //! discare changes and reload values from database
            int                 reload();


            QString             getSignatureName();
            // keys
            QList<QString>      getPublicKeyNames();
            QList<QString>      getPrivateKeyNames();
            QList<QString>      getCertificateNames();

            QByteArray          getPublicKey(QString name);
            SecureArray    getPrivateKey(QString name);
            QByteArray          getCertificate(QString name);

            // contacts


            // channels

private:
    QString fIdentityKey;
};

#endif // USERIDENTITY_H

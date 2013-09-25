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
class UserIdentity : public DatabaseEncryption
{
public:
    UserIdentity(DatabaseInterface *database, CryptoInterface *crypto,
                 const QString branch = "identities");

    int createNewIdentity(const SecureArray& password);
    int open(const SecureArray &password, const QString id);

    static QStringList getIdenties(DatabaseInterface *database, const QString branch = "identities");

    QString getId();

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
    QString fIdentityName;
    SecureArray fMasterKey;
    QByteArray fMasterKeyIV;
};

#endif // USERIDENTITY_H

#ifndef USERIDENTITY_H
#define USERIDENTITY_H


#include <QMap>
#include <QString>
#include <QTextStream>

#include <cryptointerface.h>
#include <databaseinterface.h>


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
class UserIdentity
{
public:
    UserIdentity();
    UserIdentity(DatabaseInterface *database, const QString id, const SecureArray &password, const QString branch = "identities");

    static int createNewIdentity(DatabaseInterface *database, const SecureArray& password,
                                 const QString branch = "identities");

    static QStringList getIdenties(DatabaseInterface *database, const QString branch = "identities");

    void printToStream(QTextStream& stream);

            //! write changes to database
            int                 commit();
            //! discare changes and reload values from database
            int                 reload();

            void                setIdentityName(const QString& id);
            QString             getIdentityName();

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
    DatabaseInterface *fDatabase;
    QString fBranch;
    QString fIdentityName;
    SecureArray fMasterKey;
};

#endif // USERIDENTITY_H

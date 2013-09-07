#ifndef CRYPTOINTERFACE_H
#define CRYPTOINTERFACE_H

#include <QtCrypto/QtCrypto>


class CryptoInterface
{
public:
    CryptoInterface();

    void generateKeyPair(const char* certificateFile, const char* publicKeyFile, const char* privateKeyFile, const char *keyPassword);
    void encryptionTest(const char* certificateFile, const char* publicKeyFile, const char* privateKeyFile, const char *keyPassword);

    int generateKeyPair(QString &certificate, QString &publicKey,
                        QString &privateKey, const QCA::SecureArray &keyPassword);

    QCA::SecureArray deriveKey(const QCA::SecureArray &secret, const QString& kdf, const QString &kdfAlgo, const QCA::SecureArray &salt,
                                         unsigned int keyLength, unsigned int iterations);

    QByteArray generateSalt(const QString& value);
    QByteArray generateInitalizationVector(int size);
    QCA::SecureArray generateSymetricKey(int size);

    int encryptSymmetric(const QCA::SecureArray &input, QByteArray &encrypted, const QCA::SecureArray &key, const QByteArray &iv);
    int decryptSymmetric(const QByteArray &input, QCA::SecureArray &decrypted, const QCA::SecureArray &key, const QByteArray &iv);

    int encyrptData(const QByteArray &input, QByteArray &encrypted, const QByteArray& certificate);
    int decryptData(const QByteArray &input, QByteArray &plain, const QString &privateKey,
                    const char *keyPassword, const QByteArray& certificate);

private:
    bool        encyrptMessage(const QByteArray& input, QByteArray& encrypted, const char* certificateFile);
    bool        decryptMessage(const QByteArray& input, QByteArray& plain,
                               const char* privateKeyFile, const char *keyPassword, const char* certificateFile);

    int         sign(const QByteArray& input, QByteArray &signatur, const char* privateKeyFile, const char *keyPassword);
    bool        verifySignatur(const QByteArray& message, const QByteArray &signatur, const char* publicKeyFile);
};

#endif // CRYPTOINTERFACE_H

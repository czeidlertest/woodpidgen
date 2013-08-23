#ifndef CRYPTOINTERFACE_H
#define CRYPTOINTERFACE_H

#include <QtCrypto/QtCrypto>


class CryptoInterface
{
public:
    CryptoInterface();

    void generateKeyPair(const char* certificateFile, const char* publicKeyFile, const char* privateKeyFile, const char *keyPassword);
    void encryptionTest(const char* certificateFile, const char* publicKeyFile, const char* privateKeyFile, const char *keyPassword);

private:
    bool        encyrptMessage(const QByteArray& input, QByteArray& encrypted, const char* certificateFile);
    bool        decryptMessage(const QByteArray& input, QByteArray& plain,
                               const char* privateKeyFile, const char *keyPassword, const char* certificateFile);

    int         sign(const QByteArray& input, QByteArray &signatur, const char* privateKeyFile, const char *keyPassword);
    bool        verifySignatur(const QByteArray& message, const QByteArray &signatur, const char* publicKeyFile);
};

#endif // CRYPTOINTERFACE_H

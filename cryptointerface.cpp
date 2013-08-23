#include "cryptointerface.h"

#include <stdio.h>


CryptoInterface::CryptoInterface()
{

}

void CryptoInterface::generateKeyPair(const char* certificateFile, const char *publicKeyFile, const char *privateKeyFile, const char *keyPassword)
{
    QCA::Initializer init;
    if(!QCA::isSupported("pkey") || !QCA::PKey::supportedIOTypes().contains(QCA::PKey::RSA)) {
        printf("RSA not supported!\n");
        return;
    }

    QCA::PrivateKey secretKey = QCA::KeyGenerator().createRSA(2048);
    if(secretKey.isNull()) {
        printf("Failed to make private RSA key\n");
        return;
    }

    secretKey.toPEMFile(privateKeyFile, keyPassword);

    // public
    QCA::CertificateOptions opts;
    QCA::Certificate cert(opts, secretKey);

    cert.toPEMFile(certificateFile);

    QCA::PublicKey pubkey = secretKey.toPublicKey();
    pubkey.toPEMFile(publicKeyFile);
}

#include <iostream>
void CryptoInterface::encryptionTest(const char* certificateFile, const char *publicKeyFile, const char *privateKeyFile, const char *keyPassword)
{
    const char* testMessage = "Hallo Welt";
    QByteArray encrypted;
    encyrptMessage(testMessage, encrypted, certificateFile);
    QByteArray plain;
    decryptMessage(encrypted, plain, privateKeyFile, keyPassword, certificateFile);

    // sign
    QByteArray signatur;
    sign(testMessage, signatur, privateKeyFile, keyPassword);
    // output the resulting signature
    QString rstr = QCA::arrayToHex(signatur);
    std::cout << "Signature for \"" << testMessage << "\" using RSA, is ";
    std::cout << "\"" << qPrintable(rstr) << "\"" << std::endl;

    verifySignatur(testMessage, signatur, publicKeyFile);

}

bool CryptoInterface::encyrptMessage(const QByteArray &input, QByteArray &encrypted, const char *certificateFile)
{
    QCA::Initializer init;

    // Read in a matching public key cert
    // you could also build this using the fromPEMFile() method
    QCA::Certificate pubCert(certificateFile);
    if (pubCert.isNull()) {
        std::cout << "Sorry, could not import public key certificate" << std::endl;
        return false;
    }
    // We are building the certificate into a SecureMessageKey object, via a
    // CertificateChain
    QCA::SecureMessageKey secMsgKey;
    QCA::CertificateChain chain;
    chain += pubCert;
    secMsgKey.setX509CertificateChain(chain);

    // build up a SecureMessage object, based on our public key certificate
    QCA::CMS cms;
    QCA::SecureMessage msg(&cms);
    msg.setRecipient(secMsgKey);

    // Now use the SecureMessage object to encrypt the plain text.
    msg.startEncrypt();
    msg.update(input);
    msg.end();
    // I think it is reasonable to wait for 1 second for this
    msg.waitForFinished(1000);

    // check to see if it worked
    if(!msg.success())
    {
        std::cout << "Error encrypting: " << msg.errorCode() << std::endl;
        return false;
    }

    // get the result
    encrypted = msg.read();
    QCA::Base64 enc;
    std::cout << input.data() << " encrypts to (in base 64): ";
    std::cout << qPrintable(enc.arrayToString(encrypted)) << std::endl;
    return true;
}

bool CryptoInterface::decryptMessage(const QByteArray &input, QByteArray &plain,
            const char *privateKeyFile, const char *keyPassword, const char *certificateFile)
{
    QCA::Initializer init;

    QCA::PrivateKey privKey;
    QCA::ConvertResult convRes;
    privKey = QCA::PrivateKey::fromPEMFile(privateKeyFile, keyPassword, &convRes);
    if (convRes != QCA::ConvertGood) {
        std::cout << "Sorry, could not import Private Key" << std::endl;
        return false;
    }

    // Read in a matching public key cert
    // you could also build this using the fromPEMFile() method
    QCA::Certificate pubCert(certificateFile);
    if (pubCert.isNull()) {
        std::cout << "Sorry, could not import public key certificate" << std::endl;
        return false;
    }
    // We are building the certificate into a SecureMessageKey object, via a
    // CertificateChain
    QCA::CertificateChain chain;
    chain += pubCert;

    QCA::SecureMessageKeyList skeys;
    QCA::SecureMessageKey skey;
    skey.setX509CertificateChain(chain);
    skey.setX509PrivateKey(privKey);
    skeys += skey;
    QCA::CMS sms;
    sms.setPrivateKeys(skeys);

    QCA::SecureMessage decryptedMessage(&sms);
    decryptedMessage.startDecrypt();
    decryptedMessage.update(input);
    decryptedMessage.end();
    decryptedMessage.waitForFinished(-1);

    plain = decryptedMessage.read();
    QCA::Base64 enc;
    std::cout << qPrintable(enc.arrayToString(input));
    std::cout << " (in base 64) decrypts to: ";
    std::cout << plain.data() << std::endl;

    return true;
}

int CryptoInterface::sign(const QByteArray &input, QByteArray& signatur, const char *privateKeyFile, const char *keyPassword)
{
    QCA::Initializer init;

    QCA::PrivateKey privateKey;
    QCA::ConvertResult convRes;
    privateKey = QCA::PrivateKey::fromPEMFile(privateKeyFile, keyPassword, &convRes);
    if (convRes != QCA::ConvertGood) {
        std::cout << "Sorry, could not import Private Key" << std::endl;
        return -1;
    }
    if(!privateKey.canSign()) {
        std::cout << "Error: this kind of key cannot sign" << std::endl;
        return -1;
    }
    privateKey.startSign(QCA::EMSA3_MD5);
    privateKey.update(input); // just reuse the same message
    signatur = privateKey.signature();
    return 0;
}

bool CryptoInterface::verifySignatur(const QByteArray& message, const QByteArray& signatur, const char *publicKeyFile)
{
    QCA::Initializer init;

    QCA::PublicKey publicKey;
    QCA::ConvertResult convRes;
    publicKey = QCA::PublicKey::fromPEMFile(publicKeyFile, &convRes);
    if (convRes != QCA::ConvertGood) {
        std::cout << "Sorry, could not import Public Key" << std::endl;
        return false;
    }
    // to check a signature, we must check that the key is
    // appropriate
    if(publicKey.canVerify()) {
        publicKey.startVerify(QCA::EMSA3_MD5);
        publicKey.update(message);
        if (publicKey.validSignature(signatur) ) {
            std::cout << "Signature is valid" << std::endl;
            return true;
        } else {
            std::cout << "Bad signature" << std::endl;
            return false;
        }
    }
    return true;
}
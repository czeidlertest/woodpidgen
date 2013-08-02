#include "cryptolib.h"

//#define __UNIX__


#include <stdio.h>
#include <string.h>

CryptoLib::CryptoLib()
{
    C_RET error = cryptInit();
    if (error != CRYPT_OK)
        printf("cryptInit failed: %i\n", error);
    cryptAddRandom(NULL, CRYPT_RANDOM_SLOWPOLL);

}


CryptoLib::~CryptoLib()
{
    cryptEnd();
}


void CryptoLib::AddKey(const char *keyFile, const char *keyName, const char* keyPassword)
{
    CRYPT_CONTEXT privKeyContext;
    cryptCreateContext(&privKeyContext, CRYPT_UNUSED, CRYPT_ALGO_RSA);
    cryptSetAttributeString(privKeyContext, CRYPT_CTXINFO_LABEL, keyName, strlen(keyName));
    cryptGenerateKey(privKeyContext);

    CRYPT_KEYSET cryptKeyset;
    C_RET error = cryptKeysetOpen(&cryptKeyset, CRYPT_UNUSED, CRYPT_KEYSET_FILE, keyFile, CRYPT_KEYOPT_CREATE);
    if (error != CRYPT_OK)
        printf("cryptKeysetOpen failed: %i\n", error);
    /* Load/store keys */
    cryptAddPrivateKey(cryptKeyset, privKeyContext, keyPassword);



    CRYPT_CERTIFICATE cryptCertificate;
    // Create a self-signed CA certificate
    cryptCreateCert(&cryptCertificate, CRYPT_UNUSED, CRYPT_CERTTYPE_CERTIFICATE);
    cryptSetAttribute(cryptCertificate, CRYPT_CERTINFO_XYZZY, 1);
    /* Add the public key and certificate owner name and sign the
    certificate with the private key */
    //TODO: privKeyContext correct??
    cryptSetAttribute(cryptCertificate, CRYPT_CERTINFO_SUBJECTPUBLICKEYINFO, privKeyContext);
    cryptSetAttributeString(cryptCertificate, CRYPT_CERTINFO_COMMONNAME, "Dave Smith", 10);

    // Sign the certificate with the private key and update the still-open keyset with it
    cryptSignCert(cryptCertificate, privKeyContext );
    cryptAddPublicKey(cryptKeyset, cryptCertificate);

    cryptKeysetClose(cryptKeyset);
    cryptDestroyObject(cryptCertificate);
    cryptDestroyContext(privKeyContext);
}

int CryptoLib::GetPublicKey(const char *keyFile, const char *keyName, CRYPT_HANDLE& publicKey)
{
    CRYPT_KEYSET cryptKeyset;
    C_RET status = cryptKeysetOpen(&cryptKeyset, CRYPT_UNUSED, CRYPT_KEYSET_FILE, keyFile, CRYPT_KEYOPT_READONLY);
    if (status != CRYPT_OK)
        printf("cryptKeysetOpen failed: %i\n", status);
    /* Load/store keys */
    status = cryptGetPublicKey(cryptKeyset, &publicKey, CRYPT_KEYID_NAME, keyName);
    if (status != CRYPT_OK)
        printf("cryptGetPublicKey failed: %i\n", status);
    cryptKeysetClose(cryptKeyset);
    return status;
}

int CryptoLib::PGPEncrypte(CRYPT_HANDLE publicKey, const void* bufferIn, int inLength, void *bufferOut, int outLength)
{
    CRYPT_ENVELOPE cryptEnvelope;
    //C_RET status = cryptCreateEnvelope(&cryptEnvelope, CRYPT_UNUSED, CRYPT_FORMAT_CRYPTLIB);
    C_RET status = cryptCreateEnvelope(&cryptEnvelope, CRYPT_UNUSED, CRYPT_FORMAT_PGP);
    if (status != CRYPT_OK)
        printf("PGPEncrypte: cryptCreateEnvelope failed: %i\n", status);

    int bytesCopied;

    // Add the public key
    cryptSetAttribute(cryptEnvelope, CRYPT_ENVINFO_PUBLICKEY, publicKey);

    /* Add the data size information and data, wrap up the processing, and
    pop out the processed data */
    cryptSetAttribute(cryptEnvelope, CRYPT_ENVINFO_DATASIZE, inLength);
    cryptPushData(cryptEnvelope, bufferIn, inLength, &bytesCopied);
    cryptFlushData(cryptEnvelope);
    status = cryptPopData(cryptEnvelope, bufferOut, outLength, &bytesCopied);

    if (status != CRYPT_OK)
        printf("cryptPopData failed: %i\n", status);
    /* Clean up */
    cryptDestroyEnvelope(cryptEnvelope);
    return bytesCopied;
}

int CryptoLib::PGPEncrypte(CRYPT_KEYSET &privKeyKeyset, const char *keyName, const void *bufferIn, int inLength, void *bufferOut, int outLength)
{
    CRYPT_ENVELOPE cryptEnvelope;
    //C_RET status = cryptCreateEnvelope(&cryptEnvelope, CRYPT_UNUSED, CRYPT_FORMAT_CRYPTLIB);
    C_RET status = cryptCreateEnvelope(&cryptEnvelope, CRYPT_UNUSED, CRYPT_FORMAT_PGP);
    if (status != CRYPT_OK)
        printf("PGPEncrypte: cryptCreateEnvelope failed: %i\n", status);

    int bytesCopied;

    // Add the encryption keyset and recipient email address
    cryptSetAttribute(cryptEnvelope, CRYPT_ENVINFO_KEYSET_ENCRYPT, privKeyKeyset);
    cryptSetAttributeString(cryptEnvelope, CRYPT_ENVINFO_RECIPIENT, keyName, strlen(keyName));

    /* Add the data size information and data, wrap up the processing, and
    pop out the processed data */
    cryptSetAttribute(cryptEnvelope, CRYPT_ENVINFO_DATASIZE, inLength);
    cryptPushData(cryptEnvelope, bufferIn, inLength, &bytesCopied);
    cryptFlushData(cryptEnvelope);
    status = cryptPopData(cryptEnvelope, bufferOut, outLength, &bytesCopied);

    if (status != CRYPT_OK)
        printf("cryptPopData failed: %i\n", status);
    /* Clean up */
    cryptDestroyEnvelope(cryptEnvelope);
    return bytesCopied;
}

void CryptoLib::PGPDecrypte(CRYPT_KEYSET& privKeyKeyset, const char* password, const void *bufferIn, int inLength, void *bufferOut, int outLength)
{
    CRYPT_ENVELOPE cryptEnvelope;
    int bytesCopied, status;
    // Create the envelope and add the private key keyset and data
    status = cryptCreateEnvelope(&cryptEnvelope, CRYPT_UNUSED, CRYPT_FORMAT_AUTO);
    if (status != CRYPT_OK)
        printf("PGPDecrypte: cryptCreateEnvelope failed: %i\n", status);
    status = cryptSetAttribute(cryptEnvelope, CRYPT_ENVINFO_KEYSET_DECRYPT,
        privKeyKeyset);
    if (status != CRYPT_OK)
        printf("PGPDecrypte: cryptSetAttribute CRYPT_ENVINFO_KEYSET_DECRYPT failed: %i\n", status);

    status = cryptPushData(cryptEnvelope, bufferIn, inLength, &bytesCopied);
    if (status != CRYPT_OK)
        printf("PGPDecrypte: cryptPushData failed: %i bytesCopied: %i inLength: %i\n", status, bytesCopied, inLength);

    // Find out what we need to continue and, if it's a private key, add the password to recover it
    CRYPT_ATTRIBUTE_TYPE requiredAttribute;
    cryptGetAttribute(cryptEnvelope, CRYPT_ATTRIBUTE_CURRENT, (int*)&requiredAttribute);
    if (requiredAttribute != CRYPT_ENVINFO_PRIVATEKEY) {
        printf("PGPDecrypte: error\n");
    }

    char label[CRYPT_MAX_TEXTSIZE + 1];
    int labelLength;
    status = cryptGetAttributeString(cryptEnvelope, CRYPT_ENVINFO_PRIVATEKEY_LABEL, label, &labelLength);
    if (status != CRYPT_OK)
        printf("PGPDecrypte: cryptGetAttributeString CRYPT_ENVINFO_PRIVATEKEY_LABEL failed: %i\n", status);
    label[labelLength] = '\0';
    printf("password requested for label: %s\n", label);

    status = cryptSetAttributeString(cryptEnvelope, CRYPT_ENVINFO_PASSWORD, password, strlen(password));
    if (status != CRYPT_OK)
        printf("PGPDecrypte: cryptSetAttributeString CRYPT_ENVINFO_PASSWORD failed: %i\n", status);

    status = cryptFlushData(cryptEnvelope);
    if (status != CRYPT_OK)
        printf("PGPDecrypte: cryptFlushData failed: %i\n", status);
    /* Pop the data and clean up */
    status = cryptPopData(cryptEnvelope, bufferOut, outLength, &bytesCopied);
    if (status != CRYPT_OK)
        printf("PGPDecrypte: cryptPopData failed: %i\n", status);
    cryptDestroyEnvelope(cryptEnvelope);
}


void CryptoLib::EncrypteData()
{
    C_RET error;

    const char* kKeyFileName = "keyset.p15";
    const char* kKeyName = "test_label";
    const char* kKeyPassword = "test1";
    AddKey(kKeyFileName, kKeyName, kKeyPassword);

    //const char* kKeyFileNamePublic = "pub_rsa.gpg";
    //const char* kKeyFileNamePrivate = "sec_rsa.gpg";

    const char* kKeyFileNamePublic = "keyset.p15";
    const char* kKeyFileNamePrivate = "keyset.p15";

    CRYPT_KEYSET cryptKeyset;
    error = cryptKeysetOpen(&cryptKeyset, CRYPT_UNUSED, CRYPT_KEYSET_FILE, kKeyFileNamePublic, CRYPT_KEYOPT_READONLY);
    if (error != CRYPT_OK)
        printf("cryptKeysetOpen failed: %i\n", error);

    //CRYPT_HANDLE publicKey;
    //GetPublicKey(kKeyFileName, kKeyName, publicKey);
    const int bufferLength = 1024;
    char buffer[bufferLength];
    int outSize = PGPEncrypte(cryptKeyset, kKeyName, "Test Messag2", 12, buffer, bufferLength);
    printf("crypt: %s\n", buffer);
    //cryptDestroyObject(publicKey);
    cryptKeysetClose(cryptKeyset);

     // decrypte
    error = cryptKeysetOpen(&cryptKeyset, CRYPT_UNUSED, CRYPT_KEYSET_FILE, kKeyFileNamePrivate, CRYPT_KEYOPT_READONLY);
    if (error != CRYPT_OK)
        printf("cryptKeysetOpen failed: %i\n", error);
    PGPDecrypte(cryptKeyset, kKeyPassword, buffer, outSize, buffer, bufferLength);
    printf("decrypt: %s\n", buffer);
    cryptKeysetClose(cryptKeyset);
}

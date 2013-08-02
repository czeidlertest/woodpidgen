#ifndef CRYPTOLIB_H
#define CRYPTOLIB_H

#include "cryptlib.h"
//#include "crypt.h"
//#include "cryptkrn.h"


class CryptoLib
{
public:
    CryptoLib();
    ~CryptoLib();

    void AddKey(const char* keyFile, const char* keyName, const char *keyPassword);
    int GetPublicKey(const char* keyFile, const char* keyName, CRYPT_HANDLE &publicKey);

    int PGPEncrypte(CRYPT_HANDLE key, const void* bufferIn, int inLength, void *bufferOut, int outLength);
    int PGPEncrypte(CRYPT_KEYSET &privKeyKeyset, const char* keyName, const void* bufferIn, int inLength, void *bufferOut, int outLength);
    void PGPDecrypte(CRYPT_KEYSET &privKeyKeyset, const char* password, const void* bufferIn, int inLength, void *bufferOut, int outLength);
    void EncrypteData();
};

#endif // CRYPTOLIB_H

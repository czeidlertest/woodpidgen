#include "mail.h"

#include <QString>

RawMailMessage::RawMailMessage(const QString &header, const QString &body)
{
    fHeader.append(header);
    fBody.append(body);
}

const QByteArray &RawMailMessage::getHeader() const
{
    return fHeader;
}

const QByteArray &RawMailMessage::getBody() const
{
    return fBody;
}


SecureParcel::SecureParcel()
{

}

void SecureParcel::initNew()
{
    CryptoInterface *crypto = CryptoInterfaceSingleton::getCryptoInterface();
    fIV = crypto->generateInitalizationVector(kSymmetricKeySize);
    fSymmetricKey = crypto->generateSymmetricKey(kSymmetricKeySize);
}

WP::err SecureParcel::initFromPublic(Contact *receiver, const QString &keyId, const QByteArray &iv, const QByteArray &encryptedSymmetricKey)
{
    fIV =iv;

    QString certificate;
    QString publicKey;
    QString privateKey;
    WP::err error = receiver->getKeys()->getKeySet(keyId, certificate, publicKey, privateKey);
    if (error != WP::kOk)
        return error;
    CryptoInterface *crypto = CryptoInterfaceSingleton::getCryptoInterface();
    error = crypto->decryptAsymmetric(encryptedSymmetricKey, fSymmetricKey, privateKey, "", certificate);
    if (error != WP::kOk)
        return error;
    return error;
}

void SecureParcel::initFromPrivate(const QByteArray &iv, const QByteArray &symmetricKey)
{
    fIV = iv;
    fSymmetricKey = symmetricKey;
}

WP::err SecureParcel::cloakData(const QByteArray &data, QByteArray &cloakedData)
{
    if (fSymmetricKey.count() == 0)
        return WP::kNotInit;
    CryptoInterface *crypto = CryptoInterfaceSingleton::getCryptoInterface();
    WP::err error = crypto->encryptSymmetric(data, cloakedData, fSymmetricKey, fIV);
    return error;
}

WP::err SecureParcel::uncloakData(const QByteArray &cloakedData, QByteArray &data)
{
    if (fSymmetricKey.count() == 0)
        return WP::kNotInit;
    CryptoInterface *crypto = CryptoInterfaceSingleton::getCryptoInterface();
    WP::err error = crypto->decryptSymmetric(cloakedData, data, fSymmetricKey, fIV);
    return error;
}

const QByteArray &SecureParcel::getIV() const
{
    return fIV;
}

const QByteArray &SecureParcel::getSymmetricKey() const
{
    return fSymmetricKey;
}

WP::err SecureParcel::getEncryptedSymmetricKey(Contact *receiver, const QString &keyId, QByteArray &encryptedSymmetricKey)
{
    QString certificate;
    QString publicKey;
    WP::err error = receiver->getKeys()->getKeySet(keyId, certificate, publicKey);
    if (error != WP::kOk)
        return error;
    CryptoInterface *crypto = CryptoInterfaceSingleton::getCryptoInterface();
    error = crypto->encyrptAsymmetric(fSymmetricKey, encryptedSymmetricKey, certificate);
    if (error != WP::kOk)
        return error;
    return error;
}


SecureChannel::SecureChannel(SecureChannel *parent) :
    fParent(parent),
    fSecureParcel(NULL)
{

}

SecureChannel::~SecureChannel()
{
    delete fSecureParcel;
}

WP::err SecureChannel::createNew()
{
    CryptoInterface *crypto = CryptoInterfaceSingleton::getCryptoInterface();
    // derive uid
    QByteArray uid = crypto->generateInitalizationVector(512);
    fUid = crypto->toHex(crypto->sha1Hash(uid));

    fSecureParcel = new SecureParcel();
    fSecureParcel->initNew();
    return WP::kOk;
}

void SecureChannel::setSecureParcel(SecureParcel *parcel)
{
    fSecureParcel = parcel;
}

SecureParcel *SecureChannel::getSecureParcel()
{
    if (fSecureParcel != NULL)
        return fSecureParcel;
    else if (fParent != NULL)
        return fParent->getSecureParcel();
    return NULL;
}

const QString &SecureChannel::getUid() const
{
    return fUid;
}

void SecureChannel::setUid(const QString &value)
{
    fUid = value;
}

MessageChannel::MessageChannel(MessageChannel *parent) :
    SecureChannel(parent),
    fCreator(NULL)
{

}

WP::err MessageChannel::createNew(Contact *creator)
{
    WP::err error = SecureChannel::createNew();
    if (error != WP::kOk)
        return error;
    setCreator(creator);
    return WP::kOk;
}

Contact *MessageChannel::getCreator() const
{
    return fCreator;
}

void MessageChannel::setCreator(Contact *creator)
{
    fCreator = creator;
}


const char *kLockStanze = "lock";


WP::err XMLSecureChannel::toPublicXML(ProtocolOutStream *outStream, SecureChannel *channel, Contact *receiver, QString keyId)
{
    if (keyId == "")
        keyId = receiver->getKeys()->getMainKeyId();

    SecureParcel *secureParcel = channel->getSecureParcel();
    QByteArray encryptedSymmetricKey;
    WP::err error = secureParcel->getEncryptedSymmetricKey(receiver, keyId, encryptedSymmetricKey);
    if (error != WP::kOk)
        return error;

    OutStanza *channelStanze = new OutStanza("channel");
    channelStanze->addAttribute("uid", channel->getUid());
    outStream->pushStanza(channelStanze);

        OutStanza *lockStanze = new OutStanza(kLockStanze);
        lockStanze->addAttribute("key_id", keyId);
        outStream->pushChildStanza(lockStanze);

            OutStanza *ivStanza = new OutStanza("iv");
            ivStanza->setText(secureParcel->getIV().toBase64());
            outStream->pushStanza(ivStanza);

            OutStanza *ckeyStanza = new OutStanza("ckey");
            ckeyStanza->setText(encryptedSymmetricKey.toBase64());
            outStream->pushStanza(ckeyStanza);
            outStream->cdDotDot();

        outStream->cdDotDot();
    return WP::kOk;
}

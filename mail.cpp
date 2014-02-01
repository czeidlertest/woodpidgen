#include "mail.h"

#include <QBuffer>
#include <QString>

Message::Message(MessageChannelFinder *_channelFinder) :
    channelFinder(_channelFinder)
{

}

Message::Message(MessageChannel *channel) :
    ChannelParcel(channel)
{

}

const QByteArray &Message::getBody() const
{
    return body;
}

void Message::setBody(const QByteArray &_body)
{
    body = _body;
}

WP::err Message::writeConfidentData(QDataStream &stream)
{
    stream << body;
    return WP::kOk;
}

WP::err Message::readConfidentData(QBuffer &mainData)
{
    char *buffer;
    QDataStream stream(&mainData);
    stream >> buffer;
    body = buffer;
    delete[] buffer;
    return WP::kOk;
}

SecureChannel *Message::findChannel(const QString &channelUid)
{
    return channelFinder->find(channelUid);
}


const QString DataParcel::getSignature() const
{
    return signature;
}

const QString DataParcel::getSignatureKey() const
{
    return signatureKey;
}

const Contact *DataParcel::getSender() const
{
    return sender;
}

QString DataParcel::getUid() const
{
    return uid;
}

WP::err DataParcel::toRawData(Contact *_sender, const QString &_signatureKey, QIODevice &rawData)
{
    sender = _sender;
    signatureKey = _signatureKey;

    CryptoInterface *crypto = CryptoInterfaceSingleton::getCryptoInterface();

    QDataStream stream(&rawData);

    QByteArray containerData;
    QDataStream containerDataStream(containerData);

    containerDataStream << sender->getUid().toLatin1();
    containerDataStream << "\0";
    containerDataStream << signatureKey.toLatin1();
    containerDataStream << "\0";

    QByteArray mainData;
    QDataStream mainDataStream(mainData);
    WP::err error = writeMainData(mainDataStream);
    if (error != WP::kOk)
        return error;
    uid = crypto->toHex(crypto->sha1Hash(mainData));

    containerDataStream << mainData.length();
    containerDataStream << "\0";
    if (containerDataStream.device()->write(mainData) != mainData.length())
        return WP::kError;

    QByteArray signatureHash = crypto->sha2Hash(containerData);
    error = sender->sign(signatureKey, signatureHash, signature);
    if (error != WP::kOk)
        return error;

    // write signature header
    stream << signature;
    stream << "\0";
    if (rawData.write(containerData) != containerData.length())
        return WP::kError;

    return WP::kOk;
}

WP::err DataParcel::fromRawData(ContactFinder *contactFinder, QIODevice &rawData)
{
    char *buffer;
    QDataStream stream(&rawData);
    stream >> buffer;
    signature = buffer;
    delete[] buffer;
    stream >> buffer;
    QString senderUid;
    senderUid = buffer;
    delete[] buffer;
    stream >> buffer;
    signatureKey = buffer;
    delete[] buffer;

    sender = contactFinder->find(senderUid);
    if (sender == NULL)
        return WP::kContactNotFound;

    int mainDataLength;
    stream >> mainDataLength;

    // TODO do proper QIODevice reading
    QByteArray mainData = rawData.read(mainDataLength);
    if (mainData.length() != mainDataLength)
        return WP::kError;


    QBuffer mainDataBuffer(&mainData);
    WP::err error = readMainData(mainDataBuffer);
    if (error != WP::kOk)
        return error;

    // validate data
    CryptoInterface *crypto = CryptoInterfaceSingleton::getCryptoInterface();
    QByteArray signatureHash = crypto->sha2Hash(mainData);
    uid = crypto->toHex(crypto->sha1Hash(mainData));

    if (!sender->verify(signatureKey, signatureHash, signature))
        return WP::kBadValue;

    return WP::kOk;
}

void ParcelCrypto::initNew()
{
    CryptoInterface *crypto = CryptoInterfaceSingleton::getCryptoInterface();
    iv = crypto->generateInitalizationVector(kSymmetricKeySize);
    symmetricKey = crypto->generateSymmetricKey(kSymmetricKeySize);
}

WP::err ParcelCrypto::initFromPublic(Contact *receiver, const QString &keyId, const QByteArray &iv, const QByteArray &encryptedSymmetricKey)
{
    this->iv = iv;

    QString certificate;
    QString publicKey;
    QString privateKey;
    WP::err error = receiver->getKeys()->getKeySet(keyId, certificate, publicKey, privateKey);
    if (error != WP::kOk)
        return error;
    CryptoInterface *crypto = CryptoInterfaceSingleton::getCryptoInterface();
    error = crypto->decryptAsymmetric(encryptedSymmetricKey, symmetricKey, privateKey, "", certificate);
    if (error != WP::kOk)
        return error;
    return error;
}

void ParcelCrypto::initFromPrivate(const QByteArray &_iv, const QByteArray &_symmetricKey)
{
    iv = _iv;
    symmetricKey = _symmetricKey;
}

WP::err ParcelCrypto::cloakData(const QByteArray &data, QByteArray &cloakedData)
{
    if (symmetricKey.count() == 0)
        return WP::kNotInit;
    CryptoInterface *crypto = CryptoInterfaceSingleton::getCryptoInterface();
    WP::err error = crypto->encryptSymmetric(data, cloakedData, symmetricKey, iv);
    return error;
}

WP::err ParcelCrypto::uncloakData(const QByteArray &cloakedData, QByteArray &data)
{
    if (symmetricKey.count() == 0)
        return WP::kNotInit;
    CryptoInterface *crypto = CryptoInterfaceSingleton::getCryptoInterface();
    WP::err error = crypto->decryptSymmetric(cloakedData, data, symmetricKey, iv);
    return error;
}

const QByteArray &ParcelCrypto::getIV() const
{
    return iv;
}

const QByteArray &ParcelCrypto::getSymmetricKey() const
{
    return symmetricKey;
}

WP::err ParcelCrypto::getEncryptedSymmetricKey(Contact *receiver, const QString &keyId, QByteArray &encryptedSymmetricKey)
{
    QString certificate;
    QString publicKey;
    WP::err error = receiver->getKeys()->getKeySet(keyId, certificate, publicKey);
    if (error != WP::kOk)
        return error;
    CryptoInterface *crypto = CryptoInterfaceSingleton::getCryptoInterface();
    error = crypto->encyrptAsymmetric(symmetricKey, encryptedSymmetricKey, certificate);
    if (error != WP::kOk)
        return error;
    return error;
}

SecureChannel::SecureChannel(Contact *_receiver) :
    receiver(_receiver)
{

}

SecureChannel::SecureChannel(Contact *_receiver, const QString &asymKeyId) :
    asymmetricKeyId(asymKeyId),
    receiver(_receiver)
{
    parcelCrypto.initNew();
}

SecureChannel::~SecureChannel()
{
}

WP::err SecureChannel::writeDataSecure(QDataStream &stream, const QByteArray &data)
{
    QByteArray encryptedData;
    WP::err error = parcelCrypto.cloakData(data, encryptedData);
    if (error != WP::kOk)
        return error;

    stream << encryptedData.length();
    stream << "\0";
    if (stream.device()->write(encryptedData) != encryptedData.length())
        return WP::kError;
    return WP::kOk;
}

WP::err SecureChannel::readDataSecure(QDataStream &stream, QByteArray &data)
{
    int length;
    stream >> length;
    QByteArray encryptedData = stream.device()->read(length);

    return parcelCrypto.uncloakData(encryptedData, data);
}

WP::err SecureChannel::writeMainData(QDataStream &stream)
{
    QByteArray encryptedSymmetricKey;
    WP::err error = parcelCrypto.getEncryptedSymmetricKey(receiver, asymmetricKeyId, encryptedSymmetricKey);
    if (error != WP::kOk)
        return error;

    stream << asymmetricKeyId.toLatin1();
    stream << "\0";
    stream << encryptedSymmetricKey;
    stream << "\0";
    stream << parcelCrypto.getIV();
    stream << "\0";
    return WP::kOk;
}

WP::err SecureChannel::readMainData(QBuffer &mainData)
{
    QDataStream inStream(&mainData);

    char *buffer;

    inStream >> buffer;
    asymmetricKeyId = buffer;
    delete[] buffer;

    inStream >> buffer;
    QByteArray encryptedSymmetricKey = buffer;
    delete[] buffer;

    inStream >> buffer;
    QByteArray iv = buffer;
    delete[] buffer;

    return parcelCrypto.initFromPublic(receiver, asymmetricKeyId, iv, encryptedSymmetricKey);
}

QString SecureChannel::getUid() const
{
    QByteArray data;
    data += parcelCrypto.getIV();
    CryptoInterface *crypto = CryptoInterfaceSingleton::getCryptoInterface();
    return crypto->toHex(crypto->sha1Hash(data));
}


MessageChannel::MessageChannel(Contact *receiver) :
    SecureChannel(receiver)
{

}

MessageChannel::MessageChannel(Contact *receiver, const QString &asymKeyId, MessageChannel *parent) :
    SecureChannel(receiver, asymKeyId),
    parentChannelUid(parent->getUid())
{

}

WP::err MessageChannel::writeMainData(QDataStream &stream)
{
    WP::err error = SecureChannel::writeMainData(stream);
    if (error != WP::kOk)
        return error;

    QByteArray extra;
    QDataStream extraStream(extra);
    error = writeConfidentData(extraStream);
    if (error != WP::kOk)
        return error;

    return writeDataSecure(stream, extra);
}

WP::err MessageChannel::readMainData(QBuffer &mainData)
{
    WP::err error = SecureChannel::readMainData(mainData);
    if (error != WP::kOk)
        return error;

    QDataStream inStream(&mainData);
    QByteArray extra;
    error = SecureChannel::readDataSecure(inStream, extra);
    if (error != WP::kOk)
        return error;

    QBuffer extraBuffer(&extra);
    return readConfidentData(extraBuffer);
}

WP::err MessageChannel::writeConfidentData(QDataStream &stream)
{
    stream << parentChannelUid;
    stream << "\0";

    return WP::kOk;
}

WP::err MessageChannel::readConfidentData(QBuffer &mainData)
{
    QDataStream inStream(&mainData);

    char *buffer;
    inStream >> buffer;
    parentChannelUid = buffer;
    delete[] buffer;

    return WP::kOk;
}

QString MessageChannel::getParentChannelUid() const
{
    return parentChannelUid;
}

WP::err XMLSecureParcel::write(ProtocolOutStream *outStream, Contact *sender,
                               const QString &signatureKeyId, DataParcel *parcel,
                               const QString &stanzaName)
{
    QBuffer data;
    WP::err error = parcel->toRawData(sender, signatureKeyId, data);
    if (error != WP::kOk)
        return error;

    OutStanza *parcelStanza = new OutStanza(stanzaName);
    parcelStanza->addAttribute("uid", parcel->getUid());
    parcelStanza->addAttribute("sender", parcel->getSender()->getUid());
    parcelStanza->addAttribute("signatureKey", parcel->getSignatureKey());
    parcelStanza->addAttribute("signature", parcel->getSignature());

    parcelStanza->setText(data.buffer().toBase64());

    outStream->pushChildStanza(parcelStanza);
    outStream->cdDotDot();
    return WP::kOk;
}


ChannelParcel::ChannelParcel(SecureChannel *_channel) :
    channel(_channel)
{

}

SecureChannel *ChannelParcel::getChannel() const
{
    return channel;
}

WP::err ChannelParcel::writeMainData(QDataStream &stream)
{
    stream << channel->getUid();
    stream << "\0";

    QByteArray secureData;
    QDataStream secureStream(secureData);
    WP::err error = writeConfidentData(secureStream);
    if (error != WP::kOk)
        return error;

    return channel->writeDataSecure(stream, secureData);
}

WP::err ChannelParcel::readMainData(QBuffer &mainData)
{
    QDataStream inStream(&mainData);

    char *buffer;
    inStream >> buffer;
    QString channelId = buffer;
    delete[] buffer;

    channel = findChannel(channelId);
    if (channel == NULL)
        return WP::kError;

    QByteArray confidentData;
    WP::err error = channel->readDataSecure(inStream, confidentData);
    if (error != WP::kOk)
        return error;

    QBuffer confidentDataBuffer(&confidentData);
    return readConfidentData(confidentDataBuffer);
}

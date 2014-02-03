#ifndef MAIL_H
#define MAIL_H

#include <QBuffer>

#include "contact.h"
#include "protocolparser.h"


class ParcelCrypto {
public:
    void initNew();
    WP::err initFromPublic(Contact *receiver, const QString &keyId, const QByteArray &iv, const QByteArray &encryptedSymmetricKey);
    void initFromPrivate(const QByteArray &iv, const QByteArray &symmetricKey);

    WP::err cloakData(const QByteArray &data, QByteArray &cloakedData);
    WP::err uncloakData(const QByteArray &cloakedData, QByteArray &data);

    const QByteArray &getIV() const;
    const QByteArray &getSymmetricKey() const;
    WP::err getEncryptedSymmetricKey(Contact *receiver, const QString &keyId, QByteArray &encryptedSymmetricKey);

protected:
    const static int kSymmetricKeySize = 256;

    QByteArray iv;
    QByteArray symmetricKey;
};


class DataParcel {
public:
    virtual ~DataParcel() {}

    const QByteArray getSignature() const;
    const QString getSignatureKey() const;
    const Contact *getSender() const;

    virtual QString getUid() const;

    virtual WP::err toRawData(Contact *sender, const QString &signatureKey, QIODevice &rawData);
    virtual WP::err fromRawData(ContactFinder *contactFinder, QByteArray &rawData);

protected:
    virtual WP::err writeMainData(QDataStream &stream) = 0;
    virtual WP::err readMainData(QBuffer &mainData) = 0;

    QString readString(QIODevice &data) const;

protected:
    QByteArray signature;
    QString signatureKey;
    Contact *sender;

    QString uid;
};


class SecureChannel : public DataParcel {
public:
    // incoming
    SecureChannel(Contact *receiver);
    // outgoing
    SecureChannel(Contact *receiver, const QString &asymKeyId);
    virtual ~SecureChannel();

    /*! The id should be the same for all users. Thus the asym part can't be part of the hash.
    At the moment the channel id is just the sha1 hash of the iv.
    */
    virtual QString getUid() const;

    virtual WP::err writeDataSecure(QDataStream &stream, const QByteArray &data);
    virtual WP::err readDataSecure(QDataStream &stream, QByteArray &data);

protected:
    virtual WP::err writeMainData(QDataStream &stream);
    virtual WP::err readMainData(QBuffer &mainData);

private:
    Contact *receiver;

    QString asymmetricKeyId;
    ParcelCrypto parcelCrypto;
};


class ChannelParcel : public DataParcel {
public:
    // outgoing
    ChannelParcel(SecureChannel *channel = NULL);

    SecureChannel *getChannel() const;

protected:
    virtual WP::err writeMainData(QDataStream &stream);
    virtual WP::err readMainData(QBuffer &mainData);

    virtual WP::err writeConfidentData(QDataStream &stream) = 0;
    virtual WP::err readConfidentData(QBuffer &mainData) = 0;

    virtual SecureChannel *findChannel(const QString &channelUid) = 0;

protected:
    SecureChannel *channel;
};


class MessageChannel;

class MessageChannelFinder {
public:
    virtual ~MessageChannelFinder() {}
    virtual MessageChannel *find(const QString &channelUid) = 0;
};


class MessageChannel : public SecureChannel {
public:
    // incoming:
    MessageChannel(Contact *receiver);
    // outgoing:
    MessageChannel(Contact *receiver, const QString &asymKeyId, MessageChannel *parentChannel = NULL);

    QString getParentChannelUid() const;

protected:
    virtual WP::err writeMainData(QDataStream &stream);
    virtual WP::err readMainData(QBuffer &mainData);

    virtual WP::err writeConfidentData(QDataStream &stream);
    virtual WP::err readConfidentData(QBuffer &mainData);

private:
    QString parentChannelUid;
};

class Message : public ChannelParcel {
public:
    Message(MessageChannelFinder *channelFinder);
    Message(MessageChannel *channel);

    const QByteArray& getBody() const;
    void setBody(const QByteArray &body);

protected:
    virtual WP::err writeConfidentData(QDataStream &stream);
    virtual WP::err readConfidentData(QBuffer &mainData);

    virtual SecureChannel *findChannel(const QString &channelUid);

private:
    QByteArray body;
    MessageChannelFinder *channelFinder;
};

class XMLSecureParcel {
public:
    static WP::err write(ProtocolOutStream *outStream, Contact *sender,
                         const QString &signatureKeyId, DataParcel *parcel,
                         const QString &stanzaName);
};


#endif // MAIL_H

#ifndef MAIL_H
#define MAIL_H

#include "contact.h"
#include "protocolparser.h"


class RawMailMessage {
public:
    RawMailMessage(const QString &header, const QString &body);
    RawMailMessage();

    const QString &getUid() const;

    const QByteArray& getHeader() const;
    const QByteArray& getBody() const;

    QByteArray& getHeader();
    QByteArray& getBody();

private:
    QString fUid;

    QByteArray fHeader;
    QByteArray fBody;
};


class SecureParcel {
public:
    SecureParcel();

    void initNew();
    WP::err initFromPublic(Contact *receiver, const QString &keyId, const QByteArray &iv, const QByteArray &encryptedSymmetricKey);
    void initFromPrivate(const QByteArray &iv, const QByteArray &symmetricKey);

    WP::err cloakData(const QByteArray &data, QByteArray &cloakedData);
    WP::err uncloakData(const QByteArray &cloakedData, QByteArray &data);

    const QByteArray &getIV() const;
    const QByteArray &getSymmetricKey() const;
    WP::err getEncryptedSymmetricKey(Contact *receiver, const QString &keyId, QByteArray &encryptedSymmetricKey);

protected:
    const int kSymmetricKeySize = 256;

    QByteArray fIV;
    QByteArray fSymmetricKey;
};

class SecureChannel {
public:
    SecureChannel(SecureChannel *parent = NULL);
    virtual ~SecureChannel();

    virtual WP::err createNew();

    void setSecureParcel(SecureParcel *parcel);
    SecureParcel *getSecureParcel();

    const QString &getUid() const;
    void setUid(const QString &value);

protected:
    SecureChannel *fParent;

private:
    QString fUid;
    SecureParcel *fSecureParcel;
};


class MessageChannel : public SecureChannel {
public:
    MessageChannel(MessageChannel *parent = NULL);

    virtual WP::err createNew(Contact *creator);

    Contact *getCreator() const;
    void setCreator(Contact *creator);

protected:
    Contact *fCreator;
};


class XMLSecureChannel {
public:
    // ready to send out to a receiver
    static WP::err toPublicXML(ProtocolOutStream *outStream, SecureChannel* channel, Contact *receiver, QString keyId = "");
};


#endif // MAIL_H

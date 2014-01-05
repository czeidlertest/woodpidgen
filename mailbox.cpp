#include "mailbox.h"

#include <exception>

#include <QStringList>

#include "protocolparser.h"
#include "remoteauthentication.h"
#include "remoteconnection.h"
#include "useridentity.h"


class GitIOException : public std::exception
{
  virtual const char* what() const throw()
  {
    return "git io error";
  }
} git_io_exception;


MessageListModel::MessageListModel(QObject *parent) :
    QAbstractListModel(parent)
{
}

QVariant
MessageListModel::data(const QModelIndex &index, int role) const
{
    if (role == Qt::DisplayRole)
        return fMessages.at(index.row())->getBody();

    return QVariant();
}

int MessageListModel::rowCount(const QModelIndex &parent) const
{
    return fMessages.count();
}

void MessageListModel::addMessage(MailRef *messageRef)
{
    int count = fMessages.count();
    beginInsertRows(QModelIndex(), count, count);
    fMessages.append(messageRef);
    endInsertRows();
}

MailRef *MessageListModel::removeMessageAt(int index)
{
    MailRef *entry = fMessages.at(index);
    beginRemoveRows(QModelIndex(), index, index);
    fMessages.removeAt(index);
    endRemoveRows();
    return entry;
}

void MessageListModel::clear()
{
    beginRemoveRows(QModelIndex(), 0, fMessages.count() - 1);
    foreach (MailRef *ref, fMessages)
        delete ref;
    fMessages.clear();
    endRemoveRows();
}

MailRef::MailRef(const QString &messageId, Mailbox *mailbox) :
    fMessageId(messageId),
    fMailbox(mailbox)
{
}

const QByteArray &MailRef::getHeader()
{
    if (fHeader.count() == 0) {
        WP::err error = fMailbox->readMessage(fMessageId, fHeader);
        if (error != WP::kOk)
            throw git_io_exception;
    }
    return fHeader;
}

const QByteArray &MailRef::getBody()
{
    if (fBody.count() == 0) {
        WP::err error = fMailbox->readMessage(fMessageId, fBody);
        if (error != WP::kOk)
            throw git_io_exception;
    }
    return fBody;
}

Mailbox::Mailbox(DatabaseBranch *branch, const QString &baseDir) :
    fMessageList(this)
{
    setToDatabase(branch, baseDir);
}

Mailbox::~Mailbox()
{
    QMap<QString, MessageChannel*>::iterator it= fMessageChannelCache.begin();
    for (; it != fMessageChannelCache.end(); it++)
        delete it.value();
}

WP::err Mailbox::createNewMailbox(KeyStore *keyStore, const QString &defaultKeyId, bool addUidToBaseDir)
{
    // derive uid
    QString uidHex = fCrypto->generateUid();

    // start creating the identity
    WP::err error = EncryptedUserData::create(uidHex, keyStore, defaultKeyId, addUidToBaseDir);
    if (error != WP::kOk)
        return error;

    return error;
}

WP::err Mailbox::open(KeyStoreFinder *keyStoreFinder)
{
    WP::err error = EncryptedUserData::open(keyStoreFinder);
    if (error != WP::kOk)
        return error;

    readMailDatabase();
    return error;
}

void Mailbox::setOwner(UserIdentity *userIdentity)
{
    fUserIdentity = userIdentity;
}

QAbstractListModel &Mailbox::getMessages()
{
    return fMessageList;
}

QStringList Mailbox::getMailIds()
{
    QStringList messageIds;
    QStringList shortIds = listDirectories("");
    for (int i = 0; i < shortIds.count(); i++) {
        const QString &shortId = shortIds.at(i);
        if (shortId == "channels")
            continue;
        QStringList restIds = listDirectories(shortId);
        for (int a = 0; a < restIds.count(); a++) {
            QString completeId = shortId;
            completeId.append(restIds.at(a));
            messageIds.append(completeId);
        }
    }
    return messageIds;
}

class MessageData {
public:
    QString uid;
    QString channelUid;
    QString from;
    QString signatureKey;
    QByteArray signature;
    QByteArray data;

};

class MessagePrimDataHandler : public InStanzaHandler {
public:
    MessagePrimDataHandler(MessageData *d) :
        InStanzaHandler("primary_data"),
        data(d)
    {
    }

    bool handleStanza(const QXmlStreamAttributes &attributes)
    {
        if (!attributes.hasAttribute("signature_key"))
            return false;
        if (!attributes.hasAttribute("signature"))
            return false;

        data->signatureKey = attributes.value("signature_key").toString();
        data->signature = QByteArray::fromBase64(attributes.value("signature").toLatin1());
        return true;
    }

    bool handleText(const QStringRef &text)
    {
        data->data = QByteArray::fromBase64(text.toLatin1());
        return true;
    }

public:
    MessageData *data;
};


class MessageParserHandler : public InStanzaHandler {
public:
    MessageParserHandler(MessageData *d) :
        InStanzaHandler("message"),
        data(d)
    {
        addChildHandler(new MessagePrimDataHandler(data));
    }

    bool handleStanza(const QXmlStreamAttributes &attributes)
    {
        if (!attributes.hasAttribute("uid"))
            return false;
        if (!attributes.hasAttribute("channel_uid"))
            return false;
        if (!attributes.hasAttribute("from"))
            return false;

        data->uid = attributes.value("uid").toString();
        data->channelUid = attributes.value("channel_uid").toString();
        data->from = attributes.value("from").toString();
        return true;
    }

public:
    MessageData *data;
};


WP::err Mailbox::readMessage(const QString &messageId, QByteArray &body)
{
    QString path = pathForMessageId(messageId);
    path += "/message";
    QByteArray data;
    WP::err error = read(path, data);
    if (error != WP::kOk)
        return error;


    MessageData messageData;
    MessageParserHandler handler(&messageData);
    ProtocolInStream inStream(data);
    inStream.addHandler(&handler);
    inStream.parse();

    if (!handler.hasBeenHandled())
        return WP::kError;

    Contact *myself = fUserIdentity->getMyself();
    if (!myself->verify(messageData.signatureKey, messageData.data, messageData.signature))
        return WP::kError;
    MessageChannel *channel = findMessageChannel(messageData.channelUid);
    if (channel == NULL)
        return WP::kChannelNotFound;

    return channel->getSecureParcel()->uncloakData(messageData.data, body);
}

QStringList Mailbox::getChannelIds()
{
    QStringList channelIds;
    QStringList shortIds = listDirectories("channels");
    for (int i = 0; i < shortIds.count(); i++) {
        const QString &shortId = shortIds.at(i);
        QStringList restIds = listDirectories(shortId);
        for (int a = 0; a < restIds.count(); a++) {
            QString completeId = shortId;
            completeId.append(restIds.at(a));
            channelIds.append(completeId);
        }
    }
    return channelIds;
}


class MessageChannelData {
public:
    QString uid;
    QString asymKeyId;
    QByteArray iv;
    QByteArray ckey;
};

class MessageChannelIVParserHandler : public InStanzaHandler {
public:
    MessageChannelIVParserHandler(MessageChannelData *c) :
        InStanzaHandler("iv"),
        data(c)
    {
    }

    bool handleStanza(const QXmlStreamAttributes &attributes)
    {
       return true;
    }

    bool handleText(const QStringRef &text)
    {
        data->iv = QByteArray::fromBase64(text.toLatin1());
        return true;
    }

public:
    MessageChannelData *data;
};

class MessageChannelCKeyParserHandler : public InStanzaHandler {
public:
    MessageChannelCKeyParserHandler(MessageChannelData *c) :
        InStanzaHandler("ckey"),
        data(c)
    {
    }

    bool handleStanza(const QXmlStreamAttributes &attributes)
    {
       return true;
    }

    bool handleText(const QStringRef &text)
    {
        data->ckey = QByteArray::fromBase64(text.toLatin1());
        return true;
    }

public:
    MessageChannelData *data;
};

class MessageChannelParserHandler : public InStanzaHandler {
public:
    MessageChannelParserHandler(MessageChannelData *c) :
        InStanzaHandler("channel"),
        data(c)
    {
        addChildHandler(new MessageChannelIVParserHandler(data));
        addChildHandler(new MessageChannelCKeyParserHandler(data));
    }

    bool handleStanza(const QXmlStreamAttributes &attributes)
    {
       if (!attributes.hasAttribute("uid"))
            return false;
        if (!attributes.hasAttribute("asym_key_id"))
            return false;

        data->uid = attributes.value("uid").toString();
        data->asymKeyId = attributes.value("asym_key_id").toString();
        return true;
    }

public:
    MessageChannelData *data;
};


MessageChannel *Mailbox::findMessageChannel(const QString &channelId)
{
    QMap<QString, MessageChannel*>::iterator it= fMessageChannelCache.find(channelId);
    if (it != fMessageChannelCache.end())
        return it.value();

    if (!fMessageChannels.contains(channelId))
        return NULL;

    QString path = "channels/";
    path += pathForMessageId(channelId);
    QByteArray data;
    WP::err error = read(path, data);
    if (error != WP::kOk)
        return NULL;

    MessageChannelData channelData;
    MessageChannelParserHandler handler(&channelData);
    ProtocolInStream inStream(data);
    inStream.addHandler(&handler);
    inStream.parse();

    if (!handler.hasBeenHandled())
        return NULL;

    Contact *myself = fUserIdentity->getMyself();
    SecureParcel *secureParcel = new SecureParcel();
    error = secureParcel->initFromPublic(myself, channelData.asymKeyId, channelData.iv, channelData.ckey);
    if (error != WP::kOk) {
        delete secureParcel;
        return NULL;
    }

    MessageChannel *channel = new MessageChannel();
    channel->setUid(channelData.uid);
    channel->setSecureParcel(secureParcel);

    fMessageChannelCache[channelId] = channel;

    return channel;
}

void Mailbox::onNewCommits(const QString &startCommit, const QString &endCommit)
{
    readMailDatabase();
}

QString Mailbox::pathForMessageId(const QString &messageId)
{
    QString path = messageId.left(2);
    path += "/";
    path += messageId.mid(2);
    return path;
}

WP::err Mailbox::readMailDatabase()
{
    readMessageChannels();

    fMessageList.clear();

    QStringList mails = getMailIds();
    for (int i = 0; i < mails.count(); i++) {
        const QString &mailId = mails.at(i);
        MailRef *ref = new MailRef(mailId, this);
        fMessageList.addMessage(ref);
    }
    emit databaseRead();
    return WP::kOk;
}

WP::err Mailbox::readMessageChannels()
{
    QMap<QString, MessageChannel*>::iterator it= fMessageChannelCache.begin();
    for (; it != fMessageChannelCache.end(); it++)
        delete it.value();
    fMessageChannelCache.clear();

    fMessageChannels = getChannelIds();

    return WP::kOk;
}


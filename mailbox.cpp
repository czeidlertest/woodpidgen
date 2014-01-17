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
        return fMessages.at(index.row())->getData();

    return QVariant();
}

int MessageListModel::rowCount(const QModelIndex &parent) const
{
    return getMessageCount();
}

int MessageListModel::getMessageCount() const
{
    return fMessages.count();
}

void MessageListModel::addMessage(MessageData *messageRef)
{
    int count = fMessages.count();
    beginInsertRows(QModelIndex(), count, count);
    fMessages.append(messageRef);
    endInsertRows();
}

bool MessageListModel::removeMessage(MessageData *message)
{
    int index = fMessages.indexOf(message);
    if (index < 0)
        return false;
    removeMessageAt(index);
    return true;
}

MessageData *MessageListModel::removeMessageAt(int index)
{
    MessageData *entry = fMessages.at(index);
    beginRemoveRows(QModelIndex(), index, index);
    fMessages.removeAt(index);
    endRemoveRows();
    return entry;
}

MessageData *MessageListModel::messageAt(int index)
{
    return fMessages.at(index);
}

void MessageListModel::clear()
{
    beginRemoveRows(QModelIndex(), 0, fMessages.count() - 1);
    foreach (MessageData *ref, fMessages)
        delete ref;
    fMessages.clear();
    endRemoveRows();
}


Mailbox::Mailbox(DatabaseBranch *branch, const QString &baseDir) :
    fUserIdentity(NULL),
    fMessageList(this)
{
    setToDatabase(branch, baseDir);
}

Mailbox::~Mailbox()
{
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

    return error;
}

void Mailbox::setOwner(UserIdentity *userIdentity)
{
    fUserIdentity = userIdentity;
    readMailDatabase();
}

UserIdentity *Mailbox::getOwner() const
{
    return fUserIdentity;
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

WP::err Mailbox::readMessage(const QString &messageId, QByteArray &body)
{
    MessageData *message = findMessage(messageId);
    if (message == NULL)
        return WP::kEntryNotFound;

    Contact *sender = fUserIdentity->findContactByUid(message->getFrom());
    if (sender != NULL)
        body.append(QString(sender->getAddress() + ": ").toLatin1());

    body.append(message->getData());
    return WP::kOk;
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

MessageData *Mailbox::findMessage(const QString &uid)
{
    for (int i = 0; i < fMessageList.getMessageCount(); i++) {
        MessageData *message = fMessageList.messageAt(i);
        if (message->getUid() == uid)
            return message;
    }
    return false;
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


MessageThread *Mailbox::findMessageThread(const QString &channelId)
{
    for (int i = 0; i < fThreadList.getChannelCount(); i++) {
        MessageThread *thread = fThreadList.channelAt(i);
        if (thread->getMessageChannel()->getUid() == channelId)
            return thread;
    }
    return NULL;
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
        MessageData *data = new MessageData(this);
        if (data->load(mailId) != WP::kOk) {
            delete data;
            continue;
        }
        fMessageList.addMessage(data);
    }

    emit databaseRead();
    return WP::kOk;
}

WP::err Mailbox::readMessageChannels()
{
    fThreadList.clear();

    QStringList messageChannels = getChannelIds();
    foreach (const QString & uid, messageChannels) {
        MessageChannel* channel = readChannel(uid);
        fThreadList.addChannel(new MessageThread(channel));
    }

    return WP::kOk;
}

MessageChannel* Mailbox::readChannel(const QString &channelId) {
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

    return channel;
}


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

        data->setSignatureKey(attributes.value("signature_key").toString());
        data->setSignature(QByteArray::fromBase64(attributes.value("signature").toLatin1()));
        return true;
    }

    bool handleText(const QStringRef &text)
    {
        data->setData(QByteArray::fromBase64(text.toLatin1()));
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

        data->setUid(attributes.value("uid").toString());
        data->setChannelUid(attributes.value("channel_uid").toString());
        data->setFrom(attributes.value("from").toString());
        return true;
    }

public:
    MessageData *data;
};


MessageData::MessageData(Mailbox *mailbox) :
    fMailbox(mailbox),
    fThread(NULL)
{

}

MessageData::~MessageData()
{
    if (fThread != NULL)
        fThread->getMessages()->removeMessage(this);
}

WP::err MessageData::load(const QString &uid)
{
    fUid = uid;

    QString path = pathForMessageId(fUid);
    path += "/message";
    QByteArray data;
    WP::err error = fMailbox->read(path, data);
    if (error != WP::kOk)
        return error;


    MessageParserHandler handler(this);
    ProtocolInStream inStream(data);
    inStream.addHandler(&handler);
    inStream.parse();

    if (!handler.hasBeenHandled())
        return WP::kError;

    UserIdentity *userIdentity = fMailbox->getOwner();
    Contact *myself = userIdentity->getMyself();
    if (!myself->verify(fSignatureKey, fData, fSignature))
        return WP::kError;

    fThread = fMailbox->findMessageThread(fChannelUid);
    if (fThread == NULL)
        return WP::kChannelNotFound;
    fThread->getMessages()->addMessage(this);

    MessageChannel *channel = fThread->getMessageChannel();
    error = channel->getSecureParcel()->uncloakData(fData, fData);
    if (error != WP::kOk)
        return error;

    return WP::kOk;
}

const QString &MessageData::getUid() const
{
    return fUid;
}

const QString &MessageData::getChannelUid() const
{
    return fChannelUid;
}

const QString &MessageData::getFrom() const
{
    return fFrom;
}

const QString &MessageData::getSignatureKey() const
{
    return fSignatureKey;
}

const QByteArray &MessageData::getSignature() const
{
    return fSignature;
}

const QByteArray &MessageData::getData() const
{
    return fData;
}

void MessageData::setUid(const QString &uid)
{
    fUid = uid;
}

void MessageData::setChannelUid(const QString &channelUid)
{
    fChannelUid = channelUid;
}

void MessageData::setFrom(const QString &from)
{
    fFrom = from;
}

void MessageData::setSignatureKey(const QString &signatureKey)
{
    fSignatureKey = signatureKey;
}

void MessageData::setSignature(const QByteArray &signature)
{
    fSignature = signature;
}

void MessageData::setData(const QByteArray &data)
{
    fData = data;
}

QString MessageData::pathForMessageId(const QString &messageId)
{
    QString path = messageId.left(2);
    path += "/";
    path += messageId.mid(2);
    return path;
}

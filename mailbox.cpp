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
    return getMessageCount();
}

int MessageListModel::getMessageCount() const
{
    return fMessages.count();
}

void MessageListModel::addMessage(Message *messageRef)
{
    int count = fMessages.count();
    beginInsertRows(QModelIndex(), count, count);
    fMessages.append(messageRef);
    endInsertRows();
}

bool MessageListModel::removeMessage(Message *message)
{
    int index = fMessages.indexOf(message);
    if (index < 0)
        return false;
    removeMessageAt(index);
    return true;
}

Message *MessageListModel::removeMessageAt(int index)
{
    Message *entry = fMessages.at(index);
    beginRemoveRows(QModelIndex(), index, index);
    fMessages.removeAt(index);
    endRemoveRows();
    return entry;
}

Message *MessageListModel::messageAt(int index)
{
    return fMessages.at(index);
}

void MessageListModel::clear()
{
    beginRemoveRows(QModelIndex(), 0, fMessages.count() - 1);
    foreach (Message *ref, fMessages)
        delete ref;
    fMessages.clear();
    endRemoveRows();
}


Mailbox::Mailbox(DatabaseBranch *branch, const QString &baseDir) :
    fUserIdentity(NULL),
    fMessageList(this),
    channelFinder(&fThreadList)
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

MessageListModel &Mailbox::getMessages()
{
    return fMessageList;
}

MessageThreadDataModel &Mailbox::getThreads()
{
    return fThreadList;
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
    Message *message = findMessage(messageId);
    if (message == NULL)
        return WP::kEntryNotFound;

    Contact *sender = fUserIdentity->findContactByUid(message->getSender()->getUid());
    if (sender != NULL)
        body.append(QString(sender->getAddress() + ": ").toLatin1());

    body.append(message->getBody());
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

Message *Mailbox::findMessage(const QString &uid)
{
    for (int i = 0; i < fMessageList.getMessageCount(); i++) {
        Message *message = fMessageList.messageAt(i);
        if (message->getUid() == uid)
            return message;
    }
    return NULL;
}

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
    fMessageList.clear();

    readMessageChannels();

    QStringList mails = getMailIds();
    for (int i = 0; i < mails.count(); i++) {
        const QString &mailId = mails.at(i);
        Message *message = new Message(&channelFinder);

        QString path = pathForMessageId(mailId);
        path += "/message";
        QByteArray data;
        WP::err error = read(path, data);
        if (error != WP::kOk)
            continue;

        error = message->fromRawData(fUserIdentity->getContactFinder(), data);
        if (error != WP::kOk) {
            delete message;
            continue;
        }

        fMessageList.addMessage(message);

        // add to thread
        MessageThread *thread = findMessageThread(message->getChannel()->getUid());
        if (thread != NULL)
            thread->getMessages()->addMessage(message);
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
        if (channel == NULL) {
            delete channel;
            continue;
        }
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

    Contact *myself = fUserIdentity->getMyself();
    MessageChannel *channel = new MessageChannel(myself);
    error = channel->fromRawData(fUserIdentity->getContactFinder(), data);
    if (error != WP::kOk) {
        delete channel;
        return NULL;
    }
    return channel;
}

Mailbox::MailboxMessageChannelFinder::MailboxMessageChannelFinder(MessageThreadDataModel *_threads) :
    threads(_threads)
{

}

MessageChannel *Mailbox::MailboxMessageChannelFinder::find(const QString &channelUid)
{
    MessageThread *thread = threads->findChannel(channelUid);
    if (thread == NULL)
        return NULL;
    return thread->getMessageChannel();
}

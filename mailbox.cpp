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

MessageThreadDataModel &Mailbox::getThreads()
{
    return fThreadList;
}

QStringList Mailbox::getUids(QString path)
{
    QStringList messageIds;
    QStringList shortIds = listDirectories(path);
    for (int i = 0; i < shortIds.count(); i++) {
        const QString &shortId = shortIds.at(i);
        if (shortId.size() != 2)
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


QStringList Mailbox::getChannelPaths()
{
    return getUids("");
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

QString Mailbox::pathForChannelId(const QString &threadId)
{
    QString path = threadId.left(2);
    path += "/";
    path += threadId.mid(2);
    return path;
}

QString Mailbox::pathForMessagelId(const QString &threadPath, const QString &messageId)
{
    QString path = threadPath;
    path += "/";
    path = messageId.left(2);
    path += "/";
    path += messageId.mid(2);
    return path;
}

WP::err Mailbox::readMailDatabase()
{
    fThreadList.clear();

    QStringList messageChannels = getChannelPaths();
    foreach (const QString & uidPath, messageChannels) {
        MessageChannel* channel = readChannel(uidPath);
        if (channel == NULL) {
            delete channel;
            continue;
        }
        MessageThread *thread = new MessageThread(channel);
        readThreadContent(uidPath, thread);
        fThreadList.addChannel(thread);
    }
    return WP::kOk;
}


WP::err Mailbox::readThreadContent(const QString &threadPath, MessageThread *thread)
{
    MessageListModel &messages = thread->getMessages();
    QList<MessageChannelInfo*> &infos = thread->getChannelInfos();


    QStringList messageUids = getUids(threadPath);
    for (int i = 0; i < messageUids.count(); i++) {
        const QString &messageId = messageUids.at(i);
        QString path = pathForMessagelId(threadPath, messageId);
        QByteArray data;
        WP::err error = read(path, data);
        if (error != WP::kOk)
            return error;

        // test for message
        Message *message = new Message(&channelFinder);
        error = message->fromRawData(fUserIdentity->getContactFinder(), data);
        if (error == WP::kOk) {
            messages.addMessage(message);
            continue;
        }
        delete message;
    }

    QStringList infoUids = getUids(threadPath + "/i");
    for (int i = 0; i < infoUids.count(); i++) {
        const QString &infoId = infoUids.at(i);
        QString path = pathForMessagelId(threadPath + "/i", infoId);
        QByteArray data;
        WP::err error = read(path, data);
        if (error != WP::kOk)
            return error;

        // test for channel info
        MessageChannelInfo *info = new MessageChannelInfo(&channelFinder);
        error = info->fromRawData(fUserIdentity->getContactFinder(), data);
        if (error == WP::kOk) {
            infos.append(info);
            continue;
        }
        delete info;
    }

    emit databaseRead();
    return WP::kOk;
}

MessageChannel* Mailbox::readChannel(const QString &channelId) {
    QString path = pathForChannelId(channelId);
    path += "r";
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

MessageChannel *Mailbox::MailboxMessageChannelFinder::findChannel(const QString &channelUid)
{
    MessageThread *thread = threads->findChannel(channelUid);
    if (thread == NULL)
        return NULL;
    return thread->getMessageChannel();
}

MessageParcel *Mailbox::MailboxMessageChannelFinder::findParcel(const QString &channelUid, const QString &parcelUid)
{

}

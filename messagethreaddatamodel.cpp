#include "messagethreaddatamodel.h"

#include "mailbox.h"


MessageThread::MessageThread(MessageChannel *channel) :
    channel(channel)
{
    messages = new MessageListModel();
}

MessageThread::~MessageThread()
{
    delete messages;
    delete channel;

    foreach (MessageChannelInfo *info, channelInfoList)
        delete info;
}

MessageChannel *MessageThread::getMessageChannel() const
{
    return channel;
}

MessageListModel &MessageThread::getMessages() const
{
    return *messages;
}

QList<MessageChannelInfo*> &MessageThread::getChannelInfos()
{
    return channelInfoList;
}

MessageThreadDataModel::MessageThreadDataModel(QObject *parent) :
    QAbstractListModel(parent)
{

}

QVariant MessageThreadDataModel::data(const QModelIndex &index, int role) const
{
    if (role == Qt::DisplayRole)
        return channelMessages.at(index.row())->getMessageChannel()->getUid();

    return QVariant();
}

int MessageThreadDataModel::rowCount(const QModelIndex &parent) const
{
    return getChannelCount();
}

int MessageThreadDataModel::getChannelCount() const
{
    return channelMessages.count();
}

void MessageThreadDataModel::addChannel(MessageThread *channel)
{
    int count = channelMessages.count();
    beginInsertRows(QModelIndex(), count, count);
    channelMessages.append(channel);
    endInsertRows();
}

MessageThread *MessageThreadDataModel::removeChannelAt(int index)
{
    MessageThread *channel = channelMessages.at(index);
    beginRemoveRows(QModelIndex(), index, index);
    channelMessages.removeAt(index);
    endRemoveRows();
    return channel;
}

MessageThread *MessageThreadDataModel::channelAt(int index) const
{
    return channelMessages.at(index);
}

MessageThread *MessageThreadDataModel::findChannel(const QString &channelId) const
{
    foreach (MessageThread *thread, channelMessages) {
        if (thread->getMessageChannel()->getUid() == channelId)
            return thread;
    }
    return NULL;
}

void MessageThreadDataModel::clear()
{
    beginRemoveRows(QModelIndex(), 0, channelMessages.count() - 1);
    foreach (MessageThread *ref, channelMessages)
        delete ref;
    channelMessages.clear();
    endRemoveRows();
}



MessageThreadDataModel::~MessageThreadDataModel()
{
    clear();
}

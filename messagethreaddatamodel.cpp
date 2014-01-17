#include "messagethreaddatamodel.h"

#include "mailbox.h"


MessageThread::MessageThread(MessageChannel *channel) :
    fChannel(channel)
{
    fMessages = new MessageListModel();
}

MessageThread::~MessageThread()
{
    delete fChannel;
    delete fMessages;
}

MessageChannel *MessageThread::getMessageChannel() const
{
    return fChannel;
}

MessageListModel *MessageThread::getMessages() const
{
    return fMessages;
}

MessageThreadDataModel::MessageThreadDataModel(QObject *parent) :
    QAbstractListModel(parent)
{

}

QVariant MessageThreadDataModel::data(const QModelIndex &index, int role) const
{
    if (role == Qt::DisplayRole)
        return fChannelMessages.at(index.row())->getMessageChannel()->getUid();

    return QVariant();
}

int MessageThreadDataModel::rowCount(const QModelIndex &parent) const
{
    return getChannelCount();
}

int MessageThreadDataModel::getChannelCount() const
{
    return fChannelMessages.count();
}

void MessageThreadDataModel::addChannel(MessageThread *channel)
{
    int count = fChannelMessages.count();
    beginInsertRows(QModelIndex(), count, count);
    fChannelMessages.append(channel);
    endInsertRows();
}

MessageThread *MessageThreadDataModel::removeChannelAt(int index)
{
    MessageThread *channel = fChannelMessages.at(index);
    beginRemoveRows(QModelIndex(), index, index);
    fChannelMessages.removeAt(index);
    endRemoveRows();
    return channel;
}

MessageThread *MessageThreadDataModel::channelAt(int index) const
{
    return fChannelMessages.at(index);
}

void MessageThreadDataModel::clear()
{
    beginRemoveRows(QModelIndex(), 0, fChannelMessages.count() - 1);
    foreach (MessageThread *ref, fChannelMessages)
        delete ref;
    fChannelMessages.clear();
    endRemoveRows();
}



MessageThreadDataModel::~MessageThreadDataModel()
{
    clear();
}

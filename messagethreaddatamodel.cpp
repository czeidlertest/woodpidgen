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
    if (role != Qt::DisplayRole)
        return QVariant();

    QString text;

    MessageThread* thread = channelMessages.at(index.row());
    if (thread->getChannelInfos().size() > 0) {
        MessageChannelInfo *info = thread->getChannelInfos().at(0);
        text += info->getSubject();
        if (text == "")
            text += " ";
        if (info->getParticipants().size() > 0) {
            text += "(";
            QVector<MessageChannelInfo::Participant> &participants = info->getParticipants();
            for (int i = 0; i < participants.size(); i++) {
                text += participants.at(i).address;
                if (i < participants.size() - 1)
                    text += ",";
            }
            text += ")";
        }

    }

    if (text == "")
        text += thread->getMessageChannel()->getUid();

    return text;
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

bool MessageThreadDataModel::removeChannel(MessageThread *channel)
{
    int index = channelMessages.indexOf(channel);
    if (index < 0)
        return false;
    removeChannelAt(index);
    return true;
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

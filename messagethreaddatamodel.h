#ifndef MESSAGETHREADDATAMODEL_H
#define MESSAGETHREADDATAMODEL_H

#include <QAbstractListModel>
#include <QList>

class MessageChannel;
class MessageListModel;


class MessageThread {
public:
    MessageThread(MessageChannel *channel);
    ~MessageThread();

    MessageChannel *getMessageChannel() const;
    MessageListModel *getMessages() const;
private:
    MessageChannel *fChannel;
    MessageListModel *fMessages;
};

class MessageThreadDataModel : public QAbstractListModel {
public:
    MessageThreadDataModel(QObject * parent = 0);
    ~MessageThreadDataModel();

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    int rowCount(const QModelIndex & parent = QModelIndex()) const;

    int getChannelCount() const;
    void addChannel(MessageThread *channel);
    MessageThread *removeChannelAt(int index);
    MessageThread *channelAt(int index) const;

    void clear();

private:
    QList<MessageThread*> fChannelMessages;
};

#endif // MESSAGETHREADDATAMODEL_H

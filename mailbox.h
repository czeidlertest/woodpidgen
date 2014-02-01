#ifndef MAILBOX_H
#define MAILBOX_H

#include <QAbstractListModel>

#include "databaseutil.h"
#include "mail.h"
#include "messagethreaddatamodel.h"


class UserIdentity;

class MessageListModel : public QAbstractListModel {
public:
    MessageListModel(QObject * parent = 0);

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    int rowCount(const QModelIndex & parent = QModelIndex()) const;

    int getMessageCount() const;
    void addMessage(Message *message);
    bool removeMessage(Message *message);
    Message *removeMessageAt(int index);
    Message *messageAt(int index);

    void clear();
private:
    QList<Message*> fMessages;
};

class Mailbox : public EncryptedUserData
{
Q_OBJECT
public:
    Mailbox(DatabaseBranch *branch, const QString &baseDir = "");
    ~Mailbox();

    WP::err createNewMailbox(KeyStore *keyStore, const QString &defaultKeyId,
                             bool addUidToBaseDir = true);
    WP::err open(KeyStoreFinder *keyStoreFinder);

    void setOwner(UserIdentity *userIdentity);
    UserIdentity *getOwner() const;

    MessageListModel &getMessages();
    MessageThreadDataModel &getThreads();

    QStringList getMailIds();
    WP::err readMessage(const QString &messageId, QByteArray &body);

    QStringList getChannelIds();
    Message *findMessage(const QString &uid);
    MessageThread *findMessageThread(const QString &channelId);

signals:
    void databaseReadProgress(float progress);
    void databaseRead();

private slots:
    virtual void onNewCommits(const QString &startCommit, const QString &endCommit);

private:
    class MailboxMessageChannelFinder : public MessageChannelFinder {
    public:
        MailboxMessageChannelFinder(MessageThreadDataModel *threads);
        virtual MessageChannel *find(const QString &channelUid);
    private:
        MessageThreadDataModel *threads;
    };

    QString pathForMessageId(const QString &messageId);
    MessageChannel* readChannel(const QString &channelId);

    WP::err readMailDatabase();
    WP::err readMessageChannels();

    UserIdentity *fUserIdentity;

    MessageListModel fMessageList;
    MessageThreadDataModel fThreadList;
    MailboxMessageChannelFinder channelFinder;
};


class MailboxFinder {
public:
    virtual ~MailboxFinder() {}
    virtual Mailbox *find(const QString &mailboxId) = 0;
};

#endif // MAILBOX_H

#ifndef MAILBOX_H
#define MAILBOX_H

#include <QAbstractListModel>

#include "databaseutil.h"
#include "mail.h"


class Mailbox;

class MailRef {
public:
    MailRef(const QString &messageId, Mailbox *mailbox);

    const QByteArray &getHeader();
    const QByteArray &getBody();
private:
    QString fMessageId;
    Mailbox *fMailbox;

    QByteArray fHeader;
    QByteArray fBody;
};


class UserIdentity;

class MessageListModel : public QAbstractListModel {
public:
    MessageListModel(QObject * parent = 0);

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    int rowCount(const QModelIndex & parent = QModelIndex()) const;

    void addMessage(MailRef *messageRef);
    void removeMessage(MailRef *messageRef);
    MailRef *removeMessageAt(int index);
    MailRef *messageAt(int index);

    void clear();
private:
    QList<MailRef*> fMessages;
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

    QAbstractListModel &getMessages();

    QStringList getMailIds();
    WP::err readMessage(const QString &messageId, QByteArray &body);

    QStringList getChannelIds();
    MessageChannel* findMessageChannel(const QString &channelId);

signals:
    void databaseReadProgress(float progress);
    void databaseRead();

private slots:
    virtual void onNewCommits(const QString &startCommit, const QString &endCommit);

private:
    QString pathForMessageId(const QString &messageId);

    WP::err readMailDatabase();
    WP::err readMessageChannels();

    UserIdentity *fUserIdentity;

    MessageListModel fMessageList;
    QStringList fMessageChannels;
    QMap<QString, MessageChannel*> fMessageChannelCache;
};


class MailboxFinder {
public:
    virtual ~MailboxFinder() {}
    virtual Mailbox *find(const QString &mailboxId) = 0;
};

#endif // MAILBOX_H

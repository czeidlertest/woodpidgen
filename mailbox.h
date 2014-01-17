#ifndef MAILBOX_H
#define MAILBOX_H

#include <QAbstractListModel>

#include "databaseutil.h"
#include "mail.h"
#include "messagethreaddatamodel.h"


class Mailbox;

class MessageData {
public:
    MessageData(Mailbox *mailbox);
    ~MessageData();

    WP::err load(const QString &uid);

    const QString &getUid() const;
    const QString &getChannelUid() const;
    const QString &getFrom() const;
    const QString &getSignatureKey() const;

    const QByteArray &getSignature() const;
    const QByteArray &getData() const;

private:
    friend class MessageParserHandler;
    friend class MessagePrimDataHandler;

    void setUid(const QString &uid);
    void setChannelUid(const QString &channelUid);
    void setFrom(const QString &from);
    void setSignatureKey(const QString &signatureKey);

    void setSignature(const QByteArray &signature);
    void setData(const QByteArray &data);

    QString pathForMessageId(const QString &messageId);

private:
    Mailbox *fMailbox;
    MessageThread *fThread;

    QString fUid;
    QString fChannelUid;
    QString fFrom;
    QString fSignatureKey;
    QByteArray fSignature;
    QByteArray fData;

};


class UserIdentity;

class MessageListModel : public QAbstractListModel {
public:
    MessageListModel(QObject * parent = 0);

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    int rowCount(const QModelIndex & parent = QModelIndex()) const;

    int getMessageCount() const;
    void addMessage(MessageData *messageRef);
    bool removeMessage(MessageData *message);
    MessageData *removeMessageAt(int index);
    MessageData *messageAt(int index);

    void clear();
private:
    QList<MessageData*> fMessages;
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

    QAbstractListModel &getMessages();

    QStringList getMailIds();
    WP::err readMessage(const QString &messageId, QByteArray &body);

    QStringList getChannelIds();
    MessageData *findMessage(const QString &uid);
    MessageThread *findMessageThread(const QString &channelId);

signals:
    void databaseReadProgress(float progress);
    void databaseRead();

private slots:
    virtual void onNewCommits(const QString &startCommit, const QString &endCommit);

private:
    QString pathForMessageId(const QString &messageId);
    MessageChannel* readChannel(const QString &channelId);

    WP::err readMailDatabase();
    WP::err readMessageChannels();

    UserIdentity *fUserIdentity;

    MessageListModel fMessageList;
    MessageThreadDataModel fThreadList;
};


class MailboxFinder {
public:
    virtual ~MailboxFinder() {}
    virtual Mailbox *find(const QString &mailboxId) = 0;
};

#endif // MAILBOX_H

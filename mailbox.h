#ifndef MAILBOX_H
#define MAILBOX_H

#include <QAbstractListModel>

#include "contactrequest.h"
#include "databaseutil.h"
#include "remoteconnection.h"


class Profile;
class UserIdentity;

class RawMailMessage {
public:
    RawMailMessage(const QString &header, const QString &body);
    RawMailMessage();

    const QByteArray& getHeader() const;
    const QByteArray& getBody() const;

    QByteArray& getHeader();
    QByteArray& getBody();

private:
    QByteArray fHeader;
    QByteArray fBody;
};


class MailMessenger : QObject {
Q_OBJECT
public:
    MailMessenger(const QString &targetAddress, Profile *profile, UserIdentity *identity);
    ~MailMessenger();

    WP::err postMessage(const RawMailMessage *message);

private slots:
    void handleReply(WP::err error);
    void authConnected(WP::err error);
    void onContactFound(WP::err error);

private:
    void parseAddress(const QString &targetAddress);

    UserIdentity *fIdentity;
    QString fAddress;
    QString fTargetServer;
    QString fTargetUser;
    ContactRequest* fContactRequest;

    const RawMailMessage *fMessage;
    RemoteConnection *fRemoteConnection;
    RemoteConnectionReply *fServerReply;
    RemoteAuthentication *fAuthentication;
};


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
    Mailbox(const DatabaseBranch *branch, const QString &baseDir = "");
    ~Mailbox();

    WP::err createNewMailbox(KeyStore *keyStore, const QString &defaultKeyId,
                             bool addUidToBaseDir = true);
    WP::err open(KeyStoreFinder *keyStoreFinder);

    QAbstractListModel &getMessages();

    QStringList getMailIds();
    WP::err readHeader(const QString &messageId, QByteArray &header);
    WP::err readBody(const QString &messageId, QByteArray &body);

signals:
    void databaseReadProgress(float progress);
    void databaseRead();

private slots:
    virtual void onNewCommits(const QString &startCommit, const QString &endCommit);

private:
    QString pathForMessageId(const QString &messageId);

    WP::err readMailDatabase();

    MessageListModel fMessageList;
};


class MailboxFinder {
public:
    virtual ~MailboxFinder() {}
    virtual Mailbox *find(const QString &mailboxId) = 0;
};

#endif // MAILBOX_H

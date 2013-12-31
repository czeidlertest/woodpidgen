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
    return fMessages.count();
}

void MessageListModel::addMessage(MailRef *messageRef)
{
    int count = fMessages.count();
    beginInsertRows(QModelIndex(), count, count);
    fMessages.append(messageRef);
    endInsertRows();
}

MailRef *MessageListModel::removeMessageAt(int index)
{
    MailRef *entry = fMessages.at(index);
    beginRemoveRows(QModelIndex(), index, index);
    fMessages.removeAt(index);
    endRemoveRows();
    return entry;
}

void MessageListModel::clear()
{
    beginRemoveRows(QModelIndex(), 0, fMessages.count() - 1);
    foreach (MailRef *ref, fMessages)
        delete ref;
    fMessages.clear();
    endRemoveRows();
}

MailRef::MailRef(const QString &messageId, Mailbox *mailbox) :
    fMessageId(messageId),
    fMailbox(mailbox)
{
}

const QByteArray &MailRef::getHeader()
{
    if (fHeader.count() == 0) {
        WP::err error = fMailbox->readHeader(fMessageId, fHeader);
        if (error != WP::kOk)
            throw git_io_exception;
    }
    return fHeader;
}

const QByteArray &MailRef::getBody()
{
    if (fBody.count() == 0) {
        WP::err error = fMailbox->readBody(fMessageId, fBody);
        if (error != WP::kOk)
            throw git_io_exception;
    }
    return fBody;
}

Mailbox::Mailbox(DatabaseBranch *branch, const QString &baseDir) :
    fMessageList(this)
{
    setToDatabase(branch, baseDir);
}

Mailbox::~Mailbox()
{

}

WP::err Mailbox::createNewMailbox(KeyStore *keyStore, const QString &defaultKeyId, bool addUidToBaseDir)
{
    // derive uid
    QByteArray uid = fCrypto->generateInitalizationVector(512);
    QString uidHex = fCrypto->toHex(fCrypto->sha1Hash(uid));

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

    readMailDatabase();
    return error;
}

QAbstractListModel &Mailbox::getMessages()
{
    return fMessageList;
}

QStringList Mailbox::getMailIds()
{
    QStringList messageIds;
    QStringList shortIds = listDirectories("");
    for (int i = 0; i < shortIds.count(); i++) {
        const QString &shortId = shortIds.at(i);
        QStringList restIds = listDirectories(shortId);
        for (int a = 0; a < restIds.count(); a++) {
            QString completeId = shortId;
            completeId.append(restIds.at(a));
            messageIds.append(completeId);
        }
    }
    return messageIds;
}

WP::err Mailbox::readHeader(const QString &messageId, QByteArray &header)
{
    QString path = pathForMessageId(messageId);
    path += "/header";
    return read(path, header);
}

WP::err Mailbox::readBody(const QString &messageId, QByteArray &body)
{
    QString path = pathForMessageId(messageId);
    path += "/body";
    return read(path, body);
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

    QStringList mails = getMailIds();
    for (int i = 0; i < mails.count(); i++) {
        const QString &mailId = mails.at(i);
        MailRef *ref = new MailRef(mailId, this);
        fMessageList.addMessage(ref);
    }
    emit databaseRead();
    return WP::kOk;
}


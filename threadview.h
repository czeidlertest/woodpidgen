#ifndef THREADVIEW_H
#define THREADVIEW_H

#include <QPushButton>
#include <QLineEdit>
#include <QListView>
#include <QSplitter>
#include <QTextEdit>

class Mailbox;
class MessageListModel;
class Profile;

class ThreadView: public QSplitter
{
    Q_OBJECT
public:
    explicit ThreadView(Profile *profile, QWidget *parent = 0);

    void setMailbox(Mailbox *mailbox);
    void setMessages(MessageListModel *messages);

signals:

public slots:

private slots:
    void onSendButtonClicked();

private:
    QListView *fMessageDisplay;
    QLineEdit *fReceiver;
    QTextEdit *fMessageComposer;
    QPushButton *fSendButton;

    Mailbox *fMailbox;
    Profile *fProfile;
};

#endif // THREADVIEW_H

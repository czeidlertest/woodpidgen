#ifndef MESSAGEVIEW_H
#define MESSAGEVIEW_H

#include <QPushButton>
#include <QLineEdit>
#include <QListView>
#include <QSplitter>
#include <QTextEdit>
#include <QWidget>

#include "mailbox.h"
#include "profile.h"


class MessageView : public QSplitter
{
    Q_OBJECT
public:
    explicit MessageView(Profile *profile, QWidget *parent = 0);
    
    void setMailbox(Mailbox *mailbox);

signals:
    
public slots:
    
private slots:
    void onSendButtonClicked();

private:
    QListView *fMessageDisplay;
    QLineEdit *fReceiver;
    QTextEdit *fMessageComposer;
    QPushButton *fSendButton;

    Profile *fProfile;
    Mailbox *fMailbox;
};

#endif // MESSAGEVIEW_H

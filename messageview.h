#ifndef MESSAGEVIEW_H
#define MESSAGEVIEW_H

#include <QPushButton>
#include <QLineEdit>
#include <QListView>
#include <QSplitter>
#include <QTextEdit>
#include <QWidget>

#include "mailbox.h"


class MessageView : public QSplitter
{
    Q_OBJECT
public:
    explicit MessageView(QWidget *parent = 0);
    
    void setMailbox(Mailbox *mailbox);

signals:
    
public slots:
    
private:
    QListView *fMessageDisplay;
    QLineEdit *fReceiver;
    QTextEdit *fMessageComposer;
    QPushButton *fSendButton;

    Mailbox *fMailbox;
};

#endif // MESSAGEVIEW_H

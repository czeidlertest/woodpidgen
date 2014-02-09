#include "newmessageview.h"

#include <QVBoxLayout>

NewMessageView::NewMessageView(QWidget *parent) :
    QWidget(parent)
{
    QVBoxLayout* composerLayout = new QVBoxLayout();
    setLayout(composerLayout);

    receiver = new QLineEdit();
receiver->setText("lec@localhost");
    messageComposer = new QTextEdit();
    sendButton = new QPushButton("Send");
    composerLayout->addWidget(receiver);
    composerLayout->addWidget(messageComposer);
    composerLayout->addWidget(sendButton);
}

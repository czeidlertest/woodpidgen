#include "newmessageview.h"

#include <QVBoxLayout>

#include "mailmessenger.h"
#include "profile.h"

NewMessageView::NewMessageView(Profile *_profile, QWidget *parent) :
    QWidget(parent),
    profile(_profile)
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

    connect(sendButton, SIGNAL(clicked()), this, SLOT(onSendButtonClicked()));
}

void NewMessageView::setMailbox(Mailbox *box)
{
    mailbox = box;
}

void NewMessageView::onSendButtonClicked()
{
    QString address = receiver->text();
    if (address == "")
        return;

    MessageChannelInfo *channelInfo = new MessageChannelInfo((SecureChannel*)NULL);
    channelInfo->addParticipant(address, "");

    QString body = messageComposer->toPlainText();
    MultiMailMessenger *messenger = new MultiMailMessenger(mailbox, profile);

    Message *message = new Message(channelInfo);
    message->setBody(body.toLatin1());
    messenger->postMessage(message);

    // todo progress bar
    messageComposer->clear();
}

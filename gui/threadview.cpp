#include "threadview.h"

#include <QVBoxLayout>

#include "mailbox.h"
#include "mailmessenger.h"
#include "profile.h"
#include "useridentity.h"


ThreadView::ThreadView(Profile *profile, QWidget *parent) :
    QSplitter(Qt::Vertical, parent),
    fMailbox(NULL),
    fProfile(profile)
{
    fMessageDisplay = new QListView(this);
    QWidget *composerWidget = new QWidget(this);
    setStretchFactor(0, 5);
    setStretchFactor(1, 2);

    QVBoxLayout* composerLayout = new QVBoxLayout();
    composerWidget->setLayout(composerLayout);

    fReceiver = new QLineEdit(composerWidget);
fReceiver->setText("lec@localhost");
    fMessageComposer = new QTextEdit(composerWidget);
    fSendButton = new QPushButton("Send", composerWidget);
    composerLayout->addWidget(fReceiver);
    composerLayout->addWidget(fMessageComposer);
    composerLayout->addWidget(fSendButton);

    connect(fSendButton, SIGNAL(clicked()), this, SLOT(onSendButtonClicked()));
}

void ThreadView::setMailbox(Mailbox *mailbox)
{
    fMailbox = mailbox;
}

void ThreadView::setMessages(MessageListModel *messages)
{
    fMessageDisplay->setModel(messages);
}

void ThreadView::onSendButtonClicked()
{
    QString receiver = fReceiver->text();
    if (receiver == "")
        return;

    QString body = fMessageComposer->toPlainText();
    MailMessenger *messenger = new MailMessenger(fMailbox, receiver, fProfile, fProfile->getIdentityList()->identityAt(0));
    RawMailMessage *message = new RawMailMessage("header", body);
    messenger->postMessage(message);

    // todo progress bar
    fMessageComposer->clear();
}

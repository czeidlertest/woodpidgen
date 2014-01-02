#include "messageview.h"

#include <QVBoxLayout>

#include "useridentity.h"
#include "mailmessenger.h"


MessageView::MessageView(Profile *profile, QWidget *parent) :
    QSplitter(Qt::Vertical, parent),
    fProfile(profile),
    fMailbox(NULL)
{
    fMessageDisplay = new QListView(this);
    QWidget *composerWidget = new QWidget(this);
    QVBoxLayout* composerLayout = new QVBoxLayout();
    composerWidget->setLayout(composerLayout);

    fReceiver = new QLineEdit(composerWidget);
fReceiver->setText("cle@localhost");
    fMessageComposer = new QTextEdit(composerWidget);
    fSendButton = new QPushButton("Send", composerWidget);
    composerLayout->addWidget(fReceiver);
    composerLayout->addWidget(fMessageComposer);
    composerLayout->addWidget(fSendButton);

    connect(fSendButton, SIGNAL(clicked()), this, SLOT(onSendButtonClicked()));

    IdentityListModel *identityList = fProfile->getIdentityList();
    if (identityList->rowCount() > 0)
        setMailbox(identityList->identityAt(0)->getMailbox());
}

void MessageView::setMailbox(Mailbox *mailbox)
{
    fMailbox = mailbox;
    QAbstractListModel &listModel = fMailbox->getMessages();
    fMessageDisplay->setModel(&listModel);
}

void MessageView::onSendButtonClicked()
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

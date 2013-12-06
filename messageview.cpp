#include "messageview.h"

#include <QVBoxLayout>

MessageView::MessageView(QWidget *parent) :
    QSplitter(Qt::Vertical, parent),
    fMailbox(NULL)
{
    fMessageDisplay = new QListView(this);
    QWidget *composerWidget = new QWidget(this);
    QVBoxLayout* composerLayout = new QVBoxLayout();
    composerWidget->setLayout(composerLayout);

    fReceiver = new QLineEdit(composerWidget);
    fMessageComposer = new QTextEdit(composerWidget);
    fSendButton = new QPushButton("Send", composerWidget);
    composerLayout->addWidget(fReceiver);
    composerLayout->addWidget(fMessageComposer);
    composerLayout->addWidget(fSendButton);

}

void MessageView::setMailbox(Mailbox *mailbox)
{
    fMailbox = mailbox;
    QAbstractListModel &listModel = fMailbox->getMessages();
    fMessageDisplay->setModel(&listModel);
}

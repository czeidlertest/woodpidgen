#include "messageview.h"

#include "profile.h"
#include "threadview.h"


MessageView::MessageView(Profile *profile, QWidget *parent) :
    QSplitter(Qt::Horizontal, parent),
    fProfile(profile),
    fMailbox(NULL)
{
    fThreadListView = new QListView(this);
    fThreadView = new ThreadView(profile, this);

    setStretchFactor(0, 2);
    setStretchFactor(1, 5);

    connect(fThreadListView, SIGNAL(activated(QModelIndex)), this, SLOT(onThreadSelected(QModelIndex)));
}

void MessageView::setMailbox(Mailbox *mailbox)
{
    fMailbox = mailbox;

    fThreadView->setMailbox(fMailbox);
    fThreadListView->setModel(&fMailbox->getThreads());
}

void MessageView::onThreadSelected(QModelIndex index)
{
    MessageThread *messageThread = fMailbox->getThreads().channelAt(index.row());
    fThreadView->setMessages(messageThread->getMessages());
}


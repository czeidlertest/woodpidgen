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
}

void MessageView::setMailbox(Mailbox *mailbox)
{
    fThreadView->setMailbox(mailbox);
}

#ifndef MESSAGEVIEW_H
#define MESSAGEVIEW_H

#include <QListView>
#include <QSplitter>

class Mailbox;
class Profile;
class ThreadView;

class MessageView : public QSplitter
{
    Q_OBJECT
public:
    explicit MessageView(Profile *profile, QWidget *parent = 0);
    
    void setMailbox(Mailbox *mailbox);

private:
    Profile *fProfile;
    Mailbox *fMailbox;

    QListView *fThreadListView;

    ThreadView *fThreadView;
};

#endif // MESSAGEVIEW_H

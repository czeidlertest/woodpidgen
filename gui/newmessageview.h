#ifndef NEWMESSAGEVIEW_H
#define NEWMESSAGEVIEW_H

#include <QPushButton>
#include <QLineEdit>
#include <QTextEdit>


class NewMessageView : public QWidget
{
    Q_OBJECT
public:
    explicit NewMessageView(QWidget *parent = 0);

signals:

public slots:

private:
    QLineEdit *receiver;
    QTextEdit *messageComposer;
    QPushButton *sendButton;
};

#endif // NEWMESSAGEVIEW_H

#ifndef MESSAGEVIEW_H
#define MESSAGEVIEW_H

#include <QPushButton>
#include <QLineEdit>
#include <QSplitter>
#include <QTextEdit>
#include <QWidget>

class MessageView : public QSplitter
{
    Q_OBJECT
public:
    explicit MessageView(QWidget *parent = 0);
    
signals:
    
public slots:
    
private:
    QTextEdit *fMessageDisplay;
    QLineEdit *fReceiver;
    QTextEdit *fMessageComposer;
    QPushButton *fSendButton;
};

#endif // MESSAGEVIEW_H

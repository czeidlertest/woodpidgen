#include "messageview.h"

#include <QVBoxLayout>

MessageView::MessageView(QWidget *parent) :
    QSplitter(Qt::Vertical, parent)
{
    fMessageDisplay = new QTextEdit(this);
    fMessageDisplay->setReadOnly(true);
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

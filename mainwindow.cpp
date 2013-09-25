#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <messageview.h>
#include <useridentityview.h>


MainWindow::MainWindow(Profile *profile, QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    ui->toolBar->setMovable(false);

    QAction *accountAction = ui->toolBar->addAction("Accounts");
    accountAction->setCheckable(true);
    QAction *messageAction = ui->toolBar->addAction("Messages");
    messageAction->setCheckable(true);

    QActionGroup* actionGroup = new QActionGroup(this);
    actionGroup->addAction(accountAction);
    actionGroup->addAction(messageAction);

    fIdentityView = new UserIdentityView(profile->getIdentityList(), this);
    fMessageView = new MessageView(this);
    ui->horizontalLayout->setMargin(0);
    ui->stackedWidget->addWidget(fIdentityView);
    ui->stackedWidget->addWidget(fMessageView);

    connect(accountAction, SIGNAL(toggled(bool)), this, SLOT(accountActionToggled(bool)));
    connect(messageAction, SIGNAL(toggled(bool)), this, SLOT(messageActionToggled(bool)));

    accountAction->setChecked(true);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::accountActionToggled(bool checked)
{
    if (checked)
        ui->stackedWidget->setCurrentWidget(fIdentityView);
}

void MainWindow::messageActionToggled(bool checked)
{
    if (checked)
        ui->stackedWidget->setCurrentWidget(fMessageView);
}

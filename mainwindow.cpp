#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "cryptolib.h"

#include "cryptointerface.h"


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    ui->toolBar->setMovable(false);
    QAction *accountAction = ui->toolBar->addAction("Accounts");
    QAction *messageAction = ui->toolBar->addAction("Messages");

    ui->horizontalLayout->setMargin(0);


    const char* kKeyName = "test_label";
    const char* kKeyPassword = "test1";

    //const char* kKeyFileNamePublic = "pubrsa1.gpg";
    //const char* kKeyFileNamePrivate = "secrsa1.gpg";

    const char* kKeyFileNameCert = "certkeyset.p15";
    const char* kKeyFileNamePublic = "pubkeyset.p15";
    const char* kKeyFileNamePrivate = "seckeyset.p15";

    CryptoInterface crypto;
    crypto.generateKeyPair(kKeyFileNameCert, kKeyFileNamePublic, kKeyFileNamePrivate, kKeyPassword);
    crypto.encryptionTest(kKeyFileNameCert, kKeyFileNamePublic, kKeyFileNamePrivate, kKeyPassword);
    //CryptoLib crypto;
    //crypto.EncrypteData();
}

MainWindow::~MainWindow()
{
    delete ui;
}

#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "cryptolib.h"


MainWindow::MainWindow(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->pushButtonEncrypt->setToolTip("Encrypt");

    CryptoLib crypto;
    crypto.EncrypteData();
}

MainWindow::~MainWindow()
{
    delete ui;
}

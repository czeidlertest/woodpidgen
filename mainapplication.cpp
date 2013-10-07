#include "mainapplication.h"

#include <QMessageBox>
#include <QtNetwork/QNetworkCookieJar>

#include <messagereceiver.h>

#include <useridentity.h>


#include <serverconnection.h>


MainApplication::MainApplication(int &argc, char *argv[]) :
    QApplication(argc, argv)
{
    fNetworkAccessManager = new QNetworkAccessManager(this);
    fNetworkAccessManager->setCookieJar(new QNetworkCookieJar(this));

    fProfile = new Profile(".git", "profile");

    fMainWindow = new MainWindow(fProfile);
    fMainWindow->show();

    SecureArray password("test_password");
    if (fProfile->open(password) != 0) {
        fProfile->createNewProfile(password);
        fProfile->commit();
        if (fProfile->open(password) != 0) {
            QMessageBox::information(NULL, "Error", "Unable to create or load a profile!");
            quit();
        }
    }

    EncryptedPHPConnection *encryptedPHPConnection = new EncryptedPHPConnection(QUrl("http://clemens-zeidler.de/woodpidgin/portal.php"), this);
    encryptedPHPConnection->connectToServer();

    PingRCReply *replyTest = new PingRCReply(encryptedPHPConnection, this);
    connect(encryptedPHPConnection, SIGNAL(connectionAttemptFinished(QNetworkReply::NetworkError)), replyTest, SLOT(connectionAttemptFinished(QNetworkReply::NetworkError)));

    QByteArray key = QByteArray::fromBase64("jh6y5VcEuDXpUTrLWq8pJYyMHmLQCBOGSGWh2YiC0GBr0VRzsBV79HdxQJ8t7utUniNr6LG4Av8TT7MSwQnIPA==");
    QByteArray iv = QByteArray::fromBase64("qf90IciaKTsBk/yS4r27 vCRXgLWYrQCWq1vAz54DVw1JUfTYy45vMXmQMZmsGAVCi/dJqP6hE1PmkjHV9lOGdcut6Syd6Vc1y2eVT21Obpa4woS6ku9oTi6TVAP1bVDsKrblM/F63UJYXRc/7og5n kBx3N6kMYvc2L3clLbUwBWxctA SpeUDuhftEARn0YsNubw/tWObH5sSi1FEMzzyq0oaoPXmTeq9clE5TX6Q U7sBnJtk3 T4sJFaXAoLNVAJAAOupDD6fzcOOYHbpRCirkFmMRWLay9lSyDR/ORM lAMNDxkLnrgPfEcex5ioblmNj1o2TDw3yNItFQDr9dRd0SNWVAddc0OuekutZ/ZATeeuSHIBXuFve0K4NgWFGJmYsw0kHtIP28hQS8KhN71B3vBUIJfMknQXDLDIWsW9oMP1xrOOzrQ2hQZHgwIjgawmUfbA4246AM6cuXEWLWoF4vHf2g0HG1UJSdt5pGWeycUee82wmcKiYnalRxWCqaCXP0eE0bfBaWAkScuMJuZwscGCsVTy8Sl3MF5vKpqQ6M3frYgsTW1zZYsow9Yzkm1rZBCWh1VbBtO9TNP6ksU2/eOdQSWuPAQQo9qm6OB8IW5gDI3Ndz9OIZERt5/FNkB872qZNMtET2ddFWHVXGQhURR6nwR7TWjYk5k Ck=");

    for (int i = key.count(); i < 256; i++)
        key.append('\0');

    QByteArray testData("Hello World");
    QByteArray encrypted;
    CryptoInterfaceSingleton::getCryptoInterface()->encryptSymmetric(testData, encrypted, key, iv, "aes256");

    qDebug() << "encrypted: " << encrypted.toBase64() << endl;

    //UserIdentity identity(&gitInterface, &crypto);
    //identity.createNewIdentity(password);
    //identity.commit();
    //identity.open(password, list.at(0));

    /*
    MessageReceiver receiver(&gitInterface);
    QDeclarativeView view;
    view.rootContext()->setContextProperty("messageReceiver", &receiver);
    view.setSource(QUrl::fromLocalFile("qml/woodpidgin/main.qml"));
    view.show();
*/


}

MainApplication::~MainApplication()
{
    delete fMainWindow;
    delete fProfile;
    CryptoInterfaceSingleton::destroy();
}

QNetworkAccessManager *MainApplication::getNetworkAccessManager()
{
    return fNetworkAccessManager;
}

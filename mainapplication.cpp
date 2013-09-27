#include "mainapplication.h"

#include <QMessageBox>
#include <QtNetwork/QNetworkCookieJar>

#include <messagereceiver.h>

#include <gitinterface.h>
#include <useridentity.h>


#include <serverconnection.h>


MainApplication::MainApplication(int &argc, char *argv[]) :
    QApplication(argc, argv)
{
    fNetworkAccessManager = new QNetworkAccessManager(this);
    fNetworkAccessManager->setCookieJar(new QNetworkCookieJar(this));

    fDatabase = new GitInterface;
    fDatabase->setTo(QString(".git"));

    fCrypto = new CryptoInterface;
    fProfile = new Profile(fDatabase, fCrypto);

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

    EncryptedPHPConnection *encryptedPHPConnection = new EncryptedPHPConnection(QUrl("http://clemens-zeidler.de/woodpidgin/portal.php"), fCrypto, this);
    encryptedPHPConnection->connectToServer();

    PingRCReply *replyTest = new PingRCReply(encryptedPHPConnection, this);
    connect(encryptedPHPConnection, SIGNAL(connectionAttemptFinished(QNetworkReply::NetworkError)), replyTest, SLOT(connectionAttemptFinished(QNetworkReply::NetworkError)));

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
    delete fDatabase;
    delete fCrypto;
}

QNetworkAccessManager *MainApplication::getNetworkAccessManager()
{
    return fNetworkAccessManager;
}

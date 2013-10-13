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
    if (fProfile->open(password) != WP::kOk) {
        WP::err error = fProfile->createNewProfile(password);
        if (error != WP::kOk) {
            QMessageBox::information(NULL, "Error", "Unable to create or load a profile!");
            quit();
        }
        fProfile->commit();
    }

    //EncryptedPHPConnection *encryptedPHPConnection = new EncryptedPHPConnection(QUrl("http://clemens-zeidler.de/woodpidgin/portal.php"), this);
    EncryptedPHPConnection *encryptedPHPConnection = new EncryptedPHPConnection(QUrl("localhost/php_server/portal.php"), this);
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
    CryptoInterfaceSingleton::destroy();
}

QNetworkAccessManager *MainApplication::getNetworkAccessManager()
{
    return fNetworkAccessManager;
}

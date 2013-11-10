#include "mainapplication.h"

#include <QMessageBox>
#include <QtNetwork/QNetworkCookieJar>

#include <messagereceiver.h>

#include <useridentity.h>
#include "phpremotestorage.h"

// test
#include <serverconnection.h>
#include "protocolparser.h"
#include <QBuffer>
#include "remotesync.h"
#include <QDebug>

class TestHandler : public InStanzaHandler {
public:
    TestHandler(QString stanza, QString text) :
        InStanzaHandler(stanza),
        fText(text)
    {
    }

    void handleStanza(const QXmlStreamAttributes &attributes)
    {
        qDebug() << fText << endl;
    }

private:
    QString fText;
};

MainApplication::MainApplication(int &argc, char *argv[]) :
    QApplication(argc, argv)
{
    fNetworkAccessManager = new QNetworkAccessManager(this);
    fNetworkAccessManager->setCookieJar(new QNetworkCookieJar(this));

    fProfile = new Profile(".git", "profile");

    fMainWindow = new MainWindow(fProfile);
    fMainWindow->show();
/*
    // pull test
    DatabaseInterface *syncDatabase;
    DatabaseFactory::open(".git", "profile", &syncDatabase);
    RemoteConnection *connection = new HTTPConnection(QUrl("http://localhost/php_server/portal.php"));
    RemoteSync *pullSync = new RemoteSync(syncDatabase, connection, this);
    pullSync->sync();
    return;
*/
    SecureArray password("test_password");
    if (fProfile->open(password) != WP::kOk) {
        RemoteDataStorage* remote = new HTTPRemoteStorage("http://localhost/php_server/portal.php");
        //RemoteDataStorage* remote = new PHPEncryptedRemoteStorage("http://localhost/php_server/portal.php");
        WP::err error = fProfile->createNewProfile(password, remote);
        if (error != WP::kOk) {
            delete remote;
            QMessageBox::information(NULL, "Error", "Unable to create or load a profile!");
            quit();
        }
    }

    DatabaseBranch *branch = fProfile->getBranches().at(0);
    RemoteSync *sync = new RemoteSync(branch->getDatabase(), branch->getRemoteConnectionAt(0), this);
    sync->sync();
/*
    QByteArray data;
    ProtocolOutStream outStream(&data);
    outStream.pushStanza(new IqOutStanza(kGet));
    outStream.pushChildStanza(new OutStanza("moep"));
    outStream.flush();

    qDebug() << data << endl;

    InStanzaHandler *iqHandler = new InStanzaHandler("iq");
    iqHandler->addChildHandler(new TestHandler("moep", "test1"));
    iqHandler->addChildHandler(new TestHandler("moep", "test2"));

    ProtocolInStream inStream(data);
    inStream.addHandler(iqHandler);
    inStream.parse();

    //EncryptedPHPConnection *encryptedPHPConnection = new EncryptedPHPConnection(QUrl("http://clemens-zeidler.de/woodpidgin/portal.php"), this);

    EncryptedPHPConnection *encryptedPHPConnection = new EncryptedPHPConnection(QUrl("http://localhost/php_server/portal.php"), this);
    encryptedPHPConnection->connectToServer();

    PingRCCommand *replyTest = new PingRCCommand(encryptedPHPConnection, this);
    connect(encryptedPHPConnection, SIGNAL(connectionAttemptFinished(QNetworkReply::NetworkError)), replyTest, SLOT(connectionAttemptFinished(QNetworkReply::NetworkError)));
    */
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

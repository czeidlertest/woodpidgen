#include "mainapplication.h"

#include <QMessageBox>
#include <QtNetwork/QNetworkCookieJar>

#include <messagereceiver.h>

#include "useridentity.h"
#include "syncmanager.h"

// test
#include <remoteconnection.h>
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

    bool handleStanza(const QXmlStreamAttributes &attributes)
    {
        qDebug() << fText << endl;
        return true;
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

    SecureArray password("test_password");
    QString serverUser = "cle";
    QString server = "localhost";
    QString remoteUrl = "http://" + server + "/php_server/portal.php";
    if (fProfile->open(password) != WP::kOk) {
        WP::err error = fProfile->createNewProfile(password);
        if (error != WP::kOk) {
            QMessageBox::information(NULL, "Error", "Unable to create or load a profile!");
            quit();
        }
        RemoteDataStorage *remote = fProfile->addHTTPRemote(remoteUrl);
        //RemoteDataStorage *remote = fProfile->addPHPRemote(remoteUrl);
        UserIdentity *mainIdentity = fProfile->getIdentityList()->identityAt(0);
        Contact *myself = mainIdentity->getMyself();
        myself->setServer(server);
        myself->setServerUser(serverUser);
        myself->writeConfig();
        fProfile->setSignatureAuth(remote, myself->getUid(),
                                   mainIdentity->getKeyStore()->getUid(),
                                   myself->getKeys()->getMainKeyId(), myself->getServerUser());

        fProfile->connectFreeBranches(remote);
        fProfile->commit();
    }

    DatabaseBranch *branch = NULL;
    QList<DatabaseBranch*> &branches = fProfile->getBranches();
    SyncManager *syncManager = new SyncManager(branches.at(0)->getRemoteAt(0));
    foreach (branch, branches)
        syncManager->keepSynced(branch->getDatabase());
    syncManager->startWatching();

    fMainWindow = new MainWindow(fProfile);
    fMainWindow->show();
/*
    MailMessenger *messenger = new MailMessenger("cle@localhost", fProfile, fProfile->getIdentityList()->identityAt(0));
    RawMailMessage *message = new RawMailMessage("header", "body");
    messenger->postMessage(message);
    */
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
    connect(encryptedPHPConnection, SIGNAL(connectionAttemptFinished(WP::err)), replyTest, SLOT(connectionAttemptFinished(WP::err)));
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

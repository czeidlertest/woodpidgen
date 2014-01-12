#include "mainapplication.h"

#include <QtNetwork/QNetworkCookieJar>

#include "createprofiledialog.h"
#include "passworddialog.h"
#include "useridentity.h"
#include "syncmanager.h"


#include <QFile>
QString readPassword() {
    QFile file("password");
    file.open(QIODevice::ReadOnly);
    char buffer[255];
    buffer[0] = '\0';
    file.readLine(buffer, 256);
    QString password(buffer);
    password.replace("\n", "");
    return password;
}

MainApplication::MainApplication(int &argc, char *argv[]) :
    QApplication(argc, argv)
{
    fNetworkAccessManager = new QNetworkAccessManager(this);
    fNetworkAccessManager->setCookieJar(new QNetworkCookieJar(this));

    fProfile = new Profile(".git", "profile");

    // convenient hack
    QString password = readPassword();

    WP::err error = WP::kBadKey;
    while (true) {
        error = fProfile->open(password.toLatin1());
        if (error != WP::kBadKey)
            break;

        PasswordDialog passwordDialog;
        int result = passwordDialog.exec();
        if (result != QDialog::Accepted) {
            exit(-1);
            return;
        }
        password = passwordDialog.getPassword();
    }

    if (error != WP::kOk) {
        CreateProfileDialog createDialog(fProfile);
        int result = createDialog.exec();
        if (result != QDialog::Accepted) {
            exit(-1);
            return;
        }
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

#include <QApplication>
#if QT_VERSION < 0x050000
#include <QtDeclarative/QDeclarativeContext>
#include <QtDeclarative/QDeclarativeView>
#else
#include <QtGui/QGuiApplication>
#include <QQmlContext>
#endif

#include <gitinterface.h>
#include <mainwindow.h>
#include <messagereceiver.h>
#include <useridentity.h>

#include <stdio.h>
int main(int argc, char *argv[])
{
    GitInterface gitInterface;

    gitInterface.setBranch("master");
    gitInterface.setTo(QString(".git"));

    /*QByteArray data;
    data.setRawData("fake", 4);
    gitInterface.add("//base/dir/dir3/test6", data);
    gitInterface.commit();

    QByteArray out;
    gitInterface.get("base/dir/dir3/test6", out);*/

    SecureArray password("test_password");
    UserIdentity::createNewIdentity(&gitInterface, password);

    QStringList list = UserIdentity::getIdenties(&gitInterface);
    for (int i = 0; i < list.count(); i++)
        printf("item %s\n", list.at(i).toAscii().data());

    UserIdentity identity(&gitInterface, list.at(0), password);
    QString out;
    QTextStream stream(&out);
    identity.printToStream(stream);
    printf("identidy:%s\n", out.data());

    //gitInterface.AddMessage("test message");

    QApplication app(argc, argv);
    MainWindow window;
    window.show();

    /*
    MessageReceiver receiver(&gitInterface);
    QDeclarativeView view;
    view.rootContext()->setContextProperty("messageReceiver", &receiver);
    view.setSource(QUrl::fromLocalFile("qml/woodpidgin/main.qml"));
    view.show();
*/


    return app.exec();
}

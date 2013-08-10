#include <QApplication>
#include <QQmlContext>
#include <QtGui/QGuiApplication>
#include "qtquick2applicationviewer.h"

#include <gitinterface.h>
#include <mainwindow.h>


class MessageReceiver : public QObject
{
    Q_OBJECT
public:

    virtual ~MessageReceiver() {}
    Q_INVOKABLE void messageDataReceived(void* data, size_t length) {
        printf("Data: %s\n", (char*)data);
    }
};



int main(int argc, char *argv[])
{
    GitInterface gitInterface;

    gitInterface.setTo(QString(".gitTest"));
    gitInterface.AddMessage("test message");

    QGuiApplication app(argc, argv);

    //QApplication app(argc, argv);
    //MainWindow window;
    //window.show();
    MessageReceiver receiver;

    QtQuick2ApplicationViewer viewer;
    viewer.rootContext()->setContextProperty("messageReceiver", &receiver);
    viewer.setMainQmlFile(QStringLiteral("qml/woodpidgin/main.qml"));
    viewer.showExpanded();

    return app.exec();
}

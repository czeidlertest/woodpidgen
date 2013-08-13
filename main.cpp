#include <QApplication>
#include <QQmlContext>
#include <QtGui/QGuiApplication>
#include "qtquick2applicationviewer.h"

#include <gitinterface.h>
#include <mainwindow.h>
#include <messagereceiver.h>


int main(int argc, char *argv[])
{
    GitInterface gitInterface;

    gitInterface.setTo(QString(".git"));
    //gitInterface.AddMessage("test message");

    QGuiApplication app(argc, argv);

    //QApplication app(argc, argv);
    //MainWindow window;
    //window.show();
    MessageReceiver receiver(&gitInterface);

    QtQuick2ApplicationViewer viewer;
    viewer.rootContext()->setContextProperty("messageReceiver", &receiver);
    viewer.setMainQmlFile(QStringLiteral("qml/woodpidgin/main.qml"));
    viewer.showExpanded();

    return app.exec();
}

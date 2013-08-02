#include <QApplication>
#include <QtGui/QGuiApplication>
#include "qtquick2applicationviewer.h"

#include <mainwindow.h>


int main(int argc, char *argv[])
{
    //QGuiApplication app(argc, argv);

    QApplication app(argc, argv);
    MainWindow window;
    window.show();
    //QtQuick2ApplicationViewer viewer;
    //viewer.setMainQmlFile(QStringLiteral("qml/woodpidgin/main.qml"));
    //viewer.showExpanded();

    return app.exec();
}

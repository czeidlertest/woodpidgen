# Add more folders to ship with the application, here
folder_01.source = qml/woodpidgin
folder_01.target = qml
DEPLOYMENTFOLDERS = folder_01

QT       += core gui
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
QT += declarative

# Additional import path used to resolve QML modules in Creator's code model
QML_IMPORT_PATH =

# If your application uses the Qt Mobility libraries, uncomment the following
# lines and add the respective components to the MOBILITY variable.
# CONFIG += mobility
# MOBILITY +=

# The .cpp file which was generated for your project. Feel free to hack it.
SOURCES += main.cpp \
    mainwindow.cpp \
    gitinterface.cpp \
    cryptolib.cpp \
    messagereceiver.cpp \
    cryptointerface.cpp \
    useridentity.cpp

# Installation path
# target.path =

HEADERS += \
    mainwindow.h \
    cryptolib.h \
    gitinterface.h \
    messagereceiver.h \
    cryptointerface.h \
    useridentity.h \
    databaseinterface.h

FORMS += \
    mainwindow.ui

win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../cl342/release/ -lcl
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../cl342/debug/ -lcl
else:unix: LIBS += -L$$PWD/../cl342/ -lcl

INCLUDEPATH += $$PWD/../cl342
DEPENDPATH += $$PWD/../cl342

win32:CONFIG(release, debug|release): PRE_TARGETDEPS += $$PWD/../cl342/release/cl.lib
else:win32:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$PWD/../cl342/debug/cl.lib
else:unix: PRE_TARGETDEPS += $$PWD/../cl342/libcl.a

unix|win32: LIBS += -lgit2

unix|win32: LIBS += -lqca

OTHER_FILES += \
    qml/woodpidgin/main.qml

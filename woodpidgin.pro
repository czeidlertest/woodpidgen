# Add more folders to ship with the application, here
#folder_01.source = qml/woodpidgin
#folder_01.target = qml
#DEPLOYMENTFOLDERS = folder_01

QT += core gui
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
QT += network

# Additional import path used to resolve QML modules in Creator's code model
#QML_IMPORT_PATH =

# If your application uses the Qt Mobility libraries, uncomment the following
# lines and add the respective components to the MOBILITY variable.
# CONFIG += mobility
# MOBILITY +=

# The .cpp file which was generated for your project. Feel free to hack it.
SOURCES += main.cpp \
    mainwindow.cpp \
    gitinterface.cpp \
    messagereceiver.cpp \
    cryptointerface.cpp \
    useridentity.cpp \
    useridentityview.cpp \
    messageview.cpp \
    profile.cpp \
    databaseutil.cpp \
    mainapplication.cpp \
    BigInteger/BigUnsigned.cpp \
    BigInteger/BigIntegerUtils.cpp \
    BigInteger/BigIntegerAlgorithms.cpp \
    BigInteger/BigInteger.cpp \
    BigInteger/BigUnsignedInABase.cc \
    databaseinterface.cpp \
    remotesync.cpp \
    protocolparser.cpp \
    remotestorage.cpp \
    remoteconnection.cpp \
    remoteauthentication.cpp \
    mailbox.cpp

# Installation path
# target.path =

HEADERS += \
    mainwindow.h \
    gitinterface.h \
    messagereceiver.h \
    cryptointerface.h \
    useridentity.h \
    databaseinterface.h \
    useridentityview.h \
    messageview.h \
    profile.h \
    databaseutil.h \
    mainapplication.h \
    BigInteger/BigUnsigned.hh \
    BigInteger/BigIntegerUtils.hh \
    BigInteger/BigIntegerAlgorithms.hh \
    BigInteger/BigInteger.hh \
    BigInteger/NumberlikeArray.hh \
    BigInteger/BigUnsignedInABase.hh \
    error_codes.h \
    remotesync.h \
    protocolparser.h \
    remotestorage.h \
    remoteconnection.h \
    remoteauthentication.h \
    mailbox.h

FORMS += \
    mainwindow.ui

unix: LIBS += -L$$PWD/../libgit2/build -lgit2 -lz

INCLUDEPATH += $$PWD/../libgit2/include
DEPENDPATH += $$PWD/../libgit2

#unix: PRE_TARGETDEPS += $$PWD/../libgit2/.a

#unix|win32: LIBS += -lgit2
unix|win32: LIBS += -lqca

#OTHER_FILES += \
 #   qml/woodpidgin/main.qml

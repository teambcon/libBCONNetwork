#-------------------------------------------------
#
# Project created by QtCreator 2019-03-23T21:36:28
#
#-------------------------------------------------

QT -= gui
QT += network

TARGET = BCONNetwork
TEMPLATE = lib
CONFIG += staticlib

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    src/datastore.cpp \
    src/bconnetwork.cpp \
    src/nfcmanager.cpp

HEADERS += \
    src/datastore.h \
    src/bconnetwork.h \
    src/nfcmanager.h

mac: LIBS += -framework PCSC

unix:!macx: CONFIG += link_pkgconfig
unix:!macx: PKGCONFIG += libpcsclite

macx {
    target.path = "$$_PRO_FILE_PWD_/lib/macOS/"
    headers.path = "$$_PRO_FILE_PWD_/lib/macOS/include/BCONNetwork"
    headers.files = src/*.h
    INSTALLS += target headers
}

unix:!macx {
    target.path = "$$_PRO_FILE_PWD_/lib/Linux/"
    headers.path = "$$_PRO_FILE_PWD_/lib/Linux/include/BCONNetwork"
    headers.files = src/*.h
    INSTALLS += target headers
}

TEMPLATE = app

QT += qml quick widgets network

SOURCES += main.cpp \
    easywsclient.cpp

RESOURCES += qml.qrc

# Additional import path used to resolve QML modules in Qt Creator's code model
QML_IMPORT_PATH =

# Default rules for deployment.
include(deployment.pri)

HEADERS += \
    websocketclient.h \
    easywsclient.hpp

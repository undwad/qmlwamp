TEMPLATE = lib
TARGET = qmlwebsockets
QT += qml quick network
CONFIG += qt plugin

TARGET = $$qtLibraryTarget($$TARGET)
uri = qmlwebsockets

# Input
SOURCES += \
    qmlwebsockets_plugin.cpp

HEADERS += \
    qmlwebsockets_plugin.h \
    websocketclient.h

DISTFILES = qmldir \
    qmlwebsockets.pro.user

!equals(_PRO_FILE_PWD_, $$OUT_PWD) {
    copy_qmldir.target = $$OUT_PWD/qmldir
    copy_qmldir.depends = $$_PRO_FILE_PWD_/qmldir
    copy_qmldir.commands = $(COPY_FILE) \"$$replace(copy_qmldir.depends, /, $$QMAKE_DIR_SEP)\" \"$$replace(copy_qmldir.target, /, $$QMAKE_DIR_SEP)\"
    QMAKE_EXTRA_TARGETS += copy_qmldir
    PRE_TARGETDEPS += $$copy_qmldir.target
}

qmldir.files = qmldir
unix {
    installPath = $$[QT_INSTALL_QML]/$$replace(uri, \\., /)
    qmldir.path = $$installPath
    target.path = $$installPath
    INSTALLS += target qmldir
}


QT += core gui widgets network

CONFIG += c++17

LIBS += -L$$PWD/libraries/ -llua -ldiscord-rpc
PRE_TARGETDEPS += $$PWD/libraries/discord-rpc.lib

RC_ICONS = resources/iconMain.ico
RC_FILE = Windows.rc

SOURCES += \
    AboutFrontend.cpp \
    LuaThread.cpp \
    Main.cpp \
    Console.cpp \
    LuaBackend.cpp \
    MainWindow.cpp \
    WaitDialog.cpp \

HEADERS += \
    AboutFrontend.hpp \
    Console.hpp \
    LuaThread.hpp \
    WaitDialog.hpp \
    LuaBackend.hpp \
    MainWindow.hpp \
    version.h

FORMS += \
    AboutFrontend.ui \
    Console.ui \
    MainWindow.ui \
    WaitDialog.ui

INCLUDEPATH += \
    $$PWD/include/ \
    $$PWD/include/lua \
    $$PWD/include/sol2 \
    $$PWD/include/crcpp \
    $$PWD/include/toml11 \
    $$PWD/include/discord

DEPENDPATH += \
    $$PWD/include/lua \
    $$PWD/include/discord

RESOURCES += \
    Resources.qrc

DISTFILES += \
    Windows.rc

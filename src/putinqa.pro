QT += core widgets network websockets

CONFIG += c++17

SOURCES += \
    client/authorization.cpp \
    client/client.cpp \
    client/serverworkload.cpp \
    client/session/actions.cpp \
    client/session/websocketconnection.cpp \
    client/session/session.cpp \
    client/session/sessionstate.cpp \
    main.cpp \
    mainwindow.cpp

HEADERS += \
    client/authorization.h \
    client/client.h \
    client/serverworkload.h \
    client/session/actions.h \
    client/session/websocketconnection.h \
    client/session/session.h \
    client/session/sessionstate.h \
    mainwindow.h

FORMS += \
    mainwindow.ui

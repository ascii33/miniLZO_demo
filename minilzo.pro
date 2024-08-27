TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

INCLUDEPATH += $$PWD/minilzo-2.10

SOURCES += \
        minilzo-2.10/minilzo.c \
        testmini.c

HEADERS += \
    minilzo-2.10/lzoconf.h \
    minilzo-2.10/lzodefs.h \
    minilzo-2.10/minilzo.h

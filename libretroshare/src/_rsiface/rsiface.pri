INCLUDEPATH += $$PWD \
    ../$$PWP
DEPENDPATH += $$PWD
SOURCES = $$PWP/rsinit.cc \
    $$PWP/rsiface.cc \
    $$PWP/rscontrol.cc \
    $$PWP/notifybase.cc \
    $$PWP/rsifacereal.cc \
    $$PWP/rstypes.cc \
    rsexpr.cc
HEADERS = $$PWP/rsinit.h \
    $$PWP/rsiface.h \
    $$PWP/rscontrol.h \
    $$PWP/notifybase.h \
    $$PWP/rsifacereal.h \
    $$PWP/rstypes.h \
    $$PWP/rsfiles.h \
    $$PWP/rsexpr.h \
    $$PWP/rsgame.h

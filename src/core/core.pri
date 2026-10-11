QT += sql
INCLUDEPATH += $$PWD
HEADERS += \
    $$PWD/types.h \
    $$PWD/database.h \
    $$PWD/store.h \
    $$PWD/analytes.h \
    $$PWD/scheduleengine.h \
    $$PWD/ical.h \
    $$PWD/backup.h \
    $$PWD/vocab.h
SOURCES += \
    $$PWD/database.cpp \
    $$PWD/store.cpp \
    $$PWD/analytes.cpp \
    $$PWD/scheduleengine.cpp \
    $$PWD/ical.cpp \
    $$PWD/backup.cpp \
    $$PWD/vocab.cpp

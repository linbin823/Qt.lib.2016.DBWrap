QT += sql

HEADERS += \    
    $$PWD/dbwrap.h
LIBS += -ldl #显式加载动态库的动态函数库

SOURCES += \
    $$[QT_INSTALL_PREFIX]/../Src/qtbase/src/3rdparty/sqlite/sqlite3.c \
    $$PWD/dbwrap.cpp

INCLUDEPATH += $$PWD\
    $$[QT_INSTALL_PREFIX]/../Src/qtbase/src/3rdparty/sqlite

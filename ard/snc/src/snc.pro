include( ../../ard.pro )

######################################################################
TEMPLATE = lib
TARGET = snc

INC = ../include
DESTDIR = ../lib
INCLUDEPATH += $${INC}
DEPENDPATH += $${INC}
CONFIG += staticlib qt


HEADERS += $$files($${INC}/*.h, false)
SOURCES += $$files(*.cpp, false)


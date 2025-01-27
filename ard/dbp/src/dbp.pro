include( ../../ard.pro )

######################################################################
TEMPLATE = lib
TARGET = dbp
INC = ../include
DESTDIR = ../lib
INCLUDEPATH += $${INC} ../../snc/include
DEPENDPATH += $${INC} ../../snc/include
CONFIG += staticlib qt exceptions

HEADERS += $$files($${INC}/*.h, false)
SOURCES += $$files(*.cpp, false)


if(!isEmpty( ARD_DBOX )){
 HEADERS += $${INC}/dbox-syncpoint.h
 SOURCES += dbox-syncpoint.cpp
}

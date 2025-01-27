include( ../../ard.pro )

######################################################################
TEMPLATE = lib
TARGET = exg
INC = ../include
DESTDIR = ../lib
INCLUDEPATH += $${INC} ../../snc/include ../../dbp/include ../../gui/include
DEPENDPATH += $${INC} ../../snc/include ../../dbp/include ../../gui/include
CONFIG += staticlib qt


HEADERS += $$files($${INC}/*.h, false)
SOURCES += $$files(*.cpp, false)

    
ios{
	INCLUDEPATH += $${INC}/ios
	HEADERS += $${INC}/ard-iphone.h
	HEADERS += $${INC}/ios/test-view.h
	OBJECTIVE_SOURCES += ios/ard-iphone.mm
    OBJECTIVE_SOURCES += ios/test-view.mm
	SOURCES += ios/ard-ios.cpp
}

android{
	INCLUDEPATH += $${INC}/android
	SOURCES += android/ard-android.cpp
}


win32-msvc2010 {
    QMAKE_CXXFLAGS += -EHsc
}

win32{
    INC += $$files(../win/*.h, false)
    SOURCES += $$files(../win/*.cpp, false)
}

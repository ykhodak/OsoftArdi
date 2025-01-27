ARD_DEBUG = $$(ARD_DEBUG)
ARD_PROFILE = $$(ARD_PROFILE)
ARD_HOME_BUILD = $$(ARD_HOME_BUILD)
ARD_BIG = $$(ARD_BIG)
ARD_BETA = $$(ARD_BETA)
ARD_GD = $$(ARD_GD)
ARD_X64 = $$(ARD_X64)
ARD_CHROME = $$(ARD_CHROME)
ARD_AUTOTEST=$$(ARD_AUTOTEST)
ARD_OPENSSL=$$(ARD_OPENSSL)
ARD_TDA=$$(ARD_TDA)
    
ARD_CONFIG_DESKTOP_MSG = ""
#ARD_GRAPHVIZ = $$(ARD_GRAPH)

QT += sql network xml widgets core
CONFIG += c++11
if(!isEmpty( ARD_CHROME )){
    QT     +=  webenginewidgets
}

if(!isEmpty( ARD_BIG )){
	ARD_CONFIG_DESKTOP_MSG = BIG
	DEFINES += ARD_BIG
}
else{
	ARD_CONFIG_DESKTOP_MSG = SMALL
}

if(!isEmpty( ARD_DEBUG )){
	  DEFINES += _DEBUG
	  CONFIG += debug
	  CONFIG -= release
	  ARD_CONFIG_MSG = "+DEBUG"

    DEFINES += _DEBUG
    unix {
#         DEFINES += _SQL_PROFILER
         QMAKE_CXXFLAGS += -O0
	 QMAKE_LFLAGS += -g -O0
#         QMAKE_CXXFLAGS += -O0 -rdynamic
#	 QMAKE_LFLAGS += -g -O0 -rdynamic
    }
    win32-msvc2010 {
    	 QMAKE_CXXFLAGS += /Od
    }
}

if(isEmpty( ARD_DEBUG )){
	CONFIG -= debug
	CONFIG += release

    if(isEmpty(ARD_BETA)){
        ARD_CONFIG_MSG = "RELEASE"
    }
}

    
if(!isEmpty( ARD_PROFILE )){
    QMAKE_LFLAGS += -pg
}
ARD_CONFIG_GDRIVE_MSG = "-gdrive"
ARD_CONFIG_HOME_BUILD_MSG = "-home"
ARD_CONFIG_CHROME_MSG = "-chrome"
ARD_CONFIG_BETA_MSG = ""
ARD_CONFIG_WIN64_MSG = ""
ARD_CONFIG_OPENSSL_MSG = "-openssl"
ARD_CONFIG_TDA_MSG = "-tda"
ARD_QT_VER = "QT"$$QT_MAJOR_VERSION

if(!isEmpty( ARD_TDA )){
    ARD_CONFIG_TDA_MSG = "+tda"
    DEFINES += ARD_TDA
}


unix:!macx:release {
  LIB3RD=../../../3rd-party/LIB3RD/unix
}
unix:!macx:debug {
  LIB3RD=../../../3rd-party/LIB3RD/unix-debug
}
macx:debug{
  LIB3RD=../../../3rd-party/LIB3RD/osx-debug
}
macx:release{
  LIB3RD=../../../3rd-party/LIB3RD/osx-release
}

win32-msvc2010:debug{
 LIB3RD=../../../3rd-party/LIB3RD/win32-2010-debug
}
win32-msvc2010:release{
 LIB3RD=../../../3rd-party/LIB3RD/win32-2010-release
}
win32-msvc2013:debug{
 LIB3RD=../../../3rd-party/LIB3RD/win32-2013-debug
}
win32-msvc2013:release{
 LIB3RD=../../../3rd-party/LIB3RD/win32-2013-release
}

win32-msvc2015:debug{
 if(!isEmpty( ARD_X64 )){
     ARD_CONFIG_WIN64_MSG = "(win64)"
     LIB3RD=../../../3rd-party/LIB3RD/win64-2015-debug
 }
 else{
     LIB3RD=../../../3rd-party/LIB3RD/win32-2015-debug
 }
}
win32-msvc2015:release{
# QMAKE_CXXFLAGS += /wd4267
 if(!isEmpty( ARD_X64 )){
     ARD_CONFIG_WIN64_MSG = "(win64)"
     LIB3RD=../../../3rd-party/LIB3RD/win64-2015-release
 }
 else{
     LIB3RD=../../../3rd-party/LIB3RD/win32-2015-release
 }
}

win32-msvc:debug{
 if(!isEmpty( ARD_X64 )){
     ARD_CONFIG_WIN64_MSG = "(win64)"
     LIB3RD=../../../3rd-party/LIB3RD/2019-debug
 }
 else{
     LIB3RD=../../../3rd-party/LIB3RD/win32-2015-debug
 }
}



win32-g++:debug{
 LIB3RD=../../../3rd-party/LIB3RD/mingw-debug
}
win32-g++:release{
 LIB3RD=../../../3rd-party/LIB3RD/mingw-release
}
android:debug{
 LIB3RD=../../../3rd-party/LIB3RD/android-armeabi-v7a-debug
}
android:release{
 LIB3RD=../../../3rd-party/LIB3RD/android-armeabi-v7a-release
}

android{
	QT += androidextras
}

macx|ios{
 QMAKE_CXXFLAGS += -Winconsistent-missing-override
# -Wno-inconsistent-missing-override
}

ios{
 QMAKE_IOS_DEPLOYMENT_TARGET = 12.0
 QMAKE_APPLE_TARGETED_DEVICE_FAMILY = 1,2
  debug{
	LIB3RD=../../../3rd-party/LIB3RD/ios-emu-clang-debug
  }
iphoneos{
  release{
	LIB3RD=../../../3rd-party/LIB3RD/ios-static-release
  }
}

}

LIB3RD_QTBASED = $$LIB3RD/$$ARD_QT_VER

if(!isEmpty( ARD_HOME_BUILD )){
 DEFINES += ARD_HOME_BUILD
 ARD_CONFIG_HOME_BUILD_MSG = "+home"
}

if(!isEmpty( ARD_GD )){
     DEFINES += ARD_GD
     INCLUDEPATH += ../../../3rd-party/googleQt/src
     ARD_CONFIG_GDRIVE_MSG = "+gdive"
}

if(!isEmpty( ARD_CHROME )){
     DEFINES += ARD_CHROME
     ARD_CONFIG_CHROME_MSG = "+chrome"
}

if(!isEmpty( ARD_OPENSSL )){
     DEFINES += ARD_OPENSSL
     ARD_CONFIG_OPENSSL_MSG = "+openssl"
     !iphoneos
	 {     
         INCLUDEPATH += ../../../3rd-party/openssl/install/include
     }
     iphoneos
	 {     
         INCLUDEPATH += ../../../3rd-party/openssl/dist-ios/arm64/include
     }     
}


win32 {
      DEFINES += _CRT_SECURE_NO_WARNINGS
}

if(!isEmpty(ARD_BETA)){
    DEFINES += ARD_BETA
    ARD_CONFIG_BETA_MSG = "BETA"
    win32 {
#this will enable debug symbols in release build
    	  QMAKE_CXXFLAGS += /Zi
	  QMAKE_LFLAGS += /INCREMENTAL:NO
	  QMAKE_LFLAGS += /DEBUG
	  QMAKE_LFLAGS += /OPT:REF
	  QMAKE_LFLAGS += /OPT:ICF
    }
}

if(!isEmpty( ARD_AUTOTEST )){
    DEFINES += API_QT_AUTOTEST
}

!build_pass:message($$ARD_QT_VER $$QMAKESPEC $$ARD_CONFIG_DESKTOP_MSG $$ARD_CONFIG_BETA_MSG $$ARD_CONFIG_HOME_BUILD_MSG $$ARD_CONFIG_MSG $$ARD_CONFIG_CHROME_MSG  $$ARD_CONFIG_GDRIVE_MSG $$ARD_CONFIG_WIN64_MSG $$DESTDIR $$OBJECTS_DIR $$ARD_CONFIG_OPENSSL_MSG $$ARD_CONFIG_TDA_MSG))


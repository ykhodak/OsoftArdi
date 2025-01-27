include( ../gvz-ard-common.pro )
INCLUDEPATH += $${CDT_DIR}
SOURCES += $${CDT_SOURCES}

win32 {
#      DEFINES += __EXPORT__
}

include( ../print-config.pro )

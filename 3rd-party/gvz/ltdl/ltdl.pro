include( ../gvz-ard-common.pro )
INCLUDEPATH += $${LTDL_DIR} $${LTDL_DIR}/libltdl $${GVC_DIR}
SOURCES += $${LTDL_SOURCES}
SOURCES += ../src/gvz-resolve.c

DEFINES += LTDLOPEN=libltdlc
DEFINES += LTDL
include( ../print-config.pro )

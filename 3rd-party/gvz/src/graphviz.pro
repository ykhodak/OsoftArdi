
win32{
	include( ../gvz-ard-common.pro )
	INCLUDEPATH = $${GVZ_INSTALL_PATH}/include/graphviz
	SOURCES += builtins.c $${GVC_DIR}/regex_win32.c
	CONFIG -= staticlib
	CONFIG += shared
	LIBS += -L../lib -lcommon -lcgraph -lcdt  -lgvc  -lxdot -lpack -lortho -llabel -lpathplan -ldotgen -lsfdpgen -lfdpgen -ltwopigen -lneatogen -lcircogen -losage -lsparse -lpatchwork -lgvplugin_core -lgvplugin_dot_layout -lgvplugin_neato_layout
	include( ../print-config.pro )

	QMAKE_LFLAGS += /DEF:"graphviz.def"
}

unix{
	include( ../gvz-ard-common.pro )
	INCLUDEPATH = $${GVZ_INSTALL_PATH}/include/graphviz
	SOURCES += builtins.c
	CONFIG += staticlib
	CONFIG -= shared
	TARGET = bogus
	include( ../print-config.pro )

}

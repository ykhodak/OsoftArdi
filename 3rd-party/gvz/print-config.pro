#if(!isEmpty( ARD_LOCAL_BUILD )){
#	     message( "LOCAL BUILD: CXXFLAGS:" $${QMAKE_CXXFLAGS} " CFLAGS:" $${QMAKE_CFLAGS} "CONFIG:" $${CONFIG})
#}
#else{
#	     message( "FINAL BUILD: CXXFLAGS:" $${QMAKE_CXXFLAGS} " CFLAGS:" $${QMAKE_CFLAGS} "CONFIG:" $${CONFIG})
#}

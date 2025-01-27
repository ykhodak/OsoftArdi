#!/bin/csh

source ./gvz.envs

if ( ! $?GVZ_CODE ) then
    echo "ERROR required environment variable GVZ_CODE, see README"
    exit 0
endif

if ( ! $?GVZ_INSTALL_PATH ) then
    echo "ERROR required environment variable GVZ_INSTALL_PATH, see README"
    exit 0
endif


if ( ! -d $GVZ_INSTALL_PATH ) then
    echo "ERROR graphviz install directory not found or not configured:"$GVZ_INSTALL_PATH
    exit 0
endif


set sname = `uname -s| tr "[a-z]" "[A-Z]"|awk '{print substr($0,0,6)}'`
set GVZ_INSTALL_INCLUDE = $GVZ_INSTALL_PATH

if($sname == "DARWIN")then
    set deploy_prefix = "osx"
    set glib="libgraphviz.dylib"
else
    set deploy_prefix = "unix"
    set glib="libgraphviz.so"
endif

set static_glib=libgraphviz-static.a

set liblist=`echo $ARD_GRAPHVIZ_LIB_DIRECTORIES:q | sed 's/,/ /g'`

foreach d($liblist)
#echo "============ " $d " ================="
echo "CC     " $d
cd $d
qmake
make -j 2
cd .. 
end


set pglib=./lib/$glib
set pstatic_glib=./lib/$static_glib

if ( -e $pglib ) then
    \rm $pglib
endif

if($sname == "DARWIN")then
    echo compiling $pglib ..

gcc -dynamiclib -O2 -fPIC -Wl,-all_load ./lib/libcgraph.a ./lib/libgvc.a ./lib/libcdt.a ./lib/libxdot.a ./lib/libpathplan.a  -L./lib -lcommon  -lpack -lortho -llabel  -ldotgen -lsfdpgen -lfdpgen -ltwopigen -lneatogen -lcircogen -losage -lsparse -lpatchwork -lgvplugin_core -lgvplugin_dot_layout -lgvplugin_neato_layout -Wl,-noall_load -I$GVZ_INSTALL_INCLUDE/include/graphviz -o ./lib/libgraphviz.dylib ./src/builtins.c -Wl,-install_name,libgraphviz.dylib

else
    gcc -shared -O2 -fPIC -Wl,--whole-archive -I$GVZ_INSTALL_INCLUDE/include/graphviz -L./lib -lcdt -lcgraph -lgvc -lcommon -lxdot -lpack -lortho -llabel -lpathplan -ldotgen -lsfdpgen -lfdpgen -ltwopigen -lneatogen -lcircogen -losage -lsparse -lpatchwork -lgvplugin_core -lgvplugin_dot_layout -lgvplugin_neato_layout -Wl,--no-whole-archive -o $pglib ./src/builtins.c

echo "=================================="
echo "creating static library.."
ar cqs $pstatic_glib ./common/__tmp/*.o ./cgraph/__tmp/*.o ./cdt/__tmp/*.o ./gvc/__tmp/*.o ./xdot/__tmp/*.o ./pack/__tmp/*.o ./ortho/__tmp/*.o ./label/__tmp/*.o ./pathplan/__tmp/*.o ./dotgen/__tmp/*.o ./sfdpgen/__tmp/*.o ./fdpgen/__tmp/*.o ./twopigen/__tmp/*.o ./neatogen/__tmp/*.o ./circogen/__tmp/*.o ./osage/__tmp/*.o ./sparse/__tmp/*.o ./patchwork/__tmp/*.o ./gvplugin_core/__tmp/*.o ./gvplugin_dot_layout/__tmp/*.o ./gvplugin_neato_layout/__tmp/*.o ./src/__tmp/*.o
endif

echo ""
echo ""
echo ""
echo "checking libraries.."
echo "=================================="

foreach l($liblist)
    set fname="lib"$l".a"
    set pname="./lib/$fname"
    if (-e $pname ) then
	set fsize=`ls -lah $pname | awk '{print $5}'`
	printf "OK\t$fsize\t$fname\n"
    else
	switch ($fname)
	case "libsrc.a":
	breaksw
	default:
	    echo "ERROR    "$fname
	    goto errorexit
	endsw
    endif
end

echo "=================================="

if( -e $pglib ) then
     set fsize=`ls -lah $pglib | awk '{print $5}'`
     printf "OK\t$fsize\t$glib\n"
else
    echo "ERROR    "$pglib
    goto errorexit
endif

if( -e $pstatic_glib ) then
     set fsize=`ls -lah $pstatic_glib | awk '{print $5}'`
     printf "OK\t$fsize\t$static_glib\n"
else
    echo "ERROR    "$pstatic_glib
    goto errorexit
endif


echo "=================================="


echo ""
echo "FINISHED"
exit 0
errorexit:
echo "ERROR..."
exit -1

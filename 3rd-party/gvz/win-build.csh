#!/bin/csh

set sname = `uname -s| tr "[a-z]" "[A-Z]"|awk '{print substr($0,0,6)}'`
echo "under:"$sname

if($sname == "CYGWIN")then
    echo "unsetting tmp temp environments, otherwise VC compiler is getting confused by duplicates - tmp & TMP etc.."
    unsetenv tmp
    unsetenv temp
else
    echo "this script should be started from Windows/Cygwin only";
    exit 1;
endif

if($?ARD_LOCAL_BUILD) then
    set deploy_prefix = "wind"
    set prjconfig = "Debug|Win32"
    set sln_name = "Debug"
else
    set deploy_prefix = "win"
    set prjconfig = "Release|Win32"
    set sln_name = "Release"
endif

source ./gvz.envs
set liblist=`echo $ARD_GRAPHVIZ_LIB_DIRECTORIES:q | sed 's/,/ /g'`

set custom_config_h = `cat $GVZ_CODE/config.h | grep ykh | wc -l`
echo "found marked customized records in config.h: " $custom_config_h
if($custom_config_h == 0)then
    echo "========== now this is gona be tricky ======================"
    echo "required cutomization of graphviz/config.h"
    echo "the config.h generated in unix environment needs further customization"
    echo "take config.h from ./src/ or merge with it graphviz/config.h"
    echo "PS. config.h provided with graphviz distribution for Win is also not good."
    echo "RUN THIS FIRST: cp ./src/config-2be-placed-in-graphviz-code.h $GVZ_CODE/config.h"
    echo "============================================================"
    echo "after this you will get compilation error." 
    echo "1. in file src/utils-customized4graphviz.c"
    echo "   search for ykh apply modify original graphviz-x.xx.x/lib/common/utils.c accordinly"
    echo "2. search in graphviz-x.xx.x/lib for *.c and comment out all #pragma comment( lib, \"xxx\");"
    echo " - ./lib/gvc/gvc.c"
    echo " - ./plugin/dot_layout/gvlayout_dot_layout.c"
    echo " - ./plugin/neato_layout/gvlayout_neato_layout.c"
    echo "3. in file geomprocs.h comment out sections for 'extern' definition "
    echo "4. in file cdt.h comment out section starting with _BEGIN_EXTERNS_ (before Dttree)"
    echo "============================================================"
    exit 1
endif
echo "proceeding with customized (for Windows) config.h "

foreach d($liblist)
echo "============ " $d " ================="
cd $d
qmake -tp vc
cd .. 
#devenv.exe $slnfile /build $sln_name /project $d /projectconfig $prjconfig
end


echo "============ " graphviz " ================="
cd src
qmake -tp vc graphviz.pro
cd .. 

set slnfile = graphviz.sln
echo "opening " $slnfile
cygstart $slnfile

#!/bin/csh

source ./gvz.envs
set liblist=`echo $ARD_GRAPHVIZ_LIB_DIRECTORIES:q | sed 's/,/ /g'`

set sname = `uname -s| tr "[a-z]" "[A-Z]"|awk '{print substr($0,0,6)}'`
echo "under:"$sname


if($sname == "CYGWIN")then
    echo "skipping make clean under Windows.."
else
    foreach d($liblist)
    echo "============ " $d " ================="
    cd $d
    qmake
    make clean
    cd .. 
    end
endif

foreach p("debug" "release" "__tmp" "lib")
    set ftemp=$p
    echo  $p `find . -type d -name "$ftemp" | wc -l`
    find . -type d -name "$ftemp" -exec rm -r {} \;
end


echo "cleaning tmp-files.."
foreach p("pdb" "o" "a" "suo" "ilk" "res" "exe" "sdf" "pro.user" "vcxproj" "vcxproj.filters" "vcxproj.user")
    set ftemp="*."$p
    echo  $p `find . -type f -name "$ftemp" | wc -l`
    find . -type f -name "$ftemp" -exec rm {} \;
end

foreach p("Makefile" "core")
    set ftemp=$p"*"
    echo $p `find . -type f -name "$ftemp" | wc -l`
    find . -type f -name "$ftemp" -exec rm {} \;
end

\rm -rf build-*Android_*

#!/bin/csh

set sname = `uname -s| tr "[a-z]" "[A-Z]"|awk '{print substr($0,0,6)}'`
#echo "under:"$sname

switch ($sname)
case "CYGWIN":
    ./bin/win.bat
    exit 1
#    breaksw
endsw


set QMAKE=qmake
set MAKE=make
set MAKE_FLAGS="-j 4"
set osname=`uname`
set qtver=`qmake -v | grep Using | awk '{print $4}'`

    
set shadowPrefix="Release"
set debugORrelease="release"    
if($?ARD_DEBUG) then
        set shadowPrefix="Debug"
        set debugORrelease="debug"
else
        set shadowPrefix="Release"
        set debugORrelease="release"
endif
set shadowBuild="build-ariadne-Desktop-"$qtver$shadowPrefix
echo "building for " $osname " on shadow:" $shadowBuild
echo
    
echo osname is $osname
    # when doing intense work on googleQt uncomment
    # block below to auto update static lib in 3rd-parties
switch ($osname)
case "Darwin":
    echo "upgrading googleQt from:" ../3rd-party/googleQt/prj/libgoogleQt.a
        cp -v ../3rd-party/googleQt/prj/libgoogleQt.a ../3rd-party/LIB3RD/osx-$debugORrelease/QT5/
        breaksw
case "Linux":
    echo "upgrading googleQt from:" ../3rd-party/googleQt/prj/libgoogleQt.a
        cp -v ../3rd-party/googleQt/prj/libgoogleQt.a ../3rd-party/LIB3RD/unix-$debugORrelease/QT5/
        breaksw
endsw        


echo
echo
touch gui/src/AAA.cpp    
    
set blist=(snc dbp exg gui)
foreach b($blist)
    echo "============ building $b.. ========================"
#    cd $b/src/
    #    $QMAKE
    cd $b
    mkdir $shadowBuild
    cd $shadowBuild/
    $QMAKE ../src/
    $MAKE $MAKE_FLAGS	
    #../src/$MAKE $MAKE_FLAGS
    echo "============ finished building $b.. ==============="
    cd ../../
end


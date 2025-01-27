#!/usr/bin/bash

sname=`uname -s| tr "[a-z]" "[A-Z]"|awk '{print substr($0,0,6)}'`

IFS=,

if [ $sname = MINGW6 ]; then
    echo "running windows build"
    ./bin/win.bat
    exit 1
fi
echo "running unix-based build"

QMAKE=qmake
MAKE=make
MAKE_FLAGS="-j 4"
osname=`uname`
qtver=`qmake -v | grep Using | awk '{print $4}'`

    
shadowPrefix="Release"
debugORrelease="release"    
if($?ARD_DEBUG) then
        shadowPrefix="Debug"
        debugORrelease="debug"
else
        shadowPrefix="Release"
        debugORrelease="release"
endif
shadowBuild="build-ariadne-Desktop-"$qtver$shadowPrefix
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
    
blist="snc,dbp,exg,gui"
#foreach b($blist)
for b in $blist
do
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
done


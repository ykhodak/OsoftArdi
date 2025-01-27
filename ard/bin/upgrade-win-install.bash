#/usr/bin/bash

export SHELLOPTS
set -o igncr
    
#
# update-win-qt-install.bash - script to update win install folder - it's
# folder used to create msi-installation files. The script will copy Qt-dll
# files from Qt-distribution dir, usefull during upgrade to new Qt version
#
# 'deploy_root' - set path with files structure that gets packaged
# with install builder. Script requires 'qmake' in search path
# qmake must be built with same version of QT
# The depending dlls should be in path (so binary can start with '-v'), 
# this includes Qt dlls and 3rd-party/LIB3RD folder
#


deploy_root=/cygdrive/c/projects/ariadne/win7-install/vc-2015-files
#exe_path=$deploy_root"/Ardi.exe"
exe_path=../gui/src/release/Ardi.exe
qtver=""
if [ -f $exe_path ]
then
    qtver=`$exe_path -v | grep qt | cut -d ":" -f 2`    
    cfg=$(echo `$exe_path -v | grep config | cut -d ":" -f 2| tr -d '\n'` |tr -d '\n')
    if [ "$cfg" == "release" ]
    then
       echo "Processing " $cfg
    else
       echo "ERROR, not configured/built for 'release' at: "$exe_path"|"$cfg"|"
       $exe_path -v
       exit    
    fi
else
    echo "ERROR, executable not found at: "$exe_path
    exit
fi

source_root=""
if [ -f $exe_path ]
then
    source_root=`which qmake`
    source_root=`dirname $source_root`
    source_root=`dirname $source_root`
    qm_ver=`qmake -v | grep "Qt version"|cut -d " " -f 4`
else
    echo "ERROR, qmake not found, did you forget to source env?"
    exit    
fi

if [ "$qtver" == "$qm_ver" ]
then
    ardDate=`$exe_path -v | grep "date"|cut -d ":" -f 2`
    echo "date: "$ardDate
    echo "ArdiQtVer: "$qtver" QMakeVer: "$qm_ver" source: "$source_root
    echo "deploy="$deploy_root
    echo ""
else
    echo "ardQtVer="$qtver" QtQmakeVer="$qm_ver" qm="$source_root
    echo "ERROR, qmake is different version than binary built, exiting."
    exit
fi

cd $deploy_root
echo "----------------------------"
runlist(){
	for f in $(find -name "*.dll" -or -name "*.dat" -or -name "*.exe" -or -name "*.pak")
	do
        fbase=`basename $f`
        if [ "$fbase" == "Ardi.exe" ] || [ "$fbase" == "graphviz.dll" ] || [ "$fbase" == "libeay32.dll" ] || [ "$fbase" == "ssleay32.dll" ]
        then
            continue
        fi
		sf=`find $source_root -name $fbase`
		if [ -z "$sf" ]		
		then
			echo "NOT found in QT source: $fbase"
		else
			src_size=`du -k $sf | cut -f 1`
			local_size=`du -k $f | cut -f 1`
			if [ $copy_confirm = true ]
			then
                cp --verbose -f $sf $f
			else
				echo "located: $sf $src_size -> $local_size"
			fi
        fi
	done
}
    
copy_confirm=false
runlist

echo "----------------------------"
echo "dest= "`pwd`
echo "source= "$source_root
echo "Update? [YES/NO]:" 
read user_confirm

if [ "$user_confirm" == "YES" ] 
then
	echo "update started"
	copy_confirm=true
	runlist
    cd -
    \cp -f $exe_path $deploy_root
else
	echo "cancelled"
fi




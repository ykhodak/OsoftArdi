#!/usr/bin/bash


if [ -f m ];then
    echo "folder may contain old update package - file 'm'"
    echo "delete the file first"
    exit 1
fi

IFS=,

flist="x64,debug,release,__tmp,lib,_a-docs,iphoneos,iphonesimulator,Debug,Release-iphoneos,Release-iphonesimulator,Ardi.xcodeproj,project.xcworkspace,xcshareddata"

### delete specified folders
for p in $flist
do
    ftemp=$p
    echo  $p `find . -type d -name "$ftemp" | wc -l`
    find . -type d -name "$ftemp" -exec rm -r {} \;
done

flist="snc,dbp,exg,ariadne"

### delete shadow build-folders
for p in $flist
do
    ftemp="build-"$p"-*"
    echo "directory:"  $p  `find . -type d -name "$ftemp" | wc -l`
    find . -type d -name "$ftemp" -exec rm -r {} \;
done

find . -type d -name "build-ardi-*" -exec rm -r {} \;
    
### delete specified files by extention
echo "cleaning tmp-files.."
flist="pdb,o,a,lib,so,ilk,res,exe,sdf,pro.user,vcxproj,vcxproj.user,vcxproj.filters,so-deployment-settings.json,qmake.stash,DS_Store,log,mak,pbxproj,plist"
for p in $flist
do
    ftemp="*."$p
    echo  $p `find . -type f -name "$ftemp" | wc -l`
    find . -type f -name "$ftemp" -exec rm {} \;
done

flist="Ardi.VC.db,Makefile,core,a-docs.tar.gz,._,._*cpp"
### delete specified files by name
for p in $flist
do
    ftemp=$p"*"
    echo $p `find . -type f -name "$ftemp" | wc -l`
    find . -type f -name "$ftemp" -exec rm {} \;
done

find . -type f -name "._*.cpp" -exec rm {} \;


echo "qrc_.....-res.cpp" `find . -type f -name "qrc_*-res.cpp" | wc -l`
find . -type f -name "qrc_*-res.cpp" -exec rm {} \;




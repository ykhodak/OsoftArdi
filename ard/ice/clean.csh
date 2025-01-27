#!/bin/csh


if(-e m)then
    echo "folder may contain old update package - file 'm'"
    echo "delete the file first"
    exit 1
endif

#"project.xcworkspace" "AriadneOrganizer.xcodeproj" "xcshareddata" "Release-iphoneos"
    
### delete specified folders
foreach p("x64" "debug" "release" "__tmp" "lib" "_a-docs" "iphoneos" "iphonesimulator" "Debug" "Release-iphoneos" "Release-iphonesimulator" "Ardi.xcodeproj" "project.xcworkspace" "xcshareddata")
    set ftemp=$p
    echo  $p `find . -type d -name "$ftemp" | wc -l`
    find . -type d -name "$ftemp" -exec rm -r {} \;
end

### delete shadow build-folders
foreach p("snc" "dbp" "exg" "ariadne")
    set ftemp="build-"$p"-*"
    echo "directory:"  $p  `find . -type d -name "$ftemp" | wc -l`
    find . -type d -name "$ftemp" -exec rm -r {} \;
end
find . -type d -name "build-ardi-*" -exec rm -r {} \;

    
### delete specified files by extention
echo "cleaning tmp-files.."
foreach p("pdb" "o" "a" "lib" "so" "ilk" "res" "exe" "sdf" "pro.user" "vcxproj" "vcxproj.user" "vcxproj.filters" "so-deployment-settings.json" "qmake.stash" "DS_Store" "log" "mak" "pbxproj" "plist")
    set ftemp="*."$p
    echo  $p `find . -type f -name "$ftemp" | wc -l`
    find . -type f -name "$ftemp" -exec rm {} \;
end

### delete specified files by name
foreach p("Ardi.VC.db" "Makefile" "core" "a-docs.tar.gz" "._" "._*cpp")
    set ftemp=$p"*"
    echo $p `find . -type f -name "$ftemp" | wc -l`
    find . -type f -name "$ftemp" -exec rm {} \;
end

find . -type f -name "._*.cpp" -exec rm {} \;


echo "qrc_.....-res.cpp" `find . -type f -name "qrc_*-res.cpp" | wc -l`
find . -type f -name "qrc_*-res.cpp" -exec rm {} \;




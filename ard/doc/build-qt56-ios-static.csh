#!/bin/csh


set cmd="./configure -xplatform macx-ios-clang -release -static -opensource -confirm-license -prefix $HOME/Qt/5.6-static"
set fmake="-qt-sql-sqlite -qt-zlib -qt-libpng -qt-libjpeg"
#set fnomake="-nomake examples -no-exceptions -no-qt3support -no-scripttools -no-webkit -no-phonon -no-style-motif -no-style-cde -no-style-cleanlooks -no-style-plastique"

#echo $cmd" "$fmake" "$fnomake

echo $cmd" "$fmake

    

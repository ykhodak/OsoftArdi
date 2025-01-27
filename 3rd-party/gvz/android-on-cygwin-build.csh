#!/bin/csh

set AR=/cygdrive/c/ykh/android/android-ndk-r8e/toolchains/arm-linux-androideabi-4.7/prebuilt/windows/bin/arm-linux-androideabi-ar
set static_glib=libgraphviz-static.a
set pstatic_glib=./lib/$static_glib

set DebugOrRelease=Debug
set AP=./build-
set AS=-Android_for_armeabi_v7a_GCC_4_7_Qt_5_2_0-$DebugOrRelease


$AR cqs $pstatic_glib $APcommon$AS/*.o $APcgraph$AS/*.o $APcdt$AS/*.o $APgvc$AS/*.o $APxdot$AS/*.o $APpack$AS/*.o $APortho$AS/*.o $APlabel$AS/*.o $APpathplan$AS/*.o $APdotgen$AS/*.o $APsfdpgen$AS/*.o $APfdpgen$AS/*.o $APtwopigen$AS/*.o $APneatogen$AS/*.o $APcircogen$AS/*.o $APosage$AS/*.o $APsparse$AS/*.o $APpatchwork$AS/*.o $APgvplugin_core$AS/*.o $APgvplugin_dot_layout$AS/*.o $APgvplugin_neato_layout$AS/*.o $APsrc$AS/*.o


cd build-graphviz-Android_for_armeabi_v7a_GCC_4_7_Qt_5_2_0-Debug


C:\ykh\android\android-ndk-r8e/toolchains/arm-linux-androideabi-4.7/prebuilt/windows/bin/arm-linux-androideabi-g++ --sysroot=C:\ykh\android\android-ndk-r8e/platforms/android-14/arch-arm/ -g -O0 -rdynamic -Wl,--no-undefined -Wl,-z,noexecstack -shared  __tmp\builtins.obj  -LC:\ykh\android\android-ndk-r8e/sources/cxx-stl/gnu-libstdc++/4.7/libs/armeabi-v7a -LC:\ykh\android\android-ndk-r8e/platforms/android-14/arch-arm//usr/lib -LC:\Qt\Qt5.2.0\5.2.0\android_armv7\lib -lQt5Gui -Lc:\Utils\android\ndk/sources/cxx-stl/gnu-libstdc++/4.8/libs/armeabi-v7a -Lc:\Utils\android\ndk/platforms/android-9/arch-arm//usr/lib -LC:\Utils\icu32_51_1_mingw48\lib -LC:\utils\postgresql\pgsql\lib -LC:\utils\mysql\mysql\lib -LC:\Utils\pgsql\lib -LC:\temp\opensll-android-master\openssl-android-master\lib -LC:\Qt\Qt5.2.0\5.2.0\android_armv7/lib -lQt5Core -lGLESv2 -lgnustl_shared -llog -lz -lm -ldl -lc -lgcc -Wl,--whole-archive  -L../lib -lcgraph -lcommon -lcdt -lgvc -lxdot -lpack -lortho -llabel -lpathplan -ldotgen -lsfdpgen -lfdpgen -ltwopigen -lneatogen -lcircogen -losage -lsparse -lpatchwork -lgvplugin_core -lgvplugin_dot_layout -lgvplugin_neato_layout -Wl,--no-whole-archive  -o libgraphviz.so

cd ..

echo "========================================"
echo "building static library"

set AR=C:\ykh\android\android-ndk-r8e/toolchains/arm-linux-androideabi-4.7/prebuilt/windows/bin/arm-linux-androideabi-ar
set static_glib=libgraphviz-static.a
set pstatic_glib=./lib/%static_glib%

set DebugOrRelease=Debug
set AP=./build-
set AS=-Android_for_armeabi_v7a_GCC_4_7_Qt_5_2_0-%DebugOrRelease%


%AR% cqs %pstatic_glib% %AP%common%AS%/*.o %AP%cgraph%AS%/*.o %AP%cdt%AS%/*.o %AP%gvc%AS%/*.o %AP%xdot%AS%/*.o %AP%pack%AS%/*.o %AP%ortho%AS%/*.o %AP%label%AS%/*.o %AP%pathplan%AS%/*.o %AP%dotgen%AS%/*.o %AP%sfdpgen%AS%/*.o %AP%fdpgen%AS%/*.o %AP%twopigen%AS%/*.o %AP%neatogen%AS%/*.o %AP%circogen%AS%/*.o %AP%osage%AS%/*.o %AP%sparse%AS%/*.o %AP%patchwork%AS%/*.o %AP%gvplugin_core%AS%/*.o %AP%gvplugin_dot_layout%AS%/*.o %AP%gvplugin_neato_layout%AS%/*.o %AP%src%AS%/*.o

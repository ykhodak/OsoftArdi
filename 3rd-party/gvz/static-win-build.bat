echo "========================================"
echo "building static library"

set AR=lib
set static_glib=graphviz-static.lib
set pstatic_glib=./lib/%static_glib%

REM set DebugOrRelease=Debug
set AP=./
set AS=/__tmp/*.obj

%AR% %AP%common%AS% %AP%cgraph%AS% %AP%cdt%AS% %AP%gvc%AS% %AP%xdot%AS% %AP%pack%AS% %AP%ortho%AS% %AP%label%AS% %AP%pathplan%AS% %AP%dotgen%AS% %AP%sfdpgen%AS% %AP%fdpgen%AS% %AP%twopigen%AS% %AP%neatogen%AS% %AP%circogen%AS% %AP%osage%AS% %AP%sparse%AS% %AP%patchwork%AS% %AP%gvplugin_core%AS% %AP%gvplugin_dot_layout%AS% %AP%gvplugin_neato_layout%AS% %AP%src%AS% /OUT:%pstatic_glib%


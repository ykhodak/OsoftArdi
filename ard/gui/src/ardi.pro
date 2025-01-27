include( ../../ard.pro )

######################################################################
TEMPLATE = app

ARD_HOME_BUILD = $$(ARD_HOME_BUILD)

INC = ../include

DEPENDPATH += . $${INC} ../../snc/include ../../dbp/include ../../exg/include
INCLUDEPATH += $${INC} ../../snc/include ../../dbp/include ../../exg/include
CONFIG += qt
QT     += gui xml

QT += concurrent

if(!isEmpty( ARD_X64 )){
  LIBS += -L../../../x64/Debug -lsnc -ldbp -lexg
}else{
  LIBS += -L../../snc/lib -lsnc
  LIBS += -L../../dbp/lib -ldbp
  LIBS += -L../../exg/lib -lexg
}

    
HEADERS += $$files($${INC}/*.h, false)
SOURCES += $$files(*.cpp)

    
if(!CONFIG(static)){
        SOURCES -= ariadneorganizer_plugin_import.cpp
}

RESOURCES += ardi-res.qrc

if(!isEmpty( ARD_GD )){
 LIBS += -L$$LIB3RD_QTBASED -lgoogleQt
}

if(!isEmpty( ARD_OPENSSL )){
win32{
   LIBS += ws2_32.lib crypt32.lib
   LIBS += -L$$LIB3RD_QTBASED -llibcrypto
}
unix{
   LIBS += -L$$LIB3RD_QTBASED -lcrypto
   !macx{
      LIBS += -ldl
   }
}
}


if(isEmpty( ARD_HOME_BUILD )){
 TARGET = testapp
}
else{
 TARGET = Ardi
}


win32-msvc2010 {
    QMAKE_CXXFLAGS += -EHsc
}

iphoneos{
  release{
#  message("ykh-iphoneos-static")
#    CONFIG += static
#    DEFINES += ARD_STATIC_BUILD
}
}

supplied.files = $$files($$PWD/../../../images/supplied/supplied.json)
QMAKE_BUNDLE_DATA += supplied

ios{
	assets_catalogs.files = $$files($$PWD/../../../images/ios/*.xcassets)
    	QMAKE_BUNDLE_DATA += assets_catalogs
	QMAKE_INFO_PLIST = $$PWD/../../../images/ios/Info.plist
}

win32{
    RC_FILE = ../../../images/win/ard-rc.rc
    INCLUDEPATH += ../../exg/win
}

OTHER_FILES += \
	    ../../images/unix/format-justify-right.png \
	    ../../images/unix/format-justify-center.png \
	    ../../images/unix/format-justify-left.png \
            ../../images/unix/format-justify-fill \
	    ../../images/unix/format-text-underline.png \
	    ../../images/unix/format-text-italic.png \
	    ../../images/unix/format-text-bold.png \
	    ../../images/unix/insert-image.png \
	    ../../images/unix/go-previous.png \
	    ../../images/unix/go-next.png \
	    ../../images/unix/edit-select-all.png \
	    ../../images/unix/edit-paste.png \
	    ../../images/unix/edit-paste-plain.png \    
	    ../../images/unix/edit-delete.png \
	    ../../images/unix/edit-cut.png \
	    ../../images/unix/edit-copy.png \
	    ../../images/unix/edit-redo.png \
	    ../../images/unix/edit-undo.png \
	    ../../images/unix/document-open.png \
	    ../../images/unix/document-new.png \
            ../../images/unix/window-close.png \
	    ../../images/unix/folder.png \
	    ../../images/unix/outline-bulb.png \
	    ../../images/unix/add.png \
	    ../../images/unix/add-tab.png \    
        ../../images/unix/remove.png \
	    ../../images/unix/checkbox_checked.png \
	    ../../images/unix/checkbox_unchecked.png \
	    ../../images/unix/pazzle.png \
	    ../../images/unix/wait-blue.png \
	    ../../images/unix/unplugged.png \
	    ../../images/unix/tab-slide-on.png \    
	    ../../images/unix/tab-slide-off.png \    
	    ../../images/unix/2dots.png \
	    ../../images/unix/3dots.png \
	    ../../images/unix/check.png \
	    ../../images/unix/check-black.png \
        ../../images/unix/check-gray.png \    
        ../../images/unix/check-white.png \
        ../../images/unix/uncheck-white.png \
        ../../images/unix/check2select.png \    
	    ../../images/unix/red-exclamation.png \
	    ../../images/unix/exclamation-in-circle.png \
	    ../../images/unix/settings1.png \
	    ../../images/unix/pen-lime.png \
	    ../../images/unix/find.png \    
	    ../../images/unix/search-filter.png \
	    ../../images/unix/search-view-gray.png \    
	    ../../images/unix/locate.png \
	    ../../images/unix/dots.png \
	    ../../images/unix/dots-v.png \    
	    ../../images/unix/select-config.png \
	    ../../images/unix/select-dots.png \
	    ../../images/unix/select-image.png \
	    ../../images/unix/select-note.png \
	    ../../images/unix/icon-bookmark.png \
	    ../../images/unix/icon-dots.png \
	    ../../images/unix/icon-synchronize.png \
	    ../../images/unix/icon-topic-closed.png \
	    ../../images/unix/icon-topic-open.png \
	    ../../images/unix/graph-logo.png \
	    ../../images/unix/button-add.png \
            ../../images/unix/edit-find.png \
            ../../images/unix/folder-up.png \
            ../../images/unix/tab-pin.png \
            ../../images/unix/layout.png \
            ../../images/unix/movedown.png \
            ../../images/unix/moveup.png \
            ../../images/unix/moveleft.png \
            ../../images/unix/moveright.png \
            ../../images/unix/organize-gray.png \
            ../../images/unix/view.png \
            ../../images/unix/recycle.png \
            ../../images/unix/resizable.png \
            ../../images/unix/index-card.png \
            ../../images/unix/todo-view.png \
            ../../images/unix/box-rect.png \
            ../../images/unix/search-view.png \
            ../../images/unix/4tiles2.png \
            ../../images/unix/new-todo.png \
            ../../images/unix/refresh.png \
	    ../../images/unix/search-glass.png \
	    ../../images/unix/hud-br-down.png \
	    ../../images/unix/pencil.png \
	    ../../images/unix/chain.png \
	    ../../images/unix/note.png \
	    ../../images/unix/page.png \
	    ../../images/unix/sync.png \
	    ../../images/unix/add-table.png \
	    ../../images/unix/target-mark.png \
	    ../../images/unix/clock.png \
	    ../../images/unix/dollar.png \
	    ../../images/unix/percent.png \
	    ../../images/unix/up-32.png \
	    ../../images/unix/cloud.png \
	    ../../images/unix/font-x-generic.png \
	    ../../images/unix/show-tabs.png \
	    ../../images/unix/minus-collapse.png \
	    ../../images/unix/plus-expand.png \
	    ../../images/unix/hresource.png \
	    ../../images/unix/sort-down.png \
	    ../../images/unix/resource.png \
	    ../../images/unix/empty-box.png \
	    ../../images/unix/home-folder.png \
	    ../../images/unix/reference-folder.png \
	    ../../images/unix/kboard.png \
	    ../../images/unix/bkg-text.png \
	    ../../images/unix/font-size.png \
	    ../../images/unix/sortbox.png \
	    ../../images/unix/red-clock.png \
	    ../../images/unix/add-note.png \
	    ../../images/unix/format-date.png \
	    ../../images/unix/format-time.png \
	    ../../images/unix/box-out.png \
	    ../../images/unix/process-task.png \
	    ../../images/unix/search-filter.png \
	    ../../images/unix/trash-bin.png \
	    ../../images/unix/box-minus.png \
	    ../../images/unix/box-plus.png \
	    ../../images/unix/icon-single-project.png \
	    ../../images/unix/open_folder_gray.png \
	    ../../images/unix/folders-bundle.png \
	    ../../images/unix/shelf.png \
	    ../../images/unix/email-compose.png \    
	    ../../images/unix/email-forward.png \
	    ../../images/unix/email-new.png \
	    ../../images/unix/email-reply.png \
	    ../../images/unix/email-reply-all.png \
	    ../../images/unix/email.png \
	    ../../images/unix/email-closed-env.png \
	    ../../images/unix/email-in-env.png \
	    ../../images/unix/send-email.png \
        ../../images/unix/cc32.png \
   	    ../../images/unix/attach.png \
	    ../../images/unix/icon-milestone.png \
	    ../../images/unix/one-dot.png \
	    ../../images/unix/draft.png \
	    ../../images/unix/star_on.png \
	    ../../images/unix/star_off.png \
	    ../../images/unix/important_on.png \
	    ../../images/unix/important_off.png \
	    ../../images/unix/sale.png \
	    ../../images/unix/spam.png \
	    ../../images/unix/forum.png \
        ../../images/unix/arrow-link.png \    
        ../../images/unix/mail-board.png \
        ../../images/unix/selector-board.png \        
        ../../images/unix/b-plus.png \
        ../../images/unix/t-plus.png \
        ../../images/unix/board-shape.png \     
        ../../images/unix/update.png \
        ../../images/unix/rename.png \    
        ../../images/unix/social.png \
        ../../images/unix/close-tab.png \
        ../../images/unix/arrange-tab.png \
        ../../images/unix/slide-locked-tab.png \    
        ../../images/unix/close-tab-white.png \    
        ../../images/unix/goback.png \
        ../../images/unix/annotation.png \
        ../../images/unix/annotation-white.png \    
        ../../images/unix/gray-pin.png \    
        ../../images/unix/board-go-center.png \
        ../../images/unix/board-go-left.png \
        ../../images/unix/board-go-right.png \
        ../../images/unix/board-group-go-right.png \
        ../../images/unix/board-group-go-down.png \    
        ../../images/unix/board-go-single.png \
        ../../images/unix/blue-globe.png \
        ../../images/unix/btn_google_light_normal_ios.png \    
        ../../images/unix/b-template.png \
        ../../images/unix/moveto.png \
        ../../images/unix/x-more.png \
        ../../images/unix/x-box.png \
        ../../images/unix/x-trash.png \ 
        ../../images/unix/x-copy.png \
        ../../images/unix/x-lock-open.png \
        ../../images/unix/x-lock-closed.png \
        ../../images/unix/brush-stroke-blue.png \
        ../../images/unix/brush-stroke-geen.png \
        ../../images/unix/brush-stroke-red.png \
        ../../images/unix/brush-stroke-purple.png \
        ../../images/unix/empty-board.png \
        ../../images/unix/empty-picture.png \
        ../../images/unix/rule-filter.png \
	    ../../images/unix/yes.svg \
	    ../../images/unix/no.svg \
	    ../../images/unix/doit.svg \
	    ../../images/unix/actionable.svg \
	    ../../images/unix/singlestep.svg \
	    ../../images/osx/ard-osx-icons.icns \
	    ../../images/osx/favorites.icns \
	    ../../images/osx/moveto.icns \
    ../../../android/AndroidManifest.xml \

macx{
	!iphoneos
	{
		ICON = ../../../images/osx/ard-osx-icons.icns
	}
}

mac{
  LIBS += -framework Foundation
}

android{
# OTHER_FILES += ../../../android/src/com/ykhodak/ariadneorganizer/AriadneNotifier.java
    message("install:" $$INSTALL_ROOT "out:" $$OUT_PWD)
	ANDROID_PACKAGE_SOURCE_DIR = $$PWD/../../../images/android
}




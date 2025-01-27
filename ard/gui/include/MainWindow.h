#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "a-db-utils.h"
#include "utils.h"


class QSplitter;
class QBoxLayout;
class OutlineMain;
class TabControl;
class QSystemTrayIcon;

typedef std::vector<QToolBar*>  TBAR_LIST;

namespace ard 
{
	class workspace;
	workspace*	wspace();
};


class MainWindow : public QMainWindow
{
    Q_OBJECT    
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    void    selectOutlineView   (EOutlinePolicy op = outline_policy_Pad);
    void    updateMenu();

    void    rebuildOutline(EOutlinePolicy pol, bool force );

    EOutlinePolicy  currPolicy()const;
    EOutlinePolicy  mainTabPolicy()const;
    void            setupMainTabPolicy(EOutlinePolicy pol);
    void			selectPrevPolicy();
	void			restoreSplitter();
	void			moveSplitter(int delta);
	void			toggleSplitterButton();

	ard::workspace*		wspace(){return m_wspace; }
    void                resetGui();

    void                requestSyncAfterGuiAttached() {m_requestSyncAfterGuiAttached = true;}
    void                setupSystemTrayIcon();

	void				pasteIntoSelector();
	void				pasteIntoBoard();

public slots:
    void    detachMainWindowGui();
    void    attachGui();
    void    finalSaveCall();
    void    helpAbout();

    void    outlinePolicyChanged();
    void    importBookmarks();
    void    importTextFiles();
    void    importCProject();
    void    importContactsCvs();
    void    importNativeDB();

    void    fileBackup();
    void    viewLog();
    void    viewFolders();
	void    viewRules();
    void    viewThreads();
    void    viewBackups();
	void    viewContacts();
    void    viewContactGroups();
    void    supportWindow();
    void    sqlWindow();
    void    clearGmailCache();
    void    viewProperties();
    void    mainTabChanged(int newIdx);
    void    mainCurrentTabClicked();
    void    onScreenOrientationChanged(int);
  
#ifdef _DEBUG    
    void    debugFunction();
    void    debugFunction2();
#endif

    void    localSync();
    void    synchronizeData();
	void	closeInstance();
	void	prepareToClose();
protected:
    void    setupMenu();  
    void    makeOutlineMainActions(QMenu *menuEdit);
    void    hideAllToolbars();
    void    updateToolbars();
    void    selectOutlinePolicy(EOutlinePolicy p);
    
    void    closeEvent(QCloseEvent *e)override;

protected:
    void showEvent(QShowEvent * event)override;
	void resizeEvent(QResizeEvent *e)override;
public:

private:
    OutlineMain*			m_outline_main{nullptr};
	QWidget*				m_selector_panel{nullptr};
	ard::workspace*			m_wspace{nullptr};
    QBoxLayout				*m_selector_box{nullptr},*m_tab_box{nullptr};
    TabControl*				m_mainTab{nullptr};
    bool					m_ExteriorVisible{true}, m_requestSyncAfterGuiAttached{ false };

    QToolBar *m_outline_toolbar{nullptr},
        *m_grep_toolbar{ nullptr },
        *m_idx_card_toolbar{ nullptr },
        *m_board_select_toolbar{ nullptr },
        *m_kring_table_toolbar{ nullptr },
        *m_kring_form_toolbar{ nullptr },
        *m_email_pad_toolbar{ nullptr },
        *m_email_view_toolbar{ nullptr },
        *m_task_ring_toolbar{ nullptr },
        *m_notes_toolbar{ nullptr },
		*m_bookmarks_toolbar{ nullptr },
		*m_pictures_toolbar{ nullptr },
        *m_annotate_toolbar{ nullptr },
        *m_color_toolbar{ nullptr };

	QSplitter				*m_splitter{ nullptr };
	int						m_normal_splitter_width{0};
    QSystemTrayIcon         *m_sys_tray{ nullptr };
	bool					m_quit_requested{false};

    TBAR_LIST m_toolbars;    
    void registerToolbar(QToolBar* t);
    void hideListedToolbarsExcept(QToolBar* exceptThis);

    ///current hoisted should have tab in folder tabs
    void locateHoistedFolderInLocusTab();

    EOutlinePolicy m_curr_policy{ outline_policy_Uknown };
    EOutlinePolicy m_prev_policy{ outline_policy_Uknown };
    ACTION_LIST outlineActions;
    typedef std::map<EOutlinePolicy, int> POLICY_2_TAB_INDEX;
    POLICY_2_TAB_INDEX m_policy2tabidx;

    friend class OutlineMain;
};

extern MainWindow* main_wnd();

#endif // MAINWINDOW_H

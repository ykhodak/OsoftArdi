#ifndef OUTLINEEDIT_H
#define OUTLINEEDIT_H

#include <map>
#include <QMainWindow>
#include "OutlineView.h"
#include "utils.h"
#include "dbp.h"
#include "TabControl.h"
#include "custom-menus.h"


class QGraphicsView;
class QButtonGroup;
class anItem;
class anGPinnedNoteItem;
class QToolButton;
class FoldersFrame;
class QComboBox;
class ProtoGItem;
class OutlineSceneBase;
class QVBoxLayout;
class QSplitter;
class NotePanel;
class LineEdit;
class OutlineToolbar;
class TabControl;
class QHBoxLayout;
class QLineEdit;
class QSlider;
class QTextCursor;
class TabSpace;
class ColorTopicTabBar;
class MainWindow;
class ArdModel;

namespace ard 
{
	class topic;
	class search_edit;
};

#ifdef ARD_CHROME
#include <QWebEngineView>
#else
#include <QTextBrowser>
class ArdQTextBrowser;
#endif

using LINE_EDIT_LIST = std::vector<ard::search_edit*>;
typedef std::vector<QSlider*> SLIDER_LIST;
typedef std::vector<QComboBox*> COMBO_LIST;

using RTAB_LIST         = std::vector<std::pair<QWidget*, TabControl*>>;
using COMBO_MAP         = std::map<EOutlinePolicy, QComboBox*>;

class OutlineMain : public QWidget
{
    Q_OBJECT
public:
    OutlineMain();
    ~OutlineMain();

    ProtoGItem*     selectedGitem();

    topic_ptr         selectedItem();

    ProtoGItem*     selectNext(bool go_up, ProtoGItem* gi = nullptr);

    void            detachGui();
    void            attachGui();

    OutlineSceneBase*   scene(){return m_scene;}
    OutlineView*        view(){return m_oview;}

    void                buildToolbarsSet();
    QToolBar*           buildDefaultGrepToolbar4Big();
    
#ifdef _DEBUG  
    void             debugFunction();
#endif//_DEBUG 

    void                updateMainWndTitle();
    void                addSearchFilterControls(QToolBar* tb);
    topic_ptr           insertNewTopic(QString stitle = "", topic_ptr parent4new_item = nullptr);
    topic_ptr           createFromClipboard(topic_ptr destination_parent = nullptr);
    void                rebuild(bool force_rebuild_panels = false);
    void                rebuildTabs();
    void                requestSelectorFocus() { m_selector_focus_request = true; }
    void                check4new_mail(bool deep_check);
    void                show_selector_regular_context_menu(topic_ptr f, const QPoint& pt);
    void                show_selector_contact_context_menu(topic_ptr f, const QPoint& pt);
    void                show_selector_email_context_menu(topic_ptr f, const QPoint& pt);
    void                show_selector_board_context_menu(topic_ptr f, const QPoint& pt);
public slots:

    void    toggleNewCustomFolder();
    void    toggleRename();
    void    toggleAnnotation();
    void    toggleRetired();
    void    toggleRemoveItem();  
	void    toggleCopy();
	void    togglePaste();
	void    togglePasteIntoBoardSelector();
    void    toggleFolderSelect();

    void    toggleMoveUp();
    void    toggleMoveDown();
    void    toggleMoveLeft();
    void    toggleMoveRight();
    void    toggleMoveToDestination();
    void    togglePadMoreAction();

    void    secondaryTabChanged(int newIdx);
    void    toggleSynchronize();
    void    toggleInsertTopic();
    void    toggleInsertPopupTopic();
    void    toggleInsertKRing();
    void    toggleRemoveKRing();
    void    toggleKRingLock();

    void    toggleInsertBBoard();
    void    toggleRemoveBBoard();
    void    toggleCloneBBoard();
    void    toggleCloneTopic();

    void    toggleNotesUpdateAfterSearch();
    void    toggleGlobalSearch();
    void    toggleGlobalReplace();
    void    toggleLocateInOutline();
    void    togglePopupTopic();

    void    togglePrevView();    
    void    toggleEmailNew();
    void    toggleEmailReply();
    void    toggleEmailReplyAll();
    void    toggleEmailForward();    ;   
    void    toggleKRingDots();
	void    toggleViewEmailAttachement();
    void    toggleEmailMarkRead();
    void    toggleEmailMarkUnread();
    void    toggleEmailReload();
	void    toggleEmailSetRule();
    void    toggleEmailOpenInBlackboard();
private slots:
    void applyFilter();
    void cancelFilter();
    void toggleFilterButton();
  
private:
    QToolBar*           buildPadToolbar();
    QToolBar*           buildEmailPadToolbar();
    QToolBar*           buildBoardSelectToolbar();

    QToolBar*           buildKRingTableToolbar();
    QToolBar*           buildKRingFormToolbar();

    QToolBar*           buildTaskRingToolbar();
    QToolBar*           buildNotesToolbar();
	QToolBar*           buildBookmarksToolbar();
	QToolBar*           buildPicturesToolbar();
    QToolBar*           buildAnnotateToolbar();
    QToolBar*           buildColorToolbar();
    QToolBar*           buildGrepToolbar();

    void           updateSearchMenuItems();
    void           storeSearchFilterKey();
    QString        getNewItemTitle(QString suggestedTitle);    
    void           createRightTabs(QHBoxLayout* l);
    void           allocateRTabLocusSpace(EOutlinePolicy mainPolicy, const std::vector<EOutlinePolicy>&, QHBoxLayout* l, TabControl* tc, QWidget* topWidget = nullptr);
    void           allocateRTab(EOutlinePolicy, QHBoxLayout* l, TabControl* tc);
    void           reenableRightTabs();  

    void           doToggleFilterButton(bool bActivateFilter);
    void           doApplyFilter(QString key); 
    void            processFolderMenuSelection(uint32_t menu_value);
    void            processNonLocusedFolderMenuSelection(uint32_t menu_value);
    bool            preProcessMenuCommand(ard::menu::MCmd c, uint32_t p);
    void            processCommandFormatNotes();
    void            processCommandMergeNotes(TOPICS_LIST& lst);
    void            processCommandMarkAsUnReadEmails(bool markAsUnread);
    void            processCommandOpenInBlackboard();
    void            processCommandReloadEmails();
#ifdef _DEBUG
    void            processCommandDebug1();
#endif

    std::pair<bool, WrappedTabControl> policy2rcontroll(EOutlinePolicy pol);
    std::list<TabControl*> locusedTabSpace(const std::set<EOutlinePolicy>& pset);
private:
    TAB_CONTROLS             m_outline_secondary_tabs;///tabs on the right

    void           clone_mselected(TOPICS_LIST& lst);
    void           clear_mselected();
    void           mselect_all();
    void           move_mselected_with_option(TOPICS_LIST& lst);
    void           move_mselected(TOPICS_LIST& lst, topic_ptr dest);
    void           do_move_mselected(TOPICS_LIST& lst, topic_ptr dest);
    
    TabControl*     createFolderLocusTabSpace(QBoxLayout* l, EOutlinePolicy pol);
    void            rebuildLocusTabs();
    void            rebuildSelectorLocusTabs();
    void            rebuildRulesLocusTabs();
    void            updateKRingLockStatus();
    void            onEmailCheckTimer();
    void            onCheckAsyncCall();
    void            onColorMarkGrepFilter(ard::EColor clr_indx);
#ifdef ARD_BIG
    LabelTabBar*        labelTab(EOutlinePolicy pol);
#endif    

private:
    OutlineSceneBase*       m_scene;
    OutlineView*            m_oview;
    AccountButton*          m_account_button{nullptr};
    qreal                   m_outline_x_start;
    QGraphicsLineItem       *m_pinned_line1, *m_pinned_line2;
    LINE_EDIT_LIST          m_search_filter_edits;
    SLIDER_LIST             m_font_zoom_sliders;
    ACTION_LIST             m_search_filter_actions;
	ACTION_LIST             m_selector_link_actions;
    ACTION_CLR_MAP          m_color_grep_actions;
    QAction*                m_kring_toggle_action{nullptr};
    QComboBox*              m_index_card_combo;
    bool                    m_selector_focus_request{false};
    QTimer                  m_email_check_timer, m_delayed_asyn_call_timer;
    COMBO_MAP               m_contact_group_combos;

    bool            m_HighlightRequested;
    QString         m_last_search_filter;

    friend class ::MainWindow;
    friend class ::ArdModel;

};

extern OutlineMain* outline();

#endif // OUTLINEEDIT_H

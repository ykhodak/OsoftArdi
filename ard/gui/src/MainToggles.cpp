#include <QGraphicsView>
#include <QMenu>
#include <QGraphicsItem>
#include <QLineEdit>
#include <QPalette>
#include <QToolBar>
#include <QScrollBar>
#include <QFileDialog>
#include <QToolBar>
#include <QHBoxLayout>
#include <QSplitter>
#include <QSlider>
#include <QClipboard>
#include <QApplication>
#include <QButtonGroup>
#include <QCheckBox>
#include <QRadioButton>
#include <QShortcut>
#include <QComboBox>
#include <QWidgetAction>
#include <QPushButton>
#include <QFileDialog>
#include <QToolBar>

#include "a-db-utils.h"
#include "OutlineMain.h"
#include "utils.h"
#include "anfolder.h"
#include "ansyncdb.h"
#include "anGItem.h"
#include "ardmodel.h"
#include "MainWindow.h"
#include "NoteFrameWidget.h"
#include "OutlineScene.h"
#include "custom-g-items.h"
#include "custom-widgets.h"
#include "search_edit.h"
#include "SearchBox.h"
#include "TabControl.h"
#include "TablePanel.h"
#include "UFolderBox.h"
#include "email.h"
#include "contact.h"
#include "kring.h"
#include "email_draft.h"
#include "gmail/GmailRoutes.h"
#include "gcontact/GcontactRoutes.h"
#include "gcontact/GcontactCache.h"
#include "ContactGroupMembershipDlg.h"
#include "address_book.h"
#include "ChoiceBox.h"
#include "ethread.h"
#include "popup-widgets.h"
#include "contact.h"
#include "rule.h"
#include "ard-reporter.h"
#include "board.h"
#include "BlackBoard.h"
#include "CardPopupMenu.h"
#include "ADbgTester.h"
#include "extnote.h"
#include "rule_dlg.h"
#include "ard-algo.h"

using namespace ard::menu;

#define ACTION_ID_INC_PARENT_LABEL 100
#define ADD_COMBO_STR(D, S)combo->addItem(S, D);
#define ADD_COMBO_I_STR(I, D, S)combo->addItem(I, S, D);
#define DECLARE_SINGLE_SELECTED ProtoGItem* g = selectedGitem(); \
    if (!g) { ard::messageBox(this,"Select Item to proceed."); return; }    \


#define ADD_TOOLBAR_OUTLINE_IMG_ACTION2(I, T, M)a = new QAction(utils::fromTheme(I), T, this); \
    connect(a, &QAction::triggered, this, &OutlineMain::M);                   \
    tb->addAction(a);                                                   \


#define BUILD_ARD_BIG_DEFAULT_GREP_TBAR     ADD_TOOLBAR_OUTLINE_IMG_ACTION2("target-mark", "Locate", toggleLocateInOutline);\
                                            ADD_TOOLBAR_OUTLINE_IMG_ACTION("view", "Find", toggleGlobalSearch);\


//ADD_TOOLBAR_OUTLINE_IMG_ACTION("bkg-text", "Open", togglePopupTopic);


#define DECLARE_SMALL_TOOLBAR       QAction *a = nullptr;\
                                    QToolBar *tb = new QToolBar;\
                                    main_wnd()->registerToolbar(tb);\



#define ADD_DOT_ACTION(F)      \
a = new QAction(utils::fromTheme("dots"), QObject::tr(""), outline());\
QObject::connect(a, &QAction::triggered, outline(), &F);\
tb->addAction(a);\

extern QString contacts_info;


QToolBar* OutlineMain::buildPadToolbar()
{
    QAction *a;
    QToolBar *tb = new QToolBar;
    ADD_TOOLBAR_OUTLINE_IMG_ACTION("t-plus", "New", toggleInsertTopic);
	ADD_TOOLBAR_OUTLINE_IMG_ACTION("rename", "Rename", toggleRename);
	ADD_TOOLBAR_OUTLINE_IMG_ACTION("annotation", "Mark", togglePadMoreAction);
	ADD_TOOLBAR_OUTLINE_IMG_ACTION("x-trash", "Del", toggleRemoveItem);
	ADD_TOOLBAR_OUTLINE_IMG_ACTION("edit-copy", "Copy", toggleCopy);
	ADD_TOOLBAR_OUTLINE_IMG_ACTION("edit-paste", "Paste", togglePaste);

	ADD_SPACER(tb);
    addSearchFilterControls(tb);
    return tb;
}

QToolBar* OutlineMain::buildEmailPadToolbar() 
{
    QAction *a;
    QToolBar *tb = new QToolBar;
	ADD_TOOLBAR_OUTLINE_IMG_ACTION("email-in-env", "AsRead", toggleEmailMarkRead);
    ADD_TOOLBAR_OUTLINE_IMG_ACTION("email-compose", "Compose", toggleEmailNew);
	ADD_TOOLBAR_OUTLINE_IMG_ACTION("annotation", "Mark", togglePadMoreAction);
	ADD_TOOLBAR_OUTLINE_IMG_ACTION("x-trash", "Del", toggleRemoveItem);
	ADD_SPACER(tb);
    addSearchFilterControls(tb);	
    return tb;
};

QToolBar* OutlineMain::buildBoardSelectToolbar()
{
    QAction *a;
    QToolBar *tb = new QToolBar;
    ADD_TOOLBAR_OUTLINE_IMG_ACTION("b-plus", "New", toggleInsertBBoard);
	ADD_TOOLBAR_OUTLINE_IMG_ACTION("rename", "Rename", toggleRename);
	ADD_TOOLBAR_OUTLINE_IMG_ACTION("x-trash", "Del", toggleRemoveItem);
	ADD_TOOLBAR_OUTLINE_IMG_ACTION("edit-copy", "Copy", toggleCopy);
	ADD_TOOLBAR_OUTLINE_IMG_ACTION("edit-paste", "Paste", togglePasteIntoBoardSelector);
	ADD_SPACER(tb);
    addSearchFilterControls(tb);
    return tb;
};


QToolBar* OutlineMain::buildKRingTableToolbar() 
{
    QAction *a;
    QToolBar *tb = new QToolBar;
    ADD_TOOLBAR_OUTLINE_IMG_ACTION("add", "New", toggleInsertKRing);
    ADD_TOOLBAR_OUTLINE_IMG_ACTION("x-trash", "Del", toggleRemoveKRing);  
    ADD_TOOLBAR_OUTLINE_IMG_ACTION("unplugged", "Unlock Key Ring", toggleKRingLock);
    //ADD_TOOLBAR_OUTLINE_ACTION("Unlock Key Ring", toggleKRingLock);
    m_kring_toggle_action = a;

    ADD_SPACER(tb);
    addSearchFilterControls(tb);
    ADD_TOOLBAR_OUTLINE_IMG_ACTION("dots", "", toggleKRingDots);
    return tb;
};

QToolBar* OutlineMain::buildKRingFormToolbar() 
{
    QToolBar *tb = new QToolBar;
    return tb;
};

QToolBar* OutlineMain::buildGrepToolbar()
{
    return buildDefaultGrepToolbar4Big();
}

QToolBar* OutlineMain::buildTaskRingToolbar()
{
    return buildDefaultGrepToolbar4Big();
};

QToolBar* OutlineMain::buildAnnotateToolbar()
{
    return buildDefaultGrepToolbar4Big();
};

QToolBar* OutlineMain::buildNotesToolbar()
{
    return buildDefaultGrepToolbar4Big();
};

QToolBar* OutlineMain::buildBookmarksToolbar()
{
	return buildDefaultGrepToolbar4Big();
};

QToolBar* OutlineMain::buildPicturesToolbar()
{
	return buildDefaultGrepToolbar4Big();
};

QToolBar* OutlineMain::buildColorToolbar()
{
    QAction *a;
    QToolBar *tb = new QToolBar;
    ADD_TOOLBAR_OUTLINE_IMG_ACTION2("target-mark", "Locate", toggleLocateInOutline);
    ADD_TOOLBAR_OUTLINE_IMG_ACTION("view", "Find", toggleGlobalSearch);
  //  ADD_TOOLBAR_OUTLINE_IMG_ACTION("bkg-text", "Open", togglePopupTopic);

#define ADD_COLOR_MARK_GREP_ACTION(C, L)    a = new QAction(utils::colorMarkIcon(C, true), L, this);\
    a->setCheckable(true);\
    a->setChecked(true);\
    a->setData(static_cast<int>(C));\
    connect(a, &QAction::triggered, [=]() {\
        onColorMarkGrepFilter(C);\
    });\
    tb->addAction(a);\
    m_color_grep_actions[C] = a;\

    ADD_COLOR_MARK_GREP_ACTION(ard::EColor::purple,  "purple");
    ADD_COLOR_MARK_GREP_ACTION(ard::EColor::red,	"red");
    ADD_COLOR_MARK_GREP_ACTION(ard::EColor::blue,	"blue");
    ADD_COLOR_MARK_GREP_ACTION(ard::EColor::green,  "green");

#undef ADD_COLOR_MARK_GREP_ACTION
    ADD_SPACER(tb);
    addSearchFilterControls(tb);
    return tb;
};

void OutlineMain::onColorMarkGrepFilter(ard::EColor clr_indx)
{
    std::set<ard::EColor> clridx_map;
    for (auto& i : m_color_grep_actions) 
    {
        if (i.second->isChecked()) 
        {
            clridx_map.insert(i.first);
        }
    }
    dbp::configSetColorGrepInFilter(clridx_map);
    model()->selectColored();

    auto i = m_color_grep_actions.find(clr_indx);
    if (i != m_color_grep_actions.end()) 
    {
        auto a = i->second;
        a->setIcon(utils::colorMarkIcon(i->first, a->isChecked()));     
    }
};

void OutlineMain::togglePrevView()
{
    gui::selectPrevPolicy();
};

void OutlineMain::rebuildLocusTabs()
{
    if (!ard::isDbConnected()) {
        return;
    }

	rebuildSelectorLocusTabs();
	rebuildRulesLocusTabs();
}

void OutlineMain::rebuildSelectorLocusTabs()
{
    if (!ard::isDbConnected()) {
        return;
    }

    auto croot = ard::CustomSortersRoot();
    assert_return_void(croot, "expected custom sorter root");

    topic_ptr f = nullptr;
    TOPICS_LIST const_folder_tabs, locused_ufolders;
#define ADD_GTD_F(T)                                            \
    f = ard::db()->findLocusFolder(T);							\
    if (f) {                                                    \
        const_folder_tabs.push_back(f);                         \
    }                                                           \

#define CONDITIONALY_ADD_GTD_F(T)                               \
    f = ard::db()->findLocusFolder(T);							\
    if (f && f->isInLocusTab()) {                               \
        const_folder_tabs.push_back(f);                         \
    }                                                           \


    ADD_GTD_F(EFolderType::folderSortbox);
    ADD_GTD_F(EFolderType::folderReference);
    ADD_GTD_F(EFolderType::folderMaybe);
    CONDITIONALY_ADD_GTD_F(EFolderType::folderDelegated);
    CONDITIONALY_ADD_GTD_F(EFolderType::folderBacklog);
    CONDITIONALY_ADD_GTD_F(EFolderType::folderBoardTopicsHolder);

    bool procees_gufolders = true;
    if (!ard::isGoogleConnected()) {
        if (!ard::hasGoogleToken()) {
            procees_gufolders = false;
        }
    }

    if (procees_gufolders) {
        auto gm = ard::gmail_model();
        if (gm)
        {
            auto storage = ard::gstorage();
            if (storage) {
                for (auto& tp : croot->items()) {
                    auto f = dynamic_cast<ard::locus_folder*>(tp);
					ASSERT(f, "expected locus folder");
                    if (f && f->isInLocusTab()) {
                        const_folder_tabs.push_back(f);
                        locused_ufolders.push_back(f);
                    }
                }
            }//storage
        }//gm
    }

    /// now go over projects
    /*
    TOPICS_LIST prj_list;
    ard::selectProjects(prj_list);
    for (auto& f : prj_list) {
        if (f->isInLocusTab()) {
            const_folder_tabs.push_back(f);
        }        
    } */   

#undef ADD_GTD_F
#undef CONDITIONALY_ADD_GTD_F

    auto lst = locusedTabSpace({ outline_policy_Pad});
    for(auto& tab : lst){
        tab->setLocusTopics(const_folder_tabs);
    }
};


void OutlineMain::rebuildRulesLocusTabs()
{
    if (ard::isDbConnected()) {
        auto d = ard::db();
        if (d && d->rmodel()) {
            TOPICS_LIST topics_lst = d->rmodel()->allTabRules();
            auto lst = locusedTabSpace({ outline_policy_PadEmail});
            for (auto& tab : lst) {
                tab->setLocusTopics(topics_lst);
				tab->update();
            }
        }
    }
};

void OutlineMain::buildToolbarsSet()
{
#define ADD_SUB_TBAR(T, F)      T = F();main_wnd()->registerToolbar(T);

    ADD_SUB_TBAR(main_wnd()->m_email_pad_toolbar, buildEmailPadToolbar);
    ADD_SUB_TBAR(main_wnd()->m_outline_toolbar, buildPadToolbar);
    ADD_SUB_TBAR(main_wnd()->m_board_select_toolbar, buildBoardSelectToolbar);
    ADD_SUB_TBAR(main_wnd()->m_kring_table_toolbar, buildKRingTableToolbar);
    ADD_SUB_TBAR(main_wnd()->m_kring_form_toolbar, buildKRingFormToolbar);
    ADD_SUB_TBAR(main_wnd()->m_grep_toolbar, buildGrepToolbar);
    ADD_SUB_TBAR(main_wnd()->m_task_ring_toolbar, buildTaskRingToolbar);
    ADD_SUB_TBAR(main_wnd()->m_notes_toolbar, buildNotesToolbar);
	ADD_SUB_TBAR(main_wnd()->m_bookmarks_toolbar, buildBookmarksToolbar);
	ADD_SUB_TBAR(main_wnd()->m_pictures_toolbar, buildPicturesToolbar);
    ADD_SUB_TBAR(main_wnd()->m_annotate_toolbar, buildAnnotateToolbar);
    ADD_SUB_TBAR(main_wnd()->m_color_toolbar, buildColorToolbar);
#undef ADD_SUB_TBAR
};

QToolBar* OutlineMain::buildDefaultGrepToolbar4Big()
{
    QAction *a;
    QToolBar *tb = new QToolBar;
    BUILD_ARD_BIG_DEFAULT_GREP_TBAR;
    ADD_SPACER(tb);
    addSearchFilterControls(tb);
    return tb;
};

void OutlineMain::addSearchFilterControls(QToolBar* tb)
{
    QAction* a = nullptr;

    a = new QAction(utils::fromTheme("search-filter"), tr(""), this);
    a->setCheckable(true);
    connect(a, SIGNAL(triggered()),
            this, SLOT(toggleFilterButton()));
    tb->addAction(a);
    m_search_filter_actions.push_back(a);

    auto le = new ard::search_edit;
    le->setMinimumWidth(70);
    //le->setMaximumWidth(100);
    connect(le, SIGNAL(applyCommand()),
            this, SLOT(applyFilter()));
    connect(le, SIGNAL(escapeCommand()),
            this, SLOT(cancelFilter()));
    connect(le, &ard::search_edit::focusIn, [=]() {
        if (m_selector_focus_request) 
        {
            if (view()) {
                OutlineSceneBase* bs = view()->base_scene();
                if (bs)
                {
                    outline()->view()->setFocus();
                    ProtoGItem* sel_g = bs->currentGI();
                    if (sel_g) {
                        sel_g->g()->setFocus();
                        //qDebug() << "focus-switched2outline";
                    }
                }
            }
            m_selector_focus_request = false;
        }
    });

    tb->addWidget(le);
    m_search_filter_edits.push_back(le);

	/// selector link ///
	a = new QAction(utils::fromTheme("tab-slide-on"), tr(""), this);
	a->setCheckable(true);
	connect(a, &QAction::triggered, [=]()
	{
		auto linked = ard::isSelectorLinked();
		linked = !linked;
		QIcon icon = linked ? utils::fromTheme("tab-slide-on") : utils::fromTheme("tab-slide-off");
		for (auto& a : m_selector_link_actions) {
			a->setIcon(icon);
		}
		ard::linkSelector(linked);
	});

	tb->addAction(a);
	m_selector_link_actions.push_back(a);
};

static bool g__selector_linked = true;
bool ard::isSelectorLinked() { return g__selector_linked; };
void ard::linkSelector(bool link) { g__selector_linked = link; };


void OutlineMain::updateSearchMenuItems()
{  
    QString sfilter = "";

    if(globalTextFilter().isActive())    
        {
            sfilter = gui::searchFilterText();
            for(auto& a : m_search_filter_actions)
                {
                    if(!a->isChecked())
                        a->setChecked(true);
                }
        }
    else
        {
            for(auto& a : m_search_filter_actions)
                {
                    if(a->isChecked())
                        a->setChecked(false);
                }
        }

    for (auto& le : m_search_filter_edits)
        {
            le->setText(sfilter);
            le->resyncColor();
        }
};

void OutlineMain::toggleNewCustomFolder()
{
    LocusBox::addFolder();
};

void OutlineMain::toggleRetired()
{
    ProtoGItem* sel_g = selectedGitem();
    if(sel_g)
        {
            topic_ptr it = sel_g->topic();
            it->setRetired(it->isRetired());
            sel_g->g()->update();
        }
}

void OutlineMain::toggleRename()
{
    gui::renameSelected();
}

void OutlineMain::toggleCopy() 
{
	auto cb = QApplication::clipboard();
	if (cb) 
	{
		TOPICS_LIST lst = scene()->mselected();
		if (!lst.empty()) {
			auto mm = new ard::TopicsListMime(lst.begin(), lst.end());
			cb->setMimeData(mm);
		}
	}
};

void OutlineMain::togglePaste() 
{
	createFromClipboard();
};

void OutlineMain::togglePasteIntoBoardSelector() 
{
	auto broot = ard::db()->boards_model()->boards_root();
	assert_return_void(broot, "expected board root");
	createFromClipboard(broot);
};

void OutlineMain::toggleRemoveItem()
{
    scene()->gui_delete_mselected();
}

void ard::edit_selector_annotation() 
{
    auto o = outline();
    if (o) {
        o->toggleAnnotation();
    }
};

void ard::search(QString text) 
{
    auto s = text;
    if (s.isEmpty()) 
    {
        s = dbp::configFileLastSearchStr();
    }
    SearchBox::search(s, s);
};

void OutlineMain::toggleAnnotation() 
{
    auto t = ard::currentTopic();
    if (t) {
        bool ok = t->canHaveAnnotation();
        if (!ok) {
            ard::errorBox(ard::mainWnd(), QString("Selected topic '%1' can't be annotated").arg(t->impliedTitle()));
            return;
        }
        gui::renameSelected(EColumnType::Annotation);
    }
};

void OutlineMain::toggleFolderSelect()
{
    QAction* a = dynamic_cast<QAction*>(sender());
    assert_return_void(a, "expected action");
    auto f = ard::lookup(a->data().toInt());
    if (f)
        {
            gui::outlineFolder(f, outline_policy_Pad);
        }
};

void OutlineMain::toggleMoveUp()
{    
    DECLARE_SINGLE_SELECTED;
    topic_ptr topic2move = nullptr;
    auto f = g->topic();
    if (f) {
        topic2move = f->shortcutUnderlying();
    }

    if (topic2move) {
        snc::cit* it2 = ard::moveInsideParent(topic2move, true);
        if (it2) {
            topic_ptr item2 = dynamic_cast<topic_ptr>(it2);
            assert_return_void(item2, "expected anItem");
            gui::rebuildOutline();
        }
    }
};

void OutlineMain::toggleMoveDown()
{    
    DECLARE_SINGLE_SELECTED;
    topic_ptr topic2move = nullptr;
    auto f = g->topic();
    if (f) {
        topic2move = f->shortcutUnderlying();
    }

    if (topic2move) {
        snc::cit* it2 = ard::moveInsideParent(topic2move, false);
        if (it2) {
            topic_ptr item2 = dynamic_cast<topic_ptr>(it2);
            assert_return_void(item2, "expected anItem");

           // g->topic()->requestOutlineLabelGeneration();
           // item2->requestOutlineLabelGeneration();
            gui::rebuildOutline();
        }
    }
};

void OutlineMain::toggleMoveLeft()
{    
    DECLARE_SINGLE_SELECTED;
    topic_ptr it = g->topic();
    auto* p = it->parent();
    if(!p)return;
    auto p2 = p->parent();
    if(!p2)return;
    if (p == ard::hoisted())return;
    if(p->isGtdSortingFolder())return;
    int idx = p2->indexOf(p);
    assert_return_void(idx != -1, "expected valid item index");      

    if(COMPATIBLE_PARENT_CHILD(p2, it))
        {
            p->detachItem(it);
            p2->insertItem(it, idx);
            p2->ensurePersistant(1);
            p->ensurePersistant(1);
            gui::rebuildOutline();
			auto h = p->getLocusFolder();
			if (h) {
				ard::rebuildFoldersBoard(h);
			}
        }  
};

void OutlineMain::toggleMoveRight()
{    
    DECLARE_SINGLE_SELECTED;
    topic_ptr it = g->topic();
    auto p = it->parent();
    if(!p)return;
    int idx = p->indexOf(it);
    assert_return_void(idx != -1, "expected valid item index");  
    idx--;
    if(idx < 0)return;
    if((int)p->items().size() <= idx)return;
    topic_ptr it2 = dynamic_cast<topic_ptr>(p->items()[idx]);
    assert_return_void(it2, "expected valid item");  
    auto p2 = dynamic_cast<topic_ptr>(it2);
    if(!p2)return;
    if(COMPATIBLE_PARENT_CHILD(p2, it))
        {
            p->detachItem(it);
            p2->addItem(it);
            if(!p2->isExpanded())
                p2->setExpanded(true);
            p2->ensurePersistant(1);
            p->ensurePersistant(1);
            gui::rebuildOutline();
			auto h = p->getLocusFolder();
			if (h) {
				ard::rebuildFoldersBoard(h);
			}
        }  
};

void OutlineMain::toggleMoveToDestination() 
{
    auto sel_list = scene()->mselected();
    if (sel_list.empty())
    {
        ard::messageBox(this,"Select Item to proceed."); 
        return;
    }

    if (!sel_list.empty()) {
        move_mselected_with_option(sel_list);
    }
};

void OutlineMain::togglePadMoreAction() 
{
    ProtoGItem* g = selectedGitem();
    if (!g) { ard::messageBox(this,"Select Item to proceed."); return; }

    auto lst = scene()->mselected();
    QPoint pt = QCursor::pos();
    ard::SelectorTopicPopupMenu::showSelectorTopicPopupMenu(pt, lst, false);
};

void OutlineMain::do_move_mselected(TOPICS_LIST& lst, topic_ptr dest) 
{
    assert_return_void(dest, "expected destination topic");

    int action_taken = 0;
    ard::guiInterpreted_moveTopics(lst, dest, action_taken);

    if (action_taken > 0) {
        //dest->ensurePersistant(1);
        gui::rebuildOutline();
    }
};

void OutlineMain::move_mselected(TOPICS_LIST& lst, topic_ptr dest)
{
    assert_return_void(dest, "expected destination topic");
    assert_return_void(gui::isDBAttached(), "expected attached DB");
    if (ard::confirmBox(this, QString("Move to <b><font color=\"red\">%1 %2</font></b> selected items?").arg(dest->impliedTitle()).arg(lst.size()))) {
        do_move_mselected(lst, dest);
    }
};

void OutlineMain::clear_mselected()
{
    scene()->clearMSelected();
    m_oview->viewport()->update();
};

void OutlineMain::mselect_all()
{
    scene()->mselectAll();
    m_oview->viewport()->update();
};

void OutlineMain::clone_mselected(TOPICS_LIST& lst) 
{
    TOPICS_LIST lst2clone;
    TOPICS_SET parentsOfcloned;
    QString strErrorMsg;
    topic_ptr firstCloned = nullptr;

    auto h = ard::hoisted();
    for (auto i : lst) 
    {
        if (i == h) {
            strErrorMsg = "Can't clone root topic";
            continue;
        }
        lst2clone.push_back(i);
    }

    if (lst2clone.empty()) {
        if (!strErrorMsg.isEmpty()) {
            ard::messageBox(this,strErrorMsg);
        }
        return;
    }

    if (!strErrorMsg.isEmpty()) {
        if (!ard::confirmBox(this, strErrorMsg + "Some of the selected topics can't be cloned, Do you want to proceed?")) {
            return;
        }
    }

    for (auto i : lst2clone) 
    {
        auto cp = i->parent();
        if (!cp) { ASSERT(0, "expected parent topic"); continue; }
        auto idx = cp->indexOf(i);
        if(idx == -1) { ASSERT(0, "expected valid topic index"); continue; }
        auto f2 = i->clone();
        cp->insertItem(f2, idx + 1);
        parentsOfcloned.insert(cp);
        if (!firstCloned) {
            firstCloned = f2;
        }
        /*if (p2clone->isExpanded()) {
            p2clone->setExpanded(false);
        }*/
    }

    for (auto i : parentsOfcloned) {
        i->ensurePersistant(-1);
    }

    gui::rebuildOutline();
    if (firstCloned) {
        auto g_it = scene()->findGItemByUnderlying(firstCloned);
        if (g_it) {
            scene()->clearSelection();
			scene()->selectGI(g_it);
            g_it->g()->ensureVisible(QRectF(0, 0, 1, 1));
        }
    }
};

void OutlineMain::move_mselected_with_option(TOPICS_LIST& lst)
{
    assert_return_void(gui::isDBAttached(), "expected attached DB");
    assert_return_void(lst.size() > 0, "expected selected list");
	ard::move_dlg::moveIt(lst);
};

void OutlineMain::toggleSynchronize()
{
    if (!dbp::guiCheckNetworkConnection())
        return;

    gui::startSync();
};


topic_ptr gui::insertTopic(QString stitle) 
{
    if (outline()) {
        outline()->insertNewTopic(stitle);
    }
    return nullptr;
};

topic_ptr OutlineMain::insertNewTopic(QString stitle, topic_ptr parent4new_item)
{
    topic_ptr rv = nullptr;

    //topic_ptr parent4new_item = nullptr;
    int index4new_item = -1;
    if (parent4new_item) {
        index4new_item = parent4new_item->items().size();
    }

    if (!parent4new_item) {
        ProtoGItem* sel_g = scene()->currentGI();
        if (sel_g)
        {
            auto it = sel_g->topic();
            if (!it) {
                it = ard::Sortbox();
            }
            else {
                it = it->shortcutUnderlying();
            }

            if (it->isGtdSortingFolder() ||
                it->folder_type() == EFolderType::folderUserSorter ||
                (it == ard::hoisted()) ||
                !it->parent())
            {
                parent4new_item = it;
                index4new_item = 0;
            }
            else
            {
                if (it->isExpanded() && it->items().size() > 0)
                {
                    parent4new_item = it;
                    index4new_item = 0;
                }
            }

            if (!parent4new_item)
            {
                parent4new_item = it->parent();
                assert_return_null(parent4new_item, "expected folder");
                index4new_item = parent4new_item->indexOf(it);
                index4new_item++;
            }
        }
        else
        {
            auto hs = ard::hoisted();
            assert_return_null(hs, "expected hoisted");
            if (!hs->isExpanded())
            {
                hs->setExpanded(true);
            }

            parent4new_item = hs;
            index4new_item = 0;
        }
    }

    if(parent4new_item && index4new_item != -1)
        {
            rv = new ard::topic(stitle);

            if (!parent4new_item->canAcceptChild(rv)) {
                parent4new_item = ard::hoisted();
                if (!parent4new_item || !parent4new_item->canAcceptChild(rv)) {
                    parent4new_item = ard::Sortbox();
                }

                if (!parent4new_item || !parent4new_item->canAcceptChild(rv)) {
                    rv->release();
                    return nullptr;
                }
                else {
                    index4new_item = 0;
                    parent4new_item->insertItem(rv, index4new_item);
                    parent4new_item->ensurePersistant(1);
                }
            }
            else {
                parent4new_item->insertItem(rv, index4new_item);
                parent4new_item->ensurePersistant(1);
            }
        }

    if (rv) {
        ard::setHoistedInFilter(rv);
    }
    return rv;
};

topic_ptr ard::insert_new_topic(topic_ptr parent)
{
    if (!outline()) {
        ASSERT(0, "expected outline");
        return nullptr;
    }

    auto rv = outline()->insertNewTopic("", parent);
    if (rv) 
    {
        gui::rebuildOutline();
        gui::ensureVisibleInOutline(rv);
        gui::renameSelected();
    }
    return rv;
};

topic_ptr ard::insert_new_popup_topic(topic_ptr parent) 
{
    if (!outline()) {
        ASSERT(0, "expected outline");
        return nullptr;
    }

    auto rv = outline()->insertNewTopic("", parent);
    if (rv)
    {
        gui::rebuildOutline();
        ard::open_page(rv);
        /*QTimer::singleShot(2000, [=]() {
            ard::focusOnTSpace();
        });*/
    }
    return rv;
};

topic_ptr OutlineMain::createFromClipboard(topic_ptr destination_parent)
{
    topic_ptr rv = nullptr;

    const QMimeData *mm = nullptr;
    if (qApp->clipboard()) {
        mm = qApp->clipboard()->mimeData();
    }
    if (!mm) {
        return nullptr;
    }

    //topic_ptr destination_parent = nullptr;
    int destination_pos = -1;
    auto pol = gui::currPolicy();
    if (pol == outline_policy_Pad) 
    {
        destination_parent = ard::currentTopic();
        if (destination_parent) 
        {
            if (destination_parent->items().size() > 0 && destination_parent->isExpanded()) {
                destination_pos = destination_parent->items().size();
            }
            else 
            {
                if (dynamic_cast<ard::locus_folder*>(destination_parent) == nullptr)
				{
                    auto p = destination_parent->parent();
                    if (p)
                    {
                        destination_pos = p->indexOf(destination_parent) + 1;
                        destination_parent = p;
                        //destination_pos = destination_parent->items().size();
                    }
                }
            }
        }
    }

    if(!destination_parent)
    {
        destination_parent = ard::Sortbox();
        if (destination_parent) {
            destination_pos = destination_parent->items().size();
        }
    }

    if (destination_parent) {
		if (destination_pos == -1)
			destination_pos = 0;

        auto lst = ard::insertClipboardData(destination_parent, destination_pos, mm, false);
		gui::rebuildOutline();
		if (!lst.empty())
		{
			auto created = *lst.begin();
			gui::ensureVisibleInOutline(created);

			auto h = destination_parent->getLocusFolder();
			if (h) {
				ard::rebuildFoldersBoard(h);
			}
		}
    }

    ard::focusOnOutline();

    return rv;
};

void OutlineMain::toggleInsertTopic()
{
    ard::insert_new_topic();
};

void OutlineMain::toggleInsertPopupTopic() 
{
    ard::insert_new_popup_topic();
};

void OutlineMain::toggleInsertBBoard() 
{
    auto br = ard::db()->boards_model()->boards_root();
    if (br) {
        auto b = br->addBoard();
        if (b) {
            gui::rebuildOutline();
			gui::ensureVisibleInOutline(b);
        }
    }
};

void OutlineMain::toggleRemoveBBoard() 
{
	scene()->gui_delete_mselected();
    //scene()->removeSelItem();
};

void OutlineMain::toggleCloneBBoard() 
{
    ard::selector_board* b2clone = nullptr;
    ProtoGItem* g = scene()->currentGI();
    if (g && g->topic())
    {
        auto it = g->topic()->shortcutUnderlying();
        b2clone = dynamic_cast<ard::selector_board*>(it);
    }

    if (!b2clone) {
        ard::messageBox(this,"Please select board to proceed.");
        return;
    }

    auto br = ard::db()->boards_model()->boards_root();
    if (br) {
        auto b = br->cloneBoard(b2clone);
        if (b) {
            gui::rebuildOutline();
            ard::open_page(b);
            auto g_it = scene()->findGItemByUnderlying(b);
            if (g_it) {
                scene()->clearSelection();
				scene()->selectGI(g_it);
                g_it->g()->ensureVisible(QRectF(0, 0, 1, 1));
            }
        }
    }
};

void OutlineMain::toggleCloneTopic() 
{
    TOPICS_LIST lst = scene()->mselected();
    if (!lst.empty()) {
        clone_mselected(lst);
    }
};

void OutlineMain::togglePopupTopic()
{
    auto f = ard::currentTopic();
    if (f) {
        ard::open_page(f);
    };
}

void OutlineMain::toggleInsertKRing()
{
    assert_return_void(ard::isDbConnected(), "expected open DB");
    auto r = ard::db()->kmodel()->keys_root();
    if (r->isRingLocked()) {
        if (!r->guiUnlockRing()) {
            return;
        }
    }

    assert_return_void(!r->isRingLocked(), "expected unlocked kring");
    auto new_k = r->addKey("new key");
    if (new_k) {
        ard::KRingRoot::setHoistedRingKeyInFilter(new_k);
        gui::rebuildOutline(outline_policy_KRingTable, true);
        bool ok = gui::ensureVisibleInOutline(new_k);
        if (ok) {
            ard::asyncExec(AR_RenameTopic, static_cast<int>(EColumnType::KRingTitle), 1);
        }
    }
};

void OutlineMain::toggleRemoveKRing() 
{
    assert_return_void(ard::isDbConnected(), "expected open DB");
    auto r = ard::db()->kmodel()->keys_root();
    if (r->isRingLocked()) {
        if (!r->guiUnlockRing()) {
            return;
        }
    }

    assert_return_void(!r->isRingLocked(), "expected unlocked kring");
    toggleRemoveItem();
};

void OutlineMain::toggleKRingLock() 
{
    assert_return_void(ard::isDbConnected(), "expected open DB");
    auto r = ard::db()->kmodel()->keys_root();
    if (r->isRingLocked()) {
        r->guiUnlockRing();
    }
    else {
        r->lockRing();
    }
    gui::rebuildOutline();
};

void OutlineMain::updateKRingLockStatus() 
{
    if (ard::isDbConnected() && m_kring_toggle_action) {
        auto r = ard::db()->kmodel()->keys_root();
        if (r->isRingLocked()) {
            m_kring_toggle_action->setText("Unlock Key Ring");
            //r->guiUnlockRing();
        }
        else {
            m_kring_toggle_action->setText("Lock Key Ring");
            //r->lockRing();
        }
    }
};

void OutlineMain::toggleKRingDots()
{
    QAction* a = nullptr;
    QMenu m(this);
    ard::setup_menu(&m);
    ADD_MCMD("Create Report", MCmd::kring_create_report);
    ADD_MCMD("Change Master Password", MCmd::kring_change_master_pwd);
    connect(&m, &QMenu::triggered, [&](QAction* a)
    {
        auto d = unpackMcmd(a);
        switch (d.first) {
        case MCmd::kring_change_master_pwd: 
        {
            
        }break;
        case MCmd::kring_create_report: 
        {
            ard::TexDocReportBuilder::guiMergeKRingKeys();
        }break;
        default:break;
        }
    });

    m.exec(QCursor::pos());

    /*
#ifndef ARD_BIG
    QAction* a = nullptr;
    m.addSeparator();
    ADD_MCMD("Files..", MCmd::select_file);
    m.addSeparator();
    a = new QAction("Cancel", this);
    a->setData(0);
    m.addAction(a);
#endif
*/
};


void OutlineMain::toggleFilterButton()
{
    QAction* a = dynamic_cast<QAction*>(sender());
    assert_return_void(a, "expected action");

    if(a->isChecked() && globalTextFilter().isActive())
        {
            return;
        }

    if(!a->isChecked() && !globalTextFilter().isActive())
        {
            return;
        }

    doToggleFilterButton(a->isChecked());
};


void OutlineMain::storeSearchFilterKey() 
{
    m_last_search_filter = gui::searchFilterText();
};

void OutlineMain::doToggleFilterButton(bool bActivateFilter)
{
    QString sfilter;

    if(bActivateFilter)
        {
            sfilter = m_last_search_filter;
        }
    else
        {
            storeSearchFilterKey();
            sfilter = "";
        }

    doApplyFilter(sfilter);
}

void OutlineMain::cancelFilter()
{
    if(globalTextFilter().isActive())
        {
            doToggleFilterButton(false);
        }
};

void OutlineMain::doApplyFilter(QString key)
{
    auto h = model()->selectedHoistedTopic();
    ASSERT(h, "expected hoisted");
    if(h)
        {
            bool refilter = true;
            if(gui::searchFilterIsActive())
                {
                    if(gui::searchFilterText() == key)
                        {
                            refilter = false;
                        }
                }

            if(refilter)
                {
                    TextFilterContext fc;
                    fc.key_str = key;
                    fc.include_expanded_notes = false;
                    if (outline()->scene()->hasPanel())
                        {
                            auto p = outline()->scene()->panel();
                            if(p->hasProp(ProtoPanel::PP_ExpandNotes)){
                                fc.include_expanded_notes = true;
                            };
                        }
                    
                    globalTextFilter().setSearchContext(fc);
                    gui::rebuildOutline();
                }
        }

    updateSearchMenuItems();

    if(!m_HighlightRequested)
        {
            m_HighlightRequested = true;
            QTimer::singleShot(200, this, SLOT(toggleNotesUpdateAfterSearch()));
        }
};

void OutlineMain::applyFilter()
{
    QString key = "";
  
    QLineEdit* e = dynamic_cast<QLineEdit*>(sender());
    assert_return_void(e, "expected line edit");
    if(!e->isEnabled())
        {
            return;
        }

    if(e)
        key = e->text();

    doApplyFilter(key);
};



void OutlineMain::toggleNotesUpdateAfterSearch()
{
    m_HighlightRequested = false; 
};

void OutlineMain::toggleGlobalSearch()
{
    ard::search(dbp::configFileLastSearchStr());
    //auto ss = model()->lastSearchInfo();
    //SearchBox::search(dbp::configFileLastSearchStr(), dbp::configFileLastEmailSearchStr());
};

void OutlineMain::toggleGlobalReplace()
{
   // ReplaceBox::replace(model()->selectedSearchString(), "", search_scopeTopic);
};

void OutlineMain::toggleLocateInOutline()
{
    auto f = ard::currentTopic();
	if (!f) {
		ard::errorBox(ard::mainWnd(), "Select topic to locate in outline please.");
		return;
	}
    if (f->IsUtilityFolder()) {
		ard::errorBox(ard::mainWnd(), "Select topic to locate in outline please.");
        return;
    }
    gui::ensureVisibleInOutline(f/*, outline_policy_Pad*/);
};

void createForwardDraftNote(ard::email* e)
{
    googleQt::GmailRoutes* r = ard::gmail();
    if (r) {
        auto r = ard::address_book_dlg::select_receiver(ard::mainWnd(), nullptr, true);
        if (r.lookup_succeeded) 
		{
			ard::email_draft_ext::attachement_file_list attachements;
			if (r.include_attached4forward) {
				auto storage = ard::gstorage();
				assert_return_void(storage, "expected gmail storage");
				auto lst = e->getAttachments();
				for (auto i : lst) {
					if (i->status() == googleQt::mail_cache::AttachmentData::statusDownloaded) 
					{
						ard::email_draft_ext::attachement_file att = { storage->findAttachmentFile(i), ""};
						attachements.push_back(att);
					}
					else 
					{
						if (!i->attachmentId().isEmpty()) {
							ard::email_draft_ext::attachement_file att = { storage->findAttachmentFile(i), i->attachmentId() };
							attachements.push_back(att);
						}
					}
				}
			}

            
            auto m = ard::email_draft::forward_draft(e);
            auto ext = m->draftExt();
            assert_return_void(ext, QString("expected draft extension %1").arg(m->dbgHint()));
            auto str_to = ard::ContactsLookup::toAddressStr(r.to_list);
            auto str_cc = ard::ContactsLookup::toAddressStr(r.cc_list);
            ext->set_draft_data(str_to, str_cc, "", &attachements);
            auto p = ard::edit_note(m, true);
			if(p)
				p->setSlideLocked(true);
        }
    }
}

void createNewDraftNote()
{
    googleQt::GmailRoutes* r = ard::gmail();
    if (r) {
        auto r = ard::address_book_dlg::select_receiver(ard::mainWnd());
        if (r.lookup_succeeded) 
		{
            auto m = ard::email_draft::newDraft(ard::google()->userId());
            auto ext = m->draftExt();
            assert_return_void(ext, QString("expected draft extension %1").arg(m->dbgHint()));
            auto str_to = ard::ContactsLookup::toAddressStr(r.to_list);
            auto str_cc = ard::ContactsLookup::toAddressStr(r.cc_list);
            ext->set_draft_data(str_to, str_cc);
            auto p = ard::edit_note(m, true);
			if (p)
				p->setSlideLocked(true);
        }
    }
}

void OutlineMain::toggleEmailNew()
{ 
    createNewDraftNote();
};

void OutlineMain::processFolderMenuSelection(uint32_t menu_value) 
{
    auto f = ard::lookup(menu_value);
    if (f) {
        gui::outlineFolder(f);
    }
};

void  OutlineMain::processNonLocusedFolderMenuSelection(uint32_t menu_value)
{
    auto f = ard::lookup(menu_value);
    if (f) {
        auto u = dynamic_cast<ard::locus_folder*>(f);
        if (u) {
            u->setInLocusTab(true);
            gui::outlineFolder(u, outline_policy_Pad);
            ard::asyncExec(AR_RebuildLocusTab);
        }
    }
    else {
        ASSERT(0, "topic lookup failed") << menu_value;
    }
}

bool OutlineMain::preProcessMenuCommand(ard::menu::MCmd c, uint32_t p)
{
    Q_UNUSED(p);

    TOPICS_LIST lst = scene()->mselected();

    switch (c) {
    //case MCmd::open_notes:      processCommandOpenNotes(); return true;
    case MCmd::format_notes:    processCommandFormatNotes(); return true;
    case MCmd::remove:          scene()->gui_delete_mselected(); return true;
    case MCmd::clear:           clear_mselected(); return true;
    case MCmd::select_all:      mselect_all(); return true;
    case MCmd::merge_notes:processCommandMergeNotes(lst); return true;
    //case MCmd::exit_edit_mode: {
    //    dbp::configSetCheckSelectMode(false);
    //    gui::rebuildOutline(outline_policy_Uknown, true);
    //}break;
    case MCmd::email_mark_as_read: {
        processCommandMarkAsUnReadEmails(false);
    }break;
    case MCmd::email_mark_as_unread: {
        processCommandMarkAsUnReadEmails(true);
    }break;
    //case MCmd::email_delete: {
    //    processCommandMarkAsArchivedEmails();
    //}break;
    case MCmd::email_reload: {
        processCommandReloadEmails();
    }break;
    case MCmd::open_in_blackboard: {
        processCommandOpenInBlackboard();
    }break;
#ifdef _DEBUG
    case ard::menu::MCmd::debug1:
    {
        processCommandDebug1();
    }break;
#endif
    default:break;
    }
    return false;
};

void OutlineMain::toggleEmailMarkRead() 
{
    processCommandMarkAsUnReadEmails(false);
};

void OutlineMain::toggleEmailMarkUnread() 
{
    processCommandMarkAsUnReadEmails(true);
};
void OutlineMain::toggleEmailReload() 
{
    processCommandReloadEmails();
};

void OutlineMain::toggleEmailSetRule() 
{
	auto gm = ard::gmail();
	if (!gm) {
		ard::messageBox(this,"Gmail module is not authorized.");
		return;
	}

	QString rule_name;
	std::set<QString> from_set;
	TOPICS_LIST l1 = scene()->mselected();
	auto tlst = ard::reduce2emails(l1);
	if (tlst.empty()) {
		ard::errorBox(this, "Select Emails to proceed.");
		return;
	}

	for (auto& e : tlst) 
	{
		if (rule_name.isEmpty()) {
			rule_name = e->title().section(QRegExp("\\s+"), 0, 0, QString::SectionSkipEmpty).trimmed();
			rule_name.remove(QRegExp("[\"']"));
		}

		auto from = e->from();
		if (!from.isEmpty()) 
		{
			from_set.insert(from);
		}
	}

	ard::rule_dlg::addRule(rule_name, &from_set);
	//if (ard::rule_dlg::addRule(rule_name, &from_set)) {
	//	ard::asyncExec(AR_RebuildLocusTab);
	//};
};

void OutlineMain::toggleEmailOpenInBlackboard() 
{
    processCommandOpenInBlackboard();
};

void OutlineMain::toggleViewEmailAttachement()
{
	ard::email* e = ard::currentEmail();
	if (e) {
		ard::attachements_dlg::runIt(e);
	}
};

void OutlineMain::processCommandMarkAsUnReadEmails(bool markAsUnread)
{
    if (!ard::gmail()) {
        ard::messageBox(this,"Gmail module is not authorized.");
        return;
    }

    //auto cr = ard::gmail()->cacheRoutes();
    //assert_return_void(cr, "expected gmail cache");

    TOPICS_LIST lst = scene()->mselected();
    auto l2 = ard::reduce2emails(lst);
    if (l2.empty()) {
        ard::errorBox(this, "Select Emails to proceed.");
    }
    else 
	{
		auto m = ard::gmail_model();
		if (m) {
			m->markUnread(l2, markAsUnread);
		}
    }
};

void OutlineMain::processCommandReloadEmails() 
{
    auto gm = ard::gmail();
    if (!gm) {
        ard::messageBox(this,"Gmail module is not authorized.");
        return;
    }

    TOPICS_LIST l1 = scene()->mselected();
    ard::close_popup(l1);
    auto tlst = ard::reduce2ethreads(l1.begin(), l1.end());
    if (tlst.empty()) {
        ard::errorBox(this, "Select Emails to proceed.");
        return;
    }

    std::vector<QString> msg_id_list;
    for (auto i : tlst) 
    {
        msg_id_list.push_back(i->threadId());
        LOCK(i);
    }

    if (!msg_id_list.empty())
    {
        auto q = gm->cacheRoutes()->refreshCacheThreadMessages_Async(msg_id_list);
        q->then([=](std::unique_ptr<googleQt::CacheDataResult<googleQt::mail_cache::ThreadData>> res)
        {
            auto gm = ard::gmail_model();
            if (gm) 
            {
                for (auto i : tlst) {
                    auto t = ard::as_ethread(i);
                    if (t) {
                        t->resetThread();
                    }
                }
            }

            for (auto i : tlst){
                i->release();
            }
            for (auto i : res->result_list) {
                qWarning() << "msg-reloaded" << i->id();
            };

            auto pol = gui::currPolicy();
            if (pol == outline_policy_PadEmail) {
                gui::rebuildOutline();
            }
        },
            [=](std::unique_ptr<googleQt::GoogleException> ex)
        {
            for (auto i : tlst) {
                i->release();
            }
            ASSERT(0, "Failed to reload body of messages") << ex->what();
        });

    }

};

#ifdef _DEBUG
void OutlineMain::processCommandDebug1() 
{
};
#endif

void OutlineMain::processCommandFormatNotes() 
{
    TOPICS_LIST lst = scene()->mselected();
    if (lst.empty()) {
        ard::errorBox(this, "Check Topics to proceed.");
    }
    else {
        FormatFontDlg::format(lst);
    }
};

void OutlineMain::processCommandMergeNotes(TOPICS_LIST& lst)
{
    assert_return_void(!lst.empty(), "expected topics list");
    ard::TexDocReportBuilder::mergeTopics(lst);
};

void OutlineMain::processCommandOpenInBlackboard() 
{
#ifdef ARD_BIG
    auto br = ard::db()->boards_model()->boards_root();
    assert_return_void(br, "expected board root");

    TOPICS_LIST lst = scene()->mselected();
    if (lst.empty()) {
        ard::errorBox(this, "Check Topics to proceed.");
    }
    else {
		auto res = ard::wspace()->firstBlackboard();

        if (res.first) {
            int bidx = 0;
            auto bb_curr = res.first->board();
            assert_return_void(bb_curr, "expected board");
            if (!IS_VALID_DB_ID(bb_curr->id())) {
                br->ensurePersistant(1);
            }
            bb_curr->insertTopicsBList(lst, bidx);
            std::unordered_set<int> b2r;
			b2r.insert(bidx);
			res.first->resetBand(b2r);
        }
        else {
            auto b = br->addBoard();
            if (b) {
                b->insertTopicsBList(lst, 0);
                gui::rebuildOutline();
                ard::open_page(b);
            }
        }
    }
#endif
};

#undef ADD_MENU_ACTION

#define CHECK_VALID_EMAIL(M)    assert_return_void(M, "expected email"); \
    if(!M->optr()){                                                     \
        ard::errorBox(this, "Incomplete Email object");                      \
        return;                                                         \
    }                                                                   \

void OutlineMain::toggleEmailReply() 
{
    auto m = ard::currentEmail();
    CHECK_VALID_EMAIL(m);
    auto p = ard::edit_note(ard::email_draft::replyDraft(m), true);
	if (p)
		p->setSlideLocked(true);
};

void OutlineMain::toggleEmailReplyAll() 
{
    auto m = ard::currentEmail();
    CHECK_VALID_EMAIL(m);
    auto p = ard::edit_note(ard::email_draft::replyAllDraft(m), true);
	if (p)
		p->setSlideLocked(true);
};

void OutlineMain::toggleEmailForward() 
{
    auto m = ard::currentEmail();
    CHECK_VALID_EMAIL(m);

    extern void createForwardDraftNote(ard::email*);
	createForwardDraftNote(m);
};


void OutlineMain::show_selector_regular_context_menu(topic_ptr , const QPoint& pt)
{
    auto p = gui::currPolicy();
    QMenu m(outline());
    ard::setup_menu(&m);
	ADD_CONTEXT_MENU_IMG_ACTION("rename", "Rename", toggleRename);
	ADD_CONTEXT_MENU_IMG_ACTION("annotation", "Mark", togglePadMoreAction);
    ADD_CONTEXT_MENU_IMG_ACTION("x-trash", "Del", toggleRemoveItem);
    if (p == outline_policy_Pad) {
        ADD_CONTEXT_MENU_IMG_ACTION("x-copy", "Clone", toggleCloneTopic);
    }
    ADD_CONTEXT_MENU_IMG_ACTION("moveto", "MoveTo..", toggleMoveToDestination);
    m.exec(pt);
};

void OutlineMain::show_selector_contact_context_menu(topic_ptr , const QPoint& ) 
{

};

void OutlineMain::show_selector_email_context_menu(topic_ptr f, const QPoint& pt) 
{
    QAction* a = nullptr;
    QMenu m(outline());
    ard::setup_menu(&m);
    auto *tb = &m;

	auto e = ard::as_email(f);
	if (e && e->hasAttachment()) {
		//ADD_MCMD2("View attachement", MCmd::email_view_attachement, 0);
		ADD_TOOLBAR_OUTLINE_ACTION("View attachement", toggleViewEmailAttachement);
	}

    ADD_CONTEXT_MENU_IMG_ACTION("x-trash", "Del", toggleRemoveItem);
    ADD_CONTEXT_MENU_IMG_ACTION("annotation", "Mark", togglePadMoreAction);
	m.addSeparator();
	ADD_CONTEXT_MENU_IMG_ACTION("search-filter", "Set Rule", toggleEmailSetRule);
	//ADD_TOOLBAR_OUTLINE_ACTION("Set Filter", toggleEmailSetRule);
	m.addSeparator();
    //ADD_CONTEXT_MENU_IMG_ACTION("moveto", "AddTo..", toggleMoveToDestination);
    ADD_TOOLBAR_OUTLINE_ACTION("Mark As 'Read'", toggleEmailMarkRead);
    ADD_TOOLBAR_OUTLINE_ACTION("Mark As 'UnRead'", toggleEmailMarkUnread);
    ADD_TOOLBAR_OUTLINE_ACTION("Reload Email", toggleEmailReload);
	ADD_TOOLBAR_OUTLINE_ACTION("AddTo..", toggleMoveToDestination);
    m.exec(pt);
};

void OutlineMain::show_selector_board_context_menu(topic_ptr , const QPoint& pt) 
{
    QMenu m(outline());
    ard::setup_menu(&m);
    ADD_CONTEXT_MENU_IMG_ACTION("x-trash", "Del", toggleRemoveBBoard);
    ADD_CONTEXT_MENU_IMG_ACTION("x-copy", "Clone", toggleCloneBBoard);
    ADD_CONTEXT_MENU_IMG_ACTION("rename", "Rename", toggleRename);
    ADD_CONTEXT_MENU_IMG_ACTION("moveup", "Up", toggleMoveUp);
    ADD_CONTEXT_MENU_IMG_ACTION("movedown", "Down", toggleMoveDown);
    m.exec(pt);
};

void ard::show_selector_context_menu(topic_ptr f1, const QPoint& pt)
{
    assert_return_void(outline(), "expected outline");
    auto f = f1->shortcutUnderlying();
    auto e = ard::as_email(f1);
    if (e) {
        outline()->show_selector_email_context_menu(f, pt);
    }
    else {
        auto b = dynamic_cast<ard::selector_board*>(f);
        if (b) {
            outline()->show_selector_board_context_menu(b, pt);
        }
        else {
            outline()->show_selector_regular_context_menu(f, pt);
        }
    }
};

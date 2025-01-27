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
#include <QScroller>
#include <QPushButton>
#include <QButtonGroup>
#include <ctime>

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
#include "NoteEdit.h"
#include "search_edit.h"
#include "SearchBox.h"
#include "TabControl.h"
#include "TablePanel.h"
#include "EmailPreview.h"
#include "ansearch.h"
#include "rule.h"
#include "rule_runner.h"
#include "syncpoint.h"

static OutlineMain* TheOutlineMain = nullptr;
static int g__LasteSelectedColorIndex = -1;
int getLasteSelectedColorIndex(){return g__LasteSelectedColorIndex;}

OutlineMain* outline(){return TheOutlineMain;};

#define GMAIL_HISTORY_CHECK_PERIOD_IN_MS    10000
#define ASYNC_CALL_CHECK_PERIOD_IN_MS       500

void gui::setupMainOutlineSelectRequestOnRebuild(topic_ptr it)
{
    ASSERT(outline(), "expected outline widget");
    if(outline() &&
       outline()->view() &&
       it)
        {
            if(IS_VALID_DB_ID(it->id()))
                {
                    outline()->view()->setupSelectRequestOnRebuild(it);
                }
        }
};

bool gui::wasSelectOnRebuildRequested() 
{
    ASSERT(outline(), "expected outline widget");
    if (outline() &&
        outline()->view())
    {
        return outline()->view()->wasSelectOnRebuildRequested();
    }
    return false;
};

/**
   OutlineMain
*/
OutlineMain::OutlineMain()
{
    setAccessibleName("Selector");

    TheOutlineMain = this;

    m_HighlightRequested = false;
    m_last_search_filter = "a";

    QBoxLayout* olayout = new QVBoxLayout;
    setLayout(olayout);
    utils::setupBoxLayout(olayout);

    m_outline_x_start = 0.0;

    m_oview = new OutlineView(this);

    m_scene = new OutlineScene(m_oview);
    m_scene->setMainScene(true);

    QWidget* outline_panel = new QWidget(this);
    QHBoxLayout* o_layout = new QHBoxLayout(outline_panel);
    utils::setupBoxLayout(o_layout);
    
    QVBoxLayout* lheader = new QVBoxLayout;
    utils::setupBoxLayout(lheader);
#ifdef ARD_BIG  
	buildToolbarsSet();
#endif
    lheader->addWidget(m_oview);    

    o_layout->addLayout(lheader);
    createRightTabs(o_layout);
#ifdef ARD_BIG 
    olayout->addWidget(outline_panel);
#else
    olayout->addWidget(outline_panel);
#endif
    
    m_email_check_timer.start(GMAIL_HISTORY_CHECK_PERIOD_IN_MS);
    connect(&m_email_check_timer, &QTimer::timeout, this, &OutlineMain::onEmailCheckTimer);
    m_delayed_asyn_call_timer.start(ASYNC_CALL_CHECK_PERIOD_IN_MS);
    connect(&m_delayed_asyn_call_timer, &QTimer::timeout, this, &OutlineMain::onCheckAsyncCall);
}

OutlineMain::~OutlineMain()
{
}

TabControl* OutlineMain::createFolderLocusTabSpace(QBoxLayout* l, EOutlinePolicy pol)
{
    auto tab = TabControl::createLocusedToobar(pol);
    l->addWidget(tab);
    connect(tab, &TabControl::tabSelected, [=](int d)
    {       
        auto curr_pol = gui::currPolicy();      
        bool handleNotification = (pol == curr_pol);
        if (!handleNotification) {
            auto parent_pol = PolicyCategory::parentPolicy(curr_pol);
            handleNotification = (parent_pol == pol);
        }

        if (handleNotification) {
            switch (pol) {
            case outline_policy_PadEmail:
            {
                if (ard::isDbConnected()) 
				{
					auto r = ard::db()->rmodel()->findRule(d);
					if (r) {
						dbp::configFileSetELabelHoisted(r->id());
						//if (ard::isGoogleConnected()) 
						auto cl = ard::google();
						if (cl)						
						{
							auto q = ard::db()->gmail_runner();
							if (q) {
								q->run_q_param(r);
							}
							gui::rebuildOutline(curr_pol, true);
							tab->update();
						}
					}
					else 
					{
						qWarning() << "mail rule not found" << d;
					}
                }
            }break;

            /////// hoisted folders /////////
            default: {
                auto f = ard::lookup(d);
                if (f) {
                    if (f != ard::hoisted()) {
                        gui::outlineFolder(f, pol);
                        reenableRightTabs();
                    }
                }
                else {
                    ASSERT(0, "failed to locate topic") << d;
                }
            }break;
            }
        }
    });

    return tab;
}


void OutlineMain::allocateRTabLocusSpace(EOutlinePolicy mainPolicy, 
    const std::vector<EOutlinePolicy>& plst, 
    QHBoxLayout* l, 
    TabControl* tc,
    QWidget* topWidget)
{
    QWidget* wgtRSpace = new QWidget;
    QVBoxLayout* vl1 = new QVBoxLayout;
    gui::setupBoxLayout(vl1);
    wgtRSpace->setLayout(vl1);
    auto tw = WrappedTabControl{ wgtRSpace, tc, {}, false };        
    
    l->addWidget(wgtRSpace);

    if (topWidget) {
        vl1->addWidget(topWidget);
    }

    if (tc) {
        vl1->addWidget(tc);
        connect(tc, &TabControl::tabSelected, this, &OutlineMain::secondaryTabChanged);
        auto h = tc->calcBoundingHeight();
        tc->setMaximumHeight(h);
        tc->setMinimumHeight(h);
        //tc->setScrollEnabled(false);
    }

    for (auto& pol : plst) {
        auto t2 = createFolderLocusTabSpace(vl1, pol);
        tw.locus_space[pol] = t2;
    }
    m_outline_secondary_tabs[mainPolicy] = tw;
};

void OutlineMain::allocateRTab(EOutlinePolicy pol, QHBoxLayout* l, TabControl* tc) 
{
    auto tw = WrappedTabControl{ tc, tc, {}, true };
    connect(tc, &TabControl::tabSelected, this, &OutlineMain::secondaryTabChanged);
    m_outline_secondary_tabs[pol] = tw;
    l->addWidget(tc);
};

void OutlineMain::createRightTabs(QHBoxLayout* l)
{
    allocateRTab(outline_policy_2SearchView, l, TabControl::createGrepTabControl());

    m_account_button = new AccountButton;
    m_account_button->updateAccountButton();


    allocateRTabLocusSpace(outline_policy_PadEmail, { outline_policy_PadEmail }, l, nullptr, m_account_button);
    allocateRTabLocusSpace(outline_policy_Pad, { outline_policy_Pad}, l, nullptr);
    allocateRTabLocusSpace(outline_policy_KRingTable, { outline_policy_KRingTable}, l, TabControl::createKRingTabControl());
};

qreal outline_top_z_value            = 0.0;

void OutlineMain::rebuild(bool force_rebuild_panels /*= false*/)
{
    if (force_rebuild_panels) {
        scene()->clearOutline();
        scene()->freePanels();
    }

    if (!gui::isDBAttached()) {
        scene()->clearOutline();
        QGraphicsPixmapItem* g = new QGraphicsPixmapItem(QPixmap(":ard/images/unix/unplugged.png"));
        QPoint pt_center((view()->viewport()->width() - 96) / 2, (view()->viewport()->height() - 96) / 2);
        QPointF pt_center2 = view()->mapToScene(pt_center);
        g->setPos(pt_center2);
        scene()->addItem(g);

        for (auto& t : m_outline_secondary_tabs) {
            ENABLE_OBJ(t.second.parent_wdg, false);
        }

        return;
    }

    EOutlinePolicy p = gui::currPolicy();
    if (PolicyCategory::isHoistedBased(p)) {
        auto h = model()->selectedHoistedTopic();
        assert_return_void(h, "expected current hoisted");
        scene()->attachHoisted(h);
    }

    updateMainWndTitle();
    view()->rebuild();
}

void OutlineMain::rebuildTabs() 
{
    for (auto& t : m_outline_secondary_tabs) {
        if (t.second.tab_ctrl) {
            t.second.tab_ctrl->rebuildTabs();
        }
    }
};

void OutlineMain::attachGui()
{
    assert_return_void(gui::isDBAttached(), "expected attached GUI");
    
    m_last_search_filter = dbp::configFileLastSearchFilter();

    if (ard::isGoogleConnected()) {
        ///todo we can proceed to google check/init here
        ///for example we can refresh labels
    }
    //updateThumbnailOutlineToolbarControls(true);
    rebuildLocusTabs();
}

void OutlineMain::detachGui()
{
    m_scene->showEditFrame(false);
    scene()->detachGui();
	for (auto t : m_outline_secondary_tabs) 
	{
		//if (t.second.tab_ctrl) {
		//	t.second.tab_ctrl->detachGui();
		//}
		for (auto t2 : t.second.locus_space) {
			if (t2.second && t2.second->hasLocusTopics()) {
				t2.second->detachGui();
			}
		}
	}
    rebuild();
};

ProtoGItem* OutlineMain::selectedGitem()
{
    return scene()->currentGI(false);
}


topic_ptr OutlineMain::selectedItem()
{
    topic_ptr it = nullptr;
    ProtoGItem* gi = selectedGitem();
    if(gi)
        {
            it = gi->topic();
        }
    return it;
}

ProtoGItem* OutlineMain::selectNext(bool go_up, ProtoGItem* gi)
{
    return scene()->selectNext(go_up, gi);
}

topic_ptr gui::selectNext(bool goUp)
{
    auto gi = outline()->selectNext(goUp);
    if (gi) {
        return gi->topic();
    }
    return nullptr;
};

void gui::renameSelected(EColumnType field2edit, bool selectContent)
{
    if (field2edit == EColumnType::Uknown ||
        field2edit == EColumnType::Title) {
        if (outline()->scene()->hasPanel()) {
            auto p = outline()->scene()->panel();
            if (p->lastActiveColumn() != EColumnType::Uknown) {
                field2edit = p->lastActiveColumn();
            }
        }
    }
    
    outline()->view()->renameSelected(field2edit, nullptr, selectContent);
};

void OutlineMain::updateMainWndTitle()
{
    QString sTopic = "";

    auto f = model()->selectedHoistedTopic();
    if(f)
        {
#define MAX_LEN 40
            sTopic = f->title().left(20);
            if(f->title().length() > MAX_LEN)
                sTopic += "..";
#undef MAX_LEN
        }
    gui::updateMainWndTitle(sTopic);
};

QString OutlineMain::getNewItemTitle(QString suggestedTitle)
{
    QString rv;
#ifndef QT_NO_CLIPBOARD
    QClipboard *clipboard = QApplication::clipboard();
    const QMimeData *md = clipboard->mimeData();
    if(md && md->hasText())
        {
            rv = md->text().left(128).trimmed();
        }
#endif

    if(rv.length() < 3)
        rv = suggestedTitle;
    return rv;
};

void ard::updateGItem(topic_ptr f) 
{
    assert_return_void(f, "expected topic");
    ProtoGItem* p = outline()->scene()->findGItem(f);
    if (p) {
        p->g()->update();
    }
};

#ifdef _DEBUG  
void OutlineMain::debugFunction()
{
    for (auto& t : m_outline_secondary_tabs) {
        ENABLE_OBJ(t.second.parent_wdg, false);
    }
};
#endif//_DEBUG 

void OutlineMain::onEmailCheckTimer() 
{
    extern QDateTime    gmail_last_check_time;
    if (!gmail_last_check_time.isValid()) {
		gmail_last_check_time = QDateTime::currentDateTime();
    }

    auto d2 = QDateTime::currentDateTime();
    auto msd = gmail_last_check_time.msecsTo(d2);
    if (msd > GMAIL_HISTORY_CHECK_PERIOD_IN_MS) 
    {
        check4new_mail(false);
    }
};

void OutlineMain::onCheckAsyncCall() 
{
    auto m = model();
    if (m) 
    {
        m->processDelayedAsyncCallRequests();
    }
};

bool gui_can_check4new_email()
{
    if (SyncPoint::isSyncRunning())
        return false;

    EOutlinePolicy main_pol = gui::currPolicy();
    if (main_pol != outline_policy_PadEmail)
        return false;

    extern QString gmail_access_last_exception;
    if (ard::isDbConnected()) {
        if (gui::isConnectedToNetwork()) {
            if (ard::hasGoogleToken()) {
                if (gmail_access_last_exception.isEmpty())
                {
                    auto q = ard::db()->gmail_runner();
                    if (q) {
                        if (q->canCheck4NewEmails()) {
                            return true;
                        }
                    }
                }
            }
        }
    }
    return false;
}

void OutlineMain::check4new_mail(bool deep_check)
{
    if (gui_can_check4new_email()) {
        auto q = ard::db()->gmail_runner();
        if (q) {
            if (q->canCheck4NewEmails()) {
                q->checkHistory(deep_check);
            }
        }
    }
};

void gui::invoke1(QString mName)
{
    std::string s_tmp = mName.toStdString();
    const char* tmp = s_tmp.c_str();
    static char _method_name[128] = "";
    strncpy(_method_name, tmp, sizeof(_method_name));
    QMetaObject::invokeMethod(outline(), 
                              _method_name,
                              Qt::QueuedConnection);
};

void OutlineMain::secondaryTabChanged(int d)
{
    EOutlinePolicy pol  = (EOutlinePolicy)d;
    model()->storeSecPolicySelection(pol);
    auto currPol = gui::currPolicy();
    if (pol != currPol){
        gui::rebuildOutline(pol, true);
    }
};


void OutlineMain::reenableRightTabs() 
{
    //    TabControl* t = nullptr;

    auto mpol = main_wnd()->mainTabPolicy();
    //t = policy2controllingTab(mpol);
    auto res = policy2rcontroll(mpol);
    QWidget* rctrl_wdg = nullptr;
    TabControl* rctrl_tab = nullptr;
    if (res.first) {
        rctrl_wdg = res.second.parent_wdg;
        rctrl_tab = res.second.tab_ctrl;
    }

    for (auto& t2 : m_outline_secondary_tabs) {
        if (t2.second.parent_wdg != rctrl_wdg) {
            ENABLE_OBJ(t2.second.parent_wdg, false);
        }
        else {
            if (rctrl_wdg) {
                ENABLE_OBJ(rctrl_wdg, true);
                auto cp = gui::currPolicy();
                if (rctrl_tab) {
                    rctrl_tab->selectTabByData(cp);
                }
                if (t2.second.locus_space.size() > 1) {
                    for (auto it : t2.second.locus_space) {
                        if (it.first == cp) {                         
                                ENABLE_OBJ(it.second, true);
                        }
                        else {
                            ENABLE_OBJ(it.second, false);
                        }
                    }
                }
            }
        }
    }

};

std::pair<bool, WrappedTabControl> OutlineMain::policy2rcontroll(EOutlinePolicy pol)
{
    //TabControl* tb = nullptr;
    std::pair<bool, WrappedTabControl> rv;
    rv.first = false;

#define TAKE_RTAB(P)            auto it = m_outline_secondary_tabs.find(P);\
    if (it != m_outline_secondary_tabs.end()) {\
        rv.second = it->second;\
        rv.first = true;\
    }\

    switch (pol)
    {
    case outline_policy_2SearchView:
    //ykh-tr case outline_policy_TaskRing:
	//case outline_policy_Colored:
    case outline_policy_Notes:
	case outline_policy_Bookmarks:
	case outline_policy_Pictures:
    case outline_policy_Annotated:    
    {
        TAKE_RTAB(outline_policy_2SearchView);
    }break;

    case outline_policy_KRingTable:
    case outline_policy_KRingForm:
    {
        TAKE_RTAB(outline_policy_KRingTable);
    }break;

    case outline_policy_Pad:
    {
        TAKE_RTAB(outline_policy_Pad);
    }break;
    case outline_policy_PadEmail: 
    {
        TAKE_RTAB(outline_policy_PadEmail);
    }break;

    default:break;
    }

#undef TAKE_RTAB

    return rv;
};

std::list<TabControl*> OutlineMain::locusedTabSpace(const std::set<EOutlinePolicy>& pset)
{
    std::list<TabControl*> rv;
    for (auto& wc : m_outline_secondary_tabs) {
        for (auto& p : pset) {
            auto it = wc.second.locus_space.find(p);
            if (it != wc.second.locus_space.end()) {
                rv.push_back(it->second);
            }
        }
    }
    return rv;
};



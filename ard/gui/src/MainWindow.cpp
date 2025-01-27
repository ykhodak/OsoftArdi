#include <QtGui>
#include <QSplitter>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFileDialog>
#include <QDate>
#include <QMenuBar>
#include <QApplication>
#include <QToolBar>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QScrollBar>
#include <QProgressDialog>
#include <QThread>
#include <QTextCursor>
#include <QInputDialog>
#include <QProgressBar>
#include <QStatusBar>
#include <QPushButton>
#include <QListView>
#include <QButtonGroup>
#include <QRadioButton>
#include <QCheckBox>
#include <QShortcut>
#include <QSystemTrayIcon>

#include <iostream>
#include "MainWindow.h"
#include "dbp.h"
#include "ardmodel.h"
#include "OutlineMain.h"
#include "anGItem.h"
#include "anfolder.h"
#include "aboutbox.h"
#include "registerbox.h"
#include "utils.h"
#include "utils-img.h"
#include "ansyncdb.h"

#include "OutlineScene.h"
#include "syncpoint.h"
#include "importdir.h"
#include "SearchBox.h"
#include "OpenDatabase.h"
#include "LogView.h"
#include <assert.h>
#include "TabControl.h"
#include "search_edit.h"
#include "NoteToolbar.h"
#include "email.h"
#include "ethread.h"
#include "contact.h"
#include "kring.h"
#include "OutlinePanel.h"
#include "ansearch.h"
#include "locus_folder.h"
#include "popup-widgets.h"
#include "csv-util.h"
#include "db-merge.h"
#include "custom-boxes.h"
#include "ADbgTester.h"
#include "BlackBoard.h"
#include "address_book.h"
#include "rule.h"
#include "rule_runner.h"
#include "rule_dlg.h"

#define ADD_ACT(M, N, T) a  = new QAction(N, this);M->addAction(a);connect(a, SIGNAL(triggered()), this, SLOT(T()));
#define ADD_L_ACT(M, N, L) a  = new QAction(N, this);M->addAction(a);connect(a, &QAction::triggered, L);

extern QString configFilePath();

#ifdef ARD_BETA
bool checkBetaExpire();
#endif

/**
   color-theme
*/
extern QPalette default_palette;
static QString default_menu_style_sheet;
struct ColorThemeEntry
{
public:

    ColorThemeEntry(){};
    ColorThemeEntry(QColor _fore, QColor _back, QString _label, QString _ss):
        fore(_fore), back(_back), label(_label), style_sheet(_ss){};

    QColor fore, back;
    QString label;
    QString style_sheet;
};

static QColor curr_bk_color;

void initArdiInstance() 
{
    color::prepareColors();
};


class main_splitter_btn : public QWidget
{
public:
	main_splitter_btn(MainWindow* w):m_wnd(w){ setFixedHeight(80); }
protected:
	void mousePressEvent(QMouseEvent * e)override;
	void paintEvent(QPaintEvent *e)override;

	MainWindow* m_wnd;
};

void main_splitter_btn::mousePressEvent(QMouseEvent * e) 
{
	m_wnd->toggleSplitterButton();
	QWidget::mousePressEvent(e);
};

void main_splitter_btn::paintEvent(QPaintEvent *) 
{
	QRect rc = rect();
	QPainter p(this);

	auto clr = qRgb(64, 64, 64);
	QPen pen(clr);
	QBrush brush(clr);
	p.setPen(pen);
	p.setBrush(brush);
	p.drawRect(rc);
};

/**
   MainWindow
*/
static MainWindow* TheMainWnd = nullptr;
static ArdModel* TheModel = nullptr;

QWidget* ard::mainWnd() { return TheMainWnd; };
QWidget* gui::mainWnd(){return TheMainWnd;};
MainWindow* main_wnd(){return TheMainWnd;}
ArdModel* model(){return TheModel;};

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent)   
{
    setAccessibleName("Main");

    TheMainWnd = this;
    //m_ExteriorVisible = true;   
    dbp::loadFileSettings();

    if(!utils::resetFonts())
        {
            ASSERT(0, "utils-init error");
        };

    TheModel = new ArdModel;
    //installColorThemes();

  
	m_selector_panel = new QWidget;
    m_selector_box = new QVBoxLayout(m_selector_panel);
    utils::setupBoxLayout(m_selector_box);


    m_tab_box = new QHBoxLayout();
    utils::setupBoxLayout(m_tab_box);
	m_selector_box->addLayout(m_tab_box);

	m_outline_main = new OutlineMain;

    m_mainTab = TabControl::createMainTabControl();
    connect(m_mainTab, &TabControl::tabSelected,
            this, &MainWindow::mainTabChanged);
    connect(m_mainTab, &TabControl::currentTabClicked,
        this, &MainWindow::mainCurrentTabClicked);

    m_tab_box->addWidget(m_mainTab);
    
    m_tab_box->addWidget(m_outline_main);


	m_splitter = new QSplitter;
	m_wspace = new ard::workspace;
	m_splitter->addWidget(m_selector_panel);
	m_splitter->addWidget(m_wspace);
	setCentralWidget(m_splitter);

	QSplitterHandle *handle = m_splitter->handle(1);
	auto layout = ard::newLayout<QVBoxLayout>(handle);
	layout->addWidget(ard::newSpacer(true));
	layout->addWidget(new main_splitter_btn(this));
  
    setupMenu();
  
    curr_bk_color = default_palette.color(QPalette::Window);
#ifdef ARD_POPUP
#ifdef Q_OS_WIN
    setWindowFlags(Qt::Window | Qt::WindowMinimizeButtonHint | Qt::WindowCloseButtonHint | Qt::CustomizeWindowHint);
#else
    setWindowFlags(Qt::WindowTitleHint | Qt::WindowMinimizeButtonHint | Qt::WindowSystemMenuHint);
#endif
#endif //ARD_POPUP

	showMaximized();
}

void MainWindow::showEvent(QShowEvent * event)
{
    QMainWindow::showEvent(event);
    static bool firstCall = true;
    if (firstCall)
    {
#ifdef ARD_BETA
        if (!checkBetaExpire())
        {
            exit(0);
            return;
        }
#endif

        outline()->reenableRightTabs();

		if (dbp::safeGuiOpenStandardPath(dbp::configFileGetLastDB()))
		{
			//QTimer::singleShot(100, [=]() {restoreSplitter(); });
		}
		else 
		{
			for (auto& t : m_toolbars) {
				t->hide();
			}
		}

        firstCall = false;
    }
};

void MainWindow::resizeEvent(QResizeEvent *)
{
#ifdef _DEBUG
	gui::updateMainWndTitle("");
	//auto sz = frameGeometry().size();
	//auto s = QString("%1-%2X%3").arg(programName()).arg(sz.width()).arg(sz.height());
	//setWindowTitle(s);
#endif
};

void MainWindow::restoreSplitter() 
{
	QString sfile = configFilePath();
	QSettings settings(sfile, QSettings::IniFormat);

	auto w = settings.value("selector-width").toInt();
	qDebug() << "<<------ restoreSplitter" << w;
	if (w < DEFAULT_MAIN_WND_WIDTH - 100) {
		w = DEFAULT_MAIN_WND_WIDTH;
	}
	if (w > 3*DEFAULT_MAIN_WND_WIDTH) {
		w = DEFAULT_MAIN_WND_WIDTH;
	}


	auto spl_sizes = m_splitter->sizes();
	if (spl_sizes.size() > 1)
	{
		auto v = spl_sizes[0];
		if (v != w)
		{
			auto d = v - w;
			spl_sizes[0] = w;
			spl_sizes[1] = spl_sizes[1] + d;
		}
	}
	m_splitter->setSizes(spl_sizes);
};

void MainWindow::moveSplitter(int delta) 
{
	auto spl_sizes = m_splitter->sizes();
	//qDebug() << "move sizes1:" << delta << spl_sizes;
	if (spl_sizes.size() > 1)
	{
		spl_sizes[0] = spl_sizes[0] + delta;
		spl_sizes[1] = spl_sizes[1] - delta;
	}
	m_splitter->setSizes(spl_sizes);
};

void MainWindow::toggleSplitterButton() 
{
	auto spl_sizes = m_splitter->sizes();
	if (spl_sizes.size() > 1)
	{
		int delta = 0;
		auto w = spl_sizes[0];
		if (w > 0) {
			delta = w;
			m_normal_splitter_width = w;
		}
		else {
			delta = -m_normal_splitter_width;
		}
		spl_sizes[0] = spl_sizes[0] - delta;
		spl_sizes[1] = spl_sizes[1] + delta;

		m_splitter->setSizes(spl_sizes);
	}	
};

void MainWindow::closeInstance() 
{
	m_quit_requested = true; 
	prepareToClose(); 
	close();
};

void MainWindow::prepareToClose() 
{
	auto sfile = configFilePath();
	QSettings settings(sfile, QSettings::IniFormat);

	auto spl_sizes = m_splitter->sizes();
	auto w = DEFAULT_MAIN_WND_WIDTH;
	if (spl_sizes.size() > 0)
	{
		w = spl_sizes[0];
	}
	if (w < DEFAULT_MAIN_WND_WIDTH - 100) {
		w = DEFAULT_MAIN_WND_WIDTH;
	}
	settings.setValue("selector-width", w);
	//ard::save_all_popup_content();
	//detachMainWindowGui();
	dbp::close();
};

void MainWindow::closeEvent(QCloseEvent *e)
{
#ifdef Q_OS_MACOS
    if (!e->spontaneous() || !isVisible()) {
        return;
    }
#endif

    if (!m_quit_requested && dbp::configFileGetRunInSysTray()) {
        if (m_sys_tray) {
            hide();
            e->ignore();
            return;
        }
    }

	prepareToClose();

    QMainWindow::closeEvent(e);
};

MainWindow::~MainWindow()
{
    utils::clean();

    if(TheModel){
        delete TheModel;
        TheModel = nullptr;
    }

    TheMainWnd = nullptr;
}

void MainWindow::setupMenu()
{
    if(!is_big_screen())
        return;
  
    QMenu *file     = menuBar()->addMenu("&File");
    QMenu *edit     = menuBar()->addMenu("&Edit");
    QMenu *view     = menuBar()->addMenu("&View");
    QMenu *help     = menuBar()->addMenu("&Help");

    default_menu_style_sheet = file->styleSheet();

    QAction *a = nullptr;

    ADD_L_ACT(file, "Synchronize", [&]()
    {
        gui::startSync();
    });

    file->addSeparator();
	/*
    ADD_ACT(file, "Open", fileOpen);
    if (dbp::configFileSupportCmdLevel() > 0)
        {            
            ADD_ACT(file, "Close", fileClose);
            //ADD_ACT(file, "Password", filePassword);
        }
		*/
   // ADD_ACT(file, "Backup", fileBackup);
    ADD_ACT(file, "Import Bookmarks", importBookmarks);
#ifdef _DEBUG  
    file->addSeparator();
    ADD_ACT(file, "Import C-Project", importCProject);  
#endif  
    ADD_ACT(file, "Import Text Files", importTextFiles);
    ADD_ACT(file, "Import Contacts (Outlook CSV)", importContactsCvs);
    //ADD_ACT(file, "Import Ardi DB (Import & Merge)", importNativeDB);
    file->addSeparator();
    ADD_L_ACT(file, "Quit", [&]()
    {
		closeInstance();
    });

    ADD_ACT(help, "&About", helpAbout);

    ADD_ACT(help, "&Support Window", supportWindow);
    ADD_ACT(help, "Clear Data Cache", clearGmailCache);
#ifdef _DEBUG  
    ADD_ACT(help, "&SQL", sqlWindow);
#endif

    if(edit)
        {
            a = new QAction("Find", this);
            edit->addAction(a);
            connect(a, SIGNAL(triggered()), outline(), SLOT(toggleGlobalSearch()));

            a = new QAction("Create from Clipboard", this);
            edit->addAction(a);
            connect(a, &QAction::triggered, []() {outline()->createFromClipboard(); });

			/*
            a = new QAction("Open Selected", this);
            edit->addAction(a);
            connect(a, &QAction::triggered, []() {outline()->processCommandOpenNotes(); });
			*/

            a = new QAction("Format Selected Notes", this);
            edit->addAction(a);
            connect(a, &QAction::triggered, []() {outline()->processCommandFormatNotes(); });

            /*
            a = new QAction("Replace", this);
            edit->addAction(a);
            connect(a, SIGNAL(triggered()), outline(), SLOT(toggleGlobalReplace()));
            */

            makeOutlineMainActions(edit);
            //makeTextActions(edit);
        }    

    if(view)
        {
			ADD_ACT(view, "&Mail Rules", viewRules);
			view->addSeparator();
			ADD_ACT(view, "&Contacts", viewContacts);
#ifdef _DEBUG
			ADD_ACT(view, "&Contact Groups", viewContactGroups);
#endif
			view->addSeparator();
            ADD_ACT(view, "Notes &Folders", viewFolders);
            //ADD_ACT(view, "&Threads", viewThreads);
            //ADD_ACT(view, "&Backups", viewBackups);
            view->addSeparator();
            ADD_L_ACT(view, "Account", [&]()
            {
				ard::accounts_dlg::runIt();
            });
            ADD_L_ACT(view, "&Options", [&]()
            {
                OptionsBox::showOptions();
            });
            view->addSeparator();
            ADD_ACT(view, "L&og", viewLog);
            
#ifdef _DEBUG
            /*
            QMenu *m_color = view->addMenu("Color Themes..");
            a = new QAction("Default", this);
            a->setData("color-theme-default");
            m_color->addAction(a);
            m_color->addSeparator();
            for(auto& i : color_theme_order)
                {
                    THEME_2_COLORS::iterator j = color_themes.find(i);
                    if(j != color_themes.end())
                        {
                            QAction* a = new QAction(j->second.label, this);
                            a->setData(j->first);
                            m_color->addAction(a);      
                        }
                }
            connect(m_color, SIGNAL(triggered(QAction*)), this, SLOT(toggleColorTheme(QAction*)));
            */
#endif//_DEBUG for color theme menus..
            
            ADD_ACT(view, "&Properties", viewProperties);

        }

#ifdef _DEBUG     
    help->addSeparator();
    a  = new QAction("&Debug-1", this);
    help->addAction(a);
    connect(a, SIGNAL(triggered()), this, SLOT(debugFunction()));
    a  = new QAction("&Debug-2", this);
    help->addAction(a);
    connect(a, SIGNAL(triggered()), this, SLOT(debugFunction2()));
#endif

    updateMenu();
    setupSystemTrayIcon();

    /// global shortcuts
    QShortcut *sh = new QShortcut(QKeySequence("Alt+1"), this);
    sh->setContext(Qt::ApplicationShortcut);
    connect(sh, &QShortcut::activated, []() {
        ard::focusOnOutline();
    });
    sh = new QShortcut(QKeySequence("Alt+2"), this);
    sh->setContext(Qt::ApplicationShortcut);
    connect(sh, &QShortcut::activated, []() {
        ard::focusOnTSpace();
    });
    sh = new QShortcut(QKeySequence("Alt+z"), this);
    sh->setContext(Qt::ApplicationShortcut);
    connect(sh, &QShortcut::activated, []() {
        ADbgTester::test_print_app_state();
    });
    sh = new QShortcut(QKeySequence("Alt+q"), this);
    sh->setContext(Qt::ApplicationShortcut);
    connect(sh, &QShortcut::activated, []() {
        if (outline()) {
            outline()->createFromClipboard();
        };
    });
}

void MainWindow::setupSystemTrayIcon() 
{
    /// systray
    if (QSystemTrayIcon::isSystemTrayAvailable()) 
    {
        if (dbp::configFileGetRunInSysTray())
        {
            if (!m_sys_tray)
            {
                m_sys_tray = new QSystemTrayIcon(QIcon(":ard/images/unix/ard-icon-48x48"), this);
                auto m = new QMenu(this);
				auto a = new QAction(tr("Paste into Selector"), this);
				connect(a, &QAction::triggered, this, &MainWindow::pasteIntoSelector);
				m->addAction(a);

				a = new QAction(tr("Paste into Board"), this);
				connect(a, &QAction::triggered, this, &MainWindow::pasteIntoBoard);
				m->addAction(a);

                a = new QAction(tr("&Quit Ardi"), this);
				connect(a, &QAction::triggered, this, &MainWindow::closeInstance);
                //connect(a, &QAction::triggered, qApp, &QCoreApplication::quit);
                m->addAction(a);
                m_sys_tray->setContextMenu(m);
                connect(m_sys_tray, &QSystemTrayIcon::activated, [=](QSystemTrayIcon::ActivationReason r)
                {
                    if (r == QSystemTrayIcon::Trigger) {
                        bool main_visible = isVisible();
                        if (main_visible) 
                        {
                            auto st = windowState();
                            if (st == Qt::WindowMinimized)
                            {
                                show();
                                QTimer::singleShot(300, [=]()
                                {
                                    raise();
                                });
                            }
                            else {
                                hide();
                            }
                        }
                        else 
                        {
                            show();
                            show();
                            QTimer::singleShot(500, [=]() 
                            {
                                raise();
                            });
                        }
                    }
                });
                m_sys_tray->show();
            }
            else{
                if (!m_sys_tray->isVisible()){
                    m_sys_tray->show();
                }
            }
        }
        else 
        {
            if (m_sys_tray) {
                if (m_sys_tray->isVisible()){
                    m_sys_tray->hide();
                }
            }
        }
    };
};

void setupSystemTrayIcon()
{
    TheMainWnd->setupSystemTrayIcon();
}

void MainWindow::pasteIntoSelector() 
{
	outline()->createFromClipboard(ard::Sortbox());
};

void MainWindow::pasteIntoBoard() 
{
	assert_return_void(ard::isDbConnected(), "expected connected DB");

	const QMimeData *mm = nullptr;
	if (qApp->clipboard()) {
		mm = qApp->clipboard()->mimeData();
	}
	if (!mm) {
		return;
	}

	ard::selector_board* bb = nullptr;
	auto bpage = ard::wspace()->currentPageAs<ard::BlackBoard>();
	if (bpage) {
		bb = bpage->board();
	}

	if (!bb) {
		auto br = ard::db()->boards_model()->boards_root();
		if (br) {
			if (br->items().empty()) {
				bb = br->addBoard();
			}
			else{
				bb = dynamic_cast<ard::selector_board*>(br->items()[0]);
			}
		}
	}

	if (bb) 
	{
		int dest_band_idx = 0;
		int dest_ypos = 0;
		if (bpage)
		{
			auto g = bpage->firstSelected();
			if (g)
			{
				dest_band_idx = g->bandIndex();
				dest_ypos = g->ypos() + 1;
			}
		}

		auto h = bb->ensureOutlineTopicsHolder();
		if (h) {
			auto lst = ard::insertClipboardData(h, h->items().size(), mm, false);
			if (!lst.empty())
			{
				auto f = *lst.begin();
				//TOPICS_LIST lst;
				//lst.push_back(f);

				auto blst = bb->insertTopicsBList(lst, dest_band_idx, dest_ypos);
				if (bpage) {
					if (!blst.first.empty()) {
						std::unordered_set<int> b2r;
						b2r.insert(dest_band_idx);
						bpage->resetBand(b2r);
						bpage->ensureVisible(f);
					};
				}
			}
		}
	}
};

void MainWindow::makeOutlineMainActions(QMenu *menuEdit)
{
    if(!is_big_screen())
        return;
  
#define ADD_ACTION(A, H)   {QAction* _act = A;                      \
        connect(_act, SIGNAL(triggered()), outline(), SLOT(H()));   \
        menuEdit->addAction(_act);                                  \
        outlineActions.push_back(_act);}                            \

    menuEdit->addSeparator();
    ADD_ACTION(utils::actionFromTheme("ard-erase", tr("Remove"), this), toggleRemoveItem);
#undef ADD_ACTION
};

void MainWindow::updateMenu()
{
    if(is_big_screen())
        return;
  
#define ENABLE_OUTLINE_ACTIONS(V) ENABLE_OBJ_LIST(outlineActions, V);
#define ENABLE_TEXT_ACTIONS(V)     

    ENABLE_OUTLINE_ACTIONS(true);
    ENABLE_TEXT_ACTIONS(false);
};

bool _ArdGuiAttached = false;

void MainWindow::detachMainWindowGui()
{
	dbp::configFileSetBornAgainPolicy(gui::currPolicy());
    selectOutlineView(outline_policy_Uknown);//detach view
    outline()->detachGui();
    model()->detachModelGui();
	m_wspace->detachGui();

    _ArdGuiAttached = false;
};

void MainWindow::attachGui()
{
    if (ard::autotestMode())
        return;

    assert_return_void(dbp::root(), "expected valid root folder");
	auto d = ard::db();
	if (d && d->isOpen()) {
		d->rmodel()->rroot()->resetRules();
	}
    outline()->attachGui();
    model()->attachGui();
    _ArdGuiAttached = true;

    if (m_requestSyncAfterGuiAttached) 
    {
        m_requestSyncAfterGuiAttached = false;
        gui::startSync();
    }
    else 
    {
        gui::rebuildOutline(dbp::configFileBornAgainPolicy());
		/*
        QTimer::singleShot(400, [=]() {
            dbp::configRestoreCurrentTopic();
            ard::focusOnOutline();
        });
		*/
		m_wspace->restoreTabs();		
		QTimer::singleShot(700, [=]() {restoreSplitter(); });
    }    
};

void MainWindow::finalSaveCall()
{
    if(gui::isDBAttached())
        {
            dbp::configFileSetLastSearchFilter(outline()->m_last_search_filter);
			wspace()->storeTabs();
        }
};

void gui::detachGui()
{
    if (main_wnd()->thread()!=QThread::currentThread()) {
        QMetaObject::invokeMethod(main_wnd(), "detachMainWindowGui", Qt::BlockingQueuedConnection);
    }
    else{
        main_wnd()->detachMainWindowGui();
    }
};

void gui::finalSaveCall()
{
    if (main_wnd()->thread()!=QThread::currentThread()) {
        QMetaObject::invokeMethod(main_wnd(), "finalSaveCall", Qt::BlockingQueuedConnection);
    }
    else
    {
        dbp::configStoreCurrentTopic();
        main_wnd()->finalSaveCall();
    }
};

void gui::attachGui()
{
    if (main_wnd()->thread()!=QThread::currentThread()) {
        QMetaObject::invokeMethod(main_wnd(), "attachGui", Qt::BlockingQueuedConnection);
    }
    else{
        main_wnd()->attachGui();
    }
};

void ard::focusOnOutline()
{
    if (ard::autotestMode())
        return;

    QTimer::singleShot(200, [=]() 
    {
        if (outline()) 
        {
            outline()->requestSelectorFocus();
            qApp->setActiveWindow(outline());
        }
    });
};

void ard::focusOnTSpace()
{
    if (ard::autotestMode())
        return;

    QTimer::singleShot(200, [=]() {
		{
            auto ts = ard::wspace();
            if (ts) {
               // qApp->setActiveWindow(ts);
//                auto fcurr = ard::currentTopic();
                auto pg = ts->currentPage();
                if (pg) {
                    //if (pg->isSlideLocked() || pg->topic() == fcurr || !fcurr)
                    {
                        ts->setFocusOnContent();
                    }
                }
            }
        }
    });
};

void MainWindow::registerToolbar(QToolBar* t)
{
	m_selector_box->insertWidget(0, t);
	//m_selector_box->addWidget(t);
   // addToolBar(Qt::BottomToolBarArea, t);
    m_toolbars.push_back(t);
    t->setFloatable(false);
    t->setMovable(false);
    //t->setToolButtonStyle(Qt::ToolButtonIconOnly/*Qt::ToolButtonTextUnderIcon*/);
    t->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
};

void MainWindow::hideAllToolbars()
{
    hideListedToolbarsExcept(nullptr);
};


void MainWindow::hideListedToolbarsExcept(QToolBar* exceptThis)
{
    for (auto& t : m_toolbars)
    {
        if (t == exceptThis)
        {
            ENABLE_OBJ(t, true);
        }
        else
        {
            ENABLE_OBJ(t, false);
        }
    }
};

void MainWindow::updateToolbars()
{
#define POLICY2TBAR(P, T)case P: hideListedToolbarsExcept(T);break;

    auto p = gui::currPolicy();
    switch (p)
    {
        POLICY2TBAR(outline_policy_PadEmail, m_email_pad_toolbar);
        POLICY2TBAR(outline_policy_BoardSelector, m_board_select_toolbar);
        POLICY2TBAR(outline_policy_Pad, m_outline_toolbar);
        POLICY2TBAR(outline_policy_KRingTable, m_kring_table_toolbar);
        POLICY2TBAR(outline_policy_KRingForm, m_kring_form_toolbar);
        POLICY2TBAR(outline_policy_2SearchView, m_grep_toolbar);
        POLICY2TBAR(outline_policy_TaskRing, m_task_ring_toolbar);
        POLICY2TBAR(outline_policy_Notes, m_notes_toolbar);
		POLICY2TBAR(outline_policy_Bookmarks, m_bookmarks_toolbar);
		POLICY2TBAR(outline_policy_Pictures, m_pictures_toolbar);
        POLICY2TBAR(outline_policy_Annotated, m_annotate_toolbar);
        POLICY2TBAR(outline_policy_Colored, m_color_toolbar);
    default:
    {
        hideAllToolbars();
    }break;
    }
#undef POLICY2TBAR

};

void gui::outlineFolder(topic_ptr f, 
                        EOutlinePolicy pol)
{
    assert_return_void(f, "expected topic");
    ASSERT_VALID(f);
#ifdef _DEBUG
    ASSERT(f->checkTreeSanity(), "sanity test failed");
#endif

    if(pol == outline_policy_Uknown)
        {
            pol = gui::currPolicy();
            if(pol == outline_policy_Uknown)
                {
                    pol = outline_policy_Pad;
                }
        }

    model()->setSelectedHoistedTopic(f);
    main_wnd()->selectOutlineView(pol);
    model()->setAsyncCallRequest(AR_freePanelsAndRebuildOutline);
};

bool gui::ensureVisibleInOutline(topic_ptr it/*, EOutlinePolicy enforcePolicy*/)
{
    assert_return_false(it, "expected topic");
    assert_return_false(ard::isDbConnected(), "expected open DB");

	ard::trail(QString("ensure-visible[%1][%2][%3][%4]").arg(it->objName()).arg(it->id()).arg(it->wrappedId()).arg(it->title().left(20)));
	//arg(policy2name(enforcePolicy))

	EOutlinePolicy enforcePolicy = outline_policy_Uknown;

    QString eid2locate = "";
    auto et = dynamic_cast<ethread_ptr>(it);
    if (!et) {
        auto e = dynamic_cast<email_ptr>(it);
        if (e) {
            et = e->parent();
			if (!et) {
				auto o = e->optr();
				assert_return_false(o, "invalid email object");
				assert_return_false(et, QString("expected parent thread [%1][%2]").arg(o->subject().left(20)).arg(o->id()));
			}
        }
    }
    if (et) {
        bool locateByMessageId = true;
        if (locateByMessageId) {            
            eid2locate = et->wrappedId();
            enforcePolicy = outline_policy_PadEmail;
        }
    }

    bool item_located = false;

    LOCK(it);
	it->rollUpToRoot();

    //EOutlinePolicy pol_orig = gui::currPolicy();
    EOutlinePolicy pol2switch = outline_policy_Pad;
    if (outline_policy_Uknown == enforcePolicy)
    {
		auto b = dynamic_cast<ard::selector_board*>(it);
		if (b) {
			pol2switch = outline_policy_BoardSelector;
		}

		topic_ptr h = it->getLocusFolder();
		if (et) {
			h = ard::Sortbox();
		}
		if (h && h != model()->selectedHoistedTopic()) {
			model()->setSelectedHoistedTopic(h);
		}
		gui::rebuildOutline(pol2switch, true);
		/*else 
		{
			switch (pol_orig)
			{
			case outline_policy_Pad:
			case outline_policy_KRingTable:
				break;
			default:
				pol2switch = outline_policy_Pad;
			}
		}*/
    }
    else
    {
        pol2switch = enforcePolicy;
		ASSERT(et, "expected email thread");
		ASSERT(pol2switch == outline_policy_PadEmail, "expected email policy");

		//////..........
		//auto r = ard::db()->rmodel()->findRule(static_cast<int>(ard::static_rule::etype::all));
		auto r = ard::db()->rmodel()->findFirstRule(et);
		if (r) {
			dbp::configFileSetELabelHoisted(r->id());
			auto cl = ard::google();
			if (cl)
			{
				auto q = ard::db()->gmail_runner();
				if (q) {
					q->run_q_param(r);
				}
				gui::rebuildOutline(pol2switch, true);
			}
		}
		else
		{
			qWarning() << "mail all-rule not found";
		}

		///////.........

    }


    if (eid2locate.isEmpty())
    {
        item_located = outline()->scene()->ensureVisible2(it);
        ASSERT(item_located, "item is not located") << it->dbgHint();
        if (!item_located) {
            item_located = outline()->scene()->ensureVisible2(it);
        }
    }
    else
    {
        item_located = outline()->scene()->ensureVisibleByEid(eid2locate);
        if (!item_located) 
        {
            if (pol2switch == outline_policy_PadEmail) 
            {
				qWarning() << "thread injection disabled";
				/*
                auto q = ard::db()->gmail_q();
                if (q) {
                    qDebug() << "wanna inject thread" << eid2locate;
                    q->inject_thread(eid2locate);
                    gui::rebuildOutline(pol2switch, true);
                    item_located = outline()->scene()->ensureVisibleByEid(eid2locate);
                }*/
            }
        }
        
        ASSERT(item_located, "item is not located by EID") << eid2locate;
    }
    it->release();
#ifdef ARD_BIG
    //if (!gui::mainWnd()->isVisible()) 
    {
   //     gui::mainWnd()->showNormal();
		gui::mainWnd()->show();
        gui::mainWnd()->raise();
    }
    //gui::mainWnd()->showNormal();
    //gui::mainWnd()->raise();
#endif
    return item_located;
};

void MainWindow::selectOutlineView(EOutlinePolicy op)
{
    if(gui::currPolicy() != op)
        {
            m_prev_policy = m_curr_policy;
            m_curr_policy = op;

            if (op != outline_policy_Uknown) {
                setupMainTabPolicy(op);
            }

            outline()->scene()->setOutlinePolicy(op);
            updateMenu();
            hideAllToolbars();
        }

    outline()->reenableRightTabs();
    updateToolbars();
}

void MainWindow::outlinePolicyChanged()
{
    QAction* a = qobject_cast<QAction *>(QObject::sender());
    if (a)
    {
        WaitCursor w;
        EOutlinePolicy op = (EOutlinePolicy)a->data().toInt();
        gui::rebuildOutline(op, true);
    }//action
};


void MainWindow::helpAbout()
{
    model()->onIdle();
    new AboutBox;
};

void MainWindow::supportWindow()
{
    SupportWindow::runWindow();
};

void MainWindow::sqlWindow()
{
    gui::runSQLRig();
};

void MainWindow::clearGmailCache() 
{
    if (ard::confirmBox(this, "Delete local Gmail cache data file? No actual emails will be deleted.")) {
        auto r = ard::deleteGoogleCache();
        if (r && PolicyCategory::isGmailDepending(gui::currPolicy())) {
            gui::rebuildOutline(gui::currPolicy(), true);
        }
    }
};

void MainWindow::viewProperties()
{
    ArdDB* db = ard::db();
    auto f = ard::currentTopic();
    if (f && db)
    {
        f = f->shortcutUnderlying();
        if (f) {
            ard::asyncExec(AR_ViewProperties, f);
        }
    }
};

void MainWindow::viewLog()
{
    LogView::runIt();
};

void MainWindow::viewFolders()
{
	ard::folders_dlg::showFolders();
};

void MainWindow::viewRules() 
{
	ard::rules_dlg::run_it();
};

void MainWindow::viewThreads() 
{
    AdoptedThreadsBox::showThreads();
};

void MainWindow::viewContacts()
{
	ard::address_book_dlg::open_book();
}

void MainWindow::viewContactGroups() 
{
	ard::contact_groups_dlg::runIt();
};

void MainWindow::viewBackups()
{
    BackupsBox::showBackups();
};

void gui::startSync()
{
    if (!ard::isDbConnected()) {
        ard::messageBox(gui::mainWnd(),"Database file is not open.");
        return;
    }

    topic_ptr r = ard::root();
    if (r)
    {
        r->deleteEmptyTopics();
        gui::rebuildOutline();
    }

#ifdef ARD_OPENSSL
    if (!dbp::configFileIsSyncEnabled()) {
        if (ard::confirmBox(this, "Enable Cloud based synchronization? You will have set password to ensure strong encryption.")) {
            //dbp::configFileSetSyncEnabled(true);
            ard::CryptoConfig& cfg = ard::CryptoConfig::cfg();
            if (!cfg.hasPassword() && !cfg.hasPasswordChangeRequest()) {
                SyncPasswordBox::changePassword();
            }
        }
        else {
            return;
        }
    }

    ard::CryptoConfig& cfg = ard::CryptoConfig::cfg();
    if (!cfg.hasPassword() && !cfg.hasPasswordChangeRequest()) {
        if (!SyncPasswordBox::changePassword()) {
            ard::errorBox(this, ("Please provide password before starting cloud synchronization.");
            return;
        };
    }


    if (!cfg.hasPassword() && !cfg.hasPasswordChangeRequest()) {
        ard::errorBox(this, ("Please provide password before starting cloud synchronization.");
        return;
    }
#endif

#ifdef _DEBUG
    int def_c = 1;

    std::vector<QString> lst;
    lst.push_back("Cancel");
    lst.push_back("Local Sync");
    lst.push_back("Google GDrive Sync");
    QString msg = QString("Select synchronization type [%1]").arg(remoteDBPath4LocalSync());

    int c = utils::choice(lst, def_c, msg);
    switch (c)
    {
    case 0:break;
    case 1:main_wnd()->localSync(); break;
    case 2:main_wnd()->synchronizeData(); break;
    }
#else  //API_QT_AUTOTEST
    main_wnd()->synchronizeData();
#endif  //_DEBUG 

    //if()
};

void gui::emptyRecycle(topic_ptr r)
{
    assert_return_void(r, "expected topic");
    assert_return_void(r->folder_type() == EFolderType::folderRecycle, "expected recycle");

    if(ard::confirmBox(ard::mainWnd(), "Are you sure you want to empty Recycle Bin?"))
        {
            WaitCursor wait;
            r->emptyRecycle();
            gui::rebuildOutline();
        }
};

void MainWindow::synchronizeData()
{
    if (!gui::isConnectedToNetwork()) {
        ard::errorBox(this, "Network connection required.");
        return;
    }
    
    SYNC_AUX_COMMANDS sync_commands;

    #ifdef ARD_GD
        assert_return_void(ard::google(), "Expected GD client");
        assert_return_void(ard::isDbConnected(), "Expected connected DB");

        if (!dbp::isDBSyncInCurrentAccountEnabled()) {
            QString db_name = gui::currentDBName();
            if (ard::confirmBox(this, QString("Selected data '%1' has not been synchronized before. Do you want to proceed?").arg(db_name))) {
                dbp::enableBSyncInCurrentAccount();
            }
            else {
                ard::messageBox(this,"Synchronization cancelled.");
                return;
            }
        }

        SyncPoint::runGdriveSync(false, sync_commands, "");
    #else
        Q_UNUSED(sync_commands);
        ASSERT(0, "NA");
    #endif //ARD_GD
};

void MainWindow::localSync()
{
    assert_return_void(gui::isDBAttached(), "expected attached DB");

    SYNC_AUX_COMMANDS sync_commands;
    //    extern void fillSyncCommands(SYNC_AUX_COMMANDS& );
    //    fillSyncCommands(sync_commands);
    QString other_db_path = remoteDBPath4LocalSync();
    if (!QDir(other_db_path).exists())
    {
        if (ard::confirmBox(this, QString("Please confirm creating directory for SYNC: %1").arg(other_db_path))) {
            QDir d;
            if (!d.mkpath(other_db_path)) 
            {
                ard::errorBox(this, QString("Failed to create directory for SYNC: %1").arg(other_db_path));
            }
        }
        else {
            ard::messageBox(this,"Aborted.");
        }
    }
    SyncPoint::runLocalSync(false, sync_commands, "");
};

void ard::selectArdiDbFile()
{
	ard::files_dlg::showFiles();
};

void ard::closeArdiDb() 
{
	dbp::close(false);
};


void MainWindow::fileBackup() 
{
    ArdDB::guiBackup();
};

void MainWindow::importTextFiles()
{
    ImportDir::guiSelectAndImportTextFiles();
};

void MainWindow::importCProject()
{
    ImportDir::importCProject();
};

void MainWindow::importBookmarks() 
{
    ard::gui_import_html_bookmarks();
};

void MainWindow::importContactsCvs() 
{
    ard::ArdiCsv::guiSelectAndImportContactsCsvFiles();
};

void MainWindow::importNativeDB() 
{
    ArdiDbMerger::guiSelectAndImportArdiFile();
};


QColor gui::colorTheme_BkColor()
{
    return curr_bk_color;
};

QColor gui::colorTheme_CardBkColor()
{
    return curr_bk_color;
};




void MainWindow::resetGui() 
{
    m_mainTab->resetTabControl();
    for (auto& tw : m_outline_main->m_outline_secondary_tabs) {
        auto tc = tw.second.tab_ctrl;
        if (tc) {
            tc->resetTabControl();
            if (!tw.second.expand_to_fill) {
                auto h = tc->calcBoundingHeight();
                tc->setMaximumHeight(h);
                tc->setMinimumHeight(h);
            }
            for (auto& t2 : tw.second.locus_space) {
                t2.second->resetTabControl();
            }
        }
    }
    //m_outline_main-
};



EOutlinePolicy gui::currPolicy()
{
    return main_wnd()->currPolicy();
};

void MainWindow::selectPrevPolicy()
{
    if (m_prev_policy != outline_policy_Uknown) {
        gui::rebuildOutline(m_prev_policy, true);
    }
};

void gui::selectPrevPolicy()
{
    if (!main_wnd())
        return;
    main_wnd()->selectPrevPolicy();
};

void gui::updateMainWndTitle(QString s)
{
    QString s2 = programName();
    if(!s.isEmpty())
        {
            s2 += "-" + s;
        }

#ifdef _DEBUG
	auto sz = main_wnd()->frameGeometry().size();
	auto s1 = QString("%1X%2").arg(sz.width()).arg(sz.height());
	s2 += "-" + s1;
#endif

    main_wnd()->setWindowTitle(s2);
};

EOutlinePolicy MainWindow::currPolicy()const
{
    return m_curr_policy;
};

EOutlinePolicy MainWindow::mainTabPolicy()const 
{
    ASSERT(m_mainTab, "expected main tab");
    //return m_main_tab_policy;
    return (EOutlinePolicy)m_mainTab->currentData();
};

void MainWindow::setupMainTabPolicy(EOutlinePolicy pol)
{
    ASSERT(m_mainTab, "expected main tab");
    if (m_mainTab) {
        auto parent_pol = PolicyCategory::parentPolicy(pol);
        if (parent_pol != mainTabPolicy()) {
            if (parent_pol != outline_policy_Uknown) {
                m_mainTab->setCurrentTabByData(parent_pol);
            }
        }
        m_mainTab->setCurrentTabByData(pol);
    }
};

void gui::rebuildOutline(EOutlinePolicy pol /*= outline_policy_Uknown*/, bool force /*= false*/)
{
    main_wnd()->rebuildOutline(pol, force);
}

void MainWindow::rebuildOutline(EOutlinePolicy pol /*= outline_policy_Uknown*/, bool force /*= false*/)
{
    if (pol == outline_policy_Uknown) {
        pol = gui::currPolicy();
        if (pol == outline_policy_Uknown) {
            pol = outline_policy_Pad;
        }
    }

    setupMainTabPolicy(pol);

    if (gui::currPolicy() != pol || force)
    {
        selectOutlinePolicy(pol);
    }
    else {
        outline()->rebuild(force);
    }

    auto h = ard::hoisted();
    if (h) {
        if (h->isStandardLocusable()) {
            main_wnd()->locateHoistedFolderInLocusTab();
        }
    }
};

void MainWindow::mainTabChanged(int d)
{    
    //m_main_tab_policy = (EOutlinePolicy)d;

    ///drop down search filter
    if (gui::searchFilterIsActive())
    {
        outline()->storeSearchFilterKey();
        TextFilterContext fc;
        fc.key_str = "";
        fc.include_expanded_notes = false;
        globalTextFilter().setSearchContext(fc);
        outline()->updateSearchMenuItems();
    }

    EOutlinePolicy op = (EOutlinePolicy)d;
    EOutlinePolicy main_pol = gui::currPolicy();

    if(main_pol != op){
        outline()->view()->setResetHScrollOnRebuildRequest();
        op = model()->getSecondaryPolicy(op);
        gui::rebuildOutline(op, true);
    }

    if (op == outline_policy_PadEmail)
	{
        outline()->check4new_mail(true);
    }
};

void MainWindow::mainCurrentTabClicked() 
{
};

/// should be no optimisation
/// in case policy has already been selected
void MainWindow::selectOutlinePolicy(EOutlinePolicy op)
{
    topic_ptr selBeforeRebuild = ard::currentTopic();
    if (selBeforeRebuild)
        LOCK(selBeforeRebuild);

    selectOutlineView(op);

    switch (op)
    {
    case outline_policy_PadEmail:
    case outline_policy_Pad:
    {
        auto h = model()->selectedHoistedTopic();
        if (h) {
            if (!h->isStandardLocusable()) {
                h = nullptr;
            }
        }

        if (!h) {
            DB_ID_TYPE id = dbp::configLastHoistedOID();
            if (IS_VALID_DB_ID(id)){
                h = dbp::defaultDB().lookupLoadedItem(id);
            }
            if (!h) {
                if (ard::isDbConnected()) {                 
                    h = ard::Sortbox();
                }
            }
        }

        if (h) {            
            model()->setSelectedHoistedTopic(h);
        }


        gui::rebuildOutline();
    }break;
    case outline_policy_2SearchView:
    {
        model()->selectByText(dbp::configFileLastSearchStr());
    }break;
    case outline_policy_TaskRing: 
    {
        model()->selectTaskRing();
    }break;
    case outline_policy_Notes:
    {
        model()->selectNotes();
    }break;
	case outline_policy_Bookmarks:
	{
		model()->selectBookmarks();
	}break;
	case outline_policy_Pictures:
	{
		model()->selectPictures();
	}break;
    case outline_policy_Annotated:
    {
        model()->selectAnnotated();
    }break;
    case outline_policy_Colored:
    {
        model()->selectColored();
    }break;
    
    //..
    case outline_policy_KRingForm:
    {
        ard::KRingKey* k2sel = nullptr;
        if (selBeforeRebuild != nullptr) {
            k2sel = dynamic_cast<ard::KRingKey*>(selBeforeRebuild);
        }
        if (!k2sel) {
            DB_ID_TYPE cid_sel = dbp::configLastSelectedKRingKeyOID();
            if (cid_sel != 0 && IS_VALID_DB_ID(cid_sel)) {
                k2sel = ard::lookupAs<ard::KRingKey>(cid_sel);
            }
        }
        model()->selectKRingForm(k2sel);
    }break;

    //..

    default:
    {
        gui::rebuildOutline();
    }break;
    }

    if (selBeforeRebuild)
        selBeforeRebuild->release();
};


void MainWindow::onScreenOrientationChanged(int )
{
    //ard::errorBox(this, (QString("onScreenOrientationChanged:%1").arg(x));
};


void MainWindow::locateHoistedFolderInLocusTab()
{
    auto h = ard::hoisted();
    if (h) {
        if (!h->isStandardLocusable()) {
            assert_return_void(0, "Observable(Aggregator) can't be hoisted in locus tab");
        }

        auto pol = gui::currPolicy();
        if (outline_policy_Pad == pol || outline_policy_PadEmail == pol)
        {
            auto it = outline()->m_outline_secondary_tabs.find(pol);
            if (it != outline()->m_outline_secondary_tabs.end()) 
            {
                auto it2 = it->second.locus_space.find(pol);
                if(it2 != it->second.locus_space.end())
                {
                    auto tab = it2->second;
                    auto dbid = tab->currentData();
                    auto hoisted_dbid = h->id();
                    if (outline_policy_PadEmail == pol) {
                        hoisted_dbid = dbp::configFileELabelHoisted();
                    }
                    

                    if (dbid != hoisted_dbid) 
                    {                       
                        if (!tab->selectTabByData(hoisted_dbid)) 
                        {
                            if (outline_policy_Pad == pol) {
                                auto t = tab->addLocusTopic(h);
                                tab->setCurrentTab(t);
                            }
                        };
                    }
                    QTimer::singleShot(100, this, [=]() {
                        tab->ensureVisible(hoisted_dbid);
                    });
                }
            }
        }
    }//hoisted  
};



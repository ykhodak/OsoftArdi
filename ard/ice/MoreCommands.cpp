#include <QHBoxLayout>
#include <QDesktopWidget>
#include <QPushButton>
#include <QLabel>
#include <QRadioButton>
#include <QButtonGroup>
#include <QApplication>
#include <QTabWidget>
#include <QComboBox>
#include <QCheckBox>
#include <QMenu>
#include <QFileDialog>
#include <QFormLayout>

#include "ProtoScene.h"
#include "OutlineScene.h"
#include "OutlineView.h"
#include "ardmodel.h"
#include "MainWindow.h"
#include "OutlineMain.h"
#include "anfolder.h"
#include "NoteEdit.h"
#include "anfolder.h"
#include "ansyncdb.h"
#include "NoteFrameWidget.h"
#include "OpenDatabase.h"
#include "syncpoint.h"
#include "gdrive/GdriveRoutes.h"
#include "UFolderBox.h"

enum EMenuCmd
    {
        commandAbout = 1,
        commandSupport,
        commandViewLog,
        commandViewTopicProperties,
        commandSearch,
        commandReplace        
    };

/**
   MoreCommands
*/
void MoreCommands::runIt(ECommandTab t)
{
    int cmd = AR_none;
    int param = 0;
    QVariant param2 = QVariant();
    if (true)
    {
        MoreCommands d(t);
        d.exec();
        cmd = d.m_req_cmd;
        param = d.m_req_cmd_param;
        param2 = d.m_req_cmd_param2;
    }

#define SHOW_PATH(V) dbp::configFileSetCPM_Limit(V);gui::rebuildOutline();

    /**
       we should respond to AR_BuilderDelegate only, all the other commands
       should be delivered to global handler, if we respond to them it wiil be duplicate
       response
    */

    switch (cmd)
    {
    case AR_none:
    case AR_insertBBoard:
    case AR_insertCustomFolder:
    case AR_synchronize:
    case AR_selectHoisted:
    case AR_ViewTaskRing:
        break;
    case AR_BuilderDelegate:
    {
        switch (param)
        {
        case commandAbout:     main_wnd()->helpAbout(); break;
        case commandSupport:   main_wnd()->supportWindow(); break;
        case commandViewLog:   main_wnd()->viewLog(); break;
        case commandViewTopicProperties:main_wnd()->viewProperties(); break;
        case commandSearch:    outline()->toggleGlobalSearch(); break;
        case commandReplace:   outline()->toggleGlobalReplace(); break;
        }//builder-commands
    }break;
    default:ASSERT(0, "NA") << cmd; break;
    }
};

void MoreCommands::constructMultitabDlg(ECommandTab tab2select)
{
    bool hasFoldersTab = (tab2select == ECommandTab::tabFolderSelect);
    bool hasFilesTab = (tab2select == ECommandTab::tabFileSelect);

#ifndef ARD_BIG
    if (tab2select != ECommandTab::tabProjectColumns) {
        hasEditTab = true;
        hasFilesTab = true;
        hasFoldersTab = true;
    }
#endif

    if (tab2select == ECommandTab::tabFileSelect) {
        hasFoldersTab = false;
        hasFilesTab = true;
    }

    if (hasFoldersTab)addFolderTab();
    if (hasFilesTab)addFilesTab();

    if (tab2select != ECommandTab::tabNone)
    {
        TAB_2_INDEX::iterator i = m_tab2index.find(tab2select);
        if (i != m_tab2index.end())
        {
            int idx = i->second;
            m_main_tab->setCurrentIndex(idx);
        }
    }
};

void MoreCommands::constructSingletabDlg(ECommandTab t)
{
    switch(t)
        {
        case ECommandTab::tabFolderSelect:
            addFilesTab();
            break;
        case ECommandTab::tabFileSelect:
            addFilesTab();
            break;
        default:break;
        }
};

MoreCommands::MoreCommands(ECommandTab tab2select)
{
    constructMultitabDlg(tab2select);    
    setupDialog(QSize(550, 600));
};

MoreCommands::~MoreCommands()
{
	if (m_view_scene)
	{
		m_view_scene->detachGui();
	}
};

void MoreCommands::register_tab(ECommandTab t, QWidget* w, scene_view::ptr&& v)
{
    assert_return_void(w, "expected widget panel");
    assert_return_void(v->view, "expected outline view");

    QString tlabel = "";
    switch(t)
        {
        case ECommandTab::tabFolderSelect:      tlabel = "Folders";break;
        case ECommandTab::tabEdit:              tlabel = "Edit";break;
        case ECommandTab::tabView:              tlabel = "View";break;
        case ECommandTab::tabFileSelect:        tlabel = "File";break;
        case ECommandTab::tabNone: ASSERT(0, "NA");
        }

    m_main_tab->addTab(w, tlabel); 
    int idx = m_main_tab->count()-1;
    m_tab2index[t] = idx;
    m_index2view[idx] = std::move(v);
    m_index2tab[idx] = t;  
};



static QString command2label(EMenuCmd c)
{
#define CASE_CMD(C, L) case C: rv = L;break;

    QString rv = "";
    switch(c)
        {
            CASE_CMD(commandAbout, "About");
            CASE_CMD(commandSupport, "Support");
            CASE_CMD(commandViewLog, "Logs");
            CASE_CMD(commandViewTopicProperties, "Properties");            
            CASE_CMD(commandSearch, "Search");
            CASE_CMD(commandReplace, "Replace");
        }
#undef CASE_CMD
    return rv;
} 

void MoreCommands::addFilesTab()
{
	std::set<ProtoPanel::EProp> prop;
	prop.insert(ProtoPanel::PP_CurrSelect);
	auto vtab = scene_view::create_with_builder([=](OutlineScene* s)
	{
		OutlineScene::build_files(s);
	}, prop, this);

    QWidget* w = new QWidget();
    QPushButton* b;
    QHBoxLayout *h1 = new QHBoxLayout;
#define ADD_BTN(L, C)  b = new QPushButton(this);       \
    b->setText(L);                                      \
    connect(b, SIGNAL(released()), this, SLOT(C()));    \
    h1->addWidget(b);                                   \

    vtab->view->enableWithClosedDB();

#ifdef ARD_BIG
#ifdef _DEBUG
    ADD_BTN("...", cmdFileMore);
    m_moreBtn = b;
#endif //_DEBUG
#endif //ARD_BIG

    if(gui::isDBAttached())
        {           
            ADD_BTN("New", cmdFileNew);
        }

#undef ADD_BTN


    QVBoxLayout *v_main = new QVBoxLayout(w);
    v_main->addWidget(vtab->view);
    v_main->addLayout(h1);
    utils::setupBoxLayout(v_main);

    register_tab(ECommandTab::tabFileSelect, w, std::move(vtab));
    //m_vtabs.emplace(ECommandTab::tabFileSelect, std::move(vtab));
};


void MoreCommands::addFolderTab()
{
    if (gui::isDBAttached())
    {
		std::set<ProtoPanel::EProp> prop;
		prop.insert(ProtoPanel::PP_CurrSelect);
		auto vtab = scene_view::create_with_builder([=](OutlineScene* s)
		{
			OutlineScene::build_folders(s);
		}, prop, this);


        QWidget* w = new QWidget();
        auto view_layout = new QVBoxLayout(w);
        utils::setupBoxLayout(view_layout);
        view_layout->addWidget(vtab->view);

        QHBoxLayout *h1 = new QHBoxLayout;
        ard::addBoxButton(h1, "New", [&]() {
            if (LocusBox::addFolder()) {
                rebuild();
            }
        });
        ard::addBoxButton(h1, "Edit", [&]() {
            auto uf = current_v_topic<anLocusFolder>();
            if (!uf) {
                ard::errorBox(this, "Please select topic to proceed.");
                return;
            }
            //ufolder_ptr uf = dynamic_cast<anLocusFolder*>(f);
            //ASSERT(uf, "expected user folder");
            //if (uf) {
                if (ard::guiEditUFolder(uf)) {
                    rebuild();
                }
            //}
        });
        ard::addBoxButton(h1, "Del", [&]() {
            auto f = current_v_topic<>();
            if (!f) {
                ard::errorBox(this, "Please select topic to proceed.");
                return;
            }

            if (f->isSingleton() || !f->parent()) {
                ard::messageBox(this,"Can't delete selected topic.");
                return;
            }

			if (ard::confirmBox(this, QString("<b><font color=\"red\">Delete '%1'</font></b>?").arg(f->title()))) 
			{
				f->killSilently(false);
				rebuild();
			};
        });
        ard::addBoxButton(h1, "Up", [&]() {
            auto f = current_v_topic();
            if (!f) {
                ard::errorBox(this, "Please select topic to proceed.");
                return;
            }

            if (!f->canMove()) {
                ard::messageBox(this,"Can't move selected topic.");
                return;
            }

            auto it2 = snc::util::moveInsideParent(f, true);
            if (it2) {
                rebuild();
                auto g = vtab->scene->findGItemByUnderlying(f);
                if (g) {
					vtab->scene->selectGI(g);
                    g->g()->ensureVisible(QRectF(0, 0, 1, 1));
                }
            }
        });

        ard::addBoxButton(h1, "Down", [&]() {
            auto f = current_v_topic<>();
            if (!f) {
                ard::errorBox(this, "Please select topic to proceed.");
                return;
            }

            if (!f->canMove()) {
                ard::messageBox(this,"Can't move selected topic.");
                return;
            }

            auto it2 = snc::util::moveInsideParent(f, false);
            if (it2) {
                rebuild();
                auto g = vtab->scene->findGItemByUnderlying(f);
                if (g) {
					vtab->scene->selectGI(g);
                    g->g()->ensureVisible(QRectF(0, 0, 1, 1));
                }
            }
        });


        view_layout->addLayout(h1);

        register_tab(ECommandTab::tabFolderSelect, w, std::move(vtab));
        //m_vtabs.emplace(ECommandTab::tabFolderSelect, std::move(vtab));
    }
};

ECommandTab MoreCommands::currentTabType()
{
    ECommandTab rv = ECommandTab::tabNone;
    int idx = m_main_tab->currentIndex();
    if(idx != -1)
        {
            INDEX_2_TAB::iterator k = m_index2tab.find(idx);
            if(k != m_index2tab.end())
                {
                    rv = k->second;
                }
        }  
    return rv;
};

void MoreCommands::rebuild()
{
	OutlineView* v = currentView();
	if (v) {
		v->rebuild();
	}
};

void MoreCommands::currentMarkPressed(int c, ProtoGItem* g)
{
    if(g)
	{
        ECurrentCommand ec = (ECurrentCommand)c;
            switch(ec)
                {
                case ECurrentCommand::cmdOpen:
                    {
                        auto f = g->topic()->shortcutUnderlying();

                        ECommandTab tb = currentTabType();
                        if(tb == ECommandTab::tabFileSelect)
                            {
                                QString fileName = f->title();
                                if(gui::isDBAttached() && fileName == dbp::currentDBName())
                                    {
                                        ard::messageBox(this,QString("'%1' is currently opened.").arg(fileName));
                                        return;
                                    }
                                WaitCursor wait;
                                if(!dbp::openStandardPath(fileName))
                                    {
                                        ard::messageBox(this,QString("Failed to open database file '%1'. Possible  reason - local security policy.").arg(fileName));
                                        return;
                                    }
                              //  detachTopic();
                                close();
                                return;
                            }

                        EOutlinePolicy pol = gui::currPolicy();
                        gui::outlineFolder(f, pol);
                        close();
                    }break;
                default:break;
                }
        }
};

void MoreCommands::toolButtonPressed(int cmd, int param, QVariant param2)
{
    m_req_cmd = cmd;
    m_req_cmd_param = param;
    m_req_cmd_param2 = param2;

    close();
};

void MoreCommands::cmdFileNew()
{
    if(CreateDatabase::run())
        {
            close();
        };
};

void MoreCommands::cmdFileMore()
{
    assert_return_void(m_moreBtn, "Expected 'More' button.");

    QMenu m(this);
#define ADD_MORE_ACT(T, D) a = new QAction(T, this);    \
    a->setData(D);                                      \
    m.addAction(a);                                     \


    QAction* a = nullptr;
    ADD_MORE_ACT("Select DB file", 3);
    ADD_MORE_ACT("Local Autotest peer", 4);
    ADD_MORE_ACT("Local Standard peer", 5);
    auto curr_db = ard::db();
    if (curr_db && curr_db->isOpen()) {
        auto& sdb = curr_db->sqldb();
        auto str_curr_path = sdb.databaseName();
        if (str_curr_path.indexOf(get_temp_uncompressed_file_suffix()) != -1) {
            ADD_MORE_ACT("Package-back Standard peer", 6);
        }
    }
    connect(&m, SIGNAL(triggered(QAction*)), this, 
            SLOT(processMoreEx(QAction*)));


    QPoint pt = m_moreBtn->mapToGlobal(QPoint(0,0));
    m.exec(pt);


#undef ADD_MORE_ACT
};

void MoreCommands::processMoreEx(QAction* a)
{
    extern QString autoTestDbName();

    QString db_file = "";

    bool bConfirmOnDelete = true;

    int t = a->data().toInt();
    switch(t)
        {
        case 3:
            {
                db_file = QFileDialog::getOpenFileName(this,
                                                       QObject::tr("Open Ardi File"), 
                                                        dbp::configFileLastShellAccessDir(),
                                                      QString("Ardi DB (%1);Ariadne compressed (%2);All Files (*)").arg(DB_FILE_NAME).arg(get_compressed_remote_db_file_name("")));
                if (!db_file.isEmpty()) {
                    dbp::configFileSetLastShellAccessDir(db_file, true);
                }
            }break;
        case 4:
            {
                db_file = remoteDBPath4LocalSync() + "/" + get_compressed_remote_db_file_name(autoTestDbName());
                bConfirmOnDelete = false;
                ard::messageBox(this,QString("about to open master:%1").arg(db_file));
                //return;
            }break;
        case 5: 
            {
                db_file = remoteDBPath4LocalSync() + "/" + get_compressed_remote_db_file_name("");
                bConfirmOnDelete = false;
                ard::messageBox(this,QString("about to open master:%1").arg(db_file));
            }break;
        case 6: 
        {
            auto curr_db = ard::db();
            if (curr_db && curr_db->isOpen()) {
                auto& sdb = curr_db->sqldb();
                auto str_curr_path = sdb.databaseName();
                auto idx = str_curr_path.indexOf(get_temp_uncompressed_file_suffix());
                if (idx != -1) {
                    QString pkg_name = str_curr_path.left(idx);
                    if (ard::confirmBox(this, QString("Please confirm packaging back DB '%1'. Existing file will be deleted.").arg(pkg_name))) {
                        if (QFile::exists(pkg_name))
                        {
                            if (!QFile::remove(pkg_name))
                            {                               
                                ard::messageBox(this,QString("Failed to delete file %1").arg(pkg_name));
                                return;
                            }
                        }

                        if (SyncPoint::compress(str_curr_path, pkg_name)) {
                            ard::messageBox(this,QString("Repackaged '%1'").arg(pkg_name));
                            return;
                        }
                    };
                }
            }
            return;
        }break;//6
        }//switch


    if(!db_file.isEmpty())
        {
            if(!QFile::exists(db_file))
                {
                    ard::messageBox(this,"File not found: " + db_file);
#ifdef _DEBUG
                    db_file = QFileDialog::getOpenFileName(this, tr("Open File..."),
                                                           QString(), tr("Ariadne-Files (*.sqlite);;All Files (*)"));
#endif
                }

            QFileInfo fi(db_file);

            if(fi.exists())
                {
                    QString ext = fi.suffix();
                    if(ext.compare("sqlite") == 0)
                        {
                            WaitCursor wait;
                           // detachTopic();
                            dbp::openAbsolutePath(db_file);
                            close();
                            return;
                        }
                    else if(ext.compare("qpk") == 0)
                        {
                            QString dpath = fi.canonicalFilePath();
                            QString tmp_db = dpath + get_temp_uncompressed_file_suffix();
                            if(QFile::exists(tmp_db))
                                {
                                    QString s = QString("File on the way %1, delete?").arg(tmp_db);
                                    if(bConfirmOnDelete)
                                        {
                                            if(!ard::confirmBox(this, s))
                                                {
                                                    return;
                                                };
                                        }
                                    if(!QFile::remove(tmp_db))
                                        {
                                            s = QString("Failed to delete file %1").arg(tmp_db);
                                            ard::messageBox(this,s);
                                            return;         
                                        }
                                };
                            auto res = SyncPoint::uncompress(db_file, tmp_db, false);
                            if(res.status == ard::aes_status::ok)
                                {
                                    WaitCursor wait;
                                    //detachTopic();
                                    dbp::openAbsolutePath(tmp_db);
                                    close();
                                    return;
                                }
                            else
                                {
                                    QString s = QString("Failed to uncompress DB '%1' into '%2'").arg(db_file).arg(tmp_db);
                                    ASSERT(0, s);
                                    ard::messageBox(this,s);
                                };
                        }
                }
        }
 }

void MoreCommands::closeEvent(QCloseEvent *e)
{
    gui::rebuildOutline(gui::currPolicy(), true);
	ard::scene_view_dlg::closeEvent(e);
};


#include <QPushButton>
#include <QComboBox>
#include <QCheckBox>
#include <QProgressBar>
#include <QMenu>
#include <QFileDialog>
#include "small_dialogs.h"
#include "OutlineScene.h"
#include "OutlineView.h"
#include "ardmodel.h"
#include "anfolder.h"
#include "ansyncdb.h"
#include "MainWindow.h"
#include "flowlayout.h"
#include "ansearch.h"
#include "rule_dlg.h"
#include "rule.h"
#include "UFolderBox.h"
#include "OpenDatabase.h"
#include "syncpoint.h"

extern QString autoTestDbName();

ard::scene_view_dlg::scene_view_dlg(QWidget *parent) :QDialog(parent)
{
	m_main_tab = new QTabWidget();
	m_main_tab->setTabPosition(QTabWidget::East);
	m_main_layout = new QVBoxLayout();
	m_main_layout->addWidget(m_main_tab);
};


void ard::scene_view_dlg::setupDialog(QSize sz, bool addCloseButton)
{
	assert_return_void(m_main_tab, "expected main tab");

	QPushButton* b = nullptr;
	m_buttons_layout = new QHBoxLayout();
	if (!gui::isDBAttached())
	{
		ADD_BUTTON2LAYOUT1(m_buttons_layout, "About", main_wnd(), &MainWindow::helpAbout);
		ADD_BUTTON2LAYOUT1(m_buttons_layout, "Log", main_wnd(), &MainWindow::viewLog);
	}

	if (addCloseButton) {
		ADD_BUTTON2LAYOUT(m_buttons_layout, "Close", &ard::scene_view_dlg::close);
	}

	m_main_layout->addLayout(m_buttons_layout);

	MODAL_DIALOG_SIZE(m_main_layout, sz);
};


void ard::scene_view_dlg::rebuild_current_scene_view() 
{
	auto v = current_scene_view();
	if (v) {
		v->run_scene_builder();
	}
};

scene_view*	ard::scene_view_dlg::current_scene_view()
{
	int idx = m_main_tab->currentIndex();
	if (idx != -1)
	{
		auto i = m_index2view.find(idx);
		if (i != m_index2view.end())
		{
			return i->second.get();
		}
	}
	return nullptr;
};

OutlineView* ard::scene_view_dlg::currentView()
{
	auto v = current_scene_view();
	if (v) {
		return v->view;
	}
	return nullptr;
};

ProtoGItem* ard::scene_view_dlg::currentGI()
{
	ProtoGItem* rv = nullptr;
	OutlineView* v = currentView();
	if (v) {
		OutlineSceneBase* s = dynamic_cast<OutlineSceneBase*>(v->scene());
		if (s) {
			rv = s->currentGI();
		}
	}
	return rv;
};

/**
   ard::move_dlg
*/
void ard::move_dlg::moveIt(TOPICS_LIST& move_it)
{
    ard::move_dlg d(move_it);
    d.exec();
    if (d.m_got_moved) 
    {
        if (d.m_follow_dest_folder->isChecked() && d.m_DestinationFolder) 
        {
            EOutlinePolicy switch2pol = outline_policy_Pad;
            EOutlinePolicy main_pol = gui::currPolicy();
            switch (main_pol)
                {
                case outline_policy_Pad:
                    switch2pol = outline_policy_Uknown;
                    break;
                default: break;
                }
            gui::outlineFolder(d.m_DestinationFolder, switch2pol);
        }
        else {            
            utils::prepareGuiAfterTopicsMoved(move_it);
            gui::rebuildOutline();
        }
        dbp::configFileSetFollowDestination(d.m_follow_dest_folder->isChecked());
    }
};

ard::move_dlg::move_dlg(TOPICS_LIST& move_it)
{
    m_topics2move = move_it;

    addFolderTab();	
    QPushButton* b = nullptr;
    auto id = dbp::configLastDestGenericTopicOID();
    if (IS_VALID_DB_ID(id)) {
        auto fd = ard::lookup(id);
        if (fd) {
            ADD_BUTTON2LAYOUT(m_main_layout, QString("Move to '%1'").arg(fd->title()), [=]() { move2folder(fd); });
        }
    }   

    setupDialog(QSize(550, 600), false);
    ADD_BUTTON2LAYOUT(m_buttons_layout, "Close", &ard::move_dlg::close);
    m_follow_dest_folder = new QCheckBox("Follow Destination");
    if (dbp::configFileFollowDestination()) {
        m_follow_dest_folder->setChecked(true);
    }
    m_buttons_layout->insertWidget(0, m_follow_dest_folder);
};

void ard::move_dlg::addFolderTab()
{
    if (gui::isDBAttached())
    {
		std::set<ProtoPanel::EProp> prop;
		prop.insert(ProtoPanel::PP_CurrMoveTarget);
		auto vtab = scene_view::create_with_builder([=](OutlineScene* s)
		{
			OutlineScene::build_folders(s);
		}, prop, this, OutlineContext::normal);

		m_main_tab->addTab(vtab->view, "Move to folder..");
        int idx = m_main_tab->count() - 1;
        m_index2view[idx] = std::move(vtab);
    }
};

void ard::move_dlg::currentMarkPressed(int c, ProtoGItem* g)
{
    if (g) 
	{
        ECurrentCommand ec = (ECurrentCommand)c;
        if (ec == ECurrentCommand::cmdSelectMoveTarget)
        {
            m_DestinationFolder = g->topic()->shortcutUnderlying();
            assert_return_void(m_DestinationFolder, "expected topic");
            if (m_DestinationFolder)
            {
                move2folder(m_DestinationFolder);
                /*
                int action_taken = 0;
                ard::guiInterpreted_moveTopics(m_topics2move, m_DestinationFolder, action_taken);
                if (action_taken > 0) 
                {
                    dbp::configSetLastDestGenericTopicOID(m_DestinationFolder->id());

                    //m_DestinationFolder->ensurePersistant(1);
                    m_got_moved = true;
                    close();
                    return;
                }
                */
            }
            else
            {
                ard::messageBox(this,"Select destination topic to proceed");
            }
        }
    }
}

void ard::move_dlg::move2folder(topic_ptr dest) 
{
    if (dest) 
    {
        int action_taken = 0;
        ard::guiInterpreted_moveTopics(m_topics2move, dest, action_taken);
        if (action_taken > 0)
        {
            dbp::configSetLastDestGenericTopicOID(dest->id());
            m_got_moved = true;
            close();
            return;
        }
    }
};


/**
   ard::attachements_dlg
*/
void ard::attachements_dlg::runIt(email_ptr e)
{
    assert_return_void(e, "expected email");
    auto lst = e->getAttachments();
    if (lst.size() == 0){
        ard::messageBox(gui::mainWnd(), QString("Selected email '%1..'\n\n has no attachments.").arg(e->title().left(64)));
        return;
    }

    ard::attachements_dlg d(e);
    d.exec();
};


ard::attachements_dlg::attachements_dlg(email_ptr e):m_email(e)
{
    LOCK(m_email);
    addAttachmentsTab();
    addProgressBar(m_main_layout, true);
    googleQt::GmailRoutes* gm = ard::gmail();
    if (gm && gm->cacheRoutes()) {
        connect(gm->cacheRoutes(),
                &googleQt::mail_cache::GmailCacheRoutes::attachmentDownloaded,
                this,
                &ard::attachements_dlg::attachmentsDownloaded);
    }
    setupDialog(QSize(800, 400));
	if (e->hasAttachment()) 
	{
		auto m = ard::gmail_model();
		if (m) {
			connect(m, &ard::email_model::all_attachments_downloaded, this, &ard::attachements_dlg::allAttachmentsDownloaded);
		}

		auto b = new QPushButton("Download all");
		m_buttons_layout->insertWidget(0, b);
		connect(b, &QPushButton::clicked, [=]() 
		{
			m_email->download_all_attachments();
		});
	}
};

ard::attachements_dlg::~attachements_dlg() 
{
    m_email->release();
};

void ard::attachements_dlg::addAttachmentsTab()
{
    if (gui::isDBAttached())
        {
            std::set<ProtoPanel::EProp> prop;
            prop.insert(ProtoPanel::PP_CurrDownload);
            prop.insert(ProtoPanel::PP_CurrOpen);
#ifdef ARD_BIG
            prop.insert(ProtoPanel::PP_CurrFindInShell);
#endif
			auto vtab = scene_view::create_with_builder([=](OutlineScene* s)
			{
				OutlineScene::build_email_attachements(s, m_email);
			}, prop, this);

            m_main_tab->addTab(vtab->view, "Attachments..");
            int idx = m_main_tab->count() - 1;
            m_index2view[idx] = std::move(vtab);
        }
};

void ard::attachements_dlg::currentMarkPressed(int c, ProtoGItem* g)
{
    assert_return_void(m_email, "expected email");
    googleQt::GmailRoutes* gm = ard::gmail();
    assert_return_void(gm, "expected gmail module");

    if (g)
    {
        auto att_it = dynamic_cast<ard::email_attachment*>(g->topic());
        assert_return_void(att_it, "expected att-topic");

        ECurrentCommand ec = (ECurrentCommand)c;
        switch (ec)
        {
        case ECurrentCommand::cmdDownload:
        {
            const googleQt::mail_cache::att_ptr& att = att_it->attachment();
            if (att) {
                m_progress_bar->setMaximum(static_cast<int>(att->size()));
                m_email->downloadAttachment(att);
                g->regenerateCurrentActions();
                g->g()->update();
            }
        }break;
        case ECurrentCommand::cmdDelete:
        {
            //ard::messageBox(this,"cmdDelete");
        }break;
        case ECurrentCommand::cmdOpen:
        {
            const googleQt::mail_cache::att_ptr& att = att_it->attachment();
            if (att) {
                if (att->status() == googleQt::mail_cache::AttachmentData::statusDownloaded) {
                    auto storage = ard::gstorage();
                    assert_return_void(storage, "expected gmail storage");
                    QString filePath = storage->findAttachmentFile(att);
                    if (!filePath.isEmpty()) {
                        if (!QDesktopServices::openUrl(QUrl::fromLocalFile(filePath))) {
                            ASSERT(0, "failed to open file") << filePath;
                            gui::showInGraphicalShell(this, filePath);
                        };
                    }
                    else {
                        ard::errorBox(this, QString("Failed to locate attachment file: %1").arg(att->localFilename()));
                        storage->invalidateAttachmentLocalCacheFile(att);
                        g->regenerateCurrentActions();
                    }
                }
            }
        }break;
        case ECurrentCommand::cmdFindInShell:
        {
            const googleQt::mail_cache::att_ptr& att = att_it->attachment();
            if (att) {
                if (att->status() == googleQt::mail_cache::AttachmentData::statusDownloaded) {
                    auto storage = ard::gstorage();
                    assert_return_void(storage, "expected g-storage");
                    if (storage) {
                        QString filePath = storage->findAttachmentFile(att);
                        if (!filePath.isEmpty()) {
                            gui::showInGraphicalShell(this, filePath);
                        }
                        else {
                            ard::errorBox(this, QString("Failed to locate attachment file: %1").arg(att->localFilename()));
                            storage->invalidateAttachmentLocalCacheFile(att);
                            g->regenerateCurrentActions();
                        }
                    }
                }
            }
        }break;
        default:break;
        }
    }
};

void ard::attachements_dlg::attachmentsDownloaded(googleQt::mail_cache::msg_ptr,
                                                    googleQt::mail_cache::att_ptr)
{
	auto v = current_scene_view();
	if (v)
		v->run_scene_builder();
    //m_att_vtab->view->rebuild();
	//m_att_vtab->run_scene_builder();
};

void ard::attachements_dlg::allAttachmentsDownloaded(ard::email* m) 
{
	if (m_email == m)
	{
		if (!m_download_status) 
		{
			m_download_status = new QLabel("");
			m_main_layout->addWidget(m_download_status);
		}
		m_download_status->setText("attachements downloaded");
	}
};

/**
   ard::draft_attachements_dlg
*/
void ard::draft_attachements_dlg::runIt(ard::email_draft* d, QWidget *parent)
{
    auto dext = d->draftExt();
    assert_return_void(dext, "expected draft extension");    
    if (dext->attachementList().size() == 0) {
        ard::messageBox(gui::mainWnd(), QString("Selected email '%1..'\n\n has no attachments.").arg(d->title().left(64)));
        return;
    }

    ard::draft_attachements_dlg dlg(d, parent);
    dlg.exec();
};

extern ard::email_draft_ext::attachement_file_list select_attachement_files_from_shell();
ard::draft_attachements_dlg::draft_attachements_dlg(ard::email_draft* d, QWidget *parent)
	:scene_view_dlg(parent), m_draft(d)
{
    addDraftAttachmentsTab();
    setupDialog(QSize(800, 400));
	auto b = new QPushButton("Add file");
	m_buttons_layout->insertWidget(0, b);
	connect(b, &QPushButton::clicked, [=]()
	{
		auto lst = select_attachement_files_from_shell();
		if (!lst.empty() && m_draft)
		{
			auto e = m_draft->draftExt();
			if (e) {
				e->add_attachments(lst);
				rebuild_current_scene_view();
			}
		}
	});
};

void ard::draft_attachements_dlg::addDraftAttachmentsTab() 
{
    if (gui::isDBAttached())
        {
            std::set<ProtoPanel::EProp> prop;
            prop.insert(ProtoPanel::PP_CurrDelete);
            prop.insert(ProtoPanel::PP_CurrOpen);
#ifndef ARD_BIG
            prop.insert(ProtoPanel::PP_CurrFindInShell);
#endif
			auto vtab = scene_view::create_with_builder([=](OutlineScene* s)
			{
				OutlineScene::build_draft_attachements(s, m_draft);
			}, prop, this);
            m_main_tab->addTab(vtab->view, "Attachments..");
            int idx = m_main_tab->count() - 1;
            m_index2view[idx] = std::move(vtab);
        }
};

void ard::draft_attachements_dlg::currentMarkPressed(int c, ProtoGItem* g)
{
	assert_return_void(m_draft, "expected topic");
	auto dext = m_draft->draftExt();
	assert_return_void(dext, "expected draft extension");

	if (g)
	{
		auto att_it = dynamic_cast<ard::draft_attachment_item*>(g->topic());
		assert_return_void(att_it, "expected att-topic");
#if (QT_VERSION >= QT_VERSION_CHECK(5, 6, 0))
		if (dext->attachmentsHost() != QSysInfo::machineHostName()) {
			ard::messageBox(this,QString("Attachment was created on different machine '%1' and can be opened only on machine of origin. This machine is '%2'.").arg(dext->attachmentsHost()).arg(QSysInfo::machineHostName()));
			return;
		}
#endif

		QString file_path = att_it->filePath();
		ECurrentCommand ec = (ECurrentCommand)c;
		switch (ec)
		{
		case ECurrentCommand::cmdOpen:
		{
			if (QFile::exists(file_path)) {
				if (!QDesktopServices::openUrl(QUrl::fromLocalFile(file_path))) {
					ASSERT(0, "failed to open file") << file_path;
					gui::showInGraphicalShell(this, file_path);
				};
			}
			else {
				ard::errorBox(this, QString("Failed to locate attachment file: %1").arg(file_path));
			}
		}break;
		case ECurrentCommand::cmdFindInShell:
		{
			gui::showInGraphicalShell(this, file_path);
		}break;
		case ECurrentCommand::cmdDelete:
		{
			if (ard::confirmBox(this, QString("Please confirm <b>detaching</b> file from email. Actual local file fill not be deleted. File: '%1'").arg(file_path))) {
				dext->remove_attachment(att_it->att_file());
				rebuild_current_scene_view();
			}
		}break;
		default:break;
		}
	}
}


/**
   ard::accounts_dlg
*/
void ard::accounts_dlg::runIt() 
{
    if (!dbp::guiCheckNetworkConnection())
        return;

    ard::accounts_dlg d;
    d.exec();
};

ard::accounts_dlg::accounts_dlg()
{
    addAccountsTab();
    setupDialog(QSize(550, 600));
    QPushButton* b;
    b = new QPushButton("Add");
    gui::setButtonMinHeight(b);
    m_buttons_layout->insertWidget(0, b);
    connect(b, &QPushButton::released,
            [=]()
            {
                if (!gui::isConnectedToNetwork()) {
                    ard::messageBox(this,"Network connection not detected.");
                    return ;
                }

                ard::authAndConnectNewGoogleUser();
                close();
            }); 
    b = new QPushButton("Del");
    gui::setButtonMinHeight(b);
    m_buttons_layout->insertWidget(2, b);
    connect(b, &QPushButton::released,
            [=]()
            {
                auto r = ard::guiConditionalyCheckAuthorizeGoogle();
                if (!r.first) {
                    if (r.second) {
                        close();
                    }
                    return;
                }

                /// this is something off at the momemt - we have to switch to another account
                /// and delete everything related to this account
                ProtoGItem* g = currentGI();
                if (!g) {
                    ard::errorBox(this, "Select Account to delete");
                    return;
                }

                auto cl = ard::google();
                if (!cl) {
                    ard::errorBox(this, "GClient module not initialized");
                    return;
                }

                cl->cancelAllRequests();

                googleQt::GmailRoutes* mr = ard::gmail();
                if (!mr) {
                    ard::errorBox(this, "Gmail module not initialized");
                    return;
                }

                QString acc_s = g->topic()->title();
                if (ard::confirmBox(this, QString("Delete selected account '%1'?").arg(acc_s)))
                    {
                        auto acc = dynamic_cast<ard::email_account_info*>(g->topic());
                        auto storage = ard::gstorage();
                        assert_return_void(storage, "expected g-storage");
                        storage->deleteAccountFromDb(acc->accountId());
                        QFile::remove(dbp::configEmailUserTokenFilePath(acc_s));
						rebuild_current_scene_view();
                        //m_acc_vtab->view->rebuild();
                        ///@todo: have to clean up token and DB cache
                        QString s = dbp::configEmailUserId();
                        if (s == acc_s) {
                            auto storage = ard::gstorage();
                            if (storage) {
                                auto lst = storage->getAccounts();
                                if (lst.size() == 0) {
                                    /// the only account, clean it
                                    dbp::configSetEmailUserId("");
                                    ard::asyncExec(AR_ReconnectGmailUser);
                                }
                                else if (lst.size() > 0) {
                                    for (auto& i : lst) {
                                        if (i->userId() != s) {
                                            dbp::configSetEmailUserId(i->userId());
                                            ard::asyncExec(AR_ReconnectGmailUser);
                                            break;
                                        }
                                    }
                                }
                            }
                            //del_current = true;
                        }
                }
                close();
            });

    b = new QPushButton("Reset Token");
    gui::setButtonMinHeight(b);
    m_buttons_layout->insertWidget(3, b);
    connect(b, &QPushButton::released,
            [=]()
            {
                auto r = ard::guiConditionalyCheckAuthorizeGoogle();
                if (!r.first) {
                    if (r.second) {
                        close();
                    }
                    return;
                }

				///...
				QString s = dbp::configEmailUserId();
				auto storage = ard::gstorage();
				if (storage) {
					auto lst = storage->getAccounts();
					if (lst.size() == 0) {
						/// the only account, clean it
						dbp::configSetEmailUserId("");
					}
					else if (lst.size() > 0) {
						for (auto& i : lst) {
							if (i->userId() != s) {
								dbp::configSetEmailUserId(i->userId());
								//ard::asyncExec(AR_ReconnectGmailUser);
								break;
							}
						}
					}
				}

				//...

				if (ard::revokeGoogleTokenWithConfirm()) {
					close();
					return;
				};
            });
};

void ard::accounts_dlg::currentMarkPressed(int c, ProtoGItem* g)
{
	if (g)
	{
		ECurrentCommand ec = (ECurrentCommand)c;
		switch (ec)
		{
		case ECurrentCommand::cmdSelect:
		{
			/// this should be async call request
			/// changing user can throw exception
			/// and Qt doesn't like exception in event handlers
			dbp::configSetEmailUserId(g->topic()->title());
			ard::asyncExec(AR_ReconnectGmailUser);
			close();
		}break;
		default:break;
		}
	}
};

void ard::accounts_dlg::addAccountsTab() 
{
    if (gui::isDBAttached())
        {
            std::set<ProtoPanel::EProp> prop;
            prop.insert(ProtoPanel::PP_CurrDelete);
            prop.insert(ProtoPanel::PP_CurrOpen);
#ifndef ARD_BIG
            prop.insert(ProtoPanel::PP_CurrFindInShell);
#endif
			auto vtab = scene_view::create_with_builder([=](OutlineScene* s)
			{
				OutlineScene::build_gmail_accounts(s);
			}, prop, this);
            m_main_tab->addTab(vtab->view, "GMail");
            int idx = m_main_tab->count() - 1;
            m_index2view[idx] = std::move(vtab);
        }
};

/**
folders_dlg
*/
void ard::folders_dlg::showFolders() 
{
	if (!gui::isDBAttached()) {
		return;
	}
	folders_dlg d;
	d.exec();
};

ard::folders_dlg::folders_dlg() 
{
	addFolderTab();
	setupDialog(QSize(550, 600), false);
	ard::addBoxButton(m_buttons_layout, "New", [&]() {
		if (LocusBox::addFolder()) {
			rebuild_current_scene_view();
			ard::rebuildFoldersBoard(nullptr);
		}
	});

	ard::addBoxButton(m_buttons_layout, "Edit", [&]() {
		auto uf = current_v_topic<ard::locus_folder>();
		if (!uf) {
			ard::errorBox(this, "Please select topic to proceed.");
			return;
		}
		if (ard::guiEditUFolder(uf)) {
			rebuild_current_scene_view();
			ard::rebuildFoldersBoard(nullptr);
		}
	});

	ard::addBoxButton(m_buttons_layout, "Del", [&]() {
		auto f = current_v_topic<>();
		if (!f) {
			ard::errorBox(this, "Please select topic to proceed.");
			return;
		}

		if (f->isSingleton() || !f->parent()) {
			ard::messageBox(this, "Can't delete selected topic.");
			return;
		}

		if (ard::confirmBox(this, QString("<b><font color=\"red\">Delete '%1'</font></b>?").arg(f->title())))
		{
			f->killSilently(false);
			rebuild_current_scene_view();
			ard::rebuildFoldersBoard(nullptr);
		};
	});

	ard::addBoxButton(m_buttons_layout, "Up", [&]() {
		auto f = current_v_topic();
		if (!f) {
			ard::errorBox(this, "Please select topic to proceed.");
			return;
		}

		if (!f->canMove()) {
			ard::messageBox(this, "Can't move selected topic.");
			return;
		}

		auto it2 = ard::moveInsideParent(f, true);
		if (it2) {
			rebuild_current_scene_view();
			ard::rebuildFoldersBoard(nullptr);
			auto v = current_scene_view();
			if (v && v->scene) 
			{
				auto g = v->scene->findGItemByUnderlying(f);
				if (g) {
					v->scene->selectGI(g);
					g->g()->ensureVisible(QRectF(0, 0, 1, 1));
				}
			}
		}
	});

	ard::addBoxButton(m_buttons_layout, "Down", [&]() {
		auto f = current_v_topic();
		if (!f) {
			ard::errorBox(this, "Please select topic to proceed.");
			return;
		}

		if (!f->canMove()) {
			ard::messageBox(this, "Can't move selected topic.");
			return;
		}

		auto it2 = ard::moveInsideParent(f, false);
		if (it2) {
			rebuild_current_scene_view();
			ard::rebuildFoldersBoard(nullptr);
			auto v = current_scene_view();
			if (v && v->scene)
			{
				auto g = v->scene->findGItemByUnderlying(f);
				if (g) {
					v->scene->selectGI(g);
					g->g()->ensureVisible(QRectF(0, 0, 1, 1));
				}
			}
		}
	});

	ard::addBoxButton(m_buttons_layout, "Close", [&]() {ard::scene_view_dlg::close(); });
	//ADD_BUTTON2LAYOUT(m_buttons_layout, "Close", &ard::scene_view_dlg::close);
};

void ard::folders_dlg::addFolderTab() 
{
	std::set<ProtoPanel::EProp> prop;
	//prop.insert(ProtoPanel::PP_CurrSelect);
	auto vtab = scene_view::create_with_builder([=](OutlineScene* s)
	{
		OutlineScene::build_folders(s);
	}, prop, this);

	m_main_tab->addTab(vtab->view, "Folders");
	int idx = m_main_tab->count() - 1;
	m_index2view[idx] = std::move(vtab);
};

/**
files_dlg
*/
void ard::files_dlg::showFiles()
{
	if (!gui::isDBAttached()) {
		return;
	}
	files_dlg d;
	d.exec();
};

ard::files_dlg::files_dlg() 
{
	addFilesTab();
	setupDialog(QSize(550, 600), false);

	m_moreBtn = ard::addBoxButton(m_buttons_layout, "...", [&]() {
		cmdFileMore();
	});

	ard::addBoxButton(m_buttons_layout, "New", [&]() {
		if (CreateDatabase::run())
		{
			close();
		};
	});


	ard::addBoxButton(m_buttons_layout, "Close", [&]() {ard::scene_view_dlg::close(); });
};

void ard::files_dlg::addFilesTab() 
{
	std::set<ProtoPanel::EProp> prop;
	prop.insert(ProtoPanel::PP_CurrSelect);
	auto vtab = scene_view::create_with_builder([=](OutlineScene* s)
	{
		OutlineScene::build_files(s);
	}, prop, this);

	m_main_tab->addTab(vtab->view, "Files");
	int idx = m_main_tab->count() - 1;
	m_index2view[idx] = std::move(vtab);
};

void ard::files_dlg::currentMarkPressed(int c, ProtoGItem* g) 
{
	if (g)
	{
		ECurrentCommand ec = (ECurrentCommand)c;
		switch (ec)
		{
		case ECurrentCommand::cmdOpen:
		{
			auto f = g->topic()->shortcutUnderlying();

			QString fileName = f->title();
			if (gui::isDBAttached() && fileName == dbp::currentDBName())
			{
				ard::messageBox(this, QString("'%1' is currently opened.").arg(fileName));
				return;
			}
			WaitCursor wait;
			if (!dbp::openStandardPath(fileName))
			{
				ard::messageBox(this, QString("Failed to open database file '%1'. Possible  reason - local security policy.").arg(fileName));
				return;
			}
			//  detachTopic();
			close();
			return;			
		}break;
		default:break;
		}
	}
};

void ard::files_dlg::cmdFileMore() 
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
	//connect(&m, SIGNAL(triggered(QAction*)), this,
	//	SLOT(processMoreEx(QAction*)));

	connect(&m, &QMenu::triggered, [&](QAction* a) 
	{
		processMoreEx(a);
	});


	QPoint pt = m_moreBtn->mapToGlobal(QPoint(0, 0));
	m.exec(pt);


#undef ADD_MORE_ACT
};

void ard::files_dlg::processMoreEx(QAction* a)
{

	QString db_file = "";

	bool bConfirmOnDelete = true;

	int t = a->data().toInt();
	switch (t)
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
		ard::messageBox(this, QString("about to open master:%1").arg(db_file));
		//return;
	}break;
	case 5:
	{
		db_file = remoteDBPath4LocalSync() + "/" + get_compressed_remote_db_file_name("");
		bConfirmOnDelete = false;
		ard::messageBox(this, QString("about to open master:%1").arg(db_file));
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
							ard::messageBox(this, QString("Failed to delete file %1").arg(pkg_name));
							return;
						}
					}

					if (SyncPoint::compress(str_curr_path, pkg_name)) {
						ard::messageBox(this, QString("Repackaged '%1'").arg(pkg_name));
						return;
					}
				};
			}
		}
		return;
	}break;//6
	}//switch


	if (!db_file.isEmpty())
	{
		if (!QFile::exists(db_file))
		{
			ard::messageBox(this, "File not found: " + db_file);
#ifdef _DEBUG
			db_file = QFileDialog::getOpenFileName(this, tr("Open File..."),
				QString(), tr("Ariadne-Files (*.sqlite);;All Files (*)"));
#endif
		}

		QFileInfo fi(db_file);

		if (fi.exists())
		{
			QString ext = fi.suffix();
			if (ext.compare("sqlite") == 0)
			{
				WaitCursor wait;
				// detachTopic();
				dbp::openAbsolutePath(db_file);
				close();
				return;
			}
			else if (ext.compare("qpk") == 0)
			{
				QString dpath = fi.canonicalFilePath();
				QString tmp_db = dpath + get_temp_uncompressed_file_suffix();
				if (QFile::exists(tmp_db))
				{
					QString s = QString("File on the way %1, delete?").arg(tmp_db);
					if (bConfirmOnDelete)
					{
						if (!ard::confirmBox(this, s))
						{
							return;
						};
					}
					if (!QFile::remove(tmp_db))
					{
						s = QString("Failed to delete file %1").arg(tmp_db);
						ard::messageBox(this, s);
						return;
					}
				};
				auto res = SyncPoint::uncompress(db_file, tmp_db, false);
				if (res.status == ard::aes_status::ok)
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
					ard::messageBox(this, s);
				};
			}
		}
	}
}


/**
ard::contact_groups_dlg
*/
void ard::contact_groups_dlg::runIt()
{
	ard::contact_groups_dlg d;
	d.exec();
};

ard::contact_groups_dlg::contact_groups_dlg()
{
	addGroupsTab();
	setupDialog(QSize(550, 600));

	auto b = new QPushButton("Add Group");
	m_buttons_layout->insertWidget(0, b);
	connect(b, &QPushButton::clicked, [=]()
	{
		auto r = gui::edit("", "Name", true, this);
		if (r.first) 
		{
			auto cgr = ard::db()->cmodel()->groot();
			auto g = cgr->addGroup(r.second);
			if (g) 
			{
				rebuild_current_scene_view();
			}
		}
	});

	b = new QPushButton("Edit");
	m_buttons_layout->insertWidget(1, b);
	connect(b, &QPushButton::clicked, [=]()
	{
		ProtoGItem* g = currentGI();
		if (!g) {
			ard::errorBox(this, "Select Group to edit");
			return;
		}

		auto v = g->p()->s()->v();
		v->renameSelected();
	});

	b = new QPushButton("Del");
	m_buttons_layout->insertWidget(2, b);
	connect(b, &QPushButton::clicked, [=]()
	{
		ProtoGItem* g = currentGI();
		if (!g) {
			ard::errorBox(this, "Select Group to remove");
			return;
		}
		auto f = g->topic();
		if (ard::confirmBox(this, QString("Please confirm removing contact group '%1'").arg(f->title()))) 
		{
			f->killSilently(true);
			rebuild_current_scene_view();
		}
		//if(f->kil)
	});
}

void ard::contact_groups_dlg::addGroupsTab()
{
	if (gui::isDBAttached())
	{
		std::set<ProtoPanel::EProp> prop;
		//prop.insert(ProtoPanel::PP_CurrDelete);
		prop.insert(ProtoPanel::PP_CurrEdit);
		prop.insert(ProtoPanel::PP_InplaceEdit);
		
		auto vtab = scene_view::create_with_builder([=](OutlineScene* s)
		{
			OutlineScene::build_contact_groups(s);
		}, prop, this, OutlineContext::normal);
		m_main_tab->addTab(vtab->view, "Contact Groups");
		int idx = m_main_tab->count() - 1;
		m_index2view[idx] = std::move(vtab);
	}
};

void ard::contact_groups_dlg::currentMarkPressed(int c, ProtoGItem* g)
{
	if (g) 
	{
		auto gr = dynamic_cast<ard::contact_group*>(g->topic());
		assert_return_void(gr, "expected contact group");

		ECurrentCommand ec = (ECurrentCommand)c;
		switch (ec)
		{
		case ECurrentCommand::cmdEdit:
		{
			auto v = g->p()->s()->v();
			v->renameSelected();
		}break;
		case ECurrentCommand::cmdDelete:
		{
			//ard::messageBox(this,"cmdDelete");
		}break;
		default:break;
		}
	}
};
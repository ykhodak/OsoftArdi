#include <QLineEdit>
#include <QTimer>
#include <QTextDocument>
#include <QApplication>
#include <QRect>
#include <QDesktopWidget>
#include <QComboBox>

#include "ardmodel.h"
#include "anfolder.h"
#include "ansyncdb.h"
#include "OutlineMain.h"
#include "OutlineSceneBase.h"
#include "MainWindow.h"
#include "NoteFrameWidget.h"
#include "snc-tree.h"
#include "OutlineScene.h"
#include "email.h"
#include "ethread.h"
#include "contact.h"
#include "kring.h"
#include "GoogleClient.h"
#include "EmailPreview.h"
#include "EmailSearchBox.h"
#include "locus_folder.h"
#include "ansearch.h"
#include "Endpoint.h"
#include "popup-widgets.h"
#include "small_dialogs.h"
#include "board.h"
#include "ard-db.h"
#include "BlackBoard.h"
#include "rule.h"
#include "rule_runner.h"
#include "TopicBox.h"
#include "picture.h"

topic_ptr ard::hoisted()
{
    auto db = ard::db();
    if (!db || !db->isOpen()) {
        return nullptr;
    }

    auto p = gui::currPolicy();
    switch (p) {
    case outline_policy_TaskRing:   return db->task_ring();
    case outline_policy_Notes:      return db->notes();
	case outline_policy_Bookmarks:  return db->bookmarks();
	case outline_policy_Pictures:   return db->pictures();
    case outline_policy_Annotated:  return db->annotated();
    case outline_policy_Colored:    return db->colored();    
    case outline_policy_2SearchView:return db->local_search();
    case outline_policy_PadEmail:   return db->gmail_runner();
    default:break;
    }

    if (!model())
        return nullptr;
    return model()->selectedHoistedTopic();
};


topic_ptr ard::currentTopic()
{
	topic_ptr rv = nullptr;
	ProtoGItem* g = outline()->selectedGitem();
	if (g)rv = g->topic();
	return rv;
};


ArdModel::ArdModel() :
    m_penGray(color::Gray),
    m_penSelectedItem(color::Navy),
    m_brushSelectedItem(color::SELECTED_ITEM_BK),
    m_brushMSelectedItem(color::MSELECTED_ITEM_BK),
    m_brushHotSelectedItem(color::HOT_SELECTED_ITEM_BK),
    m_brushCompletedItem(color::Green),
    m_brushOutlineThumb(color::Silver),
    m_selHoistedTopic(nullptr),
    m_selIndexCardData(0),
    m_helper_text_doc(nullptr),
    m_helper_gselect_view(nullptr),
    m_helper_gselect_scene(nullptr),
    m_pmItalic(TEXT_BAR_BUTTON_SIZE, TEXT_BAR_BUTTON_SIZE),
    m_pmUnderline(TEXT_BAR_BUTTON_SIZE, TEXT_BAR_BUTTON_SIZE),
    m_pmBold(TEXT_BAR_BUTTON_SIZE, TEXT_BAR_BUTTON_SIZE),
    m_pmFontSize1(TEXT_BAR_BUTTON_SIZE, TEXT_BAR_BUTTON_SIZE),
    m_pmFontSize2(TEXT_BAR_BUTTON_SIZE, TEXT_BAR_BUTTON_SIZE),
    m_pmFontSize3(TEXT_BAR_BUTTON_SIZE, TEXT_BAR_BUTTON_SIZE),
    m_pmFontSize4(TEXT_BAR_BUTTON_SIZE, TEXT_BAR_BUTTON_SIZE),
    m_pmBulletCircle(TEXT_BAR_BUTTON_SIZE, TEXT_BAR_BUTTON_SIZE),
    m_pmBulletRect(TEXT_BAR_BUTTON_SIZE, TEXT_BAR_BUTTON_SIZE),
    m_pmBulletNumer(TEXT_BAR_BUTTON_SIZE, TEXT_BAR_BUTTON_SIZE),
    m_pmBulletAlpha(TEXT_BAR_BUTTON_SIZE, TEXT_BAR_BUTTON_SIZE)
{
    m_penSelectedItem.setWidth(2);

#ifdef ARD_GD
    m_gmail_model.reset(new ard::email_model());
#endif

    prepareButtonsPixmaps();
    prepareShadesPixmaps();

    m_syncProgress = new snc::SyncProgressStatus();
}


ArdModel::~ArdModel()
{
    releaseDBFilesList();
    if(m_syncProgress){
            delete m_syncProgress;
        }
};

int ArdModel::fontSize2Points(int size)
{
    int rv = -1;
    switch(size)
        {
        case 0:rv = 12; break;
        case 1:rv = 24;break;
        case 2:rv = 26;break;
        case 3:rv = 32;break;
        case 4:rv = 36;break;
        }
    return rv;
};

void ArdModel::detachModelGui()
{
    if (m_gmail_model) {
        m_gmail_model->detachModelGui();
    }

	for (auto& i : m_pictures_watchers) {
		i->release();
	}
	m_pictures_watchers.clear();

    m_gui_detach_started = true;
    clearSelectedHoistedTopic();
    m_mainpol2secpol.clear();
    if (m_helper_gselect_scene)
    {
        m_helper_gselect_scene->detachGui();
    }

    syncProgress()->clearProgressStatus();
};

void ArdModel::registerPictureWatcher(ard::picture* p)
{
	assert_return_void(p, "expected picture");
	LOCK(p);
	m_pictures_watchers.push_back(p);
	p->captureMediaModTime();
	if (!m_media_watcher.isActive()) 
	{
		connect(&m_media_watcher, &QTimer::timeout, [=]() 
		{
			if (gui::isDBAttached())
			{
				processPictureWatchers();
			}
			else 
			{
				m_media_watcher.stop();
			}
		});
		m_media_watcher.start(1000);
	}
};


void ArdModel::processPictureWatchers() 
{
	for (auto& p : m_pictures_watchers) 
	{
		if (p->isMediaModified()) 
		{
			qDebug() << "<<< pic modified" << p->title();
			p->reloadMedia();
		}
	}
};

void ArdModel::attachGui()
{
    m_gui_detach_started = false;
};

int getGThumbWidth()
{
    int rv = 128;
    if(is_big_screen())
        {
            rv = 256;
        }
    else
        {
            /// screens are getting more dense on small devices
            /// we recalc thumb size

            static int thum_w = 0;

            static bool firstCall = true;   
            if(firstCall)
                {
                    QSize availableSize = QGuiApplication::primaryScreen()->availableSize();
                    int sz = availableSize.width();
                    if(availableSize.height() < sz)
                        {
                            sz = availableSize.height();
                        }
                    thum_w = (sz - sz * 0.2) / 2;
                    firstCall = false;
                }
            if(thum_w > rv)
                {
                    rv = thum_w;
                }
        }
    return rv;
}

void ArdModel::selectKRingForm(ard::KRingKey* k)
{
    m_kring_in_form_view = k;
    gui::rebuildOutline(outline_policy_KRingForm);
};

void ArdModel::selectByText(QString local_search)
{
    dbp::configFileSetLastSearchStr(local_search);
    if (local_search.isEmpty()) {
        local_search = "ardi";
    }
    auto st = ard::db()->local_search();
    if (st) {
        st->selectByText(local_search);
    }
    gui::rebuildOutline(outline_policy_2SearchView);
};

void ArdModel::selectTaskRing() 
{
    auto r = ard::db()->task_ring();
    if (r) {
        r->rebuild_observed();
    }
    gui::rebuildOutline(outline_policy_TaskRing);
};

void ArdModel::selectAnnotated() 
{
    auto r = ard::db()->annotated();
    if (r) {
        r->rebuild_observed();
    }
    gui::rebuildOutline(outline_policy_Annotated);
};

void ArdModel::selectNotes()
{
    auto r = ard::db()->notes();
    if (r) {
        r->rebuild_observed();
    }
    gui::rebuildOutline(outline_policy_Notes);
};

void ArdModel::selectBookmarks()
{
	auto r = ard::db()->bookmarks();
	if (r) {
		r->rebuild_observed();
	}
	gui::rebuildOutline(outline_policy_Bookmarks);
};

void ArdModel::selectPictures()
{
	auto r = ard::db()->pictures();
	if (r) {
		r->rebuild_observed();
	}
	gui::rebuildOutline(outline_policy_Pictures);
};

void ArdModel::selectColored() 
{
    auto r = ard::db()->colored();
    if (r) {
        r->setIncludeFilter(dbp::configColorGrepInFilter());
        r->rebuild_observed();
    }
    gui::rebuildOutline(outline_policy_Colored);
};

topic_ptr ArdModel::selectedHoistedTopic()
{
    if(!m_selHoistedTopic){
            if(gui::isDBAttached()){
                    setSelectedHoistedTopic(ard::Sortbox());
                }
        }
    return m_selHoistedTopic;
}


void ArdModel::setSelectedHoistedTopic(topic_ptr h)
{
    ASSERT(h, "expected topic");
    if(h == dbp::root() || !h)
        {
            h = ard::Sortbox();
        }

    if(h)
        {
#ifdef _DEBUG
            ASSERT(h->checkTreeSanity(), "sanity test failed");
#endif

            clearSelectedHoistedTopic();
            m_selHoistedTopic = h;
            LOCK(m_selHoistedTopic);
            if (IS_VALID_DB_ID(m_selHoistedTopic->id()) &&
                m_selHoistedTopic->id() != 0 &&
                m_selHoistedTopic->isStandardLocusable())
            {
                dbp::configSetLastHoistedOID(m_selHoistedTopic->id());
            }           

            //h->prepare4Outline();
        }
};

void ArdModel::clearSelectedHoistedTopic()
{
    if(m_selHoistedTopic)
        {
            ASSERT_VALID(m_selHoistedTopic);
            m_selHoistedTopic->release();
            m_selHoistedTopic = nullptr;
        }
}

// this is asynchronous call
// we are in the event processing and
// can not call rebuild which will delete object
// with following crash..
void ArdModel::setAsyncCallRequest(EAsyncCallRequest q,
    DB_ID_TYPE id,
    DB_ID_TYPE id2,
    DB_ID_TYPE id3,
    QObject* sceneBuilder,
    int delay)
{
    if (q != AR_none)
    {
        SReqInfo ri;
        ri.req = q;
        ri.id = id;
        ri.id2 = id2;
        ri.id3 = id3;
        ri.sceneBuilder = sceneBuilder;

        m_async_req_list.push_back(ri);
        if (delay == -1)delay = 100;
        QTimer::singleShot(delay, this, &ArdModel::completeAsyncCall);
    }
}

void ArdModel::setAsyncCallRequest(EAsyncCallRequest q, QString str, DB_ID_TYPE id2, DB_ID_TYPE id3, QObject* sceneBuilder, int delay)
{
    if (q != AR_none)
    {
        SReqInfo ri;
        ri.req = q;
        ri.str = str;
        ri.id2 = id2;
        ri.id3 = id3;
        ri.sceneBuilder = sceneBuilder;

        m_async_req_list.push_back(ri);
        if (delay == -1)delay = 100;
        QTimer::singleShot(delay, this, &ArdModel::completeAsyncCall);
    }
};

void ArdModel::setAsyncCallRequest(EAsyncCallRequest q, topic_ptr t1, topic_ptr t2)
{
    if (q != AR_none)
    {        
        SReqInfo ri;
        ri.req = q;
        ri.t1 = t1;
        ri.t2 = t2;
        if (t1) {
            LOCK(t1);
        }
        if (t2) {
            LOCK(t2);
        }

        m_async_req_list.push_back(ri);
        int delay = 100;
        QTimer::singleShot(delay, this, &ArdModel::completeAsyncCall);
    }
};

void ArdModel::setDelayedAsyncCallRequest(EAsyncCallRequest q, topic_ptr t1, topic_ptr t2) 
{
    if (q != AR_none)
    {
        SReqInfo ri;
        ri.req = q;
        ri.t1 = t1;
        ri.t2 = t2;
        if (t1) {
            LOCK(t1);
        }
        if (t2) {
            LOCK(t2);
        }
        m_delayed_async_req[q] = ri;
    }
};

void ArdModel::processDelayedAsyncCallRequests() 
{
    if (!m_delayed_async_req.empty()) 
    {
        for (auto& i : m_delayed_async_req) 
        {
            m_async_req_list.push_back(i.second);
        }
        m_delayed_async_req.clear();
        int delay = 100;
        QTimer::singleShot(delay, this, SLOT(completeAsyncCall()));
    }
};

void ArdModel::completeAsyncCall()
{
    if(!m_async_req_list.empty())
        {
            OutlineSceneBase* bs = outline()->scene();
                {
                    auto i = m_async_req_list.begin();
                    SReqInfo ri = *i;
                    EAsyncCallRequest q = ri.req;
                    m_async_req_list.erase(i);
                    if(!m_async_req_list.empty()){
                        QTimer::singleShot(100, this, SLOT(completeAsyncCall()));
                    }

                    switch(q)
                        {
                        case AR_GoogleRevokeToken:
                        {
							ard::disconnectGoogle();
							auto user = dbp::configEmailUserId();							
							QString token_file = dbp::configEmailUserTokenFilePath(user);							
							if (QFile::exists(token_file)) {
								if (!QFile::remove(token_file))
								{
									ard::error(QString("failed to revoke-token [%1][%2]").arg(user).arg(token_file));
									ard::messageBox(ard::mainWnd(), "Failed to proceed with Google token authorization. It is recommended to restart Ardi.");
								}
								else
								{
									ard::trail(QString("revoked-token [%1][%2]").arg(user).arg(token_file));
									ard::messageBox(ard::mainWnd(), "Google account needs authorization.");
								}
							}
                            //if (!gui::isConnectedToNetwork()) {
							//	ard::messageBox(gui::mainWnd(), "Network connection not detected.");
                             //   return;
                            //}
                            //ard::authAndConnectNewGoogleUser();
                        }return;
                        case AR_GoogleConditianalyGetNewTokenAndConnect: 
                        {                           
                            if (!gui::isConnectedToNetwork()) {
								ard::messageBox(gui::mainWnd(), "Network connection not detected.");
                                return;
                            }

                            ///...
                            QTimer::singleShot(300, [=]() {
                                if (ard::hasGoogleToken()) 
                                {
                                    void rerun_q_param();
									rerun_q_param();
                                    ard::asyncExec(AR_rebuildOutline);
                                }
                                else {
                                    ard::authAndConnectNewGoogleUser();
                                }
                            });
                            ///...

                        }return;
                        case AR_SelectGmailUser: 
                        {
							ard::accounts_dlg::runIt();
                        }break;
                        case AR_ReconnectGmailUser:
                            {
                                QString user = dbp::configEmailUserId();
                                ard::reconnectGoogle(user);
                                ard::asyncExec(AR_GoogleConditianalyGetNewTokenAndConnect);
                            }break;
                        case AR_OnConnectedGmail:
                            {
								ard::trail("gmail-connected");
                                if (outline()) {
                                    outline()->m_account_button->updateAccountButton();
                                }
								ard::rebuildMailBoard(nullptr);
                            }break;
						case AR_RuleDataReady: 
						{
							ard::q_param* r = dynamic_cast<ard::q_param*>(ri.t1);
							ASSERT_VALID(r);
							ard::rebuildMailBoard(r);
						}break;
                        case AR_OnDisconnectedGmail:
                            {
                                if (outline()) {
                                    outline()->m_account_button->updateAccountButton();
                                }
                            }break;
                        case AR_OnChangedGmailUser: 
                        {
                            if (outline()) {
                                outline()->m_account_button->updateAccountButton();
                            }
                        }break;
                        case AR_NoteLoaded:
						case AR_PictureLoaded:
                            {
                            ard::topic* it = ri.t1;
							auto w = ard::wspace();
							if (w) {
								auto r = w->findTopicWidget(it);
								if (r) {
									r->reloadContent();
								}
							}
                            }break;
                        case AR_selectHoisted:
                            {
                                topic_ptr it = dbp::defaultDB().lookupLoadedItem(ri.id);
                                auto pol = outline_policy_Uknown;
                                if (ri.id2 >= outline_policy_Pad) {
                                    pol = (EOutlinePolicy)ri.id2;
                                }
                                ASSERT(it, "expected item") << ri.id;
                                if(it){
                                    gui::outlineFolder(it, pol);
                                    }
                            }break;
                        case AR_UpdateGItem: 
                        {
                            if (ri.t1) {
                                if (outline() && outline()->scene()) {
                                    auto pg = outline()->scene()->findGItem(ri.t1);
                                    if (pg && pg->g()) {
                                        pg->g()->update();
                                    }
                                }
                            }
                        }break;
                        case AR_RenameTopic: 
                        {
                            bool selectContent = (ri.id2 > 0);
                            gui::renameSelected(static_cast<EColumnType>(ri.id), selectContent);
                        }break;
                        case AR_rebuildOutline:
                            {
                                if(ri.sceneBuilder && ri.sceneBuilder != outline())
                                    {
                                        QMetaObject::invokeMethod(ri.sceneBuilder, "rebuild", Qt::QueuedConnection);
                                    }
                                else
                                    {
                                        if(IS_VALID_DB_ID(ri.id))
                                            {
                                                EOutlinePolicy pol = (EOutlinePolicy)ri.id;
                                                gui::rebuildOutline(pol);
                                            }
                                        else
                                            {
                                                gui::rebuildOutline();
                                            }
                                    }
                            }break;
                        case AR_freePanelsAndRebuildOutline:
                            {
                                if (!gui::wasSelectOnRebuildRequested()) {
                                    ProtoGItem* g = bs->currentGI();
                                    if (g) {
                                        gui::setupMainOutlineSelectRequestOnRebuild(g->topic());
                                    }
                                }
                                bs->freePanels();
                                if (ri.id != 0) {
                                    EOutlinePolicy pol = (EOutlinePolicy)ri.id;
                                    gui::rebuildOutline(pol);
                                }
                                else {
                                    gui::rebuildOutline();
                                }
                            }break;
                        case AR_insertBBoard:
                        {
                            outline()->toggleInsertBBoard();
                        }; break;
                        case AR_ImportSuppliedOutline: 
                        {
                            dbp::checkOnSuppliedDemoImport();
                        }break;
                        case AR_ViewTaskRing: 
                        {
                            gui::rebuildOutline(outline_policy_TaskRing);
                        }break;
                        case AR_ViewProperties: 
                        {
                            if (ri.t1) {
                                TopicBox::showTopic(ri.t1);
                                ri.t1->release();
                            }
                        }break;
                        case AR_insertCustomFolder:
                            {
                                outline()->toggleNewCustomFolder();
                            }break;
                        case AR_synchronize:
                            {
                                gui::startSync();
                            }break;
						case AR_DataSynchronized: 
						{
							auto d = ard::db();
							if (d && d->isOpen()) {
								d->rmodel()->rroot()->resetRules();
								d->rmodel()->loadBoardRules();
							}
							if (!ard::autotestMode()) ard::rebuildMailBoard(nullptr);
						}break;
                        case AR_RebuildLocusTab:
                        {
							if (!ard::autotestMode()) {
								if (outline()) {
									outline()->rebuildLocusTabs();
								}
							}
                        }break;
                        case AR_SetKeyRingPwd: 
                        {
							ard::messageBox(gui::mainWnd(), "Key Ring password.");
                        }break;
                        case AR_check4NewEmail: {
                            if (outline()) {
                                outline()->check4new_mail(true);
                            }
                        }break;
                        case AR_ToggleKRingLock: 
                        {
                            outline()->updateKRingLockStatus();
                        }break;
#ifdef ARD_BIG
                        case AR_BoardApplyShape:
                        {
							auto b = ard::wspace()->currentPageAs<ard::BlackBoard>();
                            if (b) {
                                auto sh = static_cast<ard::BoardItemShape>(ri.id2);
                                b->applyNewShape(sh);
                            }
                        }break;
                        case AR_BoardCreateTopic: 
                        {
                            auto b = ard::wspace()->currentPageAs<ard::BlackBoard>();
                            if (b) {
                                QPoint pt(ri.id2, ri.id3);
                                b->createBoardTopic(pt);
                            }
                        }break;
                        case AR_BoardInsertTemplate: 
                        {
                            auto b = ard::wspace()->currentPageAs<ard::BlackBoard>();
                            if (b) {
                                auto tmpl = static_cast<BoardSample>(ri.id2);
                                b->startCreateFromTemplateMode(tmpl);
                            }
                        }break;
                        case AR_BoardRebuild:
                        {
                            auto b = ard::wspace()->currentPageAs<ard::BlackBoard>();
                            if (b) {
                                b->rebuildBoard();
                            }
                        }break;
                        case AR_BoardRebuildForRefTopic: 
                        {
                            auto b = ard::wspace()->currentPageAs<ard::BlackBoard>();
                            if (b) {
                                auto lst = b->findGBItems(ri.t1);
                                if (!lst.empty())b->rebuildBoard();                              
                            }
                        }break;
                        case AR_RebuildGuiOnResolvedCloudCache: 
                        {
                            auto lst = ard::wspace()->allBlackboard();
                            for (auto& b : lst)
                            {
                                qDebug() << "AR_RebuildGuiOnResolvedCloudCache" << b;
                                b->rebuildBoard();
                            }
                            gui::rebuildOutline();
                        }break;
                        case AR_MovePopupToFolder: 
                        {
                            topic_ptr source_topic = ri.t1;
                            topic_ptr dest_topic = ri.t2;
                            if (source_topic && dest_topic) {
                                qDebug() << "req2move" << source_topic->dbgHint() << "to" << dest_topic->dbgHint();
                                int action_taken = 0;
                                TOPICS_LIST topics2move;
                                auto e = dynamic_cast<ard::email*>(source_topic);
                                if (e) {
                                    ///we can't move separate email, it has to moved in context of thread
                                    auto th = e->parent();
                                    if (th) {
                                        topics2move.push_back(th);
                                    }
                                    else {
                                        qWarning() << "expected thread parent for email" << e->dbgHint();
                                    }
                                }
                                else {
                                    topics2move.push_back(source_topic);
                                }
                                if (ard::guiInterpreted_moveTopics(topics2move, 
                                    dest_topic, 
                                    action_taken,                                   
                                    false)) 
                                {
                                    ard::close_popup(source_topic);
                                    gui::rebuildOutline();
                                    //g__lastDestinationFolderID = dest_topic->id();
                                };

                                ri.t1->release();
                                ri.t2->release();
                            }
                        }break;
                        case AR_MovePopupSelectDestination: 
                        {
                            topic_ptr source_topic = ri.t1;
                            if (source_topic) 
                            {
                                TOPICS_LIST topics2move;
                                topics2move.push_back(source_topic);
								ard::move_dlg::moveIt(topics2move);
                                ard::close_popup(source_topic);
                                gui::rebuildOutline();
                            }
                        }break;
                        case AR_GmailErrorDetected: 
                        {
                            ard::gui_check_and_notify_on_gmail_access_error();
                        }break;
                        case AR_ShowMessage: 
                        {
							ard::messageBox(gui::mainWnd(), ri.str);
                        }break;
						case AR_FindText: 
						{
							auto ev = ard::wspace()->currentPageAs<ard::EmailTabPage>();
							if (ev) {
								ev->findText();
							}
						}break;
#endif //ARD_BIG
                        default:break;
                        }
                }
                // m_async_req_list.clear();
        }
};

void gui::runCommand(EAsyncCallRequest cmd,
                     QObject* obj,
                     int param)
{
    if (model()) {
        model()->setAsyncCallRequest(cmd, param, 0, 0, obj);
    }
};

void ard::asyncExec(EAsyncCallRequest cmd, topic_ptr t1, topic_ptr t2) 
{
    if (model()) {
        model()->setAsyncCallRequest(cmd, t1, t2);
    }
};

void ard::asyncExec(EAsyncCallRequest cmd, int param, int param2, int param3)
{
    if (model()) {
        model()->setAsyncCallRequest(cmd, param, param2, param3);
    }
};

void ard::asyncExec(EAsyncCallRequest cmd, QString param, int param2, int param3) 
{
    if (model()) {
        model()->setAsyncCallRequest(cmd, param, param2, param3);
    }
};

void ard::delayed_asyncExec(EAsyncCallRequest cmd, topic_ptr t1, topic_ptr t2) 
{
    auto m = model();
    if (m) {
        model()->setDelayedAsyncCallRequest(cmd, t1, t2);
    }
};

bool ArdModel::locateBySYID(QString syid, bool& dbfound)
{
    dbfound = false;
    bool locate_ok = false;

    TOPICS_LIST items_list;
    dbp::findBySyid(syid, items_list, &dbp::defaultDB());
    if(items_list.size() > 0)
        {
            dbfound = true;
            if(items_list.size() > 1)
                {
                    ASSERT(0, "located more the one item by SYID") << syid;
                }
            auto it = *(items_list.begin());
            locate_ok = gui::ensureVisibleInOutline(it);
        }
    else
        {
            ASSERT(0, "failed to locate by SYID") << syid;
        }

    snc::clear_locked_vector(items_list);

    return locate_ok;
};

bool gui::locateBySYID(QString syid, bool& dbfound)
{
    if (!model())
        return false;
    return model()->locateBySYID(syid, dbfound);
}

void ArdModel::storeSecPolicySelection(EOutlinePolicy secPolicy)
{
    EOutlinePolicy parent_pol = PolicyCategory::parentPolicy(secPolicy);
    if (parent_pol != main_wnd()->mainTabPolicy()) {
        ASSERT(0, "incompatible master policy selection") << gui::policy2name(secPolicy) << gui::policy2name(main_wnd()->mainTabPolicy()) << gui::policy2name(parent_pol);
        secPolicy = parent_pol;
    }

    m_mainpol2secpol[main_wnd()->mainTabPolicy()] = secPolicy;
};

EOutlinePolicy ArdModel::getSecondaryPolicy(EOutlinePolicy mainPol)
{
    EOutlinePolicy rv = outline_policy_Uknown;
    POL_2_POL::iterator i = m_mainpol2secpol.find(mainPol);
    if(i != m_mainpol2secpol.end())
        {
            rv = i->second;
        }
    if(rv == outline_policy_Uknown)
        {
            rv = mainPol;
        }

    return rv;
};


QTextDocument* ArdModel::helperTextDocument()
{
    if(!m_helper_text_doc)
        {
            m_helper_text_doc = new QTextDocument(main_wnd());
        }
    return m_helper_text_doc;
};


void ArdModel::debugFunction()
{
};

void ArdModel::onIdle()
{
	if (gui::isDBAttached())
	{
		ard::save_all_popup_content();		
	}
};

void ArdModel::releaseDBFilesList()const
{
    for(TOPICS_LIST::iterator i = m_db_files_list.begin(); i != m_db_files_list.end(); i++)
        {
            topic_ptr t = *i;
            t->release();
        }
    m_db_files_list.clear();
};

TOPICS_LIST& ArdModel::getDBFilesList()const
{
    releaseDBFilesList();

	ard::tool_topic* tt = nullptr;

#define ADD_DB(N)tt = new ard::tool_topic(N);m_db_files_list.push_back(tt);
    ADD_DB(DEFAULT_DB_NAME);

    QString sub_dbs = defaultRepositoryPath() + "dbs/";
    QDir d(sub_dbs);
    QStringList _list = d.entryList(QDir::AllDirs|QDir::NoDotAndDotDot);
    for(int i = 0; i < _list.size(); ++i)
        {
            QString dbPath = sub_dbs + _list.at(i) + "/" + DB_FILE_NAME;
            if(QFile::exists(dbPath))
                {
                    ADD_DB(_list.at(i));
                }
        }
#undef ADD_DB


    return m_db_files_list;
};

#define DX_BULLET 5
#define SIZE_BULLET 10

enum EBulletType
    {
        bulletCircle,
        bulletRect,
        bulletDecimal,
        bulletAlpha
    };

static void drawBulletLine(EBulletType type, QPainter& painter, const QRect& r1, int idx)
{  
    QPen pen(Qt::black);
    QPen pen_line(Qt::black, 4);
    painter.setPen(pen);
    painter.setBrush(Qt::black);

    QRect r2 = r1;
    r2.setWidth(r2.width() - 4);
    r2.setHeight(r2.height() - 4);  

    QRect r3 = r1;
    r3.setWidth(r2.width() + 4);
    r3.setHeight(r2.height() + 4);  
    r3.translate(0, -4);
  
    switch(type)
        {
        case bulletCircle:
            {
                painter.drawEllipse(r2);
            }
            break;
        case bulletRect:
            painter.setRenderHint(QPainter::Antialiasing,false);
            painter.drawRect(r2);
            break;
        case bulletDecimal:
            {
                painter.drawText(r3, Qt::AlignLeft, QString("%1").arg(idx + 1));
            }break;
        case bulletAlpha:
            {
                painter.drawText(r3, Qt::AlignLeft, QString("%1").arg((char)(idx + 'a')));
            }break;
        }

    painter.setRenderHint(QPainter::Antialiasing,true);
  
    QPoint pt1(r1.right() + DX_BULLET, r1.center().y());
    QPoint pt2 = pt1;
    pt2.setX(TEXT_BAR_BUTTON_SIZE - 2 * DX_BULLET);
    painter.setPen(pen_line);
    painter.drawLine(pt1, pt2);  
 
};

void ArdModel::prepareButtonsPixmaps()
{
    QFont app_fnt = QApplication::font();                      

    //#define TEXT_BAR_BUTTON_FONT_SIZE 32
#define TEXT_BAR_BUTTON_FONT_SIZE TEXT_BAR_BUTTON_SIZE
  
#define PREPARE_SYMBOL_PIXMAP1(M, T, B, I, U, S) if(true){              \
        QFont fnt = QFont(app_fnt.family(), S);                         \
        if(B)fnt.setBold(true);                                         \
        if(I)fnt.setItalic(true);                                       \
        if(U)fnt.setUnderline(true);                                    \
        M.fill(Qt::transparent);                                        \
        QPainter painter( &M );                                         \
        painter.setFont(fnt);                                           \
        painter.drawText(QRect(0,0,TEXT_BAR_BUTTON_SIZE,TEXT_BAR_BUTTON_SIZE), Qt::AlignCenter, T); \
    }                                                                   \

#define PREPARE_SYMBOL_PIXMAP(M, T, B, I, U) PREPARE_SYMBOL_PIXMAP1(M, T, B, I, U, TEXT_BAR_BUTTON_FONT_SIZE)
  

    PREPARE_SYMBOL_PIXMAP(m_pmItalic, "i", true, true, false);
    PREPARE_SYMBOL_PIXMAP(m_pmUnderline, "u", true, false, true);
    PREPARE_SYMBOL_PIXMAP(m_pmBold, "b", true, false, false);
    PREPARE_SYMBOL_PIXMAP1(m_pmFontSize1, "1", false, false, false, 24);
    PREPARE_SYMBOL_PIXMAP1(m_pmFontSize2, "2", false, false, false, 26);
    PREPARE_SYMBOL_PIXMAP1(m_pmFontSize3, "3", false, false, false, 32);
    PREPARE_SYMBOL_PIXMAP1(m_pmFontSize4, "4", false, false, false, 36);

    //  PREPARE_SYMBOL_PIXMAP1(m_pmFontSize4, "4", false, false, false, 36);
  
#undef PREPARE_SYMBOL_PIXMAP
#undef PREPARE_SYMBOL_PIXMAP1

    //#ifndef ARD_BIG
    QFont fnt_bullet = QFont(app_fnt.family(), 10);
    fnt_bullet.setBold(true);
  
#define PREPARE_BULLET_PIXMAP(M, B){  QRect rct(DX_BULLET, 2 * DX_BULLET, SIZE_BULLET, SIZE_BULLET); \
        M.fill(Qt::transparent);                                        \
        QPainter painter( &M );                                         \
        painter.setFont(fnt_bullet);                                    \
        painter.setRenderHint(QPainter::Antialiasing,true);             \
        for(int i = 0; i < 2; i++)                                      \
            {                                                           \
                drawBulletLine(B, painter, rct, i);                     \
                rct.translate(0, 2 * DX_BULLET);                        \
            }}                                                          \


    PREPARE_BULLET_PIXMAP(m_pmBulletCircle, bulletCircle);
    PREPARE_BULLET_PIXMAP(m_pmBulletRect, bulletRect);
    PREPARE_BULLET_PIXMAP(m_pmBulletNumer, bulletDecimal);
    PREPARE_BULLET_PIXMAP(m_pmBulletAlpha, bulletAlpha);
    //#endif //ARD_BIG
};

void ArdModel::prepareShadesPixmaps()
{
#define MAKE_SHADE_PIXMAP(P, I) {                                       \
        P.fill(Qt::transparent);                                        \
        QSvgRenderer renderer(QString(":ard/images/unix/%1.svg").arg(I)); \
        QPainter painter(&P);                                           \
        renderer.render(&painter, P.rect());                            \
    }                                                                   \

#undef MAKE_SHADE_PIXMAP

    QSizeF szOrig(2*TEXT_BAR_BUTTON_SIZE, 2*TEXT_BAR_BUTTON_SIZE);
    QSize szNode = calc_text_svg_node_size(szOrig, "yes");
    svg2pixmap(&m_pmYesShade, "yes", szNode);
    szNode = calc_text_svg_node_size(szOrig, "no");
    svg2pixmap(&m_pmNoShade, "no", szNode);
};

QTextDocument* gui::helperTextDocument()
{
    if (!model())
        return nullptr;

    return model()->helperTextDocument();
};

snc::SyncProgressStatus* ard::syncProgressStatus()
{
    if (!model())
        return nullptr;

    return model()->syncProgress();
};

ard::email_model* ard::gmail_model()
{
    if (!model())
        return nullptr;
    return model()->gmodel();
};


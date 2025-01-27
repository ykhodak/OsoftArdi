#include <QPushButton>
#include <QStandardPaths>
#include <QGraphicsSceneDragDropEvent>
#include <QFileInfo>
#include <QMessageBox>
#include "a-db-utils.h"
#include "db-stat.h"
#include "dbp.h"
#include "anfolder.h"
#include "email.h"
#include "email_draft.h"
#include "ethread.h"
#include "locus_folder.h"
#include <QWaitCondition>
#include "ansearch.h"
#include "simplecrypt.h"
#include "contact.h"
#include "board.h"
#include "extnote.h"
#include "anurl.h"
#include "kring.h"
#include "picture.h"

#ifdef Q_OS_WIN
#include <windows.h>
#endif

//extern void setupLastDestinationFolderID(DB_ID_TYPE id);

RootTopic* ard::root(){return dbp::root();};
topic_ptr ard::thread_root() {return dbp::threads_root();};

ArdDB* ard::db()
{
    return &dbp::defaultDB();
};


#define RETURN_TOPIC(T) return ard::db()->findLocusFolder(T);
ard::locus_folder* ard::Sortbox(){RETURN_TOPIC(EFolderType::folderSortbox);};
ard::locus_folder* ard::Maybe(){RETURN_TOPIC(EFolderType::folderMaybe);};
ard::locus_folder* ard::Reference(){RETURN_TOPIC(EFolderType::folderReference);};
ard::locus_folder* ard::Trash(){RETURN_TOPIC(EFolderType::folderRecycle);};
ard::locus_folder* ard::Delegated(){RETURN_TOPIC(EFolderType::folderDelegated);};
ard::locus_folder* ard::CustomSortersRoot(){RETURN_TOPIC(EFolderType::folderUserSortersHolder);};
ard::locus_folder* ard::Backlog() { RETURN_TOPIC(EFolderType::folderBacklog); };
ard::locus_folder* ard::BoardTopicsHolder() { RETURN_TOPIC(EFolderType::folderBoardTopicsHolder); };


ard::locus_folder* ard::ensureCustomSorterByTitle(QString name)
{
    topic_ptr croot = ard::CustomSortersRoot();
    assert_return_null(croot, "expected sorters root");
    assert_return_null(!name.isEmpty(), "expected sorters SYID");
	ard::locus_folder* f = dynamic_cast<ard::locus_folder*>(croot->findTopicByTitle(name));
    if (!f) {
        if (ard::db()) {
            f = ard::db()->createCustomFolderByName(name);
            if (f) {
                auto pf = f->parent();
                if (pf) {
                    pf->ensurePersistant(-1);
                }
            }
        }
    }
    return f;
};


void ard::selectCustomFolders(LOCUS_LIST& plist)
{
    topic_ptr r = ard::CustomSortersRoot();
    if(r){
        for(auto& tp : r->items())
		{
            auto f = dynamic_cast<locus_folder*>(tp);
            ASSERT(f, "expected folder");
            if(f && !f->isRetired()){
                plist.push_back(f);
            }     
        }
    }
};

QPushButton* ard::addBoxButton(QBoxLayout* lt, QString text, std::function<void(void)> released_slot)
{
  QPushButton* b = new QPushButton;
  b->setText(text);
  gui::setButtonMinHeight(b);
  lt->addWidget(b);
  QObject::connect(b, &QPushButton::released, released_slot);
  return b;
};

topic_ptr ard::lookup(DB_ID_TYPE id)
{
    topic_ptr it = nullptr;
    if (isDbConnected())
    {
        it = db()->lookupLoadedItem(id);
    }
    return it;
};

void ard::sendAllDrafts()
{
    auto d = Backlog();
    assert_return_void(d, "Expected drafts root");
	/*
    for (auto& it : d->items()) {
        auto ed = dynamic_cast<ard::email_draft*>(it);
        if (ed) {
            if (!ed->isRetired()) {
                if (!ed->hasDrafExt()) {
                    ASSERT(0, "draft without extension") << ed->dbgHint();
                    ed->setRetired(true);
                }
                else {
                    ed->sendDraft();
                }
            }
        }
    }*/
};


static void moveInOutlineTopics(TOPICS_LIST& t2move,
    topic_ptr dest1,
	int  move_pos,
    int& moved_count,
    int& move_err_count,
    QString& errDescription)
{
    assert_return_void(!t2move.empty(), "expected topics list to move");
    auto gm = ard::gmail_model();
    assert_return_void(gm, "expected gmail model");
    
    for (auto& t1 : t2move) 
    {
        auto t = t1->produceMoveSource(gm);

        if (t && dest1) {
            if (COMPATIBLE_PARENT_CHILD(dest1, t)) 
            {
                bool proceedOutlineMove = true;

                auto p = t->parent();
                if (!p) 
                {
					proceedOutlineMove = false;
					/*
                    ethread_ptr t1 = dynamic_cast<ethread_ptr>(t);
                    if (t1) {
						p = gm->adoptThread(t1);
						if (!p) {
							ASSERT(0, QString("failed to adopt thread [%1]").arg(t1->title().left(20)));
							proceedOutlineMove = false;
						}
                        //auto p2 = gm->adoptThread(t1, dest1);
                        //ASSERT(p2 == dest1, "expected destination parent");
                        //proceedOutlineMove = false;
                        //moved_count++;
                    }
                    else {
                        ASSERT(0, "expected ethread obj") << t->dbgHint();
                    }
					*/
                }

                if (proceedOutlineMove) {
                    if (p) {
                        p->detachItem(t);
                    }
                    //dest1->addItem(t);
					dest1->insertItem(t, move_pos++);
                    moved_count++;
                }
            }
            else {
                if (!dest1->canAcceptChild(t)) {
                    errDescription = QString("Destination folder '%1' can't accept topic.").arg(dest1->title());
                    qWarning() << "[move-err]Destination folder can't accept topic." << dest1->dbgHint() << t->dbgHint();
                }

                if (!t->canBeMemberOf(dest1)) {
                    errDescription = QString("Topic can't be member of destination folder '%1'.").arg(dest1->title());
                    qWarning() << "[move-err]Topic can't be member of destination folder." << dest1->dbgHint() << t->dbgHint();
                }
                move_err_count++;
            }
        }
    }   
}

void ard::moveTopics(TOPICS_LIST& l1,
    topic_ptr dest1,
    int& moved_count,
    int& move_err_count,
    QString& errDescriptionOnMove,
    QString& )
{   
    moved_count = 0;
    move_err_count = 0;

    //.. move around here ..
    if (!l1.empty()) {
        moveInOutlineTopics(l1, dest1, dest1->items().size(), moved_count, move_err_count, errDescriptionOnMove);
    }
    if (moved_count > 0) {
        dest1->ensurePersistant(1);
    }
};

void ard::guiInterpreted_moveTopic(topic_ptr t, topic_ptr dest)
{
    int action_taken = 0;
    TOPICS_LIST lst;
    lst.push_back(t);
    ard::guiInterpreted_moveTopics(lst, dest, action_taken);
};

bool ard::guiInterpreted_moveTopics(TOPICS_LIST& lst,
    topic_ptr dest,
    int& affected_count,
    bool showReportOnSuccess /*= true*/)
{
    int move_err_count = 0;
    int moved_count = 0;
    affected_count = 0;
    QString errDescOnMove, errDescriptionOnLabel;

    ard::moveTopics(lst, 
                    dest, 
                    moved_count, 
                    move_err_count, 
                    errDescOnMove,
                    errDescriptionOnLabel);

    if (moved_count == 0) {
		ard::messageBox(gui::mainWnd(), QString("Nothing moved. %1 %2").arg(errDescOnMove).arg(errDescriptionOnLabel));
        return false;
    }

    if (dest) {     
        auto db = ard::db();
        if (db && db->isOpen()) {
            dbp::configSetLastDestHoistedOID(dest->id());
        }
    }

    affected_count = moved_count;

    if (!showReportOnSuccess)
        return true;

    QString msg = "";
    if (move_err_count > 0) {
        QString msg = QString("%1 of the selected items can not be moved to %2.")
            .arg(move_err_count)
            .arg(dest->title());
    }
    if (!msg.isEmpty()) {
		ard::messageBox(ard::mainWnd(), msg);
    }

    return true;
};

TOPICS_LIST ard::reduce2ancestors(TOPICS_LIST& lst) 
{
    std::function<bool(topic_cptr, const TOPICS_LIST&)> has_ancestors = [](topic_cptr f, const TOPICS_LIST& l1)
    {
        for (auto it : l1) {
            if (it->isAncestorOf(f))
                return true;
        }

        return false;
    };

    TOPICS_LIST rv;
    for (auto f : lst) {
        if (!has_ancestors(f, lst)) {
            rv.push_back(f);
        }
    }
    return rv;
};

ESET ard::reduce2emails(TOPICS_LIST& lst)
{
	ESET rv;
    for (auto f : lst) {
        auto e = dynamic_cast<ard::email*>(f);
        if (e) {
            rv.insert(e);
        }
        else {
            auto t = dynamic_cast<ard::ethread*>(f);
            if (t && t->headMsg()) {
                rv.insert(t->headMsg());
            }
        }
    }
    return rv;
};


THREAD_SET ard::select_ethreads(TOPICS_LIST& lst) 
{
	std::function<bool(ard::topic*)>selectThreads = [](ard::topic* f) 
	{
		/*
		auto e = dynamic_cast<ard::email*>(f);
		if (e) {
			auto p = e->parent();
			if (p) {
				auto t = dynamic_cast<ard::ethread*>(p);
				if (t)return true;
			}
		}
		else {
			auto t = dynamic_cast<ard::ethread*>(f);
			if (t)return true;
		}*/
		auto t = dynamic_cast<ard::ethread*>(f);
		if (t)return true;
		return false;
	};

	THREAD_SET rv;
	for (auto f : lst) 
	{
		bool explore_tree = true;
		auto e = dynamic_cast<ard::email*>(f);
		if (e) {
			explore_tree = false;
			auto p = e->parent();
			if(p)rv.insert(p);
		}
		else {
			auto t = dynamic_cast<ard::ethread*>(f);
			if (t) {
				explore_tree = false;
				rv.insert(t);
			}
		}

		if(explore_tree)f->selecFromTree(selectThreads, rv);
	}

	return rv;
};

QString TimeConstraint2String(ETimeConstraint c) 
{
    QString rv = "-";
    switch (c) {
    case tconstraintASAP:rv = "ASAP"; break;
    case tconstraintSNET:rv = "SNET"; break;
    case tconstraintMSO:rv = "MSO"; break;
    }
    return rv;
}

static topic_ptr _hoistedInFilter = nullptr;
topic_ptr ard::hoistedInFilter(){return _hoistedInFilter;};
void ard::setHoistedInFilter(topic_ptr f) { _hoistedInFilter = f; }

void ard::sleep(int msec)
{
#ifdef Q_OS_WIN
    ::Sleep(msec);
#else
    QWaitCondition waitCondition;
    QMutex mutex;
    waitCondition.wait(&mutex, msec);
#endif
};

QString ard::getAttachmentDownloadDir()
{
    QString rv = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation) + "/" + ::programName();
	auto user = googleQt::makeValidFileName(dbp::configEmailUserId());
	if (!user.isEmpty()) {
		rv += "/" + user;
	}
    return rv;
};


static std::pair<uint64_t, uint64_t> pwd2keys(QString pwd)
{
    std::pair<uint64_t, uint64_t> rv{0,0};
    auto b = QCryptographicHash::hash((pwd.toUtf8()), QCryptographicHash::Md5);
    if (b.size() != 16) {
        ASSERT(0, "expected 128 bit MD5 sum") << b.size();
        return rv;
    }

    /// pack md5 checksum into two x64 numbers
    union UN64
    {
        uint64_t n;
        struct
        {
            int c1 : 8;
            int c2 : 8;
            int c3 : 8;
            int c4 : 8;
            int c5 : 8;
            int c6 : 8;
            int c7 : 8;
            int c8 : 8;
        };
    };

    UN64 u1, u2;
    u1.c1 = b[0];
    u1.c2 = b[1];
    u1.c3 = b[2];
    u1.c4 = b[3];
    u1.c5 = b[4];
    u1.c6 = b[5];
    u1.c7 = b[6];
    u1.c8 = b[7];

    u2.c1 = b[8];
    u2.c2 = b[9];
    u2.c3 = b[10];
    u2.c4 = b[11];
    u2.c5 = b[12];
    u2.c6 = b[13];
    u2.c7 = b[14];
    u2.c8 = b[15];

    rv.first = u1.n;
    rv.second = u2.n;

    return rv;
}

#define DECLARE_2CRYPTS(K)      SimpleCrypt crypt1(K.first);    \
                                SimpleCrypt crypt2(K.second);\
                                crypt1.setCompressionMode(SimpleCrypt::CompressionAlways);\
                                crypt2.setCompressionMode(SimpleCrypt::CompressionNever);\
                                crypt1.setIntegrityProtectionMode(SimpleCrypt::ProtectionHash);\
                                crypt2.setIntegrityProtectionMode(SimpleCrypt::ProtectionNone);\


/*
QByteArray ard::encryptText(QString pwd, QString str)
{
    auto keys = pwd2keys(pwd);
    DECLARE_2CRYPTS(keys);
    auto e1 = crypt1.encryptToByteArray(str);
    auto e2 = crypt2.encryptToByteArray(e1);
    return e2;
};

QString ard::decryptText(QString pwd, QByteArray b)
{
    auto keys = pwd2keys(pwd);
    DECLARE_2CRYPTS(keys);
    auto r1 = crypt2.decryptToByteArray(b);
    auto str = crypt1.decryptToString(r1);
    return str;
};
*/

QByteArray  ard::encryptBarray(const std::pair<uint64_t, uint64_t>& k, const QByteArray& b) 
{
    DECLARE_2CRYPTS(k);
    auto e1 = crypt1.encryptToByteArray(b);
    auto e2 = crypt2.encryptToByteArray(e1);
    return e2;
};

QByteArray  ard::decryptBarray(const std::pair<uint64_t, uint64_t>& k, const QByteArray& b) 
{
    DECLARE_2CRYPTS(k);
    auto r1 = crypt2.decryptToByteArray(b);
    auto rv = crypt1.decryptToByteArray(r1);
    return rv;
};


QByteArray ard::encryptBarray(QString pwd, const QByteArray& b) 
{
    QByteArray rv;
    auto keys = pwd2keys(pwd);
    return ard::encryptBarray(keys, b);
};

QByteArray ard::decryptBarray(QString pwd, const QByteArray& b) 
{
    QByteArray rv;
    auto keys = pwd2keys(pwd);
    DECLARE_2CRYPTS(keys);
    return decryptBarray(keys, b);
};

/*
void ard::initGoogleBatchUpdate()
{
    auto gm = ard::gmail();
    if (gm && gm->cacheRoutes())
    {
        auto t = gm->cacheRoutes()->applyBatchUpdate_Async();
        t->then([=]()
        {
            qDebug() << "completed applyBatchUpdate_Async";
        },
            [=](std::unique_ptr<googleQt::GoogleException> ex)
        {
            ASSERT(0, "applyBatchUpdate_Async error") << ex->statusCode() << ex->what();
        });
    }
};*/

topic_ptr ard::buildOutlineSample(OutlineSample o, topic_ptr parent)
{
    assert_return_null(parent, "expected parent for new topic");

	std::function<ard::topic* (QString)> quote = [](QString sym) 
	{
		auto u = ard::anUrl::createUrl(sym, QString("https://finance.yahoo.com/quote/%1").arg(sym));
		return u;
	};

#define ADD_T(T)br->addItem(new ard::topic(T));
#define ADD_Q(P, S)	P->addItem(quote(S));

    auto br = new ard::topic();
    bool ok = parent->addItem(br);
    ASSERT(ok, "failed to add topic") << parent->dbgHint();
    if (br) {
        switch (o)
        {
        case OutlineSample::GreekAlphabet:
            br->setTitle("Greek alphabet");
            ADD_T("alpha");
            ADD_T("beta");
            ADD_T("gamma");
            ADD_T("delta");
            ADD_T("epsilon");
            ADD_T("zeta");
            ADD_T("eta");
            ADD_T("theta");
            ADD_T("iota");
            ADD_T("kappa");
            ADD_T("lambda");
            ADD_T("mu");
            ADD_T("nu");
            ADD_T("xi");
            ADD_T("omicron");
            ADD_T("pi");
            ADD_T("rho");
            ADD_T("tau");
            ADD_T("upsilon");
            ADD_T("phi");
            ADD_T("chi");
            ADD_T("psi");
            ADD_T("omega");
            break;
        case OutlineSample::OS:
        {
            br->setTitle("OS");
            auto u = new ard::topic("Unix");
            auto windows = new ard::topic("Windows");
            br->addItem(u);
            br->addItem(windows);
            u->addItem(new ard::topic("AIX(IBM)"));
            u->addItem(new ard::topic("HPUX"));
            u->addItem(new ard::topic("Solaris"));
            u->addItem(new ard::topic("Solaris"));
            auto l = new ard::topic("Linux");
            u->addItem(l);
            l->addItem(new ard::topic("Redhat"));
            l->addItem(new ard::topic("Suse"));
            l->addItem(new ard::topic("Debian"));
            l->addItem(new ard::topic("Ubuntu"));
            l->addItem(new ard::topic("Mint"));
        }break;
		case OutlineSample::Stocks: 
		{
			br->setTitle("Stocks");
			auto nsdq = new ard::topic("NSDQ");
			auto finance = new ard::topic("Finance");
			auto transport = new ard::topic("Transportation");
			br->addItem(nsdq);
			br->addItem(finance);
			br->addItem(transport);
			ADD_Q(nsdq, "GOOG");
			ADD_Q(nsdq, "TSLA");
			ADD_Q(nsdq, "AMZN");
			ADD_Q(nsdq, "FB");
			ADD_Q(nsdq, "MSFT");
			ADD_Q(nsdq, "INTC");
			ADD_Q(nsdq, "NVDA");
			ADD_Q(nsdq, "NFLX");
			ADD_Q(nsdq, "SQ");

			ADD_Q(finance, "JPM");
			ADD_Q(finance, "C");
			ADD_Q(finance, "BAC");
			ADD_Q(finance, "GS");
			ADD_Q(finance, "WFC");

			ADD_Q(transport, "BA");
			ADD_Q(transport, "DAL");
			ADD_Q(transport, "UAL");
			ADD_Q(transport, "AAL");
			ADD_Q(transport, "CCL");
			ADD_Q(transport, "RCL");
			ADD_Q(transport, "NCLH");
		}break;
        case OutlineSample::Programming: 
        {
            br->setTitle("Programming Languages");
            ADD_T("Assembly");
            ADD_T("C");
            auto cpp = new ard::topic("C++");
            br->addItem(cpp);       
            cpp->addItem(new ard::topic("gcc"));
            cpp->addItem(new ard::topic("clang"));
            cpp->addItem(new ard::topic("vs"));
            ADD_T("D");
            ADD_T("Java");
            ADD_T("Python");
            ADD_T("C#");
            ADD_T("Golang");
            ADD_T("Haskell");
            ADD_T("Lisp");
            ADD_T("Fortran");
            ADD_T("Algol");
        }break;
        default:break;
        }

        //br->ensurePersistant(-1);
        //gui::rebuildOutline();
    }
#undef ADD_T
    return br;
};

ard::board_item* ard::buildBBoardSample(BoardSample smpl, ard::selector_board* b, int root_band_index, int root_yDelta)
{
    Q_UNUSED(root_yDelta);
    ard::board_item* rv = nullptr;
    assert_return_null(b, "expected board");

    struct band_group
    {
        int                 ydelta;
        ard::BoardItemShape shape;
        TOPICS_LIST         topics;

        band_group& add(topic_ptr f) {
            topics.push_back(f);
            return *this;
        }
    };

    TOPICS_MAP  n2t;
    using BAND_GROUPS = std::vector<std::unique_ptr<band_group>>;
    BAND_GROUPS band_groups;

    std::function<topic_ptr(QString, QString, topic_ptr, color::Color)> def_topic =
        [&](QString name, QString title, topic_ptr parent, color::Color c)
    {
        topic_ptr rv = new ard::topic(title);
		EColor c1 = static_cast<EColor>(c);
        rv->setColorIndex(c1);
        parent->addItem(rv);
        n2t[name] = rv;
        return rv;
    };


    std::function<band_group*(int, ard::BoardItemShape)> def_band_group =
        [&](int ydelta, ard::BoardItemShape shape)
    {
        band_group* bg = new band_group;
        bg->ydelta = ydelta;
        bg->shape = shape;
        std::unique_ptr<band_group> ub(bg);
        band_groups.emplace_back(std::move(ub));
        return bg;
    };
        
    std::function<void(int, int, band_group&)>add_group = [&](int band_index, int space_put_y_delta, band_group& g)
    {
        for (auto i : g.topics) {
            i->ensurePersistant(-1);
        }
        auto res = b->insertTopicsWithBranches(ard::InsertBranchType::single_topic, g.topics, band_index, -1, g.ydelta, g.shape);
        if (space_put_y_delta > 0) {
            for (auto i : res.bitems) {
                i->setYDelta(space_put_y_delta);
            }
        }
    };

    std::function<void(QString, QString, QString)> add_link = [&](QString origin, QString target, QString lbl)
    {
        auto i = n2t.find(origin);
        if (i == n2t.end()) {
            ASSERT(0, "no origin") << origin;
            return;
        }
        auto o = b->findBItem(i->second);
        assert_return_void(o, "no bitem for origin");

        auto j = n2t.find(target);
        if (j == n2t.end()) {
            ASSERT(0, "no target") << target;
            return;
        }
        auto t = b->findBItem(j->second);
        assert_return_void(t, "no bitem for target");

        auto lnk_lst = b->addBoardLink(o, t);
        if (!lbl.isEmpty()) {
            if (lnk_lst) {
                auto lnk = lnk_lst->getAt(lnk_lst->size() - 1);
                lnk->setLinkLabel(lbl, b->syncDb());
            }
        }
    };
    


    auto h = b->ensureOutlineTopicsHolder();
    if (h) {
        std::function<void(OutlineSample, ard::InsertBranchType it, int band_idx, ard::BoardItemShape)> add_outline =
            [&](OutlineSample smp, ard::InsertBranchType it, int band_idx, ard::BoardItemShape shp)
        {
            auto f = ard::buildOutlineSample(smp, h);
            f->ensurePersistant(-1);
            TOPICS_LIST lst;
            lst.push_back(f);
            auto res = b->insertTopicsWithBranches(it, lst, band_idx, -1, 300, shp);
            if (!rv) {
                if (!res.origins.empty()) {
                    rv = res.origins[0];
                }
            }
        };

        switch (smpl)
        {
        case BoardSample::Greek2Autotest: 
        {
            add_outline(OutlineSample::GreekAlphabet, ard::InsertBranchType::branch_expanded_from_center, root_band_index/*1*/, ard::BoardItemShape::text_normal);
            add_outline(OutlineSample::OS, ard::InsertBranchType::branch_expanded_to_right, root_band_index + 1, ard::BoardItemShape::box);
            add_outline(OutlineSample::Programming, ard::InsertBranchType::branch_expanded_to_right, root_band_index + 3, ard::BoardItemShape::text_normal);
        }break;
        case BoardSample::Greek: 
        {
            add_outline(OutlineSample::GreekAlphabet, ard::InsertBranchType::branch_expanded_from_center, root_band_index, ard::BoardItemShape::text_normal);
        }break;
        case BoardSample::OS: 
        {
            add_outline(OutlineSample::OS, ard::InsertBranchType::branch_expanded_to_right, root_band_index, ard::BoardItemShape::box);
        }break;
		case BoardSample::Stocks: 
		{
			add_outline(OutlineSample::Stocks, ard::InsertBranchType::branch_expanded_from_center, root_band_index, ard::BoardItemShape::box);
		}break;
        case BoardSample::Programming: 
        {
            add_outline(OutlineSample::Programming, ard::InsertBranchType::branch_expanded_to_right, root_band_index, ard::BoardItemShape::text_normal);
        }break;
        case BoardSample::Gtd: 
        {
            if (root_band_index <= 0) {
                root_band_index = 1;
            }

            add_group(root_band_index-1, 0, def_band_group(50, ard::BoardItemShape::box)->add(def_topic("item", "Topic", h, color::Color::Black))
                .add(def_topic("email", "Email", h, color::Color::Black))
                .add(def_topic("note", "Note", h, color::Color::Black))
                .add(def_topic("idea", "Idea", h, color::Color::Black))
            );


            add_group(root_band_index, 30, def_band_group(100, ard::BoardItemShape::rombus)->add(def_topic("actionable", "Actionable?", h, color::Color::Black))
            .add(def_topic("single", "Single Step?", h, color::Color::Black))
            .add(def_topic("two_min", "More Than 2 Minutes?", h, color::Color::Black))
            .add(def_topic("for_me", "For me?", h, color::Color::Black))
            .add(def_topic("specific_time", "Specific day/time?", h, color::Color::Black)));
            
            add_link("item", "actionable", "");
            add_link("email", "actionable", "");
            add_link("note", "actionable", "");
            add_link("idea", "actionable", "");

            add_link("actionable", "single", "Yes");
            add_link("single", "two_min", "Yes");
            add_link("two_min", "for_me", "Yes");
            add_link("for_me", "specific_time", "Yes");

            add_group(root_band_index+1, 20, def_band_group(200, ard::BoardItemShape::box)->add(def_topic("trash", "Trash", h, color::Color::Red))
            .add(def_topic("someday", "Someday/maybe", h, color::Color::Red))
            .add(def_topic("reference", "Reference", h, color::Color::Red)));

            add_group(0, 0, def_band_group(100, ard::BoardItemShape::box)->add(def_topic("plan", "Plan project task", h, color::Color::Blue)));
            add_group(2, 0, def_band_group(300, ard::BoardItemShape::box)->add(def_topic("doit", "Do it!", h, color::Color::Green)));
            add_group(2, 0, def_band_group(100, ard::BoardItemShape::box)->add(def_topic("delegate", "Delegate", h, color::Color::Black)));
            add_group(0, 0, def_band_group(400, ard::BoardItemShape::box)->add(def_topic("calendar", "Calendar", h, color::Color::Green)));
            add_group(2, 0, def_band_group(200, ard::BoardItemShape::box)->add(def_topic("context", "@Context Task", h, color::Color::Black)));

            add_link("actionable", "trash", "No");
            add_link("actionable", "someday", "");
            add_link("actionable", "reference", "");
            add_link("single", "plan", "No");
            add_link("two_min", "doit", "No");
            add_link("for_me", "delegate", "No");
            add_link("specific_time", "calendar", "Yes");
            add_link("specific_time", "context", "No");

        }break;
        case BoardSample::ProductionControl: 
        {
            add_group(root_band_index, 0, def_band_group(50, ard::BoardItemShape::box)->add(def_topic("plan", "Plan", h, color::Color::Black)));
            add_group(root_band_index+1, 0, def_band_group(150, ard::BoardItemShape::box)->add(def_topic("schedule", "Schedule", h, color::Color::Black)));
            add_group(root_band_index+2, 0, def_band_group(550, ard::BoardItemShape::box)->add(def_topic("manufacture", "Manufacture", h, color::Color::Black)));

            add_link("plan", "schedule", "Instruction");
            add_link("schedule", "manufacture", "Instruction");

            add_link("schedule", "plan", "Feedback");
            add_link("manufacture", "schedule", "Feedback");
        }break;
        default:break;
        }
    }

    return rv;
};

void ard::dragEnterEvent(QGraphicsSceneDragDropEvent *e) 
{
	const QMimeData* md = e->mimeData();
	if (!md)
		return;

	auto tmd = qobject_cast<const ard::TopicsListMime*>(md);
	if (tmd) {
		e->acceptProposedAction();
	}
	else
	{
		if (md->hasUrls() ||
			md->hasText() ||
			md->hasHtml() ||
			md->hasImage())
		{
			e->acceptProposedAction();
		}
	}
};

bool ard::dragMoveEvent(ard::topic* target, QGraphicsSceneDragDropEvent *e) 
{
	if (e->isAccepted())
	{
		//qDebug() << "<<<< folders_board_g_topic::dragMoveEvent";

		const QMimeData* md = e->mimeData();
		if (!md)return false;
		auto tmd = qobject_cast<const ard::TopicsListMime*>(md);
		if (tmd) {
			auto lst = tmd->topics();
			for (auto& i : lst)
			{
				if (i == target) {
					qDebug() << "<<-- ard::dragMoveEvent this-rejected";
					return false;
				}
				if (i->isAncestorOf(target)) {
					qDebug() << "<<-- ard::dragMoveEvent ancestor-rejected";
					return false;
				}
				if (!target->canAcceptChild(i)) {
					qDebug() << "<<-- ard::dragMoveEvent target " << target->dbgHint() << "can't accept" << i->dbgHint();
					return false;
				}
				if (!i->canBeMemberOf(target)) {
					qDebug() << "<<-- ard::dragMoveEvent " << i->title() << "can't be member of" << target->dbgHint();
					return false;
				}
			}
		}


		if (md->hasUrls() ||
			md->hasText() ||
			md->hasHtml() ||
			md->hasImage() ||
			tmd)
		{
			return true;
			qDebug() << "<<-- ard::dragMoveEvent accepted";
		}
	}
	return false;
};

topic_ptr ard::dropTextFile(topic_ptr destination_parent, int destination_pos, const QUrl& url)
{
    QFileInfo fi(url.toLocalFile());
    topic_ptr f = ard::topic::createNewNote(fi.fileName());
    destination_parent->insertItem(f, destination_pos);
    auto c = f->mainNote();
    if (c) {
        QTextCursor cr(c->document());
        c->dropTextFile(url, cr);
    }
    destination_parent->ensurePersistant(1);
    return f;
};

topic_ptr ard::dropImageFile(topic_ptr destination_parent, int destination_pos, const QUrl& url) 
{
	ard::picture* pic = nullptr;
	QImage img;
	auto file_name = url.toLocalFile();
	if (img.load(file_name))
	{
		QFileInfo fi(file_name);

		pic = new ard::picture(fi.baseName());		
		destination_parent->insertItem(pic, destination_pos);
		destination_parent->ensurePersistant(1);
		pic->setFromImage(img);
	}
	return pic;
};

topic_ptr ard::dropHtml(topic_ptr destination_parent, int destination_pos, const QString& ml)
{
    topic_ptr f = ard::topic::createNewNote();
    destination_parent->insertItem(f, destination_pos);
    auto c = f->mainNote();
    if (c) {
        QTextCursor cr(c->document());
        c->dropHtml(ml, cr);
    }
    f->makeTitleFromNote();
    destination_parent->ensurePersistant(1);
    return f;
};

topic_ptr ard::dropText(topic_ptr destination_parent, int destination_pos, const QString& str)
{
    topic_ptr f = ard::topic::createNewNote();
    destination_parent->insertItem(f, destination_pos);
    auto c = f->mainNote();
    if (c) {
        QTextCursor cr(c->document());
        c->dropText(str, cr);
    }
    f->makeTitleFromNote();
    destination_parent->ensurePersistant(1);
    return f;
};

topic_ptr ard::dropUrl(topic_ptr destination_parent, int destination_pos, const QUrl& u)
{
    auto f = ard::anUrl::createUrl();
    destination_parent->insertItem(f, destination_pos);
    auto c = f->mainNote();
    if (c) {
        QTextCursor cr(c->document());
        c->dropUrl(u, cr);
    }
    f->makeTitleFromNote();
    destination_parent->ensurePersistant(1);
    return f;
};

topic_ptr ard::dropImage(topic_ptr destination_parent, int destination_pos, const QImage& img) 
{
	assert_return_null(destination_parent, "expected destination parent");

	auto pic = new ard::picture("new picture");
	auto res = destination_parent->insertItem(pic, destination_pos);
	if (!res) {
		qWarning() << "Failed to insert image into " << destination_parent->dbgHint();
		return nullptr;
	}
	destination_parent->ensurePersistant(1);
	pic->setFromImage(img);
	return pic;
};


QString textFilesExtension()
{
    QString text_files = "*.txt *.text *.log *.h *.cpp *.java *.h *.cpp *.java *.go";
    return text_files;
}

QString imageFilesExtension()
{
	QString image_files = "*.png *.jpg *.bmp";
	return image_files;
}


TOPICS_LIST ard::insertClipboardData(topic_ptr destination_parent, int destination_pos, const QMimeData* mm, bool dragMoveContent)
{
	TOPICS_LIST rv;
    //topic_ptr created = nullptr;

    if (mm->hasHtml())
    {
        auto created = ard::dropHtml(destination_parent, destination_pos, mm->html());
		if (created)rv.push_back(created);
    }
    else if (mm->hasUrls())
    {
        static QString text_files = textFilesExtension();
		static QString image_files = imageFilesExtension();
        foreach(QUrl url, mm->urls())
        {
			topic_ptr created = nullptr;
            if (url.isLocalFile())
            {
                QFileInfo fi(url.toLocalFile());
                auto s = "*." + fi.suffix().toLower();
                if (text_files.indexOf(s) != -1) {
                    created = ard::dropTextFile(destination_parent, destination_pos, url);
                }
				else 
				{
					if (image_files.indexOf(s) != -1) {
						created = ard::dropImageFile(destination_parent, destination_pos, url);
					}
				}
            }
            else
            {
                created = ard::dropUrl(destination_parent, destination_pos, url);
            }
			if (created)rv.push_back(created);
        }
    }
    else if (mm->hasText())
    {
        auto created = ard::dropText(destination_parent, destination_pos, mm->text());
		if (created)rv.push_back(created);
    }
	else if (mm->hasImage()) {
		QImage img = qvariant_cast<QImage>(mm->imageData());
		auto created = ard::dropImage(destination_parent, destination_pos, img);
		if (created)rv.push_back(created);
	}
    else 
    {
		auto gm = ard::gmail_model();
        auto tmd = qobject_cast<ard::TopicsListMime*>((QMimeData*)mm);
        if (tmd) {
            auto lst = ard::reduce2ancestors(tmd->topics());
            for (auto& i1 : lst) 
            {
				auto i = i1->produceMoveSource(gm);
				if (!i) {
					ard::error(QString("failed to produce move source").arg(i1->title().left(20)));
					continue;
				}
                auto p = i->parent();
				if (!p) {
					ASSERT(0, "expected parent");
					return rv;
				}
				//assert_return_null(p, "expected parent");
                if (COMPATIBLE_PARENT_CHILD(destination_parent, i))
                {
					if (dragMoveContent) 
					{
						p->detachItem(i);
						destination_parent->insertItem(i, destination_pos);
						destination_pos++;
						rv.push_back(i);
					}
					else 
					{
						auto f = i->clone();
						destination_parent->insertItem(f, destination_pos);
						destination_pos++;
						rv.push_back(f);
					}
                }
            }
        }
    }

    destination_parent->ensurePersistant(1);

	//auto h = destination_parent->getLocusFolder();
	//if (h) {
		//ard::rebuildFoldersBoard(h);////@todo:ykh noooo!
	//}

    return rv;
};

ard::topic* ard::moveInsideParent(ard::topic* it, bool moveUp)
{
	ard::topic* rv = nullptr;

	if (!it->canMove())
	{
		ASSERT(0, "can not move") << it->dbgHint();
		return nullptr;
	}

	auto parent = it->parent();
	assert_return_null(parent, "expecterd parent");
	int idx = parent->indexOf_cit(it);
	assert_return_null(idx != -1, "Invalid index value");

	parent->ensurePersistant(1);

	int old_idx = idx;
	if (moveUp)
	{
		idx--;
		if (idx < 0)
			idx = 0;
	}
	else
	{
		idx++;
		if (idx >= (int)parent->items().size())
			idx = parent->items().size() - 1;
	}

	if (old_idx != idx)
	{
		auto it2 = dynamic_cast<ard::topic*>(parent->items()[idx]);
		assert_return_null(it2, "expecterd topic");
		ASSERT(it != it2, "Internal logic error");
		if (!it2->canMove())
		{
			ASSERT(0, "can not move") << it2->dbgHint();
			return nullptr;
		}

		int pindex1 = it->pindex();
		int pindex2 = it2->pindex();
		it->setPindex(pindex2);
		it2->setPindex(pindex1);

		if (it->isSyncDbAttached() && it->isSynchronizable())
		{
			it->setSyncMoved();
			it->ask4persistance(np_SYNC_INFO);
		}

		auto tmp = parent->items()[old_idx];
		ASSERT(tmp == it, "Inner logical error.");
		parent->items()[old_idx] = it2;
		parent->items()[idx] = it;
		parent->rebuildCache();
		parent->ensurePersistant(1);
		rv = it2;

		auto h = parent->getLocusFolder();
		if (h) {
			ard::rebuildFoldersBoard(h);
		}
	}

	return rv;
};


email_ptr ard::currentEmail()
{
    email_ptr m = nullptr;
    auto f = ard::currentTopic();
    if (f) {
        m = ard::as_email(f);
    }
    return m;
};

ethread_ptr ard::currentThread()
{
    ethread_ptr t = nullptr;
    auto f = ard::currentTopic();
    if (f) {
        t = dynamic_cast<ard::ethread*>(f);
    }
    return t;
};

ard::KRingKey* ard::currentKRingKey()
{
    ard::KRingKey* k = nullptr;
    auto f = ard::currentTopic();
    if (f) {
        k = dynamic_cast<ard::KRingKey*>(f);
    }
    return k;
};

QString ard::recoverEmailAddress(QString name_ref)
{
	QString email;
	int idx = name_ref.indexOf("<");
	if (idx != -1) {
		email = name_ref.mid(idx + 1).remove(">").trimmed();
	}
	else {
		if (name_ref.indexOf("@") != -1) {
			email = name_ref;
		}
	}
	return email;
};


void ard::setup_menu(QMenu* m) 
{
#ifdef ARD_BIG
    Q_UNUSED(m);
#else
    m.setFont(*defaultFont());
#endif
};

void ard::messageBox(QWidget* parent, QString msg)
{
	QMessageBox::information(parent, "Ardi", msg);
};

void ard::errorBox(QWidget* parent, QString s) 
{
	ASSERT(0, "ERROR message:" + s);
	QMessageBox::critical(parent, "Ardi", s);
};

QWidget* ard::newSpacer(bool expandingY)
{
	auto spacer = new QWidget();
	spacer->setSizePolicy(QSizePolicy::Expanding, expandingY ? QSizePolicy::Expanding : QSizePolicy::Minimum);
	return spacer;
};

void ard::fetchHttpData(const QUrl& url, FetchCallback cb, std::map<QString, QString>* rawHeaders)
{
	QNetworkRequest r(url);
	QNetworkAccessManager *m = new QNetworkAccessManager(gui::mainWnd());
	if (rawHeaders) {
		for (const auto& p : *rawHeaders) {
			r.setRawHeader(p.first.toStdString().c_str(), p.second.toStdString().c_str());
		}
	}
	QObject::connect(m, &QNetworkAccessManager::finished, [m, cb](QNetworkReply* r) 
	{
		auto err = r->error();
		if (err != QNetworkReply::NoError)
		{
			qWarning() << "fetchHttpData error:" << static_cast<int>(err) << r->errorString();
		}

		cb(r);
		m->deleteLater();
	});
	m->get(r);
};

void ard::downloadHttpData(const QUrl& url, QString destinationFile, FetchCallback cb, std::map<QString, QString>* rawHeaders)
{
	QNetworkRequest r(url);
	QNetworkAccessManager *m = new QNetworkAccessManager(gui::mainWnd());
	if (rawHeaders) {
		for (const auto& p : *rawHeaders) {
			r.setRawHeader(p.first.toStdString().c_str(), p.second.toStdString().c_str());
		}
	}

	if (QFile::exists(destinationFile)) {
		if (!QFile::remove(destinationFile)) {
			qWarning() << "error failed to remove local destination file" << destinationFile;
		}
	}

	QObject::connect(m, &QNetworkAccessManager::finished, [m, cb, destinationFile](QNetworkReply* r)
	{
		auto err = r->error();
		if (err != QNetworkReply::NoError) 
		{
			qWarning() << "downloadHttpData error:" << static_cast<int>(err) << r->errorString();
		}
		else
		{
			QByteArray b = r->readAll();
			QFile file(destinationFile);
			file.open(QIODevice::WriteOnly);
			file.write(b);
		}

		cb(r);
		m->deleteLater();
	});
	m->get(r);
};

void ard::killTopicsSilently(TOPICS_LIST& lst) 
{
	if (!lst.empty()) 
	{
		auto l2 = ard::reduce2ethreads(lst.begin(), lst.end());
		auto m = ard::gmail_model();
		if (m)
		{
			m->trashThreadsSilently(l2);
		}
		for (auto& t : lst) {
			t->killSilently(true);
		}
	}
};
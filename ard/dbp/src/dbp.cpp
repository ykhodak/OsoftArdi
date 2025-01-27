#include <QtCore>
#include <QFile>
#include <QSqlError>
#include <QUrl>
#include <QTime>
#include <QFileInfo>
#include <QDir>
#include <QProgressDialog>
#include <QFutureWatcher>
#include <QPlainTextEdit>
#include <QtConcurrent/QtConcurrent>

#include <iostream>
#include <time.h>
#include "a-db-utils.h"
#include "dbp.h"
#include "anfolder.h"
#include "ansyncdb.h"
#include "a-db-utils.h"
//#include "meta-utils.h"
#include "ard-db.h"
#include "contact.h"
#include "rule.h"
#include "board.h"
#include "extnote.h"

static ArdDB                     the_db;
static QString                   __currentDBName = "";
static QString                   __lastDBName = "";
static QString                   __lastUsedPWD = "";
static bool                      __dbReadOnlyMode = false;

extern QString get_db_file_path();
extern QString get_db_tmp_file_path(QString repositoryPath = "");
extern void clear_limbo_file_set();
extern void ard_dir_set_curr_root(QString dir_path);
extern void setCustomDBPathName(QString s);

ArdDB& dbp::defaultDB(){return the_db;};

bool is_DB_read_only_mode()
{
    bool rv = is_instance_read_only_mode() || __dbReadOnlyMode;
    return rv;
};

namespace dbp
{
    bool             initdb();
    bool             reinit();
    void             bounce();
    void             setCurrDBName(QString s);
};


/**
   db-containers
*/

RootTopic* dbp::root(){return the_db.root();};
topic_ptr dbp::threads_root() { return the_db.threads_root(); };
bool gui::isDBAttached(){return (the_db.isOpen() && the_db.root() != NULL);};

bool ard::isDbConnected(){return gui::isDBAttached();};

void dbp::show_last_sql_err(QSqlQuery& q, QString sql)
{
    QString error = q.lastError().text();

    QString s = QString("%1\n%2").arg(error).arg(sql);
    std::cout << s.toStdString() << std::endl;

    s += "\n";
    s += getLastStackFrames(2);
    ASSERT(0, s) << sql;
	ard::errorBox(ard::mainWnd(), s);
}

void dbp::setCurrDBName(QString s)
{
    __currentDBName = s;
    if(!s.isEmpty())
        __lastDBName = s;
};

bool dbp::initdb()
{
    __dbReadOnlyMode = false;

    ASSERT(!the_db.isOpen(), "Expected closed DB");
    QString db_path = get_db_file_path();

    //  qDebug() << "<<<<<<<<<init:" << db_path << __currentDBName;

    if (is_instance_read_only_mode())
    {
        qDebug() << "<<<<<<<<<entering read-only mode";
        db_path = get_db_tmp_file_path();
        if (QFile::exists(get_db_tmp_file_path()))
        {
            if (!QFile::remove(get_db_tmp_file_path()))
            {
				QString s = "Can not use read-only mode. Reason - the file " + get_db_tmp_file_path() + " can not be deleted.";
				ard::errorBox(ard::mainWnd(), s);
                exit(0);
            };
        }

        if (QFile::exists(get_db_file_path()))
        {
            if (!QFile::copy(get_db_file_path(), get_db_tmp_file_path()))
            {
				QString s = "Can not use read-only mode. Reson - the file " + get_db_tmp_file_path() + " can not be created";
				ard::errorBox(ard::mainWnd(), s);
                exit(0);
            }
        }
        qWarning() << "using temporary DB file" << db_path;
    }

    bool rv = the_db.openAsMainDb("data-db", db_path);
    if (!rv || !the_db.isOpen())
    {
        QFileInfo fi(db_path);
        QString dirPath = fi.absolutePath();
        QDir di(dirPath);
        bool fileExist = QFile::exists(db_path);
        bool folderExist = di.exists();
		QString s = QString("Failed to open DB: %1 dir-exist:%2 file-exists:%3 dir-path:%4").arg(db_path).arg(folderExist).arg(fileExist).arg(dirPath);
		ard::errorBox(ard::mainWnd(), s);
        return false;
    }
    if (rv)rv = the_db.verifyMetaData();
    if (rv)
    {
        bool ROmodeRequest = false;
        if (!guiCheckPassword(ROmodeRequest, &the_db))
        {
            bool continueInROmode = false;

            the_db.close();
            if (ROmodeRequest)
            {
                QFileInfo fi(db_path);
                QString rpath = fi.absolutePath() + "/";
                QString tmp_db_path2 = get_db_tmp_file_path(rpath);
                qWarning() << "opening read-only-mode" << db_path << rpath << tmp_db_path2;

                //QFileInfo fi2(tmp_db_path2);
                if ((QFile::exists(tmp_db_path2)))
                {
                    if (!QFile::remove(tmp_db_path2))
                    {
						ard::errorBox(ard::mainWnd(), QString("Can't open DB in read-only mode because the following temporary file can't be deleted: '%1' Please reboot your computer of close program that has locked the file").arg(tmp_db_path2));
                        return false;
                    };
                }
                if (!QFile::copy(db_path, tmp_db_path2))
                {
					ard::errorBox(ard::mainWnd(), QString("Can't open DB in read-only mode because file copy operation failed: %1 %2").arg(db_path).arg(tmp_db_path2));
                    return false;
                }

                rv = the_db.openAsMainDb("data-db", tmp_db_path2);
                if (!rv || !the_db.isOpen())
                {
					QString s = "Failed to open DB(ro) " + db_path;
					ard::errorBox(ard::mainWnd(), s);
                    return false;
                }
                if (rv)rv = the_db.verifyMetaData();
                if (rv)continueInROmode = true;
            }

            if (!continueInROmode)
            {
                if (the_db.isOpen())
                {
                    the_db.close();
                }
                return false;
            }

            __dbReadOnlyMode = true;

            ASSERT(the_db.isOpen(), "expected open database");
            //qWarning() << "opened read-only-mode" << the_db.db().databaseName();
        }
    }

    if (rv)
    {
        rv = the_db.loadTree();
    }

    return rv;
}

#ifdef _DEBUG
extern void print_smart_counters_in_memory();
#endif

void dbp::close(bool keepLastPWDInMemory /*= true*/)
{
    if(gui::isDBAttached())
        {
            gui::finalSaveCall();
            gui::detachGui();
            the_db.close();      
            setCurrDBName("");
            if(!keepLastPWDInMemory)
                {
                    __lastUsedPWD = "";
                }

            __dbReadOnlyMode = false;

#ifdef _DEBUG
            QTimer::singleShot(100, [=]() {
                print_smart_counters_in_memory();
            });
            gui::sleep(300);
#endif
        }
};

bool dbp::reinit()
{
    bool rv = initdb();
    if(rv){
        //dbp::loadDBSettings();
        gui::attachGui();
    }
    return rv;
};

bool dbp::reopenLast()
{
    bool rv = false;
    if(!__lastDBName.isEmpty())
        {
            rv = openStandardPath(__lastDBName);
        }
    return rv;
};

bool dbp::create(QString dbName, QString pwd, QString hint)
{
    QString dirPath = defaultRepositoryPath() + "dbs/" + dbName + "/";
    QDir d(dirPath);
    if(d.exists(dirPath))
        {
            ASSERT(0, "Directory already exists") << dirPath;
            return false;
        }
    if(!d.mkpath(dirPath))
        {
            ASSERT(0, "Failed to create directory:") << dirPath;
            return false;
        }

    close();
    ard_dir_set_curr_root(dirPath);
    setCurrDBName(dbName);
    setCustomDBPathName("");
    reinit();
    if(!pwd.isEmpty())
        {
            dbp::configChangePwd(&dbp::defaultDB(), "", pwd, hint, false);
        }
    return true;
};

bool dbp::safeGuiOpenStandardPath(QString dbName) 
{
    bool ok = openStandardPath(dbName);
    if (!ok) {
        if (dbName != DEFAULT_DB_NAME) {
            //could be simple case - local repo folder was deleted and we can switch to 'default'
            QString dirPath = defaultRepositoryPath() + "dbs/" + dbName + "/";
            QDir d(dirPath);
            if (!d.exists(dirPath)) {
				ard::errorBox(ard::mainWnd(), QString("Local file '%1' not found, opening default repository").arg(dbName));
                ok = openStandardPath(DEFAULT_DB_NAME);
            };
        }
    }
    return ok;
}

bool dbp::openStandardPath(QString dbName)
{
    QString dirPath = "";
    if(dbName == DEFAULT_DB_NAME)
        {
            dirPath = defaultRepositoryPath();
        }
    else
        {
            dirPath = defaultRepositoryPath() + "dbs/" + dbName + "/";
            QDir d(dirPath);
            if(!d.exists(dirPath))
                {
                    ASSERT(0, "Directory not found:") << dirPath;
                    return false;
                }      
        }

    setCustomDBPathName("");
    configFileSetLastDB(dbName);
  
    close();
    ard_dir_set_curr_root(dirPath);
    setCurrDBName(dbName);
    return reinit();
    //return true;
};

bool dbp::openAbsolutePath(QString dbPathName)
{
    QFileInfo fi(dbPathName);
    QString dbName = fi.absolutePath();
  
    QDir d(dbName);
    if(!d.exists(dbName))
        {
            ASSERT(0, "Directory not found:") << dbName;
            return false;
        }
  
    close();
    ard_dir_set_curr_root(dbName);
    setCurrDBName(dbName);///ykh! - currently it's broken for local file|open
    setCustomDBPathName(dbPathName);
    reinit();
  
    return true;
};

QString gui::currentDBName()
{
    return dbp::currentDBName();
};

QString dbp::currentDBName()
{
    return __currentDBName;
};

QString dbp::lastUsedPWD()
{
    return __lastUsedPWD;
};

void dbp::setLastUsedPWD(QString s)
{
    __lastUsedPWD = s;
};

extern bool isDefaultDBName(QString db_name);
extern QString autoTestDbName();

QString dbp::compositRemoteDbPrefix4CurrentDB(QString& localDbPrefix)
{
    QString db_name = dbp::currentDBName();
    if (isDefaultDBName(db_name))
    {
        db_name = "";
    }

    QString composite_remote_db_prefix = "";

    if (!db_name.isEmpty())
    {
        localDbPrefix = db_name;
        composite_remote_db_prefix = db_name;

        if (db_name.indexOf(autoTestDbName()) == 0)
        {
            composite_remote_db_prefix = autoTestDbName();
        }
    }

    return composite_remote_db_prefix;
};

void dbp::load_note(ard::note_ext* n) 
{
    auto o = n->owner();
    assert_return_void(o, "expected owner");
    assert_return_void(o->dataDb(), "expected DB");
    assert_return_void(o->dataDb()->isOpen(), "expected open DB");
    QString sql = QString("SELECT note_text, note_plain_text FROM ard_ext_note WHERE oid = %1").arg(o->id());
    auto q = o->dataDb()->selectQuery(sql);
    if (q->next())
    {
        QString html = q->value(0).toString();
        QString plain_text = q->value(1).toString();
        n->setupFromDb(html, plain_text);
    }
};

bool dbp::create_board_links(ArdDB* db, QString board_syid, const std::vector<ard::board_link*>& links)
{
    assert_return_false(db, "expected DB");
    assert_return_false(IS_VALID_SYID(board_syid), "expected valid board syid");

    int last_linkid = db->queryInt("SELECT MAX(linkid) FROM ard_board_links");
    last_linkid++;

    QString sql = QString("INSERT INTO ard_board_links(linkid, board, origin, target, syid, pindex, link_label, mdc) VALUES(?, '%1', ?, ?, ?, ?, ?, ?)").arg(board_syid);
    auto q = db->prepareQuery(sql);
    if (!q) {
        qWarning() << "ERROR. Failed to prepare query board_link" << sql;
        return false;
    }

    db->startTransaction();
    QVariantList linkid, origins, targets, syid, pindex, link_labels, mdc;
    for (const auto& lnk : links) {
        linkid      << last_linkid;
        origins     << lnk->origin();
        targets     << lnk->target();
        syid        << lnk->link_syid();
        pindex      << lnk->linkPindex();
        link_labels << lnk->linkLabel();
        mdc         << lnk->mdc();
        lnk->setupLinkIdFromDb(last_linkid);

        last_linkid++;
    }

    q->addBindValue(linkid);
    q->addBindValue(origins);
    q->addBindValue(targets);
    q->addBindValue(syid);
    q->addBindValue(pindex);
    q->addBindValue(link_labels);
    q->addBindValue(mdc);

    if (!q->execBatch())
    {
        db->rollbackTransaction();
        qWarning() << "=========== begin/store links error =============";
        QString error = q->lastError().text();
        QString s = QString("%1\n%2").arg(error).arg(sql);
        qWarning() << s;
        qWarning() << "board:" << board_syid;
        for (const auto& lnk : links) {
            lnk->setupLinkIdFromDb(0);
            qWarning() << lnk->origin() << lnk->target();
        }
        qWarning() << "=========== end/store links error =============";
        dbp::show_last_sql_err(*q, sql);
        return false;
    }
    /*
    else {
        qWarning() << "=========== begin/added links =============";
        qWarning() << "board:" << board_syid;
        for (const auto& lnk : links) {         
            qWarning() << lnk->linkid() << lnk->origin() << "->"<< lnk->target();
        }
        qWarning() << "=========== end/added links =============";
    }*/
    db->commitTransaction();

    return true;
};

bool dbp::remove_board_links(ArdDB* db, const std::vector<ard::board_link*>& links)
{
    assert_return_false(db, "expected DB");
    QString sql = QString("DELETE FROM ard_board_links WHERE linkid=?");
    auto q = db->prepareQuery(sql);
    if (!q) {
        qWarning() << "ERROR. Failed to prepare query board_link" << sql;
        return false;
    }

    db->startTransaction();
    QVariantList linkid;
    for (const auto& lnk : links) {
        linkid << lnk->linkid();
    }

    q->addBindValue(linkid);

    if (!q->execBatch())
    {
        db->rollbackTransaction();
        dbp::show_last_sql_err(*q, sql);
        return false;
    }
    db->commitTransaction();
    return true;
};

bool dbp::update_board_links(ArdDB* db, const std::vector<ard::board_link*>& links)
{
    assert_return_false(db, "expected DB");
    QString sql = QString("UPDATE ard_board_links SET mdc=?, pindex=?, link_label=? WHERE linkid=?");
    auto q = db->prepareQuery(sql);
    if (!q) {
        qWarning() << "ERROR. Failed to prepare query board_link" << sql;
        return false;
    }

    db->startTransaction();
    QVariantList mdc, pindex, linkid, link_label;
    for (const auto& lnk : links) {
        mdc         << lnk->mdc();
        pindex      << lnk->linkPindex();
        link_label  << lnk->linkLabel();
        linkid      << lnk->linkid();
    }

    q->addBindValue(mdc);
    q->addBindValue(pindex);
    q->addBindValue(link_label);
    q->addBindValue(linkid);

    if (!q->execBatch())
    {
        db->rollbackTransaction();
        qWarning() << "=========== begin/store links error =============";
        QString error = q->lastError().text();
        QString s = QString("%1\n%2").arg(error).arg(sql);
        qWarning() << s;
        for (const auto& lnk : links) {
            qWarning() << lnk->origin() << lnk->target();
        }
        qWarning() << "=========== end/store links error =============";
        dbp::show_last_sql_err(*q, sql);
        return false;
    }
    db->commitTransaction();
    return true;
};

static QString idlist2str(IDS_LIST& oids)
{
    QString s_oids = ardi_functional::idsToStr(oids.begin(), oids.end());
    return s_oids;
}

#define RETURN_WITH_LOG_ARD_ERROR {ASSERT(0, "SQL ARD_ERROR") << sql;return false;}

static bool removeItemsByID(ArdDB* db, IDS_LIST& oids, DB_ID_TYPE, DB_ID_TYPE)
{
    assert_return_false(db, "expected DB");
    QString s_oids = idlist2str(oids);

    QString sql;

    sql = QString("DELETE FROM ard_tree WHERE oid IN (%1)").arg(s_oids);
    if(!db->execQuery(sql))RETURN_WITH_LOG_ARD_ERROR;

    sql = QString("DELETE FROM ard_ext_draft WHERE oid IN (%1)").arg(s_oids);
    if(!db->execQuery(sql))RETURN_WITH_LOG_ARD_ERROR;

    sql = QString("DELETE FROM ard_ext_q WHERE oid IN (%1)").arg(s_oids);
    if (!db->execQuery(sql))RETURN_WITH_LOG_ARD_ERROR;

    sql = QString("DELETE FROM ard_ext_ethread WHERE oid IN (%1)").arg(s_oids);
    if (!db->execQuery(sql))RETURN_WITH_LOG_ARD_ERROR;  

    sql = QString("DELETE FROM ard_ext_contact WHERE oid IN (%1)").arg(s_oids);
    if (!db->execQuery(sql))RETURN_WITH_LOG_ARD_ERROR;

    sql = QString("DELETE FROM ard_ext_kring WHERE oid IN (%1)").arg(s_oids);
    if (!db->execQuery(sql))RETURN_WITH_LOG_ARD_ERROR;

    sql = QString("DELETE FROM ard_ext_note WHERE oid IN (%1)").arg(s_oids);
    if (!db->execQuery(sql))RETURN_WITH_LOG_ARD_ERROR;  

	sql = QString("DELETE FROM ard_ext_rule WHERE oid IN (%1)").arg(s_oids);
	if (!db->execQuery(sql))RETURN_WITH_LOG_ARD_ERROR;

    return true;
}

static bool removeBoardsByID(ArdDB* db, IDS_LIST& oids, DB_ID_TYPE, DB_ID_TYPE)
{
    assert_return_false(db, "expected DB");
    QString s_oids = idlist2str(oids);

    QString sql;

    sql = QString("DELETE FROM ard_board_links WHERE board IN (SELECT syid FROM ard_tree WHERE oid IN(%1))").arg(s_oids);
    if (!db->execQuery(sql))RETURN_WITH_LOG_ARD_ERROR;

    sql = QString("DELETE FROM ard_ext_bboard WHERE oid IN (%1)").arg(s_oids);
    if (!db->execQuery(sql))RETURN_WITH_LOG_ARD_ERROR;

    sql = QString("DELETE FROM ard_ext_bboard_item WHERE oid IN (SELECT oid from ard_tree WHERE pid=%1)").arg(s_oids);
    if (!db->execQuery(sql))RETURN_WITH_LOG_ARD_ERROR;

    sql = QString("DELETE FROM ard_tree WHERE pid IN (%1)").arg(s_oids);
    if (!db->execQuery(sql))RETURN_WITH_LOG_ARD_ERROR;

    sql = QString("DELETE FROM ard_tree WHERE oid IN (%1)").arg(s_oids);
    if (!db->execQuery(sql))RETURN_WITH_LOG_ARD_ERROR;

    return true;
}

static bool removeBoardItemExByID(ArdDB* db, IDS_LIST& oids, DB_ID_TYPE, DB_ID_TYPE)
{
    assert_return_false(db, "expected DB");
    QString s_oids = idlist2str(oids);

    QString sql;
    sql = QString("DELETE FROM ard_ext_bboard_item WHERE oid IN (%1)").arg(s_oids);
    if (!db->execQuery(sql))RETURN_WITH_LOG_ARD_ERROR;

    return true;
}

static bool SQL_UPDATE_OR_DELETE_BY_ID(ArdDB* db, IDS_LIST& nids, QString sql_with_1_parameter)
{
    QString s_nids = idlist2str(nids);
    QString sql = sql_with_1_parameter.arg(s_nids);
    if(!db->execQuery(sql))return false;
    return true;
}

static bool removeQsID(ArdDB* db, IDS_LIST& nids, DB_ID_TYPE, DB_ID_TYPE)
{
    QString sql_with_1_parameter = QString("DELETE FROM ard_ext_q WHERE oid IN (%1)");
    return SQL_UPDATE_OR_DELETE_BY_ID(db, nids, sql_with_1_parameter);
}

static bool removeDraftsID(ArdDB* db, IDS_LIST& nids, DB_ID_TYPE, DB_ID_TYPE)
{
    QString sql_with_1_parameter = QString("DELETE FROM ard_ext_draft WHERE oid IN (%1)");
    return SQL_UPDATE_OR_DELETE_BY_ID(db, nids, sql_with_1_parameter);
}

template <class T>
bool process_list(ArdDB* db, std::vector<T*>& lst, FUN_IDS_DB_PROCESSOR ids_processor, DB_ID_TYPE id_param, DB_ID_TYPE param2)
{
    IDS_LIST    ids_list;
    typedef typename std::vector<T*>::reverse_iterator ITR;

    for (ITR i = lst.rbegin(); i != lst.rend(); i++)
    {
        T* o = *i;
        DB_ID_TYPE id = o->id();
        ids_list.push_back(id);
        if (ids_list.size() == MAX_IN_SQL_LIST_STEP)
        {
            if (!ids_processor(db, ids_list, id_param, param2))
                return false;
            ids_list.clear();
        }
    }

    if (!ids_list.empty())
    {
        if (!ids_processor(db, ids_list, id_param, param2))
            return false;
    }

    return true;
}

bool dbp::removeTopics(ArdDB* ard_db, TOPICS_LIST& topics_list)
{
    if(!process_list(ard_db, topics_list, removeItemsByID, 0, 0))
        {
            return false;
        };

    //----- something should be done about items_list ---------
    /*
    TOPICS_SET topics2clean_dependet;
    for(auto& f : topics_list)    {
            if(f->isProbablyDependTarget())
                {
                    topics2clean_dependet.insert(f);
                }
        }

    if(topics2clean_dependet.size() > 0)
        {
            qDebug() << "DEPENDANT-2-CLEAN" << topics2clean_dependet.size();
            ard_db->cleanupDepending(topics2clean_dependet);
        }
        */
    return true;
}

bool dbp::removeBoards(ArdDB* ard_db, TOPICS_LIST& blist)
{
    if (!process_list(ard_db, blist, removeBoardsByID, 0, 0))
    {
        return false;
    };
    return true;
};

bool dbp::removeBoardItemEx(ArdDB* ard_db, TOPICS_LIST& blist) 
{
    if (!process_list(ard_db, blist, removeBoardItemExByID, 0, 0))
    {
        return false;
    };
    return true;
};

bool dbp::removeQsByOID(ArdDB* db, IDS_SET ids)
{
    return db->sql_process_id_set(ids, ::removeQsID, 0, 0);
};


bool dbp::removeDraftsByOID(ArdDB* db, IDS_SET ids) 
{
    return db->sql_process_id_set(ids, ::removeDraftsID, 0, 0);
}; 

void commit_db_modifications();

static bool register_db_modification_enabled = true;

///@todo: we can get rid of all these SETS - just direct call to commit code
typedef std::set<EDB_MOD> MOD_SET;
typedef std::map<topic_ptr, MOD_SET> ITEM_2_MOD_SET;

static ITEM_2_MOD_SET item2modset;

void register_db_modification(topic_ptr it, EDB_MOD mod, bool reg_sync_mod /*= true*/)
{
    // assert_return_void(it->dataDb(), "extected DB object");
    if (!it->isPersistant())
    {
        return;
    }

    if (register_db_modification_enabled)
    {
        if (IS_VALID_DB_ID(it->id()))
        {
            LOCK(it);
            if (reg_sync_mod && it->isSyncDbAttached())
            {
                if (mod == dbmodMoved)
                    it->setSyncMoved();
                else
                    it->setSyncModified();
            }
            item2modset[it].insert(mod);
            commit_db_modifications();
        }
    }
};


#define REGISTER_AND_COMMIT_EXT_MODIFICATION(S, C)                  \
    if(register_db_modification_enabled && ex->hasDBRecord())       \
        {                                                           \
            LOCK(ex);                                               \
            topic_ptr it = dynamic_cast<topic_ptr>(ex->cit_owner());    \
            assert_return_void(it, "expected owner");            \
            if(it->isSyncDbAttached())                              \
                {                                                   \
                    ex->setSyncModified();                          \
                }                                                   \
            S[ex].insert(mod);                                      \
            C();                                                    \
        }                                                           \



void enable_register_db_modification(bool enable)
{
    register_db_modification_enabled = enable;
};

void commit_db_modifications()
{
	time_t mod_time = QDateTime::currentDateTime().toTime_t();

	for (auto& i : item2modset) {
		topic_ptr it = i.first;
#ifdef _DEBUG
		ASSERT((it->dataDb() != NULL), "expected valid DB pointer");
#endif
		it->set_mod_time(mod_time);
		MOD_SET& mset = i.second;
		QString s_set = QString("mod_time=%1").arg(mod_time);
		bool bind_title = false;
		bool bind_annotation = false;
		for (const auto& mod : mset) {
			switch (mod)
			{
			case dbmodToDo:
			{
				s_set += QString(", todo=%1, todo_done=%2").arg(it->getToDoPriorityAsInt()).arg(it->getToDoDonePercent());
				//  ASSERT(0, "dbmodToDo")<< it->dbgHint();
			}break;
			case dbmodRetired:
			{
				s_set += QString(", retired=%1").arg(it->mflag4serial());
			}break;
			case dbmodAnnotation:
			{
				s_set += QString(", annotation=?");
				bind_annotation = true;
			}break;
			case dbmodTitle:
			{
				s_set += QString(", title=?");
				bind_title = true;
			}break;
			case dbmodMoved:
			{
				auto parent_f = it->parent();
				if (parent_f != NULL)
				{
					s_set += QString(", pid=%1, pindex=%2").arg(parent_f->id()).arg(it->pindex());
				}
			}break;
			case dbmodForceModified:break;//time&sync info is all we want to change
			default:ASSERT(0, "invalid modification mark in commit_db_modifications");
			}
		}//for mset

		if (it->isSyncDbAttached()) {
			s_set += QString(", mdc=%1, mvc=%2").arg(it->modCounter()).arg(it->moveCounter());
		}

		QString sql = QString("UPDATE ard_tree SET ") + s_set + QString(" WHERE oid=%1").arg(it->id());
		auto q = it->dataDb()->prepareQuery(sql);
		if (!q) {
			return;
		}

		if (bind_title) {
			q->addBindValue(it->serializable_title());
		}
		if (bind_annotation) {
			q->addBindValue(it->annotation());
		}
		q->exec();
	}

	for (auto& i : item2modset) {
		topic_ptr it = i.first;
		it->release();
	}
	item2modset.clear();
}

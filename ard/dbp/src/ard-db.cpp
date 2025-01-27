#include <QSqlDatabase>
#include <QDateTime>
#include <QCryptographicHash>
#include <QFileInfo>
#include <QSqlError>
#include <algorithm>
#include <QFileDialog>
#include <ctime>
#include <thread>

#include "snc-tree.h"
#include "dbp.h"
#include "ard-db.h"
#include "a-db-utils.h"
#include "anfolder.h"
#include "ansyncdb.h"
#include "syncpoint.h"
#include "email.h"
#include "ethread.h"
#include "ard-algo.h"
#include "email_draft.h"
#include "locus_folder.h"
#include "ansearch.h"
#include "contact.h"
#include "rule.h"
#include "kring.h"
#include "board.h"
#include "fileref.h"
#include "extnote.h"
#include "anurl.h"
#include "rule.h"
#include "rule_runner.h"
#include "picture.h"

ArdDB::ArdDB():
    m_mainDB(false),
    m_WeeklyBackupChecked(false)
{
};

ArdDB::~ArdDB()
{
    clear();
};


snc::cdb* ArdDB::syncDb()
{
    RootTopic* r = root();
    if(!r)
        {
            ASSERT(0, "expected root topic");
            return nullptr;
        }
    return &(r->db());
};

const snc::cdb* ArdDB::syncDb()const
{
    const RootTopic* r = root();
    if(r == NULL)
        {
            ASSERT(0, "expected root topic");
            return NULL;
        }
    return &(r->db());
};

bool ArdDB::hasRoot()const
{
    if (!m_root) {
        return false;
    }
    return true;
};

RootTopic* ArdDB::root()
{
    if(!m_root){
        ASSERT(0, "expected data root");
            //qWarning() << "EXCEPTION: no root " << getLastStackFrames(7);
            //throw dbp_exception(1);
        }
    return m_root;
}

const RootTopic* ArdDB::root()const
{
    if(!m_root){
        ASSERT(0, "expected data root");
            //qWarning() << "EXCEPTION: no root " << getLastStackFrames(7);
            //throw dbp_exception(1);
        }
    return m_root;
}


#define EXEC_SQL(sql)       if(!_db->execQuery(sql)){return false;};


static bool create_indexes(ArdDB* _db)
{
    EXEC_SQL("CREATE INDEX IF NOT EXISTS ard_tree_pid_idx ON ard_tree(pid ASC)");
    EXEC_SQL("CREATE UNIQUE INDEX IF NOT EXISTS ard_dbs_db_id_idx ON ard_dbs(db_id)");
    EXEC_SQL("CREATE UNIQUE INDEX IF NOT EXISTS ard_dbh_idx ON ard_dbh(db_oid, synced_db_id)");
    EXEC_SQL("CREATE UNIQUE INDEX IF NOT EXISTS ard_config_name_idx ON ard_config(name)");
    return true;
};

bool ArdDB::recreateDBIndexes(int& indexes_created)
{
    indexes_created = 0;

    STRING_LIST indexes_list;

    QString sql = "SELECT name FROM sqlite_master WHERE type == 'index' AND tbl_name like 'ard_%'";

    auto q = selectQuery(sql);
    assert_return_false(q, "expected query");
    while ( q->next() ) 
        {
            QString name = q->value(0).toString();
            indexes_list.push_back(name);
        }

//    for(STRING_LIST::iterator k = indexes_list.begin(); k != indexes_list.end(); k++)
    for(const auto& name : indexes_list)
        {
            qDebug() << "deleting index:" << name;
            execQuery(QString("DROP INDEX IF EXISTS %1").arg(name));
        }

    bool rv = create_indexes(this);

    if(rv)
        {
            sql = "SELECT COUNT(*) FROM sqlite_master WHERE type == 'index' AND tbl_name like 'ard_%'";
            if(!q->prepare(sql))
                {
                    dbp::show_last_sql_err(*q, sql);
                    return false;
                };
            if(!q->exec())
                {
                    dbp::show_last_sql_err(*q, sql);
                    return false;
                }

            if ( q->next() ) 
                {
                    indexes_created = q->value(0).toInt(); 
                }
        }

    return rv;
};


#define DB_VER 49

static bool load_columns(ArdDB* _db, QString table_name, std::set<QString>& columns)
{
    QString sql = QString("PRAGMA table_info(%1)").arg(table_name);

    auto q = _db->selectQuery(sql);
    while ( q->next() ) 
        {
            QString col_name = q->value(1).toString();
            columns.insert(col_name);
        }

    return true;
};

static bool upgrade_db(ArdDB* _db, int dbver)
{
    Q_UNUSED(_db);
    Q_UNUSED(dbver);

    std::set<QString> ard_tree_columns;
    std::set<QString> ard_dbs_columns;
    std::set<QString> ard_dbh_columns;
    std::set<QString> ard_ext_draft;
    std::set<QString> ard_ext_q;
    std::set<QString> ard_ext_bboard;
    std::set<QString> ard_ext_bboard_item;
    std::set<QString> ard_bboard_links;
	std::set<QString> ard_ext_rule;

    load_columns(_db, "ard_tree", ard_tree_columns);
    load_columns(_db, "ard_dbs", ard_dbs_columns);
    load_columns(_db, "ard_dbh", ard_dbh_columns);
    load_columns(_db, "ard_ext_draft", ard_ext_draft);
    load_columns(_db, "ard_ext_q", ard_ext_q);
    load_columns(_db, "ard_ext_bboard", ard_ext_bboard);
    load_columns(_db, "ard_ext_bboard_item", ard_ext_bboard_item);
    load_columns(_db, "ard_board_links", ard_bboard_links);
	load_columns(_db, "ard_ext_rule", ard_ext_rule);

#define ADD_COLUMN(T, C, S, F)if(S.find(C) == S.end()){                 \
        if(!_db->execQuery(QString("ALTER TABLE %1 ADD %2 %3").arg(T).arg(C).arg(F)))return false; \
    }                                                                   \

    if (dbver < 33)
    {
        ADD_COLUMN("ard_ext_q", "qlbl", ard_ext_q, "TEXT");
    }

    if (dbver < 34)
    {
        ADD_COLUMN("ard_ext_q", "qfilter_on", ard_ext_q, "INTEGER DEFAULT 1");
    }


    if (dbver < 35)
    {
        ADD_COLUMN("ard_tree", "annotation", ard_tree_columns, "TEXT");
    }

    if (dbver < 37)
    {
        _db->execQuery("drop table ard_ethread");
        _db->execQuery("drop table ard_email_draft_details");
        _db->execQuery("drop table ard_q");
    }

	
    /*
    if (dbver < 38)
    {
        int pid = _db->queryInt(QString("select oid from ard_tree where subtype = %1").arg(static_cast<int>(EFolderType::folderProjectsHolder)));
        if (pid != 0) {
            QString sql = QString("update ard_tree set pid=%1 where subtype=2").arg(pid);
            _db->execQuery(sql);
        }
    }*/

    if (dbver < 39)
    {
        ADD_COLUMN("ard_tree", "popup", ard_tree_columns, "INTEGER");
    }


    if (dbver < 41)
    {
        ADD_COLUMN("ard_ext_bboard_item", "bindex", ard_ext_bboard_item, "INTEGER");
    }

    if (dbver < 42)
    {
        ADD_COLUMN("ard_ext_bboard", "bands", ard_ext_bboard, "TEXT");
    }

    if (dbver < 43)
    {
        ADD_COLUMN("ard_ext_bboard_item", "bshape", ard_ext_bboard_item, "NUMERIC(1)");
    }

    if (dbver < 44)
    {
        ADD_COLUMN("ard_ext_bboard_item", "ypos", ard_ext_bboard_item, "INTEGER");
        ADD_COLUMN("ard_ext_bboard_item", "ydelta", ard_ext_bboard_item, "INTEGER");
    }

    if (dbver < 45)
    {
        ADD_COLUMN("ard_board_links", "pindex", ard_bboard_links, "INTEGER");
    }

 //   if (dbver < 46)
 //   {
 //       //EXEC_SQL(QString("DROP TABLE ard_board_links"));
 //   }

    if (dbver < 47)
    {
        ADD_COLUMN("ard_board_links", "link_label", ard_bboard_links, "TEXT");
    }

    if (dbver < 48)
    {
        ADD_COLUMN("ard_board_links", "syid", ard_bboard_links, "TEXT");
    }

	if (dbver < 49)
	{
		ADD_COLUMN("ard_ext_rule", "exclusion_filter", ard_ext_rule, "INTEGER");
	}


    EXEC_SQL(QString("UPDATE ard_version SET version_num=%1").arg(DB_VER));

#undef ADD_COLUMN

    return true;
};



bool ArdDB::verifyMetaData()
{
    createTableIfNotExists("ard_version", "CREATE TABLE IF NOT EXISTS ard_version(version_id INTEGER PRIMARY KEY, version_num NUMERIC)");
    createTableIfNotExists("ard_tree", "CREATE TABLE IF NOT EXISTS ard_tree(oid INTEGER PRIMARY KEY, pid INTEGER, pindex INTEGER NOT NULL, otype NUMERIC(1) NOT NULL, subtype NUMERIC, title VARCHAR(255), "
        "todo NUMERIC(1), todo_done NUMERIC(1), annotation TEXT, retired NUMERIC(1), "
        "syid TEXT, mdc INTEGER, mvc INTEGER, mod_time INTEGER)");
    createTableIfNotExists("ard_dbs", "CREATE TABLE IF NOT EXISTS ard_dbs(db_id INTEGER, oid INTEGER, mdc INTEGER, rdb_string VARCHAR(64))");
    createTableIfNotExists("ard_dbh", "CREATE TABLE IF NOT EXISTS ard_dbh(db_oid INTEGER, synced_db_id INTEGER, synced_mdc INTEGER, last_sync_time INTEGER, sync_point_id INTEGER)");
    createTableIfNotExists("ard_config", "CREATE TABLE IF NOT EXISTS ard_config(config_id INTEGER PRIMARY KEY, name VARCHAR(255), value TEXT)");
    createTableIfNotExists("ard_ext_q", "CREATE TABLE IF NOT EXISTS ard_ext_q(oid INTEGER PRIMARY KEY, mdc INTEGER, q TEXT, qlbl TEXT, qfilter_on INTEGER DEFAULT 1)");
    createTableIfNotExists("ard_ext_draft", "CREATE TABLE IF NOT EXISTS ard_ext_draft(oid INTEGER PRIMARY KEY, mdc INTEGER, userid TEXT, email_to TEXT, email_cc TEXT, email_bcc TEXT, email_eid TEXT, email_references TEXT, email_threadid, sent_time INTEGER, attachments TEXT, attachments_host TEXT, labels TEXT, content_modified INTEGER)");
    createTableIfNotExists("ard_ext_ethread", "CREATE TABLE IF NOT EXISTS ard_ext_ethread(oid INTEGER PRIMARY KEY, account_email TEXT NOT NULL, thread_id TEXT NOT NULL, mdc INTEGER)");
    createTableIfNotExists("ard_ext_contact", "CREATE TABLE IF NOT EXISTS ard_ext_contact(oid INTEGER PRIMARY KEY, mdc INTEGER, xml TEXT)");
    createTableIfNotExists("ard_ext_kring", "CREATE TABLE IF NOT EXISTS ard_ext_kring(oid INTEGER PRIMARY KEY, mdc INTEGER, payload TEXT)");
    createTableIfNotExists("ard_ext_bboard", "CREATE TABLE IF NOT EXISTS ard_ext_bboard(oid INTEGER PRIMARY KEY, mdc INTEGER, bands TEXT)");
    createTableIfNotExists("ard_ext_bboard_item", "CREATE TABLE IF NOT EXISTS ard_ext_bboard_item(oid INTEGER PRIMARY KEY, mdc INTEGER, ref_syid TEXT, bshape NUMERIC(1), bindex INTEGER, ypos INTEGER, ydelta INTEGER)");
    createTableIfNotExists("ard_board_links", "CREATE TABLE IF NOT EXISTS ard_board_links(linkid INTEGER PRIMARY KEY, board TEXT, origin TEXT, target TEXT, syid TEXT, pindex INTEGER, link_label TEXT, mdc INTEGER)");
    createTableIfNotExists("ard_ext_note", "CREATE TABLE IF NOT EXISTS ard_ext_note(oid INTEGER PRIMARY KEY, mdc INTEGER, note_text TEXT, note_plain_text TEXT, mod_time INTEGER)");
	createTableIfNotExists("ard_ext_picture", "CREATE TABLE IF NOT EXISTS ard_ext_picture(oid INTEGER PRIMARY KEY, mdc INTEGER, cloud_file_id TEXT, local_media_time INTEGER)");
	createTableIfNotExists("ard_ext_rule", "CREATE TABLE IF NOT EXISTS ard_ext_rule(oid INTEGER PRIMARY KEY, mdc INTEGER, userid TEXT, words_in_subject TEXT, exact_phrase TEXT, from_list TEXT, exclusion_filter INTEGER)");

    if (!create_indexes(this)) {
        ASSERT(0, "failed to create DB indexes");
        return false;
    }

    int dbver = queryInt("SELECT version_num FROM ard_version");

    if (dbver == 0) {
        execQuery(QString("INSERT INTO ard_version(version_num) VALUES(%1)").arg(DB_VER));
    }
    else
    {
        if (dbver < DB_VER)
        {
            if (!upgrade_db(this, dbver))
                return false;
        }
        else if (dbver > DB_VER)
        {
			ard::messageBox(gui::mainWnd(), "You are opening database created by more recent version of program. Some features might not be available.");
        }
    }

    return true;
}


bool ArdDB::openDb(QString dbName, QString dbPath)
{    
    m_mainDB = false;
    m_db = QSqlDatabase::addDatabase("QSQLITE", dbName);
    m_db.setDatabaseName(dbPath);
    bool ok = m_db.open();
    if(ok)
        {
            qInfo() << "opened DB" << dbName << dbPath;
        }
    return ok;
};

bool ArdDB::openAsMainDb(QString dbName, QString dbPath)
{
    bool rv = openDb(dbName, dbPath);
    if(rv)
        {
            m_mainDB = true;
        }
    return rv;
};

void ArdDB::close()
{
    m_data_loaded = false;
    QString conn_name = m_db.connectionName();
    clear();
    m_db.close();
    m_query.reset(nullptr);	
    QSqlDatabase::removeDatabase(conn_name);
	ard::trail(QString("closed-DB [%1]").arg(conn_name));
};

QSqlQuery* ArdDB::prepareQuery(QString sql) 
{
    if (!m_db.isOpen()) {
        ASSERT(0, "expected opend DB");
        return nullptr;
    }

    if (!m_query) {
        m_query.reset(new QSqlQuery(m_db));
    }
    if (!m_query->prepare(sql))
    {
        QString error = m_query->lastError().text();
        ASSERT(0, QString("ERROR. Failed to prepare sql query (%1 %2)").arg(error).arg(sql));
        return nullptr;
    };
    return m_query.get();
};

QSqlQuery* ArdDB::selectQuery(QString sql) 
{
    auto q = prepareQuery(sql);
    if (!q)return nullptr;
    if (!q->exec(sql))
    {
        QString error = q->lastError().text();
        ASSERT(0, QString("ERROR. Failed to execute query (%1 %2)").arg(error).arg(sql));
        return nullptr;
    };
    return q;
};


QSqlQuery* ArdDB::execQuery(QString sql)
{
    /*if (!m_query) {
        m_query.reset(new QSqlQuery(m_db));
    }*/
    auto q = prepareQuery(sql);
    if (!q) {
        return nullptr;
    }
/*
    if (!m_query.prepare(sql)) {
        QString error = m_query.lastError().text();
        qWarning() << "ERROR. Failed to prepare sql query"
            << error
            << sql;
        return nullptr;
    };
    */
    if (!m_query->exec(sql)) {
        QString error = m_query->lastError().text();
        qWarning() << "ERROR. Failed to execute query"
            << error
            << sql;
        return nullptr;
    }
    return m_query.get();
};


int ArdDB::queryInt(QString sql)
{
    int rv = 0;
    auto q = selectQuery(sql);
    if (!q) {
        return 0;
    }

    if(q->first())
        {
            rv = q->value(0).toInt();
        }
    return rv;
};

QString ArdDB::queryString(QString sql)
{
    QString rv = 0;
    auto q = selectQuery(sql);
    if(q->first())
        {
            rv = q->value(0).toString();
        }
    return rv;
};

static bool table_exists(ArdDB* db, QString table_name)
{
    QString sql = QString("SELECT count(*) FROM sqlite_master WHERE type='table' AND name='%1'").arg(table_name);
    int r = db->queryInt(sql);
    bool rv = (r > 0);
    return rv;
}

bool ArdDB::createTableIfNotExists(QString table_name, QString create_sql)
{
    if(!table_exists(this, table_name))
        {
            if(!execQuery(create_sql))
                return false;
        }
    return true;
};

bool ArdDB::isOpen()const
{
    return m_db.isOpen();
};

void ArdDB::startTransaction()
{
    m_db.transaction();
};

void ArdDB::rollbackTransaction()
{
    m_db.rollback();
};

void ArdDB::commitTransaction()
{
    m_db.commit();
};

QString ArdDB::connectionName()const
{
    return m_db.connectionName();
};

QString ArdDB::databaseName()const
{
    return m_db.databaseName();
};

void ArdDB::guiBackup()
{
    if (!gui::isDBAttached()) {
		ard::errorBox(ard::mainWnd(), "Database is not connected");
        return;
    }

    ArdDB* db = ard::db();
    assert_return_void(db, "expected DB");
    assert_return_void(db->isOpen(), "expected DB");
    QString backup_file_path = get_curr_db_weekly_backup_file_path();
    QFileInfo fi(backup_file_path);
    QString msg = "Backup to default directory or a new directory? Please press 'Yes' to backup to default or 'No' to select directory.";
    if (fi.exists()) {
        msg += QString("Last time databased was backed up in standard location '%1'").arg(fi.lastModified().toString("ddd MMMM d yyyy"));
    }
    auto r = ard::YesNoCancel(ard::mainWnd(), msg);
    switch (r) 
    {
    case YesNoConfirm::yes: {
        checkRegularBackup(true);
    }break;
    case YesNoConfirm::no: {
        QString dir = QFileDialog::getExistingDirectory(gui::mainWnd(), 
                                                        "Select Directory to export",
            dbp::configFileLastShellAccessDir(),
            QFileDialog::ShowDirsOnly
            | QFileDialog::DontResolveSymlinks);
        if (!dir.isEmpty()) {
            dbp::configFileSetLastShellAccessDir(dir, false);
            QString name = "ardi_backup.qpk";
            QString fullPath = dir + "/" + name;
            int idx = 1;
            while (QFile::exists(fullPath)) {
                name = QString("ardi_backup%1.qpk").arg(idx);
                fullPath = dir + "/" + name;
                idx++;
                if (idx > 200) {
					ard::errorBox(ard::mainWnd(), "Too many backup files in same folder. Aborted.");
                    return;
                }
            }

            if (db->backupDB(fullPath)) {
                ard::messageBox(gui::mainWnd(), QString("Backup completed - '%1'").arg(fullPath));
            }
            else {
                ard::messageBox(gui::mainWnd(), "Backup failed.");
            }
        }
    }break;
    default:break;
    }
};

bool ArdDB::checkRegularBackup(bool force)
{
    Q_UNUSED(force);
    assert_return_false(gui::isDBAttached(), "expected attached DB");    
    
    bool rv = false;

#ifdef ARD_BIG
    ArdDB* db = ard::db();
    if(!force)
        {
            if(db->isWeeklyBackupChecked())
                {
                    return false;
                }
        }

    extern void check_cleanup();
    check_cleanup();

    QString backup_file_path = get_curr_db_weekly_backup_file_path();
    QFileInfo fi2(backup_file_path);
    bool proceed_with_backup = !fi2.exists() || force;
    if(proceed_with_backup)
        {
            rv = db->backupDB(backup_file_path);
        }

    db->setWeeklyBackupChecked(true);
#endif

    return rv;  
};

bool ArdDB::backupDB(QString compressed_file_path)const
{
    QString strDBPath = databaseName();
    assert_return_false(!strDBPath.isEmpty(), "expected valid DB path.");
    bool rv = SyncPoint::compress(strDBPath, compressed_file_path, 1);
    ASSERT(rv, "backup failed") << strDBPath << compressed_file_path;
    return rv;
};

RootTopic* ArdDB::detachRoot()
{
    RootTopic* rv = m_root;
    if(m_root)
        {
            m_root->detachDB();
            m_root = nullptr;
        }
    return rv;
};

template <class Container>
void detachAndClearCache(Container& c)
{
    typename Container::iterator i;
    for(i = c.begin(); i != c.end();++i)
        {
            i->second->detachDb();
        }
    c.clear();
}

void ArdDB::clear()
{
    //this will help to resolve double-link dependencies
    //all objects are about to be released, the link dependencies
    //don't needed any more, if we don't release them, the object
    //will stay it memory as we have interlocking that prevent
    //normal memory cleanup


    m_syid2topic.clear();
    detachAndClearCache(m_id2item);

    if (m_root) {
        m_root->release();
        m_root = nullptr;
    }

    if (m_ethreads_root) {
        m_ethreads_root->release();
        m_ethreads_root = nullptr;
    }

    if (m_gmail_runner) {
        m_gmail_runner->release();
        m_gmail_runner = nullptr;
    }
    m_contacts_model.reset(nullptr);
	m_rules_model.reset(nullptr);
	if (m_local_search) {
		m_local_search->release();
		m_local_search = nullptr;
	}
    m_kring_model.reset(nullptr);
    m_boards_model.reset(nullptr);

    if (m_task_ring) {
        m_task_ring->release();
        m_task_ring = nullptr;
    }

    if (m_annotated) {
        m_annotated->release();
        m_annotated = nullptr;
    }

    if (m_notes) {
        m_notes->release();
        m_notes = nullptr;
    }

	if (m_bookmarks) {
		m_bookmarks->release();
		m_bookmarks = nullptr;
	}

	if (m_pictures) {
		m_pictures->release();
		m_pictures = nullptr;
	}

    if (m_colored) {
        m_colored->release();
        m_colored = nullptr;
    }

    m_WeeklyBackupChecked = false;
};


#define SQL_INSERT_SYNC_UPDATE(o)  if(IS_VALID_SYID(o->syid()))         \
        {                                                               \
            s_insert += QString(", syid");                              \
            s_values += QString(", '%1'").arg(o->syid());               \
            if(IS_VALID_COUNTER(o->modCounter()))                       \
                {                                                       \
                    s_insert += QString(", mdc");                       \
                    s_values += QString(", %1").arg(o->modCounter());   \
                }                                                       \
            if(IS_VALID_COUNTER(o->moveCounter()))                      \
                {                                                       \
                    s_insert += QString(", mvc");                       \
                    s_values += QString(", %1").arg(o->moveCounter());  \
                }                                                       \
        }                                                               \

#define SQL_INSERT_SYNC_UPDATE_NO_MVC(o)  if(IS_VALID_SYID(o->syid()))  \
        {                                                               \
            s_insert += QString(", syid");                              \
            s_values += QString(", '%1'").arg(o->syid());               \
            if(IS_VALID_COUNTER(o->modCounter()))                       \
                {                                                       \
                    s_insert += QString(", mdc");                       \
                    s_values += QString(", %1").arg(o->modCounter());   \
                }                                                       \
        }                                                               \

#define SQL_INSERT_SYNC_UPDATE_NO_MVC_NO_SYID(o)            \
    if(IS_VALID_COUNTER(o->modCounter()))                   \
        {                                                   \
         s_insert += QString(", mdc");                      \
         s_values += QString(", %1").arg(o->modCounter());  \
         }                                                  \


bool ArdDB::storeNewTopic(topic_ptr it)
{
    time_t mod_time = QDateTime::currentDateTime().toTime_t();
	it->set_mod_time(mod_time);

    if(IS_VALID_DB_ID(it->id()))
        {
            ASSERT(0, "the id should not be initialized");
            return false;
        }

    int index_in_parent = -1;
    int parendID = 0; 

    if(!it->parent())
        {
            bool proceedWithRoot = false;
            if(it->isRootTopic())
                {
                    extern bool isPersistantRoot(topic_ptr f);
                    proceedWithRoot = isPersistantRoot(it);
                }

            if(!proceedWithRoot)
                {
                    ASSERT(0, "invalid parent pointer") << it->dbgHint();
                    return false;
                }
            else
                {
                    index_in_parent = 0;
                    parendID = 0;
                }
        }
    else
        {
            auto parent = it->parent();
            ASSERT(IS_VALID_DB_ID(parent->id()), "dbp::storeNewItem - the parent id not initialized");
            index_in_parent = it->pindex();
            if(index_in_parent == -1)
                {
                    ASSERT(0, "dbp::storeNewItem - invalid index inside the parent") << it->dbgHint();
                }
            parendID = parent->id();
        }

    QString s_insert = "INSERT INTO ard_tree(pid, otype, pindex, title, mod_time, retired";
    QString s_values = QString(" VALUES(%1, %2, %3, ?, %4, %5").arg(parendID)
        .arg(it->otype())
        .arg(index_in_parent)
        .arg(mod_time)
        .arg(it->mflag4serial());

    s_insert += QString(", subtype");
    s_values += QString(", %1").arg(static_cast<int>(it->folder_type()));

    bool has_annotation = false;
    auto str_annotation = it->annotation().trimmed();
    if (!str_annotation.isEmpty()) {
        s_insert += QString(", annotation");
        s_values += QString(", ?");
        has_annotation = true;
    }

    if(it->isToDo())
        {
            s_insert += QString(", todo");
            s_values += QString(", %1").arg(it->getToDoPriorityAsInt());
        }
  
    if(it->getToDoDonePercent() > 0)
        {
            unsigned int val = it->getToDoDonePercent();
            s_insert += QString(", todo_done");
            s_values += QString(", %1").arg(val);      
        }

    SQL_INSERT_SYNC_UPDATE(it);

    QString sql = s_insert + ") " + s_values + ")";
    auto q = prepareQuery(sql);
    if (!q) {
        return false;
    }
    q->addBindValue(it->serializable_title());
    if (has_annotation) {
        q->addBindValue(str_annotation);
    }
    if(!q->exec())
        {
            dbp::show_last_sql_err(*q, sql);
            return false;
        }

    DB_ID_TYPE oid = q->lastInsertId().toInt();
    ASSERT(IS_VALID_DB_ID(oid), "invalid last insert ID");

    it->setup_id_for_new_local_topic(this, oid);

    it->clear_persistance_request(np_ALL);
    return true;
};


bool ArdDB::createRootSyncRecord(RootTopic* sdb)
{
    qDebug() << "<<< ArdDB::createRootSyncRecord" << sdb->osid();

    ASSERT(IS_VALID_DB_ID(sdb->osid()), "expected valid syncDB oid");
    ASSERT(IS_VALID_COUNTER(sdb->db().db_id()), "expected valid syncDB db_id");
    ASSERT(IS_VALID_COUNTER(sdb->db().db_mod_counter()), "expected valid syncDB mdc");
  
    QString sql = QString("INSERT INTO ard_dbs(db_id, oid, mdc) VALUES(%1, %2, %3)").arg(sdb->db().db_id()).arg(sdb->osid()).arg(sdb->db().db_mod_counter());
    bool rv = false;
    if (execQuery(sql)) {
        rv = true;
    }
    return rv;
}


void ArdDB::insertNewExt(ard::email_draft_ext* d)
{
    assert_return_void(d->owner(), "expected owner");
    assert_return_void(IS_VALID_DB_ID(d->id()), "expected valid ext id");

    QString s_insert = "INSERT INTO ard_ext_draft(oid, userid, email_eid, email_references, email_threadid, email_to, email_cc, email_bcc, attachments, attachments_host, labels, content_modified";
    QString s_values = QString(" VALUES(%1, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?")
        .arg(d->id());

    if (IS_VALID_COUNTER(d->modCounter()))
    {
        s_insert += QString(", mdc");
        s_values += QString(", %1").arg(d->modCounter());
    }

    QString sql = s_insert + ") " + s_values + ")";
    auto q = prepareQuery(sql);
    if (!q) {
        return;
    }
    q->addBindValue(d->userIdOnDraft());
    q->addBindValue(d->originalEId());
    q->addBindValue(d->references());
    q->addBindValue(d->threadId());
    q->addBindValue(d->to());
    q->addBindValue(d->cc());
    q->addBindValue(d->bcc());
    q->addBindValue(d->attachmentsAsString());
    q->addBindValue(d->attachmentsHost());
    q->addBindValue(d->labelsAsString());
    q->addBindValue(d->isContentModified() ? 1 : 0);
    if (!q->exec())
    {
        ASSERT(0, "SQL ERROR") << sql << q->lastError();;
        return;
    }
    d->clear_persistance_request(np_ALL);
};


void ArdDB::insertNewExt(ard::ethread_ext* pe)
{
    assert_return_void(pe->owner(), "expected owner");
    assert_return_void(IS_VALID_DB_ID(pe->id()), "expected valid owner id");

    QString s_insert = "INSERT INTO ard_ext_ethread(oid, account_email, thread_id";
    QString s_values = QString(" VALUES(%1, '%2', '%3'")
        .arg(pe->id())
        .arg(pe->accountEmail())
        .arg(pe->threadId());

    if (IS_VALID_COUNTER(pe->modCounter()))
    {
        s_insert += QString(", mdc");
        s_values += QString(", %1").arg(pe->modCounter());
    }

    QString sql = s_insert + ") " + s_values + ")";
    auto q = prepareQuery(sql);
    if (!q) {
        ASSERT(0, "SQL prepare ERROR") << sql;
        return;
    }

    if (!q->exec())
    {
        ASSERT(0, "ethread_ext/SQL-exec-ERROR") << sql;
        return;
    }
    pe->clear_persistance_request(np_ALL);
};


void ArdDB::insertNewExt(ard::anKRingKeyExt* d)
{
    assert_return_void(d->owner(), "expected owner");
    assert_return_void(IS_VALID_DB_ID(d->id()), "expected valid owner id");

    QString s_insert = "INSERT INTO ard_ext_kring(oid, payload";
    QString s_values = QString(" VALUES(%1, ?")
        .arg(d->id());

    if (IS_VALID_COUNTER(d->modCounter()))
    {
        s_insert += QString(", mdc");
        s_values += QString(", %1").arg(d->modCounter());
    }

    QString sql = s_insert + ") " + s_values + ")";
    auto q = prepareQuery(sql);
    if (!q) {
        ASSERT(0, "SQL prepare error") << sql;
        return;
    }
    q->addBindValue(d->encryptedContent());
    if (!q->exec())
    {
        ASSERT(0, "SQL ERROR") << sql << q->lastError();;
        return;
    }
    d->clear_persistance_request(np_ALL);
    d->clearModified();
};

void ArdDB::insertNewExt(ard::contact_ext* e)
{
    assert_return_void(e->owner(), "expected owner");
    assert_return_void(IS_VALID_DB_ID(e->id()), "expected valid owner id");

    QString s_insert = "INSERT INTO ard_ext_contact(oid, xml";
    QString s_values = QString(" VALUES(%1, ?")
        .arg(e->id());

    if (IS_VALID_COUNTER(e->modCounter()))
    {
        s_insert += QString(", mdc");
        s_values += QString(", %1").arg(e->modCounter());
    }

    QString sql = s_insert + ") " + s_values + ")";
    auto q = prepareQuery(sql);
    if (!q) {
        ASSERT(0, "SQL prepare ERROR") << sql;
        return;
    }

    q->addBindValue(e->toXml());

    if (!q->exec())
    {
        ASSERT(0, "contact_ext/SQL-exec-ERROR") << sql << q->lastError().text();
        return;
    }
    e->clear_persistance_request(np_ALL);
};

void ArdDB::insertNewExt(ard::board_ext* d)
{
    assert_return_void(d->owner(), "expected owner");
    assert_return_void(IS_VALID_DB_ID(d->id()), "expected valid owner id");

    QString s_insert = "INSERT INTO ard_ext_bboard(oid, bands";
    QString s_values = QString(" VALUES(%1, ?")
        .arg(d->id());

    if (IS_VALID_COUNTER(d->modCounter()))
    {
        s_insert += QString(", mdc");
        s_values += QString(", %1").arg(d->modCounter());
    }

    QString sql = s_insert + ") " + s_values + ")";
    auto q = prepareQuery(sql);
    if (!q) {
        ASSERT(0, "SQL prepare error") << sql;
        return;
    }
    q->addBindValue(d->bandsPayload());
    if (!q->exec())
    {
        ASSERT(0, "SQL ERROR") << sql << q->lastError();;
        return;
    }
    d->clear_persistance_request(np_ALL);
};

void ArdDB::insertNewExt(ard::board_item_ext* d)
{
    assert_return_void(d->owner(), "expected owner");
    assert_return_void(IS_VALID_DB_ID(d->id()), "expected valid owner id");

    QString s_insert = "INSERT INTO ard_ext_bboard_item(oid, ref_syid, bshape, bindex, ypos, ydelta";
    QString s_values = QString(" VALUES(%1, '%2', %3, %4, %5, %6")
        .arg(d->id())
        .arg(d->ref_topic_syid())
        .arg(static_cast<int>(d->bshape()))
        .arg(d->bandIndex())
        .arg(d->yPos())
        .arg(d->yDelta());

    if (IS_VALID_COUNTER(d->modCounter()))
    {
        s_insert += QString(", mdc");
        s_values += QString(", %1").arg(d->modCounter());
    }

    QString sql = s_insert + ") " + s_values + ")";
    auto q = prepareQuery(sql);
    if (!q) {
        ASSERT(0, "SQL prepare error") << sql;
        return;
    }
    if (!q->exec())
    {
        ASSERT(0, "SQL ERROR") << sql << q->lastError();
        return;
    }
    d->clear_persistance_request(np_ALL);
};

void ArdDB::insertNewExt(ard::note_ext* e)
{
    assert_return_void(e->owner(), "expected owner");
    assert_return_void(IS_VALID_DB_ID(e->id()), "expected valid owner id");

    time_t mod_time = QDateTime::currentDateTime().toTime_t();
    QString s_insert = "INSERT INTO ard_ext_note(oid, mod_time, note_text, note_plain_text";
    QString s_values = QString(" VALUES(%1, %2, ?, ?")
        .arg(e->id())
        .arg(static_cast<uint>(mod_time));

    if (IS_VALID_COUNTER(e->modCounter()))
    {
        s_insert += QString(", mdc");
        s_values += QString(", %1").arg(e->modCounter());
    }

    QString sql = s_insert + ") " + s_values + ")";
    auto q = prepareQuery(sql);
    if (!q) {
        ASSERT(0, "SQL prepare ERROR") << sql;
        return;
    }

    q->addBindValue(e->html());
    q->addBindValue(e->plain_text());

    if (!q->exec())
    {
        ASSERT(0, "note_ext/SQL-exec-ERROR") << sql << q->lastError().text();
        return;
    }

    DB_ID_TYPE ins_id = q->lastInsertId().toInt();
    ASSERT(IS_VALID_DB_ID(ins_id), "invalid last insert ID");

    //qDebug() << "ArdDB=" << this << "Q=" << q;
    //qDebug() << "ykh+note" << e->id() << sql << e->html().size() << e->plain_text().size();
    //int r = queryInt(QString("select count(*) from ard_ext_note WHERE oid=%1").arg(e->id()));
    //qDebug() << "ykh-recnum=" << r;

    e->clear_persistance_request(np_ALL);
};

void ArdDB::insertNewExt(ard::picture_ext* d)
{
	assert_return_void(d->owner(), "expected owner");
	assert_return_void(IS_VALID_DB_ID(d->id()), "expected valid owner id");

	QString s_insert = "INSERT INTO ard_ext_picture(oid, cloud_file_id, local_media_time";
	QString s_values = QString(" VALUES(%1, ?, ?")
		.arg(d->id());

	if (IS_VALID_COUNTER(d->modCounter()))
	{
		s_insert += QString(", mdc");
		s_values += QString(", %1").arg(d->modCounter());
	}

	QString sql = s_insert + ") " + s_values + ")";
	auto q = prepareQuery(sql);
	if (!q) {
		ASSERT(0, "SQL prepare error") << sql;
		return;
	}
	q->addBindValue(d->cloud_file_id());
	q->addBindValue(static_cast<uint>(d->local_media_time()));
	if (!q->exec())
	{
		ASSERT(0, "SQL ERROR") << sql << q->lastError();;
		return;
	}
	d->clear_persistance_request(np_ALL);
};

void ArdDB::insertNewExt(ard::rule_ext* d) 
{
	assert_return_void(d->owner(), "expected owner");
	assert_return_void(IS_VALID_DB_ID(d->id()), "expected valid owner id");

	QString s_insert = "INSERT INTO ard_ext_rule(oid, userid, words_in_subject, exact_phrase, from_list, exclusion_filter";
	QString s_values = QString(" VALUES(%1, ?, ?, ?, ?, ?")
		.arg(d->id());

	if (IS_VALID_COUNTER(d->modCounter()))
	{
		s_insert += QString(", mdc");
		s_values += QString(", %1").arg(d->modCounter());
	}

	QString sql = s_insert + ") " + s_values + ")";
	auto q = prepareQuery(sql);
	if (!q) {
		ASSERT(0, "SQL prepare error") << sql;
		return;
	}
	q->addBindValue(d->userid());
	q->addBindValue(d->subjectStr());
	q->addBindValue(d->exact_phrase());
	q->addBindValue(d->fromStr());
	q->addBindValue(d->isExclusionFilter() ? 1 : 0);
	if (!q->exec())
	{
		ASSERT(0, "SQL ERROR") << sql << q->lastError();
		return;
	}
	d->clear_persistance_request(np_ALL);
};

static void find_children(topic_ptr parent, ID_2_SUBTOPICS& id2subitems, ID_2_FOLDER& id2folder, TOPICS_LIST& abandoned)
{
    DB_ID_TYPE parentID = parent->id();
    if (parent->isRootTopic())
    {
        //in logic of the tree the root can have 0 as dbid
        parentID = 0;
    }

    ID_2_FOLDER::iterator k = id2folder.find(parentID);
    if (k != id2folder.end())
    {
        //this should prevent recursion
        //just need some testing
        id2folder.erase(k);
    }
    else
    {
        if (parentID != 0)
        {
            ASSERT(0, "Recursion in DB structure");
            return;
        }
    }

    ID_2_SUBTOPICS::iterator i = id2subitems.find(parentID);
    if (i != id2subitems.end())
    {
        CHILDREN& children = i->second;
        for (auto& j : children) {
            topic_ptr f = dynamic_cast<topic_ptr>(j);
            if (!parent->insertItem(f, parent->items().size(), true)) {
                abandoned.push_back(f);
            };
            find_children(f, id2subitems, id2folder, abandoned);
        }

        id2subitems.erase(i);
    }

    //.........root topic can have 2 IDs - 0 and one in DB
    if (parent->isRootTopic() && parent->id() != 0)
    {
        ID_2_SUBTOPICS::iterator i = id2subitems.find(parent->id());
        if (i != id2subitems.end())
        {
            CHILDREN& children = i->second;
            for (auto& j : children) {
                topic_ptr f = dynamic_cast<topic_ptr>(j);
                if (!parent->insertItem(f, parent->items().size(), true)) {
                    abandoned.push_back(f);
                }
                find_children(f, id2subitems, id2folder, abandoned);
            }
            id2subitems.erase(i);
        }
    }

}

static void adopt_byconvertion_orphant(topic_ptr parent, topic_ptr orphant)
{
    if (parent->canAcceptChild(orphant) && orphant->canBeMemberOf(parent)) {
        parent->addItem(orphant);
    }
    else {
        ///orphant can be some special topic we have to turn it into generic topic
        TOPICS_LIST subitems;
        topic_ptr o2 = new ard::topic(orphant->title());
        if (!o2->moveSubItemsFrom(orphant)) {
            ASSERT(0, "failed to move in subitems from orphant") << o2->dbgHint() << orphant->dbgHint();
            o2->release();
            return;
        };
        parent->addItem(o2);
        orphant->killSilently(false);
    }
}

void ArdDB::doLoadTree(ArdDB* adb, 
    topic_ptr root,
    QString sql, 
    ID_2_SUBTOPICS& id2subitems, 
    ID_2_FOLDER& id2folder,
    objProducer producer,
    wrapperResolver wresolver)
{
    auto q = adb->selectQuery(sql);
    assert_return_void(q, "expected query");
    if (!q) {
        return;
    }

    while ( q->next() ) 
        {
            DB_ID_TYPE oid = q->value(0).toInt();
            DB_ID_TYPE pid = q->value(1).toInt();
            EOBJ otype = (EOBJ)q->value(3).toInt();
            EFolderType ftype = (EFolderType)q->value(11).toInt();

            auto it = producer(otype, ftype);
            if(!it){
                ASSERT(0, "failed to instantiate DB item") << oid << pid << q->value(3).toInt();
                continue;
            }
            if (!it->init_from_db(adb, *q)){
                m_serialize_duplicate_syid.push_back(it);
            }

            if (wresolver && it->isWrapper())
            {
                if (!wresolver(it)) {
                    //it->release();
                    it = nullptr;
                    continue;
                };
            }

            id2folder[oid] = it;

            ID_2_SUBTOPICS::iterator i = id2subitems.find(pid);
            if(i == id2subitems.end())
                {
                    CHILDREN children;
                    children.push_back(it);
                    id2subitems[pid] = children;
                }
            else
                {
                    CHILDREN& children = i->second;
                    children.push_back(it);
                }
        }//while

    TOPICS_LIST abandoned;
    find_children(root, id2subitems, id2folder, abandoned);

    if(!id2subitems.empty())
        {
            int orphant_count = 0;
            for(auto& i : id2subitems)    {
                    CHILDREN& lst = i.second;
                    orphant_count += lst.size();

#ifdef _DEBUG
                    for(const auto& c : lst)    {
                            qDebug() << "[orfant]" << c->dbgHint();
                        }
#endif

                }
            qWarning() << "found orphant items" << orphant_count;

            topic_ptr orphant_parent = new ard::topic(QString("Recovered ORPHANT items [%1]").arg(QDate::currentDate().toString()));
            for(auto& i : id2subitems)    {
                    CHILDREN& lst = i.second;
                    for(auto& j : lst)    {
                            //qDebug() << "orphant-child" << j->dbgHint();
                            adopt_byconvertion_orphant(orphant_parent, j);
                            //orphant_parent->addItem(j);
                        }
                }
            adb->m_serialize_orphants.push_back(orphant_parent);
        }

    if (!abandoned.empty()) {
        topic_ptr orphant_parent = new ard::topic(QString("Recovered abandoned items [%1]").arg(QDate::currentDate().toString()));
        for (auto& i : abandoned) {
            adopt_byconvertion_orphant(orphant_parent, i);
            //qDebug() << "abandoned-child" << i->dbgHint();
            //orphant_parent->addItem(i);
        }
        adb->m_serialize_orphants.push_back(orphant_parent);
    }

#ifdef _DEBUG
    ASSERT(root->checkTreeSanity(), "sanity test failed");
#endif

}

static QString rootLoadSQL = "SELECT oid, pid, pindex, otype, title, todo, todo_done, retired, syid, mdc, mvc, subtype, annotation, mod_time FROM ard_tree WHERE otype=%1";

void ArdDB::loadTopics()
{
    ASSERT(m_root == NULL, "expected NULL root"); 
    ASSERT(m_db.isOpen(), "expected open DB");

    m_root = createRoot();
    if (!m_root) {
        ASSERT(0, "failed to create root");
        return;
    }


    ID_2_SUBTOPICS      id2subitems;
    ID_2_FOLDER         id2folder;

    QString sql = QString(rootLoadSQL).arg(objFolder);
    sql += QString(" OR otype=%1 OR otype=%2 OR otype=%3 OR otype=%4 OR otype=%5 ORDER BY pid, pindex")
        .arg(objEmailDraft)
        .arg(objEThread)
        .arg(objFileRef)
        .arg(objUrl)
		.arg(objPicture);

    if (m_ethreads_root && IS_VALID_DB_ID(m_ethreads_root->id())) {
        ///! we are not loading ethread inside ethreads_root into main space
        sql = QString(rootLoadSQL).arg(objFolder);
        sql += QString(" OR otype=%1 OR otype=%2 OR otype=%3 OR (otype=%4 AND pid<>%5) OR otype=%6 ORDER BY pid, pindex")
            .arg(objEmailDraft)
            .arg(objFileRef)
            .arg(objUrl)
            .arg(objEThread)
            .arg(m_ethreads_root->id())
			.arg(objPicture);
    }

    doLoadTree(this, 
        m_root, 
        sql, 
        id2subitems, 
        id2folder,
        [](EOBJ otype, EFolderType ftype)
    {
		//if (EFolderType::folderBoardTopicsHolder == ftype) {
		//	qDebug() << "ftype=EFolderType::folderBoardTopicsHolder";
		//}

        topic_ptr rv = nullptr;
        switch (otype)
        {
        case objFolder:  
        {
            if (ftype == EFolderType::folderUserSorter ||
                ftype == EFolderType::folderRecycle ||
                ftype == EFolderType::folderMaybe ||
                ftype == EFolderType::folderReference ||
                ftype == EFolderType::folderDelegated ||
                ftype == EFolderType::folderSortbox ||
                ftype == EFolderType::folderBacklog ||
                ftype == EFolderType::folderBoardTopicsHolder ||
				ftype == EFolderType::folderUserSortersHolder)
            {
                rv = new ard::locus_folder();
            }
            else {
                rv = new ard::topic();
            }
        }break;
        case objEThread:    rv = new ard::ethread(); break;
        case objEmailDraft: rv = new ard::email_draft(); break;
        case objFileRef:    {rv = new ard::fileref();} break;
        case objUrl:        {rv = ard::anUrl::createUrl(); } break;
		case objPicture:    {rv = new ard::picture(); } break;
        default:        ASSERT(0, QString("invalid object to load, defaulted to topic %1").arg(otype));
            rv = new ard::topic(); break;
        }
        return rv; 
    },
        [&](topic_ptr it)
    {
        if (m_mainDB) {
			auto t = dynamic_cast<ard::ethread*>(it);
            ASSERT(t, "expected ethread object");
            if (t) {
                m_threads_wrappers2link_after_load.push_back(t);
            }
        }
        return true;
    }
    );
};

void ArdDB::loadContacts() 
{
    assert_return_void(m_db.isOpen(), "expected open DB");

    bool loadGroups = true;
    if (loadGroups) {
        ID_2_SUBTOPICS id2subitems;
        ID_2_FOLDER id2folder;

        QString sql = QString(rootLoadSQL).arg(objContactGroup);
        sql += QString(" ORDER BY pid, pindex");
        doLoadTree(this,
            m_contacts_model->groot(),
            sql,
            id2subitems,
            id2folder,
            [](EOBJ, EFolderType) 
            {
                return new ard::contact_group;
            },
            nullptr);
    }

    bool loadContacts = true;
    if (loadContacts) {
        ID_2_SUBTOPICS id2subitems;
        ID_2_FOLDER id2folder;

        QString sql = QString(rootLoadSQL).arg(objContact);
        sql += QString(" ORDER BY pid, pindex");
        doLoadTree(this,
            m_contacts_model->croot(),
            sql,
            id2subitems,
            id2folder,
            [](EOBJ, EFolderType) 
            {
                return new ard::contact; 
            },
            nullptr);       

        IDS_SET orfants;
        sql = QString("SELECT oid, xml, mdc FROM ard_ext_contact ORDER BY oid");
        loadExtensions<ard::contact_ext, ard::contact>(sql, orfants);
        if (!orfants.empty()) {
            removeFromTableById("ard_ext_contact", orfants);
        }
    }
};

void ArdDB::loadQFilters() 
{
	assert_return_void(m_db.isOpen(), "expected open DB");

	bool load_filters = true;
	if (load_filters)
	{
		ID_2_SUBTOPICS id2subitems;
		ID_2_FOLDER id2folder;

		QString sql = QString(rootLoadSQL).arg(objQFilter);
		sql += QString(" ORDER BY pid, pindex");
		doLoadTree(this,
			m_rules_model->rroot(),
			sql,
			id2subitems,
			id2folder,
			[](EOBJ, EFolderType)
		{
			return new ard::rule;
		},
			nullptr);

		IDS_SET orfants;
		sql = QString("SELECT oid, mdc, userid, words_in_subject, exact_phrase, from_list, exclusion_filter FROM ard_ext_rule ORDER BY oid");
		loadExtensions<ard::rule_ext, ard::rule>(sql, orfants);
		if (!orfants.empty()) {
			removeFromTableById("ard_ext_rule", orfants);
		}
	}
};

void ArdDB::loadKRing() 
{
    assert_return_void(m_db.isOpen(), "expected open DB");
    ID_2_SUBTOPICS id2subitems;
    ID_2_FOLDER id2folder;

    QString sql = QString(rootLoadSQL).arg(objKRingKey);
    sql += QString(" ORDER BY pid, pindex");
    doLoadTree(this,
        m_kring_model->keys_root(),
        sql,
        id2subitems,
        id2folder,
        [](EOBJ, EFolderType)
    {
        return new ard::KRingKey;
    },
        nullptr);
    
    IDS_SET orfants;
    sql = QString("SELECT oid, payload, mdc FROM ard_ext_kring ORDER BY oid");
    loadExtensions<ard::anKRingKeyExt, ard::KRingKey>(sql, orfants);
    if (!orfants.empty()) {
        removeFromTableById("ard_ext_kring", orfants);
    }
};

void ArdDB::loadNotesExt() 
{
    IDS_SET orfants;
    QString sql = QString("SELECT oid, mdc, mod_time FROM ard_ext_note ORDER BY oid");
    loadExtensions<ard::note_ext, ard::topic>(sql, orfants);
    if (!orfants.empty()) {
        removeFromTableById("ard_ext_note", orfants);
    }
};

void ArdDB::loadPicturesExt()
{
	IDS_SET orfants;
	QString sql = QString("SELECT oid, mdc, cloud_file_id, local_media_time FROM ard_ext_picture ORDER BY oid");
	loadExtensions<ard::picture_ext, ard::picture>(sql, orfants);
	if (!orfants.empty()) {
		removeFromTableById("ard_ext_picture", orfants);
	}
};

void ArdDB::loadBoards()
{
    assert_return_void(m_db.isOpen(), "expected open DB");
    ID_2_SUBTOPICS id2subitems;
    ID_2_FOLDER id2folder;

    QString sql = QString(rootLoadSQL).arg(objBoard);
    sql += QString(" OR otype=%1 ORDER BY pid, pindex").arg(objBoardItem);
    doLoadTree(this,
        m_boards_model->boards_root(),
        sql,
        id2subitems,
        id2folder,
        [](EOBJ otype, EFolderType)
    {
        topic_ptr rv = nullptr;
        if (otype == objBoard) {
            rv = new ard::selector_board;
        }
        else if(otype == objBoardItem){
            rv = new ard::board_item;
        }
        return rv;
    },
        nullptr);

    bool loadBoards = true;
    if (loadBoards) {
        IDS_SET orfants;
        sql = QString("SELECT oid, mdc, bands FROM ard_ext_bboard ORDER BY oid");
        loadExtensions<ard::board_ext, ard::selector_board>(sql, orfants);
        if (!orfants.empty()) {
            removeFromTableById("ard_ext_bboard", orfants);
        }
    }

    bool loadBoardItems = true;
    if (loadBoardItems) {
        IDS_SET orfants;
        sql = QString("SELECT oid, mdc, ref_syid, bshape, bindex, ypos, ydelta FROM ard_ext_bboard_item ORDER BY oid");
        loadExtensions<ard::board_item_ext, ard::board_item>(sql, orfants);
        if (!orfants.empty()) {
            removeFromTableById("ard_ext_bboard_item", orfants);
        }
    }


    IDS_SET invalid_bitems;
    
    auto br = boards_model()->boards_root();
    if (br) {
        auto q = selectQuery("SELECT linkid, board, origin, target, syid, pindex, link_label, mdc FROM ard_board_links ORDER BY board, origin, target");
        if (!q) {
            ASSERT(0, "load board failed");
            return;
        }
        br->loadBoardLinksFromDb(q);
        for (auto& i : br->items()) 
        {
            auto* b = dynamic_cast<ard::selector_board*>(i);
            if (b) {
                ard::board_link_map* lmap = nullptr;
                auto syid_str = b->syid();
                if (!syid_str.isEmpty()) {
                    lmap = br->lookupLinkMap(syid_str);
					if (!lmap) {
						ard::error(QString("expected link-map for board[%1] non-empty board can lose links during sync").arg(syid_str));
					}
                    //ASSERT(lmap, QString);
                }
                b->rebuildBoardFromDb(this, lmap, invalid_bitems);
                std::vector<int> bands_idx;
                auto Max = b->m_b2items.size();
                for (size_t i = 0; i < Max; i++) {
                    bands_idx.push_back(i);
                }
                b->resetYPosInBands(this, bands_idx);
                b->resetLinksPindex(this);
            }
        }
    }
    
    ///have to ensure persistance on broot
    if (!invalid_bitems.empty()) {
        removeFromTableById("ard_tree", invalid_bitems);
        removeFromTableById("ard_ext_bboard_item", invalid_bitems);
    }
};

void ArdDB::loadThreadRootSpace()
{
    ASSERT(m_ethreads_root, "expected resource root");
    ASSERT(m_db.isOpen(), "expected open DB");

    if (!IS_VALID_DB_ID(m_ethreads_root->id())) {
        qDebug() << "load from ethreads space is skippet due to invalid root DBID";
        return;
    }

    ID_2_SUBTOPICS      id2subitems;
    ID_2_FOLDER         id2folder;

    QString sql = QString(rootLoadSQL).arg(objEThread);
    sql += QString(" AND pid=%1 ORDER BY pid, pindex")
        .arg(m_ethreads_root->id());

    doLoadTree(this,
        m_ethreads_root,
        sql,
        id2subitems,
        id2folder,
        [](EOBJ, EFolderType) {return new ard::ethread(); },
        [&](topic_ptr it)
    {        
        if (m_mainDB) {
            auto t = dynamic_cast<ard::ethread*>(it);
            ASSERT(t, "expected ethread object");
            if (t) {
                m_threads_wrappers2link_after_load.push_back(t);
            }
        }
        return true;
    });
};


bool ArdDB::loadTree()
{
    ASSERT(m_root == nullptr, "expected NULL root"); 
    ASSERT(m_db.isOpen(), "expected open DB");
    m_data_loaded = false;

    enable_register_db_modification(false);
    m_data_adjusted_during_db_load = false;

    dbp::loadDBSettings(*this);

    CRUN(loadTopics);

    bool ok = (m_root != nullptr);
    if(ok)
        {

            CRUN(loadThreadRootSpace);
            CRUN(loadSingletons);
            CRUN(loadDraftEmails);
            CRUN(loadEThreadsExt);
            CRUN(loadContacts);
            CRUN(loadKRing);
            CRUN(loadBoards);
            CRUN(loadNotesExt);
			CRUN(loadPicturesExt);
			CRUN(loadQFilters);
        }
    
    if (ok) {
        CRUN(linkThreadWrappers);
    }

    enable_register_db_modification(true);    

	if (m_root->checkTreeSanity()){
		ard::trail("tree-sanity-check - PASSED");
	}
	else {
		ard::trail("tree-sanity-check - FAILED");
	}
//#ifdef _DEBUG
//    ASSERT(m_root->checkTreeSanity(), "sanity test failed");
//#endif

    CRUN(runLoadSanityRules);

    //qDebug() << "DB-opened" << root()->syncInfoAsString()
    //    << "items=" << m_id2item.size();
             //<< "notes=" << m_id2comment.size();
    m_data_loaded = true;
    return ok;
}

void ArdDB::loadAllNotesText()
{
    bool load_new_extension_note_text = true;
    if (load_new_extension_note_text) 
    {
        QString sql = QString("SELECT oid, note_text, note_plain_text FROM ard_ext_note");
        auto q = selectQuery(sql);
        assert_return_void(q, "expected query");
        while (q->next())
        {
            DB_ID_TYPE oid = q->value(0).toInt();
            topic_ptr it = lookupLoadedItem(oid);
            if (it) 
            {
                //if (!it->areCommentsDBLoaded())<<<< we might need this check
                {
                    QString html = q->value(1).toString();
                    QString plain_text = q->value(2).toString();
                    auto n = it->mainNote();
                    if (n) {
                        n->setupFromDb(html, plain_text);
                    }
                }
            }
            else
            {
                qWarning() << QString("loadAllNotesText - failed to locate DB-note object (%1) in local cache").arg(oid);
            }
        }
    }
};

int ArdDB::convertNotes2NewFormat() 
{
    struct old_note 
    {
        topic_ptr   dest_topic{nullptr};
        QString     html, plain_text;
    };

    std::vector<old_note> old_notes;

    int rv = 0;
    auto r = ard::root();
    if (r) 
    {
        if (!table_exists(this, "ard_notes"))
            return 0;

        loadAllNotesText();

        QString sql = QString("select oid, note_text, note_plain_text from ard_notes");
        auto q = selectQuery(sql);
        assert_return_0(q, "expected query");
        while (q->next())
        {
            DB_ID_TYPE oid = q->value(0).toInt();
            topic_ptr it = lookupLoadedItem(oid);
            if (it)
            {
                if (!it->hasNote()) 
                {
                    QString html = q->value(1).toString();
                    QString plain_text = q->value(2).toString();
                    if (!html.isEmpty())
                    {
                        old_note old;
                        old.dest_topic = it;
                        old.html = html;
                        old.plain_text = plain_text;
                        old_notes.push_back(old);
                    }
                }
            }
        }

        for (auto& i : old_notes) 
        {
            auto n = i.dest_topic->mainNote();
            if (n) {
                n->setNoteHtml(i.html, i.plain_text);
                rv++;
            }
        }

    }
    return rv;
};

RootTopic* ArdDB::createRoot()
{
    QString sdb_book_name = dbp::currentDBName();
    //    qDebug() << "current-db-name=" << sdb_book_name;
    if(sdb_book_name.compare(DEFAULT_DB_NAME) == 0)
        {
            sdb_book_name = "";
        }
    else
        {
            sdb_book_name = " - " + sdb_book_name;
        }


    ASSERT(!m_root, "Root should not be allocated");
    m_root          = new dbp::MyDataRoot(this, sdb_book_name);
    m_ethreads_root = new dbp::EThreadsRoot(this);
    m_contacts_model.reset(new ard::contacts_model(this));
	m_rules_model.reset(new ard::rules_model(this));

    m_gmail_runner		= new ard::rule_runner(m_rules_model.get());
    m_local_search		= new ard::local_search_observer;
    m_kring_model.reset(new ard::kring_model(this));
    m_boards_model.reset(new ard::boards_model(this));
    m_task_ring     = new ard::task_ring_observer;
    m_notes         = new ard::notes_observer;
	m_bookmarks		= new ard::bookmarks_observer;
	m_pictures		= new ard::pictures_observer;
    m_annotated     = new ard::comments_observer;
    m_colored       = new ard::color_observer;

    loadRootDetails();
    loadSyncHistory();

    return m_root;
};

void ArdDB::prepareForSync()
{
    loadAllNotesText();
};

void ArdDB::loadSyncHistory()
{
    assert_return_void(m_root != NULL, "expected root");

    QString sql = QString("SELECT oid, db_id, mdc FROM ard_dbs where rdb_string IS NULL or rdb_string = '' ORDER BY mdc DESC");
    auto q = selectQuery(sql);
    assert_return_void(q, "expected query");
    if( q->next() ) 
        {
            DB_ID_TYPE oid = q->value(0).toInt();
            COUNTER_TYPE db_id = q->value(1).toInt();
            COUNTER_TYPE mdc = q->value(2).toInt();

            QString sql2 = QString("SELECT synced_db_id, synced_mdc, last_sync_time, sync_point_id FROM ard_dbh WHERE db_oid=%1 ORDER BY last_sync_time DESC").arg(oid);

            ///this is case of subquery, we have to instantiate a new query object
            ///and can't utilyze internal query
            SqlQuery q2(m_db);    
            if(!q2.prepare(sql2))
                {
                    dbp::show_last_sql_err(q2, sql2);
                };
            if(!q2.exec())
                {
                    dbp::show_last_sql_err(q2, sql2);
                }
          
            m_root->initFromDB_SyncHistory(oid, db_id, mdc, q2);
            //qDebug() << "loaded root-record" << m_root->syncInfoAsString();
        }
    else
        {
            int rcount1 = queryInt("SELECT count(*) FROM ard_dbs where rdb_string IS NULL or rdb_string = ''");
            int rcount2 = queryInt("SELECT count(*) FROM ard_dbs");
            int rcount3 = queryInt("SELECT count(*) FROM ard_version");
            if(rcount1 > 0)
                {
                    ASSERT(0, "root DB record not found") << rcount1 << rcount2 << rcount3;
                }
        }
};

void ArdDB::loadRootDetails()
{
    assert_return_void(m_root, "expected root");
    //assert_return_void(m_resource_root, "expected resource root");
    assert_return_void(m_ethreads_root, "expected thread root");

    QString sql = QString(rootLoadSQL).arg(QString("%1 OR otype=%2 OR otype=%3 OR otype=%4 OR otype=%5 OR otype=%6 OR otype=%7 ORDER BY pid, pindex")
        .arg(objDataRoot)
        .arg(objEThreadsRoot)
        .arg(objContactRoot)
        .arg(objContactGroupRoot)
        .arg(objKRingRoot)
        .arg(objBoardRoot)
		.arg(objQFilterRoot));

    auto q = selectQuery(sql);
    assert_return_void(q, "expected query");
    bool dataRootLoaded         = false;
    //bool resourceRootLoaded     = false;
    bool threadsRootLoaded      = false;
    bool contactsRootLoaded     = false;
    bool contactGroupsRootLoaded = false;
    bool keysRootLoaded         = false;
    bool boardsRootLoaded       = false;
	bool qfilterRootLoaded = false;

    IDS_SET duplicate_root_records;

    while( q->next() ) 
        {
            DB_ID_TYPE oid = q->value(0).toInt();
            EOBJ otype = (EOBJ)q->value(3).toInt();
            switch(otype)
                {
                case objDataRoot:{
                        if(dataRootLoaded){
                                duplicate_root_records.insert(oid);
                            }
                        else{
                                m_root->setupRootDbItem(this, *q);
                            }
                        dataRootLoaded = true;
                    }break;
                case objEThreadsRoot: {
                    if (threadsRootLoaded) {
                        duplicate_root_records.insert(oid);
                    }
                    else {
                        m_ethreads_root->setupRootDbItem(this, *q);
                    }
                    threadsRootLoaded = true;
                }break;
                case objContactRoot: {
                    if (contactsRootLoaded) {
                        duplicate_root_records.insert(oid);
                    }
                    else {
                        m_contacts_model->croot()->setupRootDbItem(this, *q);
                        contactsRootLoaded = true;
                    }
                }break;
                case objContactGroupRoot: {
                    if (contactGroupsRootLoaded) {
                        duplicate_root_records.insert(oid);
                    }
                    else {
                        m_contacts_model->groot()->setupRootDbItem(this, *q);
                        contactGroupsRootLoaded = true;
                    }
                }break;                 
                case objKRingRoot: 
                {
                    if (keysRootLoaded) {
                        duplicate_root_records.insert(oid);
                    }
                    else {
                        m_kring_model->keys_root()->setupRootDbItem(this, *q);
                        keysRootLoaded = true;
                    }
                }break;
                //....
                case objBoardRoot:
                {
                    if (boardsRootLoaded) {
                        duplicate_root_records.insert(oid);
                    }
                    else {
                        m_boards_model->boards_root()->setupRootDbItem(this, *q);
                        boardsRootLoaded = true;
                    }
                }break;
				case objQFilterRoot: 
				{
					if (qfilterRootLoaded) {
						duplicate_root_records.insert(oid);
					}
					else {					
						m_rules_model->rroot()->setupRootDbItem(this, *q);
						qfilterRootLoaded = true;
					}
				}break;
                //....
                default:ASSERT(0, "NA") << otype;
                }
        }
};

template <class T, class D>
void ArdDB::loadReferences(QString sql, IDS_SET& orfant_oid_ref, IDS_SET& orfant_depend_oid_ref, QString referenceType)
{
    auto q = selectQuery(sql);
    assert_return_void(q, "expected query");
    while ( q->next() ) 
        {
            DB_ID_TYPE oid = q->value(1).toInt();
            DB_ID_TYPE depend_oid = q->value(2).toInt();
            topic_ptr it = lookupLoadedItem(oid);
            topic_ptr depend = lookupLoadedItem(depend_oid);

            if(!it)
                {
                    ASSERT(0, QString("[orfant-ref]load %1 - failed to locate object (%2) by oid").arg(referenceType).arg(oid));
                    orfant_oid_ref.insert(oid);
                    continue;
                }
            if(!depend)
                {
                    ASSERT(0, QString("[orfant-ref]load %1 - failed to locate object (%2) by depend_oid").arg(referenceType).arg(depend_oid));
                    orfant_depend_oid_ref.insert(depend_oid);
                    continue;
                }

            D* d = dynamic_cast<D*>(depend);
            if(!d)
                {
                    ASSERT(0, QString("load %1 - invalid depend object type %2").arg(referenceType).arg(depend->dbgHint()));
                    continue;
                }   

            assert_return_void(d, "expected dependant");
            ASSERT_VALID(it);
            ASSERT_VALID(d);
            
            #ifdef _DEBUG
            //qDebug() << "[dlink]" << it->dbgHint() << "[->]" << d->dbgHint();
            #endif            

            T* r = new T(it, this, *q, d);
            Q_UNUSED(r);    
        }
}

template <class T, class O>
int ArdDB::loadExtensions(QString sql, IDS_SET& orfants)
{
    int loaded_number = 0;

    auto q = selectQuery(sql);
    assert_return_0(q, "expected query");
    while ( q->next() )
    {
            DB_ID_TYPE oid = q->value(0).toInt();     
            topic_ptr it = lookupLoadedItem(oid);
            bool ok = true;
            if(!it){
                    // ASSERT(0, QString("load links - failed to locate object (%1) by oid").arg(oid));
                qWarning() << "failed to locate object by oid" << oid << "orfant extension will be removed";
                    orfants.insert(oid);
                    ok = false;
                }

            if(ok)  
                {
                    O* owner = dynamic_cast<O*>(it);
                    if(owner){
                            T* ex = new T(owner, *q);
                            ex->m_flags.has_db_record = 1;
                            Q_UNUSED(ex);
                            loaded_number++;
                        }
                    else
                        {
                            ASSERT(0, "expected different object type") << it->dbgHint();
                            orfants.insert(oid);
                        }
                }
        }

    return loaded_number;
};

void ArdDB::removeFromTableById(QString table_name, IDS_SET& orfants)
{
    QString ids = ardi_functional::idsToStr(orfants.begin(), orfants.end());
    QString sql = QString("DELETE FROM %1 WHERE oid IN(%2)").arg(table_name).arg(ids);
    qWarning() << "located orfant extensions will be deleted" << table_name << orfants.size();
    execQuery(sql);
};

template <class T, class O>
void ArdDB::loadAttachables(QString sql, IDS_SET& orfants)
{
    //very similiar to ====>>> loadExtensions<T, O>(sql, orfants);

    int loaded_number = 0;

    auto q = selectQuery(sql);
    assert_return_void(q, "expected query");
    while (q->next())
    {
        DB_ID_TYPE oid = q->value(0).toInt();
        topic_ptr it = lookupLoadedItem(oid);
        bool ok = true;
        if (!it) {
            // ASSERT(0, QString("load links - failed to locate object (%1) by oid").arg(oid));
            orfants.insert(oid);
            ok = false;
        }

        if (ok)
        {
            O* owner = dynamic_cast<O*>(it);
            if (owner) {
                T* ex = new T(owner, this, *q);
                Q_UNUSED(ex);
                loaded_number++;
            }
            else
            {
                ASSERT(0, "expected different object type") << it->dbgHint();
            }
        }
    }
};

void ArdDB::loadEThreadsExt() 
{
    IDS_SET orfants;
    QString sql = QString("SELECT oid, account_email, thread_id, mdc FROM ard_ext_ethread ORDER BY oid");
    loadExtensions<ard::ethread_ext, ard::ethread>(sql, orfants);
    if (!orfants.empty()) {
        removeFromTableById("ard_ext_ethread", orfants);
    }
};

void ArdDB::loadCryptoExt() 
{
    ASSERT(0, "NA");
    /*
    IDS_SET orfants;
    QString sql = QString("SELECT oid, content, mdc FROM ard_ext_crypto ORDER BY oid");
    loadExtensions<anCryptoNoteExt, ard::topic>(sql, orfants);
    if (!orfants.empty()) {
        removeOrfantExtensions("ard_ext_crypto", orfants);
    }
    */
};

/*
void ArdDB::loadQDetails() 
{
    IDS_SET orfants;
    QString sql = QString("SELECT oid, q, qlbl, qfilter_on, mdc FROM ard_ext_q ORDER BY oid");
    loadExtensions<anQExt, ard::topic>(sql, orfants);
#ifdef _DEBUG
    if (!orfants.empty()) {
        for (auto& i : orfants) {
            qWarning() << "orfant-Q-2remove" << i;
        }
        m_data_adjusted_during_db_load = true;
        dbp::removeQsByOID(this, orfants);
    }
#endif
};*/

#ifdef _DEBUG
void ArdDB::dbgPrint() 
{
    for (auto& j : m_id2item) {
        qDebug() << j.second->dbgHint();
    }
    qDebug() << "=== end db items ===";
};
#endif

void ArdDB::loadDraftEmails() 
{
    IDS_SET orfants;
    QString sql = QString("SELECT oid, email_eid, mdc, email_to, email_cc, email_bcc, attachments, attachments_host, labels, content_modified, email_references, email_threadid, userid FROM ard_ext_draft ORDER BY oid");
    loadExtensions<ard::email_draft_ext, ard::email_draft>(sql, orfants);
    if (!orfants.empty()) {
        for (auto& id : orfants) {
            qWarning() << "orfant-draft-2remove" << id;
        }
        m_data_adjusted_during_db_load = true;
        dbp::removeDraftsByOID(this, orfants);
    }
};

void ArdDB::linkThreadWrappers() 
{
    if (m_mainDB) {
        if (!ard::isGoogleConnected()) {
            if (!ard::hasGoogleToken()) {
                return;
            }
        }

        if (!m_threads_wrappers2link_after_load.empty()) {
            m_serialize_unresolvable_threads = ard::gmail_model()->linkThreadWrappers(m_threads_wrappers2link_after_load);
            m_threads_wrappers2link_after_load.clear();
        }
    }
};

QString specialGtdFolderName(EFolderType type)
{
    QString rv = "ERR";
    switch(type)
        {
        case EFolderType::folderSortbox:    rv = "Sortbox";break;
        case EFolderType::folderRecycle:    rv = "Trash";break;
        case EFolderType::folderMaybe:      rv = "Maybe/Someday";break;
        case EFolderType::folderReference:  rv = "Reference";break;
        case EFolderType::folderDelegated:  rv = "Delegated";break;
        case EFolderType::folderUserSortersHolder: rv = "Folders";break;
        case EFolderType::folderBacklog:     rv = "Backlog"; break;
        case EFolderType::folderBoardTopicsHolder:     rv = "BoardTopics"; break;
        default:ASSERT(0, "NA");
        }
    return rv;
}

void ArdDB::loadSingletons()
{
	assert_return_void(m_root != NULL, "expected root");
	typedef std::map<EFolderType, topic_ptr>   TYPE_2_TOPIC;

	TYPE_2_TOPIC type2topic;
	TOPICS_LIST items_list;
	TOPICS_LIST duplicate2kill;
	CITEMS singleton_list;

	dbp::findItemsWHERE(items_list, this, QString("otype=%1 AND subtype in (%2, %3, %4, %5, %6, %7, %8, %9)")
		.arg(objFolder)
		.arg(static_cast<int>(EFolderType::folderSortbox))
		.arg(static_cast<int>(EFolderType::folderUserSortersHolder))
		.arg(static_cast<int>(EFolderType::folderRecycle))
		.arg(static_cast<int>(EFolderType::folderMaybe))
		.arg(static_cast<int>(EFolderType::folderReference))
		.arg(static_cast<int>(EFolderType::folderDelegated))
		.arg(static_cast<int>(EFolderType::folderBacklog))
		.arg(static_cast<int>(EFolderType::folderBoardTopicsHolder))
	);

	for (auto& f : items_list) {
		auto k = type2topic.find(f->folder_type());
		if (k == type2topic.end())
		{
			type2topic[f->folder_type()] = f;
		}
		else
		{
			auto first = k->second;
			//duplicate found - merge into first
			first->takeAllItemsFrom(f);
			duplicate2kill.push_back(f);
		}
	}

	bool persistance_required = false;

	if (!duplicate2kill.empty())
	{
		ard::killSilently(duplicate2kill.begin(), duplicate2kill.end());
		persistance_required = true;
	}

	TYPE_2_TOPIC::iterator k;

#define ENSURE_GTD(T)  k = type2topic.find(T);                          \
    if(true)                                                            \
        {                                                               \
            topic_ptr gtd_folder = nullptr;                                 \
            if(k == type2topic.end())                                   \
                {                                                       \
                    QString stitle = specialGtdFolderName(T);           \
                    gtd_folder = new ard::locus_folder(stitle, T);      \
                    m_root->addItem(gtd_folder);                        \
                    persistance_required = true;                        \
                }                                                       \
            else                                                        \
                {                                                       \
                    gtd_folder = k->second;                             \
                }                                                       \
            if(gtd_folder != NULL)singleton_list.push_back(gtd_folder); \
        }                                                               \



	singleton_list.push_back(m_root);
	singleton_list.push_back(m_ethreads_root);
	singleton_list.push_back(m_contacts_model->croot());
	singleton_list.push_back(m_contacts_model->groot());
	singleton_list.push_back(m_kring_model->keys_root());
	singleton_list.push_back(m_boards_model->boards_root());
	singleton_list.push_back(m_rules_model->rroot());

	ENSURE_GTD(EFolderType::folderSortbox);
	ENSURE_GTD(EFolderType::folderReference);
	ENSURE_GTD(EFolderType::folderMaybe);
	ENSURE_GTD(EFolderType::folderDelegated);
	ENSURE_GTD(EFolderType::folderRecycle);
	ENSURE_GTD(EFolderType::folderUserSortersHolder);
	ENSURE_GTD(EFolderType::folderBacklog);
	ENSURE_GTD(EFolderType::folderBoardTopicsHolder);

	ASSERT(singleton_list.size() == SINGLETON_FOLDERS_COUNT, QString("incorrect singleton folders #, expected %1, found:").arg(SINGLETON_FOLDERS_COUNT)) << singleton_list.size();
#undef ENSURE_GTD


	m_root->db().registerSingletons(singleton_list);

	if (persistance_required)
	{
		if (!m_root->ensurePersistant(-1))
		{
			ASSERT(0, "root persistance error");
			return;
		}
	}
};

void ArdDB::runLoadSanityRules()
{
    bool persistance_required = m_data_adjusted_during_db_load;

    auto inbox = findLocusFolder(EFolderType::folderSortbox);
    assert_return_void(inbox, "failed to locate 'Sortbox'");

    /// clean up root from regular topics
    if (pullOrdinaryTopicsOutOfRoot())
    {
        persistance_required = true;
    }

    if (cleanupIncompatible(m_root))
    {
        persistance_required = true;
    }

    /// collect orphants
    if (adoptOrphants())
    {
        persistance_required = true;
    }

    if (persistance_required)
    {
        if (!m_root->ensurePersistant(-1))
        {
            ASSERT(0, "root persistance error");
        }
        if (m_ethreads_root) 
        {
            if (!m_ethreads_root->ensurePersistant(-1))
            {
                ASSERT(0, "ethread root persistance error");
            }
        }
    }
};

/**
   have to take all ordinary topics out of root (if any)
   and put into "Inbox" so root topic would contain only
   GTD sorting folders and project topics
**/
bool ArdDB::pullOrdinaryTopicsOutOfRoot()
{
    bool persistance_required = false;

    TOPICS_LIST topicsInRoot2PullOut;
    for (auto& i : m_root->items()) {
        topic_ptr it = dynamic_cast<topic_ptr>(i);
        if (it != NULL)
        {
            if (!it->isGtdSortingFolder())
            {
                topicsInRoot2PullOut.push_back(it);
            }
        }
    }

    if (!topicsInRoot2PullOut.empty())
    {
        auto inbox = findLocusFolder(EFolderType::folderSortbox);
        assert_return_false(inbox, "failed to locate 'Sortbox'");
        for (auto& it : topicsInRoot2PullOut) {
            qDebug() << "relocated:" << it->dbgHint();
            m_root->detachItem(it);
            inbox->addItem(it);
            it->m_move_counter = m_root->db().db_mod_counter() + 1;
            it->ask4persistance(np_SYNC_INFO);
            /// register_db_modification will fail here
            /// we need some special treatment for orphant insert during initial DB load
        }

        persistance_required = true;
    }

    return persistance_required;
};

bool ArdDB::cleanupIncompatible(snc::cit* parent)
{
    assert_return_false(parent != NULL, "expected parent");

    CITEMS issue_items;

    bool persistance_required = false;

    for(auto& it : parent->items())    {
            if(!parent->canAcceptChild(it))
                {
                    persistance_required = true;
                    issue_items.push_back(it);
					qWarning() << "incompatible-discovered" << it->dbgHint();
                }
        }

    for(auto& it : issue_items)    {
            topic_ptr f = dynamic_cast<topic_ptr>(it);
            assert_return_false(f != NULL, "expected topic");
            if(f != NULL)
                {
                    parent->remove_cit(f, false);
                    m_serialize_orphants.push_back(f);
                }
        }

    for(auto& it : parent->items())    {
            if(cleanupIncompatible(it))
                {
                    persistance_required = true;
                };
        }

    return persistance_required;
};

bool ArdDB::adoptOrphants()
{
    auto inbox = findLocusFolder(EFolderType::folderSortbox);
    assert_return_false(inbox, "failed to locate 'Inbox'");

    bool persistance_required = false;
    if (!m_serialize_orphants.empty())
    {
        persistance_required = true;
        for (auto& it : m_serialize_orphants) {
            it->clearSyncInfoForAllSubitems();
            inbox->addItem(it);
        }
        m_serialize_orphants.clear();
    }

    if (!m_serialize_duplicate_syid.empty()) 
    {
        persistance_required = true;
        for (auto& i : m_serialize_duplicate_syid) {
            i->regenerateSyid();
            qWarning() << "regenerated syid " << i->dbgHint();
        }
        m_serialize_duplicate_syid.clear();
    }

    if (!m_serialize_unresolvable_threads.empty()) 
    {
        persistance_required = true;
        for (auto& i : m_serialize_unresolvable_threads) {
            qWarning() << "clean unresolvable thread" << i->dbgHint();
            auto s = i->annotation().trimmed();
            if (!s.isEmpty()) {
                topic_ptr o2 = new ard::topic(QString("Adopted unresolvable thread with annotation '%1'").arg(i->id()));
                o2->setAnnotation(s);
                inbox->addItem(o2);             
            }
          //  i->killSilently(false);
        }

		TOPICS_LIST items2delete;
		for (auto t : m_serialize_unresolvable_threads) {
			if (IS_VALID_DB_ID(t->id())) {
				items2delete.push_back(t);
				ard::trail(QString("scheduled DB-delete %1").arg(t->dbgHint()));
			}
		}
		if (!items2delete.empty())
		{
			ard::trail(QString("removing from DB ext-unresolved threads [%1]").arg(items2delete.size()));
			dbp::removeTopics(this, items2delete);
			for (auto& t : items2delete) {
				auto p = t->parent();
				if (p) {
					p->remove_cit(t, false);
					t->release();
				}
			}
		}

        m_serialize_unresolvable_threads.clear();
    }

    return persistance_required;
};

/*
topic_ptr ArdDB::createCustomFolderBySyid(snc::SYID syid)
{
    assert_return_null(m_root, "expected root");    
    auto croot = ard::CustomSortersRoot();
    assert_return_null(croot, "expected sorters root");

    S2S::iterator i = defCustomFolderSyid2Name.find(syid);
    if(i == defCustomFolderSyid2Name.end()){
        ASSERT(0, "unsupported CONST SYID provided") << syid;
        return NULL;
    }

    topic_ptr rv = new ard::topic(defaultCustomFolderNameBySYID(syid), EFolderType::folderUserSorter);
    croot->addItem(rv);
    rv->m_syid = syid;
    /// set it as modified or next sync will delete it
    rv->m_mod_counter = m_root->db().db_mod_counter() + 1;
    return rv;
};
*/
ard::locus_folder* ArdDB::createCustomFolderByName(QString _name)
{
    assert_return_null(m_root, "expected root");    
    auto croot = ard::CustomSortersRoot();
    assert_return_null(croot, "expected sorters root");

    QString name = _name.trimmed();

    auto f = croot->findTopicByTitle(name);
    if (f) {
        QString s = QString("createCustomFolderByName. Folder '%1' already exists.").arg(name);
        ASSERT(0, s);
        return nullptr;
    }

    auto rv = new ard::locus_folder(name);
    croot->addItem(rv);
    return rv;
};

void ArdDB::registerObjId(topic_ptr it) 
{
#ifdef _DEBUG
    if (it->otype() == objUnknown)
    {
        ASSERT(0, "can't register objUnknown") << it->dbgHint();
        return;
    }
    auto k = m_id2item.find(it->id());
    if (k != m_id2item.end())
    {
        ASSERT(0, QString("duplicate id[%1] in cache size=[%2]").arg(it->id()).arg(m_id2item.size())) << it->dbgHint();
        topic_ptr it2 = k->second;
        ASSERT(!it2->isOnLooseBranch(), "loose branch item") << it2->dbgHint();
        qDebug() << QString("prev item by ID %1").arg(it2->id()) << it2->dbgHint();
    }
#endif
    m_id2item[it->id()] = it;
};

bool ArdDB::registerObj(topic_ptr it)
{
    if (it->id() == 0)
    {
        return true;
    }

    registerObjId(it);
    bool rv = registerSyid(it);
    return rv;
};

bool ArdDB::attach_data_db(topic_ptr o)
{
    bool rv = true;
    o->m_data_db = this;
    if (IS_VALID_DB_ID(o->m_id))
    {
        rv = o->m_data_db->registerObj(o);
    }
    return rv;
};

bool ArdDB::registerSyid(topic_ptr f)const
{
    bool rv = true;
    auto syid = f->syid();
    if (!syid.isEmpty()) 
    {
        auto k = m_syid2topic.find(syid);
        if (k != m_syid2topic.end())
        {
            rv = false;
            ASSERT(0, QString("duplicate syid[%1] in cache size=[%2]").arg(syid).arg(m_syid2topic.size())) << f->dbgHint();
        }
        else 
        {
            m_syid2topic[syid] = f;
        }
    }
    return rv;
};


template <class T>
void do_unregister_obj(std::unordered_map<DB_ID_TYPE, T*>& m, T* o)
{
#ifdef _DEBUG
    //  qDebug() << "[un-reg]" << o->dbgHint();
#endif

    if(!m.empty() && IS_VALID_DB_ID(o->id()))
        {
            typedef typename std::unordered_map<DB_ID_TYPE, T*>::iterator ITR;
            ITR j = m.find(o->id());
            if(j == m.end())
                {
                    ASSERT(0, "id not found") << o->dbgHint();
                }
            else
                {
                    m.erase(j);
                }
        }  
}

void ArdDB::unregisterObj(topic_ptr it)
{
    do_unregister_obj(m_id2item, it);

    auto syid = it->syid();
    if (!syid.isEmpty()) {
        auto i = m_syid2topic.find(syid);
        if (i != m_syid2topic.end()) {
            m_syid2topic.erase(i);
        }
    }
};

template <class T>
T* lookup_loaded(std::unordered_map<DB_ID_TYPE, T*>& m, DB_ID_TYPE oid)
{
    typedef typename std::unordered_map<DB_ID_TYPE, T*>::iterator ITR;
    T* rv = nullptr;
    ITR j = m.find(oid);
    if(j != m.end())
        {
            rv = j->second;
        }
    else
        {
            //    ASSERT(0, "id not found") << oid << m.size();
        }  
    return rv;
}

size_t ArdDB::lookupMapSize()const
{
    size_t rv = m_id2item.size();
    return rv;
};

topic_ptr ArdDB::lookupLoadedItem(DB_ID_TYPE oid)
{
    if(oid == 0)
        {
            // ASSERT(0, "root lookup doesn't allowed");
            return root();
        }

    topic_ptr it = lookup_loaded(m_id2item, oid);
    return it;
};

TOPICS_LIST ArdDB::lookupAndLockLoadedItemsList(const IDS_LIST& ids)
{
	TOPICS_LIST rv;
    for (auto& s : ids) {
        auto it = m_id2item.find(s);
        if (it != m_id2item.end())
		{
			if (!it->second->IsUtilityFolder()) 
			{
				rv.push_back(it->second);
				LOCK(it->second);
			}
        }
    }
    return rv;
};

//
//pipeItems - we take SQL that returns one column - OID
//
void ArdDB::pipeItems(QString oidSQL, TOPICS_LIST& items_list)
{
    auto q = selectQuery(oidSQL);
    assert_return_void(q, "expected query");
    while(q->next())
        {
            DB_ID_TYPE oid = q->value(0).toInt();
            topic_ptr it = lookupLoadedItem(oid);
            if(it){
                    items_list.push_back(it);
                }
        }
};

static void calcHash4Root(topic_ptr r, CompoundInfo& ci/*QString& hres, int& size, int& compound_size*/)
{
	QString tmp = "", s = "";
	snc::MemFindAllPipe mp;
	r->memFindItems(&mp);
	ci.compound_size = 0;
	for (const auto& i : mp.items())
	{
		topic_ptr it = dynamic_cast<topic_ptr>(i);
		CompoundObserver o(it, false);
		ci.compound_size += o.prim_list().size();
		s = it->calcContentHashString() + tmp;
		tmp = QCryptographicHash::hash((s.toUtf8()), QCryptographicHash::Md5).toHex(); ;
	}

	ci.hashStr = QCryptographicHash::hash((tmp.toUtf8()), QCryptographicHash::Md5).toHex();
	ci.size = mp.items().size();
};

void ArdDB::calcContentHash(STRING_MAP& hm)const
{
    assert_return_void(m_root, "expected data root");
    //assert_return_void(m_resource_root, "expected resource root");
    assert_return_void(m_ethreads_root, "expected thread root");
    //assert_return_void(m_contacts_root, "expected contacts root");
    //assert_return_void(m_contact_groups_root, "expected contact groups root");

    QString tmp = "";
	if (m_root)
	{
		CompoundInfo ci;
		calcHash4Root(m_root, ci);
		hm["outline"] = ci.toString();
	}

    if (m_ethreads_root)
    {
		CompoundInfo ci;
        calcHash4Root(m_ethreads_root, ci);
        hm["ethreads"] = ci.toString();
    }

    if (m_contacts_model->croot())
    {
		CompoundInfo ci;
        calcHash4Root(m_contacts_model->croot(), ci);
        hm["contacts"] = ci.toString();
    }

    if (m_contacts_model->groot())
    {
		CompoundInfo ci;
        calcHash4Root(m_contacts_model->groot(), ci);
        hm["contacts"] = ci.toString();
    }

    if (m_kring_model->keys_root())
    {
		CompoundInfo ci;
        calcHash4Root(m_kring_model->keys_root(), ci);
        hm["kring"] = ci.toString();
    }

    if (m_boards_model->boards_root())
    {
		snc::CompoundInfo ci;
        calcHash4Root(m_boards_model->boards_root(), ci);
        hm["boards"] = ci.toString();

		ci = m_boards_model->boards_root()->compileCompoundInfo();
		hm["b-links"] = ci.toString();	
    }

    ///@todo: add hash-calc on project resource allocations
};

bool ArdDB::ensurePersistant()
{
    /*
        the order matters, data root should be last to store
     */

    if (!m_ethreads_root->ensurePersistant(-1)) {
        ASSERT(0, "threads persistance error");
        return false;
    }

    if (!m_contacts_model->croot()->ensurePersistant(-1)) {
        ASSERT(0, "contacts persistance error");
        return false;
    }

    if (!m_contacts_model->groot()->ensurePersistant(-1)) {
        ASSERT(0, "contact groups persistance error");
        return false;
    }

    if (!m_kring_model->keys_root()->ensurePersistant(-1)) {
        ASSERT(0, "keys persistance error");
        return false;
    }

    if (!m_boards_model->boards_root()->ensurePersistant(-1)) {
        ASSERT(0, "boards persistance error");
        return false;
    }

	if (!m_rules_model->rroot()->ensurePersistant(-1)) {
		ASSERT(0, "rules persistance error");
		return false;
	}	

    if(!m_root->ensurePersistant(-1)){
            ASSERT(0, "root persistance error");
            return false;
        }
 
    return true;
};


bool ArdDB::updateChildrenPIndexesInBatch(topic_ptr parent)
{
    ASSERT_VALID(parent);

    QString sql = QString("UPDATE ard_tree SET pindex=? WHERE oid=?");
  
    auto q = prepareQuery(sql);
    if (!q) {
        return false;
    }
    m_db.transaction();
    int pindex_val = PINDEX_STEP;
    QVariantList pindexes, oids;
    for(const auto& i : parent->items())    {
            topic_ptr it = dynamic_cast<topic_ptr>(i);
            it->m_pindex = pindex_val;
            pindex_val += PINDEX_STEP;

            pindexes << it->m_pindex;
            oids << it->id();
        }
    q->addBindValue(pindexes);
    q->addBindValue(oids);

    if(!q->execBatch())
        {
            m_db.rollback();
            dbp::show_last_sql_err(*q, sql);
            return false;
        }
    m_db.commit();

    return true;
};

ard::locus_folder* ArdDB::findLocusFolder(EFolderType type)
{
    assert_return_null(m_root, "expected root");

    ESingletonType stype = ESingletonType::none;
    switch(type)
        {
        case EFolderType::folderRecycle:stype = ESingletonType::gtdRecycle;break;
        case EFolderType::folderMaybe:stype = ESingletonType::gtdInkubator;break;
        case EFolderType::folderReference:stype = ESingletonType::gtdReference;break;
        case EFolderType::folderDelegated:stype = ESingletonType::gtdDelegated;break;
        case EFolderType::folderSortbox:stype = ESingletonType::gtdSortbox;break;
        case EFolderType::folderUserSortersHolder:stype = ESingletonType::UFoldersHolder;break;
        case EFolderType::folderBacklog:stype = ESingletonType::gtdDrafts; break;
        case EFolderType::folderBoardTopicsHolder:stype = ESingletonType::boardTopicsHolder; break;
        default:ASSERT(0, "invalid folder type provided") << static_cast<int>(type);
        }

    assert_return_null(stype != ESingletonType::none, "expected valid singleton type");
  
    snc::cit* it = m_root->db().getSingleton(stype);
    ASSERT(it != NULL, "failed to locate singleton") << static_cast<int>(stype);
    auto rv = dynamic_cast<ard::locus_folder*>(it);
	/*if (rv) 
	{
		auto lcs = dynamic_cast<ard::locus_folder*>(it);
		if (!lcs) {
			qDebug() << it->dbgHint();
			assert_return_null(lcs, QString("expected locus_folder as singleton [%1]").arg(static_cast<int>(type)));
		}
	}*/
	//assert_return_null(rv, QString("expected locus_folder as singleton [%1]").arg(static_cast<int>(type)));
    return rv;
};

///ykh - this function appears too advance and too scary
bool ArdDB::mergeDuplicates(topic_ptr parent, TopicMatch* m)
{
    typedef std::map<QString, TOPICS_LIST>   NAME_2_LIST;
    NAME_2_LIST groups;
    TOPICS_LIST merge_takers;
    TOPICS_LIST topics2kill;

    /// build groups
    for(const auto& i : parent->items())    {
            topic_ptr it = dynamic_cast<topic_ptr>(i);
            QString title = it->title().trimmed();
            NAME_2_LIST::iterator j = groups.find(title);
            if(j == groups.end())
                {
                    TOPICS_LIST lst;
                    lst.push_back(it);
                    groups[title] = lst;
                }
            else
                {
                    TOPICS_LIST& lst = j->second;
                    lst.push_back(it);
                }
        }  

    /// select merge taker - the one that will absorb group
    for(auto& i : groups)    {
            if(i.second.size() > 1)
                {
                    // group of duplicates
                    topic_ptr merge_taker = nullptr;
                    TOPICS_LIST& lst = i.second;
                    for(const auto& j : lst)    {
                            if(m->match(j))
                                {
                                    merge_taker = j;
                                }
                        }
                    if(merge_taker == NULL)
                        {
                            merge_taker = *(lst.begin());
                        }
                    merge_takers.push_back(merge_taker);

                    qDebug() << "merge-group" << merge_taker->title() << lst.size() << merge_taker->syid();
                }
        }

    bool changed_data = false;

    /// merge duplicate folders into merge takers  
    for(const auto& merge_taker : merge_takers)    {
            NAME_2_LIST::iterator j = groups.find(merge_taker->title().trimmed());
            if(j != groups.end())
                {
                    TOPICS_LIST& lst = j->second;
                    for(auto& f_parent : lst)    {
                            if(f_parent != merge_taker)
                                {
                                    changed_data = true;
                                    merge_taker->takeAllItemsFrom(f_parent);
                                    topics2kill.push_back(f_parent);
                                    qDebug() << "ready2merge" << f_parent->dbgHint() << "->" << merge_taker->dbgHint();
                                }
                        }
                }
        }
  

    /// finally delete empty topics that got merged
  
    if(!topics2kill.empty())
        {
            ard::killSilently(topics2kill.begin(), topics2kill.end());
        }  

    return changed_data;
};

bool ArdDB::sql_process_id_set(const IDS_SET& ids, FUN_IDS_DB_PROCESSOR ids_processor, DB_ID_TYPE id_param, DB_ID_TYPE param2)
{
    IDS_LIST    ids_list;

    for (const auto& id : ids) {
        ids_list.push_back(id);
        if (ids_list.size() == MAX_IN_SQL_LIST_STEP)
        {
            if (!ids_processor(this, ids_list, id_param, param2))
                return false;
            ids_list.clear();
        }
    }

    if (!ids_list.empty())
    {
        if (!ids_processor(this, ids_list, id_param, param2))
            return false;
    }

    return true;
}

std::pair<snc::COUNTER_TYPE, snc::COUNTER_TYPE> ArdDB::getSyncCounters4NewRegistration()const
{
    std::pair<COUNTER_TYPE, COUNTER_TYPE> rv = {0,0};
    if (syncDb()) {
        rv.first = syncDb()->db_id();
        rv.second = syncDb()->db_mod_counter() + 1;
    }

    if (!IS_VALID_COUNTER(rv.first)) {
        static DB_ID_TYPE last_init_db_id = 0;

        rv.first = time(NULL) - SYNC_EPOCH_TIME;
        if (last_init_db_id == rv.first)
        {
            rv.first++;
            std::this_thread::yield();
        }
        last_init_db_id = rv.first;
    }

    return rv;
};

std::pair<uint64_t, uint64_t> def_encr_key() {
    std::pair<uint64_t, uint64_t> k{ 72007877432252874,7319086866514064968 };
    return k;
}

bool synchronizeBoardLinks(snc::persistantDB* p1,
    snc::persistantDB* p2, 
    COUNTER_TYPE db1_local_hist_mod_counter,
    COUNTER_TYPE db2_remote_hist_mod_counter,
    snc::SyncProgressStatus* progressStatus)
{
    bool changes_detected = false;
    auto db1 = dynamic_cast<ArdDB*>(p1);
    auto db2 = dynamic_cast<ArdDB*>(p2);
    assert_return_false(db1, "expected DB1");
    assert_return_false(db2, "expected DB2");

    ard::boards_root* br1 = nullptr;
    ard::boards_root* br2 = nullptr;
    if (db1->boards_model()) {
        br1 = db1->boards_model()->boards_root();
    }
    if (db2->boards_model()) {
        br2 = db2->boards_model()->boards_root();
    }
    assert_return_false(br1, "expected broot1");
    assert_return_false(br2, "expected broot2");

	br1->prepare_link_sync();
	br2->prepare_link_sync();

	using SLM_SET = std::unordered_set<ard::board_link_map*>;
	SLM_SET compiled_set;
	ard::SLM_LIST flst;

	std::function<void(ArdDB*, ArdDB*, ard::boards_root*, ard::boards_root*, COUNTER_TYPE, COUNTER_TYPE)>build_list =
		[&compiled_set, &flst](ArdDB* d1, ArdDB* d2, ard::boards_root* r1, ard::boards_root* r2, COUNTER_TYPE c1, COUNTER_TYPE c2)
	{
		auto& syid2m = r1->allLinkMaps();
		for (auto& i : syid2m)
		{
			auto board_syid = i.first;
			auto lm1 = i.second.get();
			auto lm2 = r2->lookupLinkMap(board_syid);
			if (!lm2) {
				lm2 = r2->createLinkMap(board_syid);
			}

			auto k = compiled_set.find(lm1);
			if (k == compiled_set.end()) {
				ard::link_map_sync_info mi;
				mi.fm1 = lm1->produceFlatLinksMap(d1, c1);
				mi.fm2 = lm2->produceFlatLinksMap(d2, c2);
				compiled_set.insert(lm1);
				compiled_set.insert(lm2);
				flst.push_back(mi);
			}
		}
	};

	build_list(db1, db2, br1, br2, db1_local_hist_mod_counter, db2_remote_hist_mod_counter);
	build_list(db2, db1, br2, br1, db2_remote_hist_mod_counter, db1_local_hist_mod_counter);
	changes_detected = ard::flat_links_sync_map::synchronizeFlatMaps(flst, progressStatus);
	bool maps_in_sync = true;
	for (auto& i : flst) {
		auto h1 = i.fm1->toHashStr();
		auto h2 = i.fm2->toHashStr();
		if (h1 != h2) {
			maps_in_sync = false;
			sync_log(QString("lmap-hash-error-after-sync [%1][%2]").arg(i.fm1->toString()).arg(i.fm2->toString()));
		}
	}

	if (maps_in_sync) {
		sync_log(QString("all-lmap-hash-OK-after-sync"));
	}

    return changes_detected;
};

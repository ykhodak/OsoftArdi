#include <QDir>
#include <QCoreApplication>
#include <time.h>
#include <QSqlError>

#include "a-db-utils.h"
#include "dbp.h"
#include "syncpoint.h"
#include "ansyncdb.h"
#include "snc-tree.h"
#include "email_draft.h"
#include "email.h"
#include "ethread.h"
#include "locus_folder.h"
#include "db-merge.h"
#include "contact.h"
#include "csv-util.h"
#include "kring.h"
#include "board.h"
#include "anurl.h"
#include "picture.h"
#include "rule.h"

static QString sync_test_db_name = "sync-auto-test";
static QString sync_test_db_name_2 = "sync-auto-test-2";
static QString working_branch_name = "OUTSTANDIN ITEMS";
#define RAND_MODIFY_PERCENTAGES 40

QString autoTestDbName(){return sync_test_db_name;}

typedef std::vector<QString> STRING_ARR;


/**
   directory structure for autotest   

   $HOME/tmp/autotest/d2 (Unix) C:/tmp/autotest/d2 (Win)
   $HOME/tmp/autotest/i4i C:/tmp/autotest/i4i
*/
#ifdef Q_OS_WIN32  
#define AUTOTEST_ROOT QString("C:/tmp/autotest")
#else
#define AUTOTEST_ROOT QString(QDir::homePath() + "/tmp/autotest")
#endif

#define AUTOTEST_CONTACTS_IMPORT_PATH   AUTOTEST_ROOT + "/c4i"
#define AUTOTEST_BACKUP_IMPORT_PATH     AUTOTEST_ROOT + "/bkp4i"



class SyncAutoTest
{
public:
    SyncAutoTest(int test_level, QString current_sync_pwd, QString kring_pwd);
    ~SyncAutoTest();
    bool run();
    static bool checkEnvironment();

    template<class T>
    int generateOutlineStructure(topic_ptr parent, QString prefix);

protected:
    bool syncLocal(QString step_desc);
    bool syncLocalAgainstMasterX(int masterDBindex, QString step_desc);
    bool generateTopics();
    bool generateDrafts();
    bool generateBBoard();
    bool generateContacts();
    bool importCsvContacts();
    bool generateKeys();
    bool randomDeleteKeys();
    bool generateAnnotations();
    bool generateTaskRingTodos();
    bool clearEverything();
    bool verifyOutline();
    bool randomizeTitle();
    bool randomizeColor();
    bool randomizeAttributes();
    bool randomizePOS();
    bool randomKill();
    bool randomizeRawData();
    bool backupRandomKillMerge();

    void logError(QString s);
    void logProgress(QString s);
    topic_ptr wbranch();

    bool openTestRepositoryOnDB(QString test_db_name);

    bool generateTableBBoard();
    bool generateGreekBBoard();
    bool generateBookmarks();
	bool generatePictures();
	bool generateRules();
protected:
    bool bingo()const;
    bool bingo(topic_ptr f, int chance_percentages = -1)const;
    int itemsBingo(snc::MemFindAllPipe& mp, 
        TOPICS_SET& items_bingo,
        bool allowAncestors = true,
        int NumOfTopics = -1);

    bool randomizeTitle(snc::cit* r);
    bool randomizeColor(snc::cit* r);
    ///this will introduce identity errors
    bool randomizeRawData(snc::cit* r);
    bool randomizeOutlineAttributes(snc::cit* r);
    bool randomGenerateAnnotations(snc::cit* r);
    bool randomTaskRingTodos(snc::cit* r);
    bool randomizeItemsInOutlinePOS();
    void updateContentHash();
    void switch2NewPassword();
    void printStepResult(int step, int total, QString desc);
    void printDiff();
    void identityDiff(cit* it, ArdDB* otherDB);
    void identityDiff(cdb* db, ArdDB* otherDB);
    template<class T>
    void attachableIdentityDiff(const std::vector<T*>& arr, ArdDB* otherDB);
    bool randomDeleteItemsInOutline();
    bool verifyLocalCache();
    QString backupCurrentIntoTmpAutoTest();
    bool    backupRandomKillMergeCurrentAutotestDB();

    QString  dbx()const{return m_db_prefix;}
    QString  gnx()const{return m_generate_prefix;}

private:
    QString nextResourceShortName();
    QString nextTopicName(QString prefix = "");

protected:
    int         m_test_level;
    QString     m_db_prefix, m_generate_prefix;
    STRING_MAP  m_dbx2hash;
    STRING_ARR  m_res_short_names;
    STRING_ARR  m_topic_names;
    size_t      m_curr_res_name_idx, m_curr_topic_name_idx;
    int         m_step, m_total;
    QString     m_sync_pwd, m_kring_pwd;
};

QString remoteDBPath4LocalSync()
{
    static QString remote_db_path = AUTOTEST_ROOT + "/d2";
    return remote_db_path;
}

QString remoteMPPath4LocalSync(QString composite_prefix)
{
    QString remote_mp_path = remoteDBPath4LocalSync();
    if (composite_prefix.isEmpty()){
        remote_mp_path += "/Ardi/";
    }
    else {
        remote_mp_path += QString("/Ardi-%1/").arg(composite_prefix);
    }
    return remote_mp_path;
}

/*
QString SyncAutoTest::imagesImportPath()
{
    static QString images_path = AUTOTEST_ROOT + "/i4i";
    return images_path;
};
*

QString SyncAutoTest::branchTmpExportPath()
{
    QString rv = remoteDBPath4LocalSync() + "/tmp_export_area";
    return rv;
};
*/

topic_ptr SyncAutoTest::wbranch()
{
    auto inbox = ard::Sortbox();
    auto b = inbox->findTopicByTitle(working_branch_name);
    if(!b)
        {
            b = new ard::topic(working_branch_name);
            inbox->addItem(b);
            logProgress(QString("recreating working branch:%1").arg(working_branch_name));
            bool rv = inbox->ensurePersistant(-1);
            assert_return_null(rv, "failed to ensure persistant");
        }
    return b;
};

bool SyncAutoTest::checkEnvironment()
{
#define ENSURE_PATH(D){  QDir dir(D);                                   \
        if (!dir.exists()) {                                            \
            if(!dir.mkpath("."))                                        \
                {                                                       \
                    ASSERT(0, "failed to create directory path") << D;  \
                    return false;                                       \
                }                                                       \
        }}                                                              \

    ENSURE_PATH(remoteDBPath4LocalSync());
    ENSURE_PATH(AUTOTEST_CONTACTS_IMPORT_PATH);
    ENSURE_PATH(AUTOTEST_BACKUP_IMPORT_PATH);

#undef ENSURE_PATH

    /// clean up log file as it can grow big during autotest //
    if(QFile::exists(get_program_appdata_log_file_name())){
            QFile::remove(get_program_appdata_log_file_name());
    }

    if (QFile::exists(get_program_appdata_sync_autotest_log_file_name())) {
        QFile::remove(get_program_appdata_sync_autotest_log_file_name());
    }

    return true;
};

static bool autotestON = false;

bool runSyncAutoTest(int test_level, QString current_sync_pwd, QString kring_pwd)
{
    if(!SyncAutoTest::checkEnvironment())
        {
            ASSERT(0, "Autotest environment test error");
            return false;
        }

    SyncAutoTest t(test_level, current_sync_pwd, kring_pwd);
    auto rv = t.run();
    if (rv) {
        autotestON = false;
        dbp::openStandardPath(sync_test_db_name);
    }
    return rv;
}

SyncAutoTest::SyncAutoTest(int test_level, QString current_sync_pwd, QString kring_pwd)
    :m_test_level(test_level), m_step(0), m_total(0), m_sync_pwd(current_sync_pwd), m_kring_pwd(kring_pwd)
{
    extern bool unlimitedLogFile;
    unlimitedLogFile = true;

    m_generate_prefix = "gen1";

    for(char c1 = 'A'; c1 != 'Z'; c1++)
        {
            for(char c2 = 'A'; c2 != 'Z'; c2++)
                {
                    QString s = QString(c1) + c2;
                    m_res_short_names.push_back(s);
                }
        }

#define ADD_SYM(S) m_topic_names.push_back(S);
    ADD_SYM("AAPL");ADD_SYM("IBM");ADD_SYM("MSFT");
    ADD_SYM("GOOG");ADD_SYM("CCJ");ADD_SYM("JPM");
    ADD_SYM("GE");ADD_SYM("QQQ");ADD_SYM("BAC");
    ADD_SYM("CFFI");ADD_SYM("MHO");ADD_SYM("QTWO");
#undef ADD_SYM

    m_curr_res_name_idx = m_curr_topic_name_idx = 0;
    autotestON = true;
};

SyncAutoTest::~SyncAutoTest() 
{
    autotestON = false;
};

bool ard::autotestMode()
{
    return autotestON;
};

QString SyncAutoTest::nextResourceShortName()
{   
    if(m_curr_res_name_idx >= m_res_short_names.size())
        {
            m_curr_res_name_idx = 0;
        }
    QString rv = m_res_short_names[m_curr_res_name_idx++];
    return rv;
};

QString SyncAutoTest::nextTopicName(QString prefix)
{
    if (m_curr_topic_name_idx >= m_topic_names.size())
    {
        m_curr_topic_name_idx = 0;
    }
    QString rv;
    if (prefix.isEmpty()) 
    {
        rv = QString("%1--%2").arg("topic").arg(m_topic_names[m_curr_topic_name_idx++]);
    }
    else 
    {
        rv = QString("%1--%2").arg(prefix).arg(m_topic_names[m_curr_topic_name_idx++]);
    }
    return rv;
};

bool SyncAutoTest::run()
{
    QDateTime dtAutoStartTime = QDateTime::currentDateTime();
    std::function<QString()> calc_spent_time = [&dtAutoStartTime]() {
        auto sec = dtAutoStartTime.msecsTo(QDateTime::currentDateTime()) / 1000;        
        int h = sec / 3600;
        sec -= h * 3600;
        int m = sec / 60;
        QString s = QString("%1h %2m").arg(h).arg(m);
        return s;
    };

#define PRINT_FOOTER_LINE logProgress(QString("AUTO-TEST FINISHED %1 (%2)").arg(m_test_level).arg(calc_spent_time()));

    m_step = 0;
    m_total = 22;
    if (m_test_level > 4)m_total += 3;

    /**
        one sync test step is actually 8 procedures combined

        1.open DB1
        2.apply operation F
        *3.sync against remote MASTER-DB
        4.open DB2
        5.apply operation F
        6.sync against remote
        7.open DB1
        8.sync against remote

        *sync against remote MASTER-DB means syncing against master DB in following manner
        localDb1 <-> master
        localDb2 <-> master

        the default path for master DB: C:\tmp\d2
    */

#define RUN_SYN_AFTER(F) {                                              \
    QDateTime t1 = QDateTime::currentDateTime();                        \
    if(gui::isDBAttached())dbp::close(false);                           \
    if(!openTestRepositoryOnDB(sync_test_db_name))return false;         \
    if(!F())return false;                                               \
    if(!syncLocal(#F))return false;                                     \
    if(gui::isDBAttached())dbp::close(false);                           \
    if(!openTestRepositoryOnDB(sync_test_db_name_2))return false;       \
    if(!F())return false;                                               \
    if(!syncLocal(#F))return false;                                      \
    updateContentHash();                                                \
    if(gui::isDBAttached())dbp::close(false);                           \
    if(!openTestRepositoryOnDB(sync_test_db_name))return false;         \
    if(!syncLocal(#F))return false;                                      \
    updateContentHash();                                                \
    printStepResult(++m_step, m_total, #F);                             \
    QString s = QString("[trun][%1]%2").arg(t1.msecsTo(QDateTime::currentDateTime())).arg(#F);\
    qWarning() << s; \
    }\
    switch2NewPassword();\

#define FINALIZE_AND_RETURN     PRINT_FOOTER_LINE;return true;

//  if(gui::isDBAttached())dbp::close(false);

    ///-------------------------
    //RUN_SYN_AFTER(clearOutline, "clean");
    //RUN_SYN_AFTER(generateProject, "gen-project");
    //RUN_SYN_AFTER(generatePResources, "gen-pres");
    //RUN_SYN_AFTER(generateAllocations, "gen-alloc");
    //return true;
    ///-------------------------

    //@todo:randomKill on pres

    RUN_SYN_AFTER(clearEverything);
	if (m_test_level == 0) {
		PRINT_FOOTER_LINE;
		return true;
	}

	///....
	//RUN_SYN_AFTER(generateTableBBoard);
	//return true;
	///....

    RUN_SYN_AFTER(generateTopics);
    RUN_SYN_AFTER(generateBookmarks);
	RUN_SYN_AFTER(generatePictures);
    RUN_SYN_AFTER(generateBBoard);
    RUN_SYN_AFTER(generateContacts);
    RUN_SYN_AFTER(importCsvContacts);
    RUN_SYN_AFTER(importCsvContacts);///import twice to check how if it can ignore duplicates
    RUN_SYN_AFTER(generateDrafts);
    RUN_SYN_AFTER(generateAnnotations);
    RUN_SYN_AFTER(generateTaskRingTodos);
    RUN_SYN_AFTER(randomizeColor);
    RUN_SYN_AFTER(randomizeTitle);
	RUN_SYN_AFTER(generateRules);

	if (m_test_level == 1) {
		FINALIZE_AND_RETURN;
	}
    ///
    /// export here, then random kill, then import/merge back
    ///
    //
    if (!backupRandomKillMerge())
        return false;
    
    RUN_SYN_AFTER(clearEverything);         
    //RUN_SYN_AFTER(generateKeys);
    //RUN_SYN_AFTER(randomDeleteKeys);
    if(m_test_level == 0)
        {
            PRINT_FOOTER_LINE;
            return true;
        }

    RUN_SYN_AFTER(generateBBoard);
    
    if(m_test_level == 1)
        {
            PRINT_FOOTER_LINE;
            return true;
        }

    RUN_SYN_AFTER(randomKill);



    m_generate_prefix = "gen2";
    RUN_SYN_AFTER(generateTopics);
    RUN_SYN_AFTER(generateDrafts);
    RUN_SYN_AFTER(randomizeAttributes);
    RUN_SYN_AFTER(randomizePOS);
    RUN_SYN_AFTER(randomizeRawData);

    //final sync step
    RUN_SYN_AFTER(verifyOutline);  
    PRINT_FOOTER_LINE;

#undef RUN_SYN_AFTER
    return true;
};

static void sync_autotest_log(QString s) 
{
    auto s_tmp = get_program_appdata_sync_autotest_log_file_name().toStdString();
    FILE* f = fopen(s_tmp.c_str(), "a+");
    if (f){
        static char msg[2096] = "";
        std::string s_tmp = s.toStdString();
        strncpy(msg, s_tmp.c_str(), sizeof(msg));
        fprintf(f, "%s\n", msg);
        fclose(f);
    }
};

void SyncAutoTest::logError(QString s)
{
    QString s2 = "[s-auto][error]" + s;
    sync_autotest_log(s);
    qWarning() << s2;
};

void SyncAutoTest::logProgress(QString s)
{
    QString s2 = "[s-auto]" + s;
    sync_autotest_log(s);
    qWarning() << s2;
};

bool SyncAutoTest::bingo()const 
{
    bool ok = (rand() % 100 + 1 < RAND_MODIFY_PERCENTAGES);
    return ok;
};

bool SyncAutoTest::bingo(topic_ptr f, int chance_percentages /*= -1*/)const
{
    if (f)
    {
        if (!f->canMove() || !f->canRename())
            return false;
    }


    bool ok = false;
    if (chance_percentages == -1)
    {
        ok = rand() % 100 + 1 < RAND_MODIFY_PERCENTAGES;
    }
    else
    {
        ok = rand() % 100 + 1 < chance_percentages;
    }
    return ok;
};

int SyncAutoTest::itemsBingo(snc::MemFindAllPipe& mp, 
    TOPICS_SET& items_bingo, bool allowAncestors, int NumOfTopics)
{
    srand (time(nullptr));
    auto r = ard::root();
    r->memFindItems(&mp);
    std::random_shuffle(mp.items().begin(), mp.items().end());
    for (auto& i : mp.items())
    {
        topic_ptr it = dynamic_cast<topic_ptr>(i);
        if (it)
        {
            bool do_it = bingo(it);
            if (do_it && !allowAncestors)
            {
                ///check if item is parent of any of the item inside set
                for (auto& topic_in_set : items_bingo) {
                    assert_return_0(topic_in_set != nullptr, "expected topic");
                        if (it->isAncestorOf(topic_in_set) ||
                            topic_in_set->isAncestorOf(it))
                        {
                            do_it = false;
                        }
                }
            }

            if (do_it)
            {
                items_bingo.insert(it);
            }

            if (NumOfTopics != -1) {
                if (static_cast<int>(items_bingo.size()) >= NumOfTopics) {
                    return items_bingo.size();
                }
            }
        }
    }   

    return mp.items().size();
};

bool SyncAutoTest::openTestRepositoryOnDB(QString test_db_name)
{
    // logProgress("..opening " + test_db_name);

    m_db_prefix = "";
    if(test_db_name.compare(sync_test_db_name) == 0)
        {
            m_db_prefix = "db1";
        }
    else
        {
            m_db_prefix = "db2";
        }

    QString dirPath = defaultRepositoryPath() + "dbs/" + test_db_name + "/";

    QDir d(dirPath);
    if(!d.exists(dirPath))
        {
            if(!dbp::create(test_db_name, "", ""))
                {
                    logError(QString("Failed to create database file '%1'. Possible  reason - local security policy.").arg(test_db_name));  
                    return false;
                }

            logProgress("created " + test_db_name);
        }
    else
        {
            if(!dbp::openStandardPath(test_db_name))
                {
                    logError(QString("Failed to open database file '%1'. Possible  reason - local security policy.").arg(test_db_name));
                    return false;
                }      

            //  logProgress("opened " + test_db_name);
        }

    return true;
};

/*bool SyncAutoTest::openTestRepository()
{
    bool ok = openTestRepositoryOnDB(sync_test_db_name);
    return ok;
};*/


bool SyncAutoTest::syncLocalAgainstMasterX(int masterDBindex, QString step_desc)
{
//    QString remote_db_path = remoteDBPath4LocalSync(/*masterDBindex*/);

    // logProgress(QString("..[%1]local sync against %2").arg(masterDBindex).arg(remote_db_path));

    QString hint = QString("%1/%2/%3").arg(m_step).arg(m_total).arg(step_desc);

    SYNC_AUX_COMMANDS sync_commands;
    SyncPoint::runLocalSync(true, sync_commands, hint);
    bool rv = false;
    auto res = SyncPoint::lastSyncResult();

    if (res.status == ard::aes_status::old_archive_key_tag) {
        auto tag = res.key_tag;
        ASSERT(0, "need implementation") << tag;
    }

    if(res.status == ard::aes_status::ok)
        {
            rv = true;
            auto r = ard::root();
            snc::MemFindAllPipe mp;
            r->memFindItems(&mp);
      
            logProgress(QString("sync - OK on %1 items").arg(mp.items().size()));
        }
    else
        {
            logProgress("sync - ERROR");
        }

    if(rv)
        {
            if(SyncPoint::isLastSyncDBIdenticalAfterSync())
                {
                    logProgress("sync - IDENTICAL (completed)");
                }
            else
                {
                    logProgress("sync - changes detected");
                    SyncPoint::runLocalSync(true, sync_commands, hint);

                    if(SyncPoint::isLastSyncDBIdenticalAfterSync())
                        {
                            logProgress("sync - IDENTICAL (completed on second path)");
                        }
                    else
                        {
                            logProgress("sync - changes detected, second path identity failed, running 3rd path");
                            SyncPoint::runLocalSync(true, sync_commands, hint);
                            if(SyncPoint::isLastSyncDBIdenticalAfterSync())
                                {
                                    logProgress("sync - IDENTICAL (completed on 3rd path)");
                                }
                            else
                                {
                                    logProgress("sync - changes detected, 3rd path identity failed, giving up");
                                    rv = true;
                                }
                            //rv = false;
                        }      
                }
        }

    if(rv){
        if(!verifyLocalCache())
            {
                logProgress("local cache verification failed, giving up");
                return false;
            }

        logProgress(QString("[%1] synchronized L=%2").arg(masterDBindex).arg(get_db_file_path()));
    }
    return rv;
};

bool SyncAutoTest::syncLocal(QString step_desc)
{
    /** code for SYNC against 2 masters - don't use for now, too slow
        if(!syncLocalAgainstMasterX(2, step_desc))
        return false;
        if(!syncLocalAgainstMasterX(3, step_desc))
        return false;
        if(!syncLocalAgainstMasterX(2, step_desc))
        return false;
    */
    if(!syncLocalAgainstMasterX(2, step_desc))
        return false;

    return true;
};

bool SyncAutoTest::verifyLocalCache()
{
    logProgress(QString("checking local cache of %1").arg(ard::db()->lookupMapSize()));

    auto r = ard::root();
    snc::MemFindAllPipe mp;
    r->memFindItems(&mp);
    for(auto& i : mp.items())    {
            topic_ptr it = dynamic_cast<topic_ptr>(i);
            assert_return_false(it, "expected topic");
            if (it->isPersistant()) {
                auto it2 = ard::lookup(it->id());
                if (!it2 || it2 != it){
                    ASSERT(0, "expected topic in cache") << it2;
                    return false;
                }
            }
        }
    logProgress(QString("local cache - OK [%1]").arg(ard::db()->lookupMapSize()));
    return true;
};

template<class T>
int SyncAutoTest::generateOutlineStructure(topic_ptr parent, QString prefix)
{
    /// total_nested_topics N * 8 * 4 * 2 * 2, where N = m_test_level + 1

    int top_branch_size = 8 * (m_test_level + 1);

    int ncount = 0;

    for(int i = 1; i < top_branch_size; i++)
        {
            QString s = QString("%1-%2%3")
                .arg(nextTopicName(prefix))
                .arg(dbx())
                .arg(gnx());
            topic_ptr f = new T(s);
            parent->addItem(f);ncount++;

            for(int k = 0; k < 4; k++)
                {
                    QString s = QString("%1-%2%3")
                        .arg(nextTopicName(prefix))
                        .arg(dbx())
                        .arg(gnx());
                    topic_ptr f2 = new T(s);
                    f->addItem(f2);ncount++;
      
                    for(int j = 0; j < 2; j++)
                        {
                            QString s = QString("%1-%2%3")
                                .arg(nextTopicName(prefix))
                                .arg(dbx())
                                .arg(gnx());
                            topic_ptr f3 = new ard::topic(s);
                            f2->addItem(f3);ncount++;

                            for(int k = 0; k < 2; k++)
                                {
                                    QString s = QString("%1-%2%3")
                                        .arg(nextTopicName(prefix))
                                        .arg(dbx())
                                        .arg(gnx());
                                    topic_ptr f4 = new T(s);
									if (f4->canAttachNote()) {
										f4->setMainNoteText(QString("Note to %1 with data(%2-%3-%4)").arg(s).arg(i).arg(j).arg(k));
									}
                                    f3->addItem(f4);ncount++;
                                }
                        }//level3-j
                }//level2-k
        }//top-i

    return ncount;
};

bool SyncAutoTest::generateTopics() 
{
    logProgress("..generating tree ");

    int ncount = 0;

    ncount += generateOutlineStructure<ard::topic>(ard::Sortbox(), "alpha");
    ncount += generateOutlineStructure<ard::topic>(ard::Maybe(), "beta");
    ncount += generateOutlineStructure<ard::topic>(ard::Reference(), "gamma");
    ncount += generateOutlineStructure<ard::topic>(ard::Trash(), "delta");

    //topic_ptr b = new ard::topic(working_branch_name);
    //inbox->addItem(b); ncount++;

    ///proceeed on custom folders
    auto cr = ard::CustomSortersRoot();
    ///create some custom folders first
    for (int i = 0; i < m_test_level; i++)
    {
        QString s = QString("%1-%2%3")
            .arg("custom-folder" + nextTopicName())
            .arg(dbx())
            .arg(gnx());

        auto f = cr->findTopicByTitle(s);
        if (!f) {
            f = ard::db()->createCustomFolderByName(s);
            ASSERT(f, "failed to create custom folder: ") << s;
        }
    }

    if (!cr->items().empty())
    {
        for (auto& i : cr->items()) {
            topic_ptr f = dynamic_cast<topic_ptr>(i);
            ncount += generateOutlineStructure<ard::topic>(f, "vega");
        }
    }

    bool rv = ard::root()->ensurePersistant(-1);
    if (rv)
    {
        logProgress(QString("generated tree of %1 items").arg(ncount));
    }
    else
    {
        logError("failed to ensure outline persistance.");
    }

    return rv;
};

std::pair<int, int> generate_demo_contacts() 
{
    std::pair<int, int> rv{0,0};

    auto cr = ard::db()->cmodel()->croot();
    auto cgr = ard::db()->cmodel()->groot();
    std::vector<QString> fnames, lnames;

#define ADD_FN(N) fnames.push_back(N);
    ADD_FN("Adam");
    ADD_FN("Bob");
    ADD_FN("Leonard");
    ADD_FN("Billy");
    ADD_FN("Andrew");
    ADD_FN("Laura");
    ADD_FN("John");
    ADD_FN("Nick");
    ADD_FN("Gregor");
#undef ADD_FN

#define ADD_LN(N) lnames.push_back(N);
    ADD_LN("Smith");
    ADD_LN("Dylan");
    ADD_LN("Cohen");
    ADD_LN("Joel");
    ADD_LN("Lincoln");
    ADD_LN("Cohen");
    ADD_LN("Bromberg");
    ADD_LN("Dorie");
    ADD_LN("Wilton");
#undef ADD_LN

    STRING_LIST added_groups;
    ard::contact_group* g = nullptr;

#define ADD_G(N) g = cgr->addGroup(N);\
    if (g) {\
        added_groups.push_back(g->syid());\
        qDebug() << "added-group" << g->title() << g->syid();\
    }\

    ADD_G("Family");
    ADD_G("Friends");
    ADD_G("Work");
    ADD_G("Photography");

    rv.first = added_groups.size();

#undef ADD_G

    std::function<void(QString, QString, STRING_LIST*)> generate_contact = [cr, &rv](QString fname, QString lname, STRING_LIST* gr)
    {
        auto c = cr->addContact(fname, lname, gr);
        if (c)
        {
            FieldParts fp_email;
            fp_email.add(FieldParts::Email, QString("%1.%2.%3@gmail.com").arg(fname).arg(lname).arg(rv.second));
            c->setFieldValues(EColumnType::ContactEmail, "main", fp_email);

            FieldParts fp_phone;
            fp_phone.add(FieldParts::Phone, QString("1-%1-123").arg(rv.second));
            c->setFieldValues(EColumnType::ContactPhone, "cell", fp_phone);

            FieldParts fp_addr;
            fp_addr.add(FieldParts::AddrStreet, QString("main str %1").arg(rv.second));
            fp_addr.add(FieldParts::AddrCity, QString("Las Vegas"));
            fp_addr.add(FieldParts::AddrRegion, QString("Nevada"));
            fp_addr.add(FieldParts::AddrZip, QString("1121%1").arg(rv.second));
            fp_addr.add(FieldParts::AddrCountry, QString("USA"));
            c->setFieldValues(EColumnType::ContactAddress, "home", fp_addr);

            rv.second++;
        }
    };

    auto it_fname = fnames.begin();
    auto it_lname = lnames.begin();

    std::function<void(int, STRING_LIST*)> generate_n_contact = [&it_fname, &it_lname, &fnames, &lnames, &rv, generate_contact](int count, STRING_LIST* gr)
    {
        int created = 0;
        for (; it_fname != fnames.end(); it_fname++)
        {
            for (; it_lname != lnames.end(); it_lname++)
            {
                generate_contact(*it_fname, *it_lname, gr);
                if (created++ > count)
                    return;
            }
            it_lname = lnames.begin();
        }
    };

    for (auto& g : added_groups) 
    {
        STRING_LIST part_g;
        part_g.push_back(g);
        generate_n_contact(10, &part_g);        
    }   

    generate_n_contact(20, &added_groups);

    cgr->ensurePersistant(-1);
    bool res = cr->ensurePersistant(-1);
    if (res)
    {
        return rv;
    }
    
    rv.first = rv.second = 0;
    return rv;
}

bool SyncAutoTest::generateContacts() 
{
    auto res = generate_demo_contacts();
    if (res.second > 0) {
        logProgress(QString("generated %1 contacts in %2 groups").arg(res.second).arg(res.first));
        return true;
    }

    logError("failed to generate contacts.");
    return true;
    /*
    assert_return_false(ard::isDbConnected(), "expected open DB");
    auto cr = ard::db()->cmodel()->croot();
    auto cgr = ard::db()->cmodel()->groot();
    std::vector<QString> added_groups;

    int groups2generate = (m_test_level + 1) * 4;
    for (int i = 0; i < groups2generate; i++) {
        auto name = QString("Gr-%1-%2").arg(dbx()).arg(i);
        auto g = cgr->addGroup(name);
        if (g) {
            added_groups.push_back(g->syid());
        }
    }

    int idx = 0;
    int contacts2generate = (m_test_level + 1) * 10;
    for (int i = 0; i < contacts2generate; i++) {
        auto firstName = QString("FName%1_%2").arg(dbx()).arg(i);
        auto lastName = QString("LName%1_%2").arg(dbx()).arg(i);
        auto c = cr->addContact(firstName, lastName, &added_groups);
        if (c) {
            FieldParts fp_email;
            fp_email.add(FieldParts::Email, QString("c%1@yahoo.com").arg(i));
            c->setFieldValues(EColumnType::ContactEmail, "main", fp_email);

            FieldParts fp_phone;
            fp_phone.add(FieldParts::Phone, QString("1-%1-123").arg(i));
            c->setFieldValues(EColumnType::ContactPhone, "cell", fp_phone);

            FieldParts fp_addr;
            fp_addr.add(FieldParts::AddrStreet, QString("main str %1").arg(i));
            fp_addr.add(FieldParts::AddrCity, QString("Las Vegas"));
            fp_addr.add(FieldParts::AddrRegion, QString("Nevada"));
            fp_addr.add(FieldParts::AddrZip, QString("1121%1").arg(i));
            fp_addr.add(FieldParts::AddrCountry, QString("USA"));
            c->setFieldValues(EColumnType::ContactAddress, "home", fp_addr);
        }
        //cm->addContact(firstName, lastName, &added_groups);
        idx++;
    }

    cgr->ensurePersistant(-1);
    bool rv = cr->ensurePersistant(-1);
    if (rv)
    {
        logProgress(QString("generated %1 contacts in %2 groups").arg(idx).arg(added_groups.size()));
    }
    else
    {
        logError("failed to ensure outline persistance.");
    }

    return rv;
    */
};

bool SyncAutoTest::importCsvContacts() 
{
    QDir cntDir(AUTOTEST_CONTACTS_IMPORT_PATH);
    if (!cntDir.exists())
    {
        logProgress(QString("Contacts directory not found: %1 Skip images import").arg(AUTOTEST_CONTACTS_IMPORT_PATH));
        return true;
    }

    QStringList namesFilter;
    namesFilter.push_back("*.csv");

    QStringList slist = cntDir.entryList(namesFilter, QDir::Files);
    if (slist.isEmpty())
    {
        logProgress(QString("Contacts directory empty: %1 Skip contacts import").arg(AUTOTEST_CONTACTS_IMPORT_PATH));
        return true;
    }

    QStringList slist2;
    for (auto& s : slist) {
        slist2.push_back(AUTOTEST_CONTACTS_IMPORT_PATH + "/" + s);
    }

    auto r = ard::ArdiCsv::importContactsCsvFiles(slist2, false);
    QString msg(QString("Imported %1 contact(s).").arg(r.first));
    if (r.second > 0) {
        msg += QString("Ignored %1 duplicate").arg(r.second);
    }

    logProgress(msg);

    return true;
};

bool SyncAutoTest::generateKeys() 
{
	return true;

    assert_return_false(ard::isDbConnected(), "expected open DB");

    if (m_kring_pwd.isEmpty()) {
        logError("KRing password is not defined. Key generation aborted.");
        return false;
    }

    auto kr = ard::db()->kmodel()->keys_root();
    if (!kr->decryptKRing(m_kring_pwd)) {
        logError("KRing unlock failed.");
        return false;
    }   

    int idx = 0;
    int contacts2generate = (m_test_level + 1) * 10;
    for (int i = 0; i < contacts2generate; i++) {
        auto title  = QString("title%1_%2").arg(dbx()).arg(i);
        auto login  = QString("login%1_%2").arg(dbx()).arg(i);
        auto pwd    = QString("pwd%1_%2").arg(dbx()).arg(i);
        auto link   = QString("link%1_%2").arg(dbx()).arg(i);
        auto note   = QString("note%1_%2").arg(dbx()).arg(i);

        auto new_k = kr->addKey(QString("new key %1").arg(i));
        if (new_k) {
            new_k->setKeyData(title, login, pwd, link, note);
        }
    }

    bool rv = kr->ensurePersistant(-1);
    if (rv)
    {
        logProgress(QString("generated %1 keys").arg(idx));
    }
    else
    {
        logError("failed to ensure outline persistance.");
    }

    return rv;
};

bool SyncAutoTest::generateDrafts() 
{
    int ncount = 0;
    auto inbox = ard::Sortbox();
    assert_return_false(inbox, "expected 'inbox'");
    int branch_size = m_test_level * 4;
    QString prefix = "draft";
    for (int i = 1; i < branch_size; i++)
    {
        QString s = QString("%1-%2%3")
            .arg(nextTopicName(prefix))
            .arg(dbx())
            .arg(gnx());
        auto f = new ard::email_draft(s);
        auto e = f->draftExt();
        if (e) {
            ///I do hope we are not going to send out all those emails
			ard::email_draft_ext::attachement_file_list attachements = { {"c:\\1.txt", ""}, {"c:\\2.txt", ""}, {"c:\\3.txt", ""} };
            e->set_draft_data("me@yahoo.com", "me@gmail.com", "me@google.com", &attachements);
        }
        inbox->addItem(f); 
        ncount++;
    }
    bool rv = ard::root()->ensurePersistant(-1);
    if (rv)
    {
        logProgress(QString("generated tree of %1 items").arg(ncount));
    }
    else
    {
        logError("failed to ensure outline persistance.");
    }

    return rv;
};

bool SyncAutoTest::generateTableBBoard() 
{
    logProgress("..generating table-bboards ");
    int generated_board_count = 0;
    int generated_bitem_count = 0;

    auto broot = ard::db()->boards_model()->boards_root();
    assert_return_false(broot, "expected board root");

    for (int i = 0; i < (m_test_level + 1) * 5; i++)
    {
        int gcount = 0;
        QString s_title = QString("table-on-board-%1-%2-%3")
            .arg(dbx())
            .arg(gnx())
            .arg(i);

        auto b = broot->addBoard();
        if (b) {
            b->setTitle(s_title);
            auto numOfitems2insert = (m_test_level + 1) * 30;
            TOPICS_LIST tlst;
            TOPICS_SET items2insert;
            snc::MemFindAllPipe mp;
            generated_bitem_count += itemsBingo(mp, items2insert, true, numOfitems2insert);

            ard::BITEMS all_new_bitems;

            int band_index = 0;
            int items_per_column = 10;
            int idx = items_per_column;
            for (auto i : items2insert) {
                idx--;
                if (idx == 0) {
                    idx = items_per_column;
                    auto bitems_lst = b->insertTopicsBList(tlst, band_index, 0, 20, ard::BoardItemShape::text_normal);
                    for (auto i : bitems_lst.first) {
                        if (bingo()) {
                            for (int j = 0; j < (m_test_level + 1); j++) {
                                ///add id links
                                auto lnk_lst = b->addBoardLink(i, i);
                                if (lnk_lst) {
                                    auto lnk = lnk_lst->getAt(lnk_lst->size() - 1);
									auto link_label = QString("%1-%2").arg(j + 1).arg(dbx());
                                    lnk->setLinkLabel(link_label, b->syncDb());
									//qDebug() << "[auto-link]" << dbx() << link_label;
                                }
                            }
                        }
                    }
                    all_new_bitems.insert(all_new_bitems.begin(), bitems_lst.first.begin(), bitems_lst.first.end());
                    tlst.clear();
                    band_index++;
                }
                tlst.push_back(i);
            }

            /// add more link to custom topics ///
            TOPICS_LIST new_origins;
            auto h = b->ensureOutlineTopicsHolder();
            if (h) {
                for (int i = 0; i < 5; i++) {
                    auto f = new ard::topic(QString("O-%1").arg(i + 1));
                    h->addItem(f);
                    new_origins.push_back(f);
                }
                h->ensurePersistant(-1);
            }

            auto blst_new_origins = b->insertTopicsBList(new_origins, band_index, 0, 20, ard::BoardItemShape::box);
            if (!blst_new_origins.first.empty()) {
                std::random_shuffle(all_new_bitems.begin(), all_new_bitems.end());
                int bcount = 5;
                for (auto i : all_new_bitems) {
                    for (auto j : blst_new_origins.first) {
                        if (bingo()) {
                            auto lnk_lst = b->addBoardLink(j, i);
                            if (lnk_lst) {
                                auto lnk = lnk_lst->getAt(lnk_lst->size() - 1);
                                lnk->setLinkLabel(QString("to %1").arg(j->refTopic()->title()), b->syncDb());
                            }
                        }
                    }

                    bcount--;
                    if (!bcount)
                        break;
                }

                /// id-links around origins
                for (auto i : blst_new_origins.first) {
                    if (bingo()) {
                        for (int j = 0; j < m_test_level + 3; j++) {
                            auto lnk_lst = b->addBoardLink(i, i);
                            if (lnk_lst) {
                                auto lnk = lnk_lst->getAt(lnk_lst->size() - 1);
                                lnk->setLinkLabel(QString("%1").arg(j + 1), b->syncDb());
                            }
                        }
                    }
                }
            }
        }

        generated_bitem_count += gcount;
        generated_board_count++;
    }

    bool rv = broot->ensurePersistant(-1);
    if (rv)
    {
        logProgress(QString("generated %1 table-bboards with total %2 tasks").arg(generated_board_count).arg(generated_bitem_count));
    }
    else
    {
        logError("failed to ensure outline persistance.");
    }

    return true;
};

bool SyncAutoTest::generateRules() 
{
	logProgress("..generating rules ");
	auto rr = ard::db()->rmodel()->rroot();
	assert_return_false(rr, "expected rules root");
	int generated_rules_count = 0;
	{
		auto r = rr->addRule(QString("home-sale-delivery-[%1]").arg(dbx()));
		ORD_STRING_SET from;
		ORD_STRING_SET subject;
		subject.insert("home");
		subject.insert("sale");
		subject.insert("delivery");
		r->rext()->setupRule(subject, "", from, false);
		generated_rules_count++;
	}
	{
		auto r = rr->addRule(QString("one-[%1]").arg(dbx()));
		ORD_STRING_SET from;
		ORD_STRING_SET subject;
		r->rext()->setupRule(subject, "one", from, false);
		generated_rules_count++;
	}
	{
		auto r = rr->addRule(QString("two-[%1]").arg(dbx()));
		ORD_STRING_SET from;
		ORD_STRING_SET subject;
		r->rext()->setupRule(subject, "two", from, false);
		generated_rules_count++;
	}
	{
		auto r = rr->addRule(QString("three-[%1]").arg(dbx()));
		ORD_STRING_SET from;
		ORD_STRING_SET subject;
		r->rext()->setupRule(subject, "three", from, false);
		generated_rules_count++;
	}

	bool rv = rr->ensurePersistant(-1);
	if (rv)
	{
		logProgress(QString("generated [%1] rules").arg(generated_rules_count));
	}
	else
	{
		logError("failed to ensure outline persistance.");
	}

	return true;
};

bool SyncAutoTest::generateGreekBBoard() 
{
    logProgress("..generating table-bboards ");
    int generated_board_count = 0;
    int generated_bitem_count = 0;

    auto broot = ard::db()->boards_model()->boards_root();
    assert_return_false(broot, "expected board root");

    for (int i = 0; i < (m_test_level + 1) * 5; i++)
    {
        QString s_title = QString("Greeks-on-board-%1-%2-%3")
            .arg(dbx())
            .arg(gnx())
            .arg(i);

        auto b = broot->addBoard();
        if (b) {
            b->setTitle(s_title);

            auto h = b->ensureOutlineTopicsHolder();
            if (h) {
                std::function<void(OutlineSample, ard::InsertBranchType it, int band_idx, ard::BoardItemShape)> add_outline =
                    [&](OutlineSample smp, ard::InsertBranchType it, int band_idx, ard::BoardItemShape shp)
                {
                    auto f = ard::buildOutlineSample(smp, h);
                    f->ensurePersistant(-1);
                    TOPICS_LIST lst;
                    lst.push_back(f);
                    b->insertTopicsWithBranches(it, lst, band_idx, -1, 300, shp);
                };


                add_outline(OutlineSample::GreekAlphabet, ard::InsertBranchType::branch_expanded_from_center, 1, ard::BoardItemShape::text_normal);
                add_outline(OutlineSample::OS, ard::InsertBranchType::branch_expanded_to_right, 2, ard::BoardItemShape::box);
                add_outline(OutlineSample::Programming, ard::InsertBranchType::branch_expanded_to_right, 4, ard::BoardItemShape::text_normal);
            }

        }
    }

    bool rv = broot->ensurePersistant(-1);
    if (rv)
    {
        logProgress(QString("generated %1 greeks-bboards with total %2 tasks").arg(generated_board_count).arg(generated_bitem_count));
    }
    else
    {
        logError("failed to ensure outline persistance.");
    }

    return true;
};

bool SyncAutoTest::generateBBoard() 
{
    if (!generateTableBBoard())
        return false;
    if (!generateGreekBBoard())
        return false;

    return true;
};

bool SyncAutoTest::generateBookmarks()
{
    logProgress("..generating bookmarks ");
    snc::MemFindAllPipe mp;
    TOPICS_SET parents4bkmrk;
    itemsBingo(mp, parents4bkmrk, true, (m_test_level + 1) * 100);
    auto r = ard::root();

    std::vector<std::pair<QString, QString>> urls_template;
    urls_template.push_back({ "Yahoo", "yahoo.com" });
    urls_template.push_back({ "Google", "google.com" });
    urls_template.push_back({ "MSN", "msn.com" });
    urls_template.push_back({ "WSJ", "wsj.com" });
    urls_template.push_back({ "bars", "bars.com" });
    size_t tmpl_idx = 0;

    for (auto i : parents4bkmrk) 
    {
        const auto& t = urls_template[tmpl_idx++];
        auto b = ard::anUrl::createUrl(t.first, t.second);
        i->addItem(b);
        if (tmpl_idx == urls_template.size())tmpl_idx = 0;
    }

    auto sb = ard::Sortbox();
    if (sb) 
    {
        for (int i = 0; i < (m_test_level + 1) * 20; i++)
        {
            const auto& t = urls_template[tmpl_idx++];
            auto b = ard::anUrl::createUrl(t.first, t.second);
            sb->addItem(b);
            if (tmpl_idx == urls_template.size())tmpl_idx = 0;
        }
    }

    r->ensurePersistant(-1);
    return true;
};

void generateNpictures(int N, QString prefix)
{
	auto r = ard::root();

	std::vector<QImage> picture_samples;
	for (int i = 0; i < 10; i++)
	{
		QImage image(QSize(400, 300), QImage::Format_RGB32);
		QPainter p(&image);
		p.setBrush(QBrush(Qt::green));
		p.fillRect(QRectF(0, 0, 400, 300), qRgb(127-i,127-i,127-i));
		p.fillRect(QRectF(100, 100, 200,100), qRgb(255, 127, 39 + 2 * i));
		p.setPen(QPen(Qt::black));
		p.setFont(*ard::defaultBoldFont());
		p.drawText(QRect(100, 100, 200, 100), Qt::AlignCenter | Qt::AlignVCenter, QString("image #%1").arg(i));
		picture_samples.push_back(image);
	}

	size_t tmpl_idx = 0;

	auto sb = ard::Sortbox();
	if (sb)
	{
		for (int i = 0; i < N; i++)
		{
			auto pic = new ard::picture(QString("auto-generated-%1-in-%2").arg(i).arg(prefix));
			sb->addItem(pic);
			sb->ensurePersistant(1);
			pic->setFromImage(picture_samples[tmpl_idx++]);
			if (tmpl_idx == picture_samples.size())tmpl_idx = 0;
		}
	}

	r->ensurePersistant(-1);
};

bool SyncAutoTest::generatePictures()
{
	logProgress("..generating pictures ");

	int N = (m_test_level + 1) * 20;
	generateNpictures(N, dbx());
	return true;
}

bool SyncAutoTest::clearEverything()
{
    bool rv = true;
    logProgress("..cleaning outline ");  
    bool cleanItems = true;
    if(cleanItems)
        {
            RootTopic* r = dbp::root();
            r->killAllItemsSilently();
            bool rv = r->ensurePersistant(-1);
            if(rv)
                {
                    logProgress(QString("cleaned tree, root size: %1").arg(r->items().size()));
                }
            else
                {
                    logError("failed to ensure outline persistance.");      
                }
        }


    bool cleanThread = true;
    if (cleanThread)
    {
        auto tr = ard::db()->threads_root();
        tr->killAllItemsSilently();
        rv = tr->ensurePersistant(-1);
        if (rv)
        {
            logProgress(QString("cleaned threads,  size: %1").arg(tr->items().size()));
        }
        else
        {
            logError("failed to ensure resources persistance.");
        }
    }

    auto cm = ard::db()->cmodel();
    if (cm) {
        auto cr = cm->croot();
        cr->killAllItemsSilently();
        cr->ensurePersistant(-1);

        auto gr = cm->groot();
        gr->killAllItemsSilently();
        gr->ensurePersistant(-1);
        logProgress("cleaned contacts");
    }

    auto bm = ard::db()->boards_model();
    if (bm) {
        auto r = bm->boards_root();
        r->killAllItemsSilently();
        r->ensurePersistant(-1);
        logProgress("cleaned boards");
    }

    return rv;
};

bool SyncAutoTest::verifyOutline()
{
    return true;
};


bool SyncAutoTest::randomizeTitle()
{
    logProgress(QString("..randomizing title with %1 percentages").arg(RAND_MODIFY_PERCENTAGES));

    if (true)
    {
        auto r = ard::root();
        if (!randomizeTitle(r)) {
            return false;
        };
    }

    return true;
};

bool SyncAutoTest::randomizeRawData() 
{
    logProgress(QString("..introducing identity errors %1 percentages").arg(RAND_MODIFY_PERCENTAGES));

    if (true)
    {
        auto r = ard::root();
        if (!randomizeRawData(r)) {
            return false;
        };
    }

    return true;
};

bool SyncAutoTest::randomizeColor()
{
    logProgress(QString("..randomizing color with %1 percentages").arg(RAND_MODIFY_PERCENTAGES));

    auto r = ard::root();
    if (!randomizeColor(r)) {
        return false;
    };
    return true;
};

bool SyncAutoTest::randomizeTitle(snc::cit* r)
{
    srand(time(nullptr));
    int ncount = 0;
    snc::MemFindAllPipe mp;
    r->memFindItems(&mp);
    for (auto& i : mp.items()) {
        topic_ptr it = dynamic_cast<topic_ptr>(i);
        if (it && it->canRenameToAnything())
        {
            bool modify_it = bingo(it);
            if (modify_it)
            {
                QString s_title = it->title();
                s_title += QString("[rand-mod%1]").arg(RAND_MODIFY_PERCENTAGES);
                it->setTitle(s_title);
                ncount++;
            }
        }
    }

    bool rv = r->ensurePersistant(-1);
    if (rv)
    {
        logProgress(QString("randomized title('%1') in %2 items out of %3 total").arg(r->title()).arg(ncount).arg(mp.items().size()));
    }
    else
    {
        logError("failed to ensure outline persistance.");
    }

    return true;
};

bool SyncAutoTest::randomizeRawData(snc::cit* r) 
{
    srand(time(nullptr));
    int ncount = 0;
    snc::MemFindAllPipe mp;
    r->memFindItems(&mp);
    for (auto& i : mp.items()) {
        topic_ptr it = dynamic_cast<topic_ptr>(i);
        if (it && it->canRenameToAnything())
        {
            bool modify_it = bingo(it);
            if (modify_it)
            {
                QString s_title = it->title();
                s_title += QString("[raw%1]").arg(RAND_MODIFY_PERCENTAGES);
                auto p = it->getToDoDonePercent();
                p += 1;
                QString sql = QString("update ard_tree set todo_done=%1, title=? where oid=%2")
                    .arg(p)
                    .arg(it->id());
                auto q = ard::db()->prepareQuery(sql);
                if (!q) {
                    ASSERT(0, "failed to prepare SQL") << sql;
                    return false;
                }
                q->addBindValue(s_title);
                if (!q->exec()){
                    ASSERT(0, "failed to run SQL") << sql << q->lastError().text();
                    return false;
                }
                ncount++;
            }
        }
    }

    bool rv = r->ensurePersistant(-1);
    if (rv)
    {
        logProgress(QString("raw modified('%1') in %2 items out of %3 total").arg(r->title()).arg(ncount).arg(mp.items().size()));
    }
    else
    {
        logError("failed to ensure outline persistance.");
    }

    return true;
};

bool SyncAutoTest::randomizeColor(snc::cit* r)
{
	std::set<ard::EColor> colors;
	colors.insert(ard::EColor::purple);
	colors.insert(ard::EColor::red);
	colors.insert(ard::EColor::green);
	colors.insert(ard::EColor::blue);
    auto it_color = colors.begin();

    srand(time(nullptr));
    int ncount = 0;
    snc::MemFindAllPipe mp;
    r->memFindItems(&mp);
    for (auto& i : mp.items()) {
        topic_ptr it = dynamic_cast<topic_ptr>(i);
        if (it && !it->IsUtilityFolder())
        {
            bool modify_it = bingo(it);
            if (modify_it)
            {
                if (it_color == colors.end()) {
                    it_color = colors.begin();
                }
                auto cl_idx = *it_color;
                it->setColorIndex(cl_idx);
                it_color++;
                ncount++;
            }
        }
    }

    return true;
};

bool SyncAutoTest::randomizeAttributes()
{
    logProgress(QString("..randomizing attributes with %1 percentages").arg(RAND_MODIFY_PERCENTAGES));

    auto r = ard::root();
    if(!randomizeOutlineAttributes(r))
        {
            return false;
        };

    return true;
};

bool SyncAutoTest::generateAnnotations() 
{
    logProgress(QString("..annotating with %1 percentages").arg(RAND_MODIFY_PERCENTAGES));

    auto r = ard::root();
    if (!randomGenerateAnnotations(r))
    {
        return false;
    };  
    return true;
};

bool SyncAutoTest::generateTaskRingTodos() 
{
    logProgress(QString("..todo in task ring with %1 percentages").arg(RAND_MODIFY_PERCENTAGES));

    auto r = ard::root();
    if (!randomTaskRingTodos(r))
    {
        return false;
    };
    return true;
};

bool SyncAutoTest::randomizeOutlineAttributes(snc::cit* r)
{
    srand (time(nullptr));
    int hspot_count = 0;
    int retired_count = 0;
    int todo_count = 0;
    snc::MemFindAllPipe mp;
    r->memFindItems(&mp);
    for(auto& i : mp.items())    {
            topic_ptr it = dynamic_cast<topic_ptr>(i);
            assert_return_false(it, "expected topic");
            bool modify_it = bingo(it);
            /*if(modify_it)
                {
                    it->setStarred(!it->isStarr);
                    hspot_count++;
                }

            modify_it = bingo(it);*/
            if(modify_it)
                {
                    it->setRetired(true);
                    retired_count++;
                }

            modify_it = bingo(it);
            if(modify_it)
                {                    
                    it->setToDo(1, ToDoPriority::normal);                    
                    todo_count++;
                }
        }

    bool rv = r->ensurePersistant(-1);
    if(rv)
        {
            logProgress(QString("randomized Attributes, hspot:%1, retired:%2, todo:%3 out of %4 total").arg(hspot_count).arg(retired_count).arg(todo_count).arg(mp.items().size()));
        }
    else
        {
            logError("failed to ensure outline persistance.");      
        }

    return true;
};

//...
bool SyncAutoTest::randomGenerateAnnotations(snc::cit* r) 
{
    srand(time(nullptr));
    int annotated_count = 0;
    snc::MemFindAllPipe mp;
    r->memFindItems(&mp);
    for (auto& i : mp.items()) {
        topic_ptr it = dynamic_cast<topic_ptr>(i);
        assert_return_false(it, "expected topic");
        bool modify_it = bingo(it);
        if (modify_it)
        {
            if (!it->canHaveAnnotation())
                continue;

            QString s = QString("[annotated%1/%2]").arg(RAND_MODIFY_PERCENTAGES).arg(annotated_count);
            it->setAnnotation(s);
            annotated_count++;
        }
    }

    bool rv = r->ensurePersistant(-1);
    if (rv)
    {
        logProgress(QString("annotated %1 out of %2 total").arg(annotated_count).arg(mp.items().size()));
    }
    else
    {
        logError("failed to ensure outline persistance.");
    }

    return true;
};

//...
bool SyncAutoTest::randomTaskRingTodos(snc::cit* r) 
{
    srand(time(nullptr));
    int changed_count = 0;
    snc::MemFindAllPipe mp;
    r->memFindItems(&mp);
    for (auto& i : mp.items()) {
        topic_ptr it = dynamic_cast<topic_ptr>(i);
        assert_return_false(it, "expected topic");
        bool modify_it = bingo(it);
        if (modify_it)
        {
            if (!it->canChangeToDo())
                continue;

            it->setToDo(17, ToDoPriority::normal);
            changed_count++;
        }
    }

    bool rv = r->ensurePersistant(-1);
    if (rv)
    {
        logProgress(QString("todos in task ring %1 out of %2 total").arg(changed_count).arg(mp.items().size()));
    }
    else
    {
        logError("failed to ensure outline persistance.");
    }

    return true;
};

bool SyncAutoTest::randomizePOS()
{
    logProgress(QString("..randomizing POS with %1 percentages").arg(RAND_MODIFY_PERCENTAGES));

    if(!randomizeItemsInOutlinePOS())
        return false;
    return true;
};

bool SyncAutoTest::randomizeItemsInOutlinePOS()
{
    auto r = ard::root();
    TOPICS_SET items2move;
    snc::MemFindAllPipe mp;
    int ncount = 0, total_items = itemsBingo(mp, items2move);

    while(!items2move.empty())
        {
            TOPICS_SET::iterator k = items2move.begin();
            topic_ptr it4move = *k;

            if(!it4move->canMove())
                {
                    items2move.erase(k);
                    continue;
                }
            bool moved = false;
            for(int l = 0; l < 10 && !moved; l++)
                {
                    for(auto& i : mp.items())    {
                            topic_ptr it = dynamic_cast<topic_ptr>(i);
                            assert_return_false(it, "expected topic");
                            bool take_it = bingo(it);
                            //bool child_has_prj1 = (it4move->prjFindProject() != nullptr);
                            //if (child_has_prj1) {
                            //    take_it = false;
                           // }

                            if(it != it4move && 
                               take_it && 
                               it->canAcceptChild(it4move) &&
                                it4move->canBeMemberOf(it))
                                {
                                    topic_ptr old_parent = dynamic_cast<topic_ptr>(it4move->cit_parent());
                                    assert_return_false(old_parent, "old_parent");
                                    old_parent->detachItem(it4move);

                                    int pos = 0;
                                    if(it->items().size() > 0)
                                        {
                                            pos = rand() % it->items().size();
                                        }
                                    if(pos > (int)it->items().size())
                                        pos = (int)it->items().size();
                                    it->insertItem(it4move, pos);
                                    if (k == items2move.end()) {//<-- crashing as k invalid
                                        continue;//ykh ?
                                    }
                                    items2move.erase(k);
                                    moved = true;

                                    if(it4move->canRename())
                                        {
                                            QString s_title = it4move->title();
                                            s_title += QString("[v%1]").arg(RAND_MODIFY_PERCENTAGES);
                                            it4move->setTitle(s_title);
                                        }
                                    ncount++;

                                }      
                            if (moved)
                                break;
                        }//for iterator
                }//for chances loop
      
            if(!moved)
                {
                    items2move.erase(k);
                    //logProgress(QString("rnd-move-skipped: %1").arg(it4move->dbgHint()));
                }
        }

    bool rv = r->ensurePersistant(-1);
    if(rv)
        {
            logProgress(QString("randomized POS of %1 items out of %2 total").arg(ncount).arg(total_items));
        }
    else
        {
            logError("failed to ensure outline persistance.");      
        }
    return rv;
};

/**
    1.export
    2.random kill
    3.Import
*/
bool SyncAutoTest::backupRandomKillMerge()
{
    if (!gui::isDBAttached()) {
        if (!openTestRepositoryOnDB(sync_test_db_name))return false;
    }
    if(!backupRandomKillMergeCurrentAutotestDB())return false;
    if (gui::isDBAttached())dbp::close(false);

    if (!gui::isDBAttached()) {
        if (!openTestRepositoryOnDB(sync_test_db_name_2))return false;
    }
    if (!backupRandomKillMergeCurrentAutotestDB())return false;
    if (gui::isDBAttached())dbp::close(false);
    return true;
};

bool SyncAutoTest::backupRandomKillMergeCurrentAutotestDB()
{
    ///1.backup
    auto exportedDBName = backupCurrentIntoTmpAutoTest();
    if (exportedDBName.isEmpty())
        return false;

    ///2.kill
    if (!randomKill())
        return false;

    ///3.merge back
    auto res = ArdiDbMerger::mergeDB(exportedDBName, true);
    logProgress(QString("merge-on-import(%1) +topics=%2 +contacts=%3 skip-topics=%4 skip-contacts=%5")
        .arg(exportedDBName)
        .arg(res.merged_topics)
        .arg(res.merged_contacts)
        .arg(res.skipped_topics)
        .arg(res.skipped_contacts));
    /*
    QDir bkpDir(AUTOTEST_BACKUP_IMPORT_PATH);
    if (!bkpDir.exists())
    {
        QStringList namesFilter;
        namesFilter.push_back("*.qpk");
        QStringList slist = bkpDir.entryList(namesFilter, QDir::Files);
        if (!slist.isEmpty()) {
            for (auto& s : slist) {
                QString bkpFileName = AUTOTEST_BACKUP_IMPORT_PATH + "/" + s;
                auto res = ArdiDbMerger::mergeDB(bkpFileName, true);
                logProgress(QString("merge-on-import(%1) +topics=%2 +contacts=%3 skip-topics=%4 skip-contacts=%5")
                    .arg(bkpFileName)
                    .arg(res.merged_topics)
                    .arg(res.merged_contacts)
                    .arg(res.skipped_topics)
                    .arg(res.skipped_contacts));
            }
        }
    }*/

    QString sync_step = "backup";

    if (!syncLocal(sync_step))return false;
    updateContentHash();
    printStepResult(++m_step, m_total, sync_step);
    return true;
};


QString SyncAutoTest::backupCurrentIntoTmpAutoTest()
{
    auto db = ard::db();
	assert_return_empty(db, "expected DB");
	assert_return_empty(db->isOpen(), "expected open DB");
    QString strDBName = db->databaseName();
	assert_return_empty(!strDBName.isEmpty(), "expected valid DB path");
    QFileInfo fi(strDBName);

    QString exportPath = AUTOTEST_BACKUP_IMPORT_PATH + "/" + QString("%1_autotest_backup.qpk").arg(fi.baseName());
    if (QFile::exists(exportPath)) {
        if (!QFile::remove(exportPath)) {
            ASSERT(0, QString("failed to delete existing backup autotest file").arg(exportPath));
            return "";
        }
    }

    bool ok = db->backupDB(exportPath);
	assert_return_empty(ok, "backup failed");

    return exportPath;
};

bool SyncAutoTest::randomKill()
{
    logProgress(QString("..random KILL with %1 percentages").arg(RAND_MODIFY_PERCENTAGES));

    if(!randomDeleteItemsInOutline())
        return false;

    return true;
};

bool SyncAutoTest::randomDeleteItemsInOutline()
{
    auto r = ard::root();
    TOPICS_SET items2delete;
    snc::MemFindAllPipe mp;
    int ncount = 0, total_items = itemsBingo(mp, items2delete, false);

    for(auto& i : items2delete)    {
            topic_ptr it = dynamic_cast<topic_ptr>(i);
            QString s = it->dbgHint();
            bool process = true;
            if(!it->canDelete()){
                process = false;
            }
            if(process)
                {
                    if(!it->killSilently(true))
                        {
                            ASSERT(0, "failed to kill") << s;
                        };
                    ncount++;
                }
        }


    bool rv = r->ensurePersistant(-1);
    if(rv)
        {
            logProgress(QString("random KILLED %1 items out of %2 total").arg(ncount).arg(total_items));
        }
    else
        {
            logError("failed to ensure outline persistance.");      
        }
    return rv;
};

bool SyncAutoTest::randomDeleteKeys()
{
    assert_return_false(ard::isDbConnected(), "expected open DB");
    auto kr = ard::db()->kmodel()->keys_root();
    if (!kr->decryptKRing(m_kring_pwd)) {
        logError("KRing unlock failed.");
        return false;
    }

    int deleted_count = 0;

    snc::MemFindAllPipe mp;
    kr->memFindItems(&mp);
    int total_items = mp.items().size();
    for (auto& i : mp.items()) {
        topic_ptr it = dynamic_cast<topic_ptr>(i);
        if (it)
        {
            bool do_it = bingo(it);
            if (do_it)
            {
                it->killSilently(true);
                deleted_count++;
            }
        }
    }


    bool rv = kr->ensurePersistant(-1);
    if (rv)
    {
        logProgress(QString("random KILLED %1 keys out of %2 total").arg(deleted_count).arg(total_items));
    }
    else
    {
        logError("failed to ensure keys persistance.");
    }
    return rv;
};

void SyncAutoTest::updateContentHash()
{
    ArdDB* db = ard::db();

    STRING_MAP hmap_db;
    db->calcContentHash(hmap_db);
    QString sh;
    for(auto& i : hmap_db)    {
            sh += i.first + "=" + i.second + " ";
        }

    qDebug() << "hashed" 
        << db->connectionName() 
        << db->databaseName()
        << "[" << sh << "]";

    m_dbx2hash[dbx()] = sh;
};

void SyncAutoTest::switch2NewPassword() 
{
#ifdef ARD_OPENSSL
    QString new_pwd = QString(QString("auto-pwd-%1").arg(m_step));
    if (new_pwd != m_sync_pwd) {
        auto r = ard::CryptoConfig::cfg().request2ChangeSyncPassword(new_pwd, m_sync_pwd, new_pwd);
        if (r == ard::aes_status::ok) {
            m_sync_pwd = new_pwd;
        }
    }
#endif
};

void SyncAutoTest::printStepResult(int step, int total, QString desc)
{
    QString hs1;
    QString hs2;
    QString hres;

    assert_return_void(m_dbx2hash.size() == 2, "expected 2 entries in hash map");

    if(m_dbx2hash.size() == 2)
        {
            STRING_MAP::iterator k = m_dbx2hash.begin();
            hs1 = k->second;
            ++k;
            hs2 = k->second;
        }

    bool hok = hs1.compare(hs2) == 0;
  
    if(hok)
        {
            hres = QString("hash-OK:%1").arg(hs1);
        }
    else
        {
            hres = QString("hash-ERROR:%1 %2").arg(hs1).arg(hs2);
        }

    logProgress(QString("====== %1/%2 (%3) ==LEVEL:%4 %5 ===")
                .arg(++step)
                .arg(total)
                .arg(desc)
                .arg(m_test_level)
                .arg(hres));
    if(!hok)
        {
            printDiff();
        }
};

void SyncAutoTest::printDiff()
{
    if(gui::isDBAttached())dbp::close(false);

    QString db1_file_path = defaultRepositoryPath() + "dbs/" + sync_test_db_name + "/" + DB_FILE_NAME;
    QString db2_file_path = defaultRepositoryPath() + "dbs/" + sync_test_db_name_2 + "/" + DB_FILE_NAME;

    ArdDB db1;
    ArdDB db2;

#define FINALIZE_DBS if(db1.isOpen())db1.close();   \
    if(db2.isOpen())db2.close();                    \


    if(!db1.openDb("db1", db1_file_path))
        {
            logError("failed to open db1");
            return;
        }
    if(!db1.verifyMetaData())
        {
            logError("failed to verify db1");
            FINALIZE_DBS;
            return;
        }
    if(!db2.openDb("db2", db2_file_path))
        {
            logError("failed to open db2");
            FINALIZE_DBS;
            return;
        }
    if(!db2.verifyMetaData())
        {
            logError("failed to verify db2");
            FINALIZE_DBS;
            return;
        }

    if(!db1.loadTree())
        {
            logError("failed to load db1");
            FINALIZE_DBS;
            return;
        }

    if(!db2.loadTree())
        {
            logError("failed to load db2");
            FINALIZE_DBS;
            return;
        }

    db1.syncDb()->createSyidMap(nullptr);
    db2.syncDb()->createSyidMap(nullptr);

    logProgress(QString("=== diff-on db1: %1 db2:%2").arg(db1_file_path).arg(db2_file_path));
    identityDiff(db1.syncDb(), &db2);
    logProgress(QString("========================="));
    identityDiff(db2.syncDb(), &db1);
    logProgress(QString("=== end-diff"));

    FINALIZE_DBS;
};

/*
template<class T>
void SyncAutoTest::attachableIdentityDiff(const std::vector<T*>& arr, ArdDB* otherDB)
{
    typedef typename std::vector<T*>::const_iterator ITR;
    for(ITR i = arr.begin(); i != arr.end(); ++i)
        {
            T* o = *i;
            attachable* att_other = otherDB->syncDb()->findAttBySyid(o->syid());
            if(att_other)
                {
                    int flags = 0;
                    if(!o->isAtomicIdenticalTo(att_other, flags))
                        {
                            logError(QString("att-ident-err [%1] %2").arg(sync_flags2string(flags)).arg(o->dbgHint()));
                        }
                    else {
                        auto s1 = o->calcContentHashString();
                        auto s2 = att_other->calcContentHashString();
                        if (s1 != s2) {
                            logError(QString("att-hash-err [%1] %2 [%3] %4")
                                .arg(o->dbgHint())
                                .arg(s1)
                                .arg(att_other->dbgHint())
                                .arg(s2));                      }

                    }
                }
            else
                {
                    logError(QString("att-ident-missing in %1: %2").arg(otherDB->connectionName()).arg(o->dbgHint()));
                }
        }
}
*/
//#define APPLY_CHECK_ON_ATTACHABLES(O, F, D) F(O->comments(), D);

void SyncAutoTest::identityDiff(cit* it, ArdDB* otherDB)
{
    cit* it_other = otherDB->syncDb()->findItem(it);
    if(it_other)
        {
            int flags = 0;
            if(!it->isAtomicIdenticalTo(it_other, flags)){
                    logError(QString("ident-err [%1] %2").arg(sync_flags2string(flags, it, it_other)).arg(it->dbgHint()));
                }
            else {
                auto s1 = it->calcContentHashString();
                auto s2 = it_other->calcContentHashString();
                if (s1 != s2) {
                    logError(QString("hash-err [%1] %2 [%3] %4")
                        .arg(it->dbgHint())
                        .arg(s1)
                        .arg(it_other->dbgHint())
                        .arg(s2));
                }
            }
        }
    else
        {
            logError(QString("ident-missing in %1: %2").arg(otherDB->connectionName()).arg(it->dbgHint()));
        }

//    APPLY_CHECK_ON_ATTACHABLES(it, attachableIdentityDiff, otherDB);

    for(auto& it2 : it->items())    {
            identityDiff(it2, otherDB);
        }  
};

void SyncAutoTest::identityDiff(cdb* db, ArdDB* otherDB)
{
    cit* r = db->getSingleton(ESingletonType::dataRoot);
    if (r)
    {
        for (auto& it : r->items()) {
            identityDiff(it, otherDB);
        }
    }
};

//#undef APPLY_CHECK_ON_ATTACHABLES

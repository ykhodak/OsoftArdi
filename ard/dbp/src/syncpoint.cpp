#include <QDir>
#include <QtCore>
#include <QtConcurrent/QtConcurrent>
#include <QDateTime>

#include "dbp.h"
#include "syncpoint.h"
#include "anfolder.h"
#include "ansyncdb.h"
#include "gdrive-syncpoint.h"
#include "snc-tree.h"
#include "contact.h"

static SyncController* g__syncController = nullptr;
static bool g__syncBreak = false;
static ard::aes_result g__syncResult = ard::aes_result{ ard::aes_status::err, 0, ""};
static bool g__syncNoChangesDetected = false;

bool is_sync_broken()
{
    return g__syncBreak;
}

void sync_log(QString s)
{
    if(g__syncController && g__syncController->sp()){
            g__syncController->sp()->log(s);
        }
};

void sync_log(const STRING_LIST& string_list)
{
    if(g__syncController && g__syncController->sp()){
            g__syncController->sp()->log(string_list);
        }
};

void sync_log_error(QString s)
{
    if(g__syncController && g__syncController->sp())
        {
            g__syncController->sp()->log_error(s);
        }
};

QString get_SqlDatabase_info(ArdDB* db)
{
    QString rv = "";
    if(g__syncController && g__syncController->sp())
        {
            if(db == &(g__syncController->sp()->m_local_clone_db))
                {
                    rv = "local";
                }
            else if(db == &(g__syncController->sp()->m_remote_clone_db))
                {
                    rv = "remote";
                }
        }
    return rv;
}

SyncPoint::SyncPoint(bool silence_mode)
    :m_changes_detected(false),
     m_hash_OK(false),
     m_first_time_remote_db_cloned(false),
     m_silence_mode(silence_mode),
     m_logFile(nullptr), m_logStream(nullptr)
{
    m_composite_remote_db_prefix = dbp::compositRemoteDbPrefix4CurrentDB(m_composite_local_db_prefix);
};

void SyncPoint::closeLog()
{
    if(m_logFile)
        {
            if(m_logStream)
                {
                    m_logStream->flush();
                    delete m_logStream;
                    m_logStream = nullptr;
                }

            m_logFile->close();
            delete (m_logFile);
            m_logFile = nullptr;
        }
};


SyncPoint::~SyncPoint()
{
    closeLog();
};

void SyncPoint::setupSyncAuxCommands(const SYNC_AUX_COMMANDS& sc)
{
    m_sync_aux_commands = sc;
    bool needPrintDataOnHashError_command = true;
    needPrintDataOnHashError_command = true;

     for(auto & c : m_sync_aux_commands)   {
            if(c == scmdPrintDataOnHashError)
                {
                    needPrintDataOnHashError_command = false;
                }
        }
    if(needPrintDataOnHashError_command)
        {
            m_sync_aux_commands.insert(m_sync_aux_commands.begin(), scmdPrintDataOnHashError);
        }
};

#define ERR_MSG(S)if(silence_mode)qWarning() << S;else ard::messageBox(gui::mainWnd(), S);

void SyncPoint::errorMessage(QString s)
{
    if(silenceMode())
        {
            qWarning() << s;
        }
    else
        {
		ard::messageBox(gui::mainWnd(), s);
        }
};

bool SyncPoint::confirmMessage(QString s)
{
    bool rv = true;

    if(silenceMode())
        {
            qWarning() << s;
        }
    else
        {
            rv = ard::confirmBox(ard::mainWnd(), s);
        }

    return rv;
};

bool SyncPoint::initSyncPoint()
{
#define FINALIZE_AND_RETURN_ERR(S) {log_error(S);finalize();archiveSyncLog();  return false;}
#define SYNC_STEP(L) {incProgress();log(L);if(isSyncBroken()){FINALIZE_AND_RETURN_ERR("User aborted");}}

#ifdef ARD_OPENSSL
    auto& crypto_cfg = ard::CryptoConfig::cfg();
#endif

    WaitCursor w;
    m_sync_progress = 0;
    incProgress();

    //SYNC_STEP("checking tmp sync directory");
    if (!ensureSyncDir()) {
        FINALIZE_AND_RETURN_ERR("Failed to rebuild sync directory.");
    }

    SYNC_STEP("loading remote db");
    if (!copyRemoteDB()) {
        FINALIZE_AND_RETURN_ERR("Failed to download remote DB.");
    }

    SYNC_STEP("uncompressing remote db");
    auto res = uncompressRemoteDB();
    if (res.status != ard::aes_status::ok) {
        g__syncResult = res;
        FINALIZE_AND_RETURN_ERR("Failed to uncompress remote DB.");
    }

    SYNC_STEP("shutting down data layer");
    dbp::close();

    SYNC_STEP("creating local-db sync snapshot");
    if (!copy_local_db()) {
        FINALIZE_AND_RETURN_ERR("Failed to create local DB snapshot.");
    }

    SYNC_STEP("opening DB-clones");
    if (!open_clones_db()) {
        FINALIZE_AND_RETURN_ERR("Failed to open sync DB. Aborted.");
    }

    //SYNC_STEP("starting sync");
    if (!sync()) {
        FINALIZE_AND_RETURN_ERR("Sync procedure failed. Aborted.");
    }else{
        log("sync completed");
    }

    //SYNC_STEP(QString("running AUX commands (%1 total)").arg(m_sync_aux_commands.size()));
    if (!checkSyncAuxCommands()) {
        FINALIZE_AND_RETURN_ERR("Aux command failed.");
    }

    if(!m_changes_detected){
        bool force_update_non_modified_db = false;
#ifdef ARD_OPENSSL
        if (crypto_cfg.hasPasswordChangeRequest() ||
            crypto_cfg.hasTryOldPassword())
        {
            force_update_non_modified_db = true;

            if (crypto_cfg.hasTryOldPassword()) {
                crypto_cfg.clearTryOldSyncPassword();
            }
        }
#endif

        if (!force_update_non_modified_db) {
            log("since DB in sync bailing out..");
            g__syncResult.status = ard::aes_status::ok;
            g__syncNoChangesDetected = true;
            finalize();
            return true;
        }
    }
    
    /*
    if(m_media_resolve_list.size() > 0)
        {
            SYNC_STEP("checking media resolution DB");
            log(QString("generating media resolution DB [%1]..").arg(m_media_resolve_list.size()));
            if (!generate_resolution_db()) {
                FINALIZE_AND_RETURN_ERR("Failed to create media resolution DB. Aborted.");
            }
            else {
                log("resolution DB created");
            }
        }
        */

    SYNC_STEP("compressing remote db");
    if (!compressRemoteDB()) {
        FINALIZE_AND_RETURN_ERR("Failed to compress remote DB.");
    }

    SYNC_STEP("upgrading remote DB");
    if (!upgradeRemoteDB()) {
        FINALIZE_AND_RETURN_ERR("Failed to upgrade remote DB. Aborted.");
    }
    else {
        log("remote DB upgraded");
    }

#ifdef ARD_OPENSSL
    if (crypto_cfg.hasPasswordChangeRequest()) {
        auto r = crypto_cfg.commitSyncPasswordChange();
        ASSERT(r == ard::aes_status::ok, "ERROR failed to commit password change.");
        SYNC_STEP(QString("pwd change commited. [%1] [%2]").arg(crypto_cfg.syncCrypto().second.tag).arg(crypto_cfg.syncCrypto().second.isValid()));
        ///@todo: we might want to enable usage of uncommitted pwd, at this point..
    }
#endif

    SYNC_STEP("upgrading local DB");
    if (!upgrade_local_db()) {
        FINALIZE_AND_RETURN_ERR("Failed to upgrade local DB. Aborted.");
    }
    else {
        log("local DB upgraded");
    }

    finalize();

    archiveSyncLog();

    g__syncResult.status = ard::aes_status::ok;

#ifdef ARD_OPENSSL
    if (crypto_cfg.hasTryOldPassword()) {
        crypto_cfg.clearTryOldSyncPassword();
    }
#endif

    return true;
};

bool SyncPoint::open_clones_db()
{   
    if(!m_local_clone_db.openDb("sync-local-clone", local_sync_area_db_clone()))
        {
            log("failed to open local DB-clone");
            return false;
        }
    log(QString("l-DB [%1] loading..").arg(m_local_clone_db.databaseName()));
    if(!m_local_clone_db.verifyMetaData())
        {
            log("failed to verify local DB-clone");
            return false;
        }

    bool ROmodeRequest = false;
    if(!dbp::guiCheckPassword(ROmodeRequest, &m_local_clone_db, "Local DB login"))
        {
            log("failed to login to local DB");
            return false;   
        }

    if(!m_local_clone_db.loadTree())
        {
            log("failed to load local DB-clone root");
            return false;
        }

    log(QString("l-DB-clone opened: %1").arg(m_local_clone_db.root()->syncInfoAsString()));
    
    if(!m_remote_clone_db.openDb("sync-remote-clone", remote_sync_area_db_clone()))
        {
            log("failed to open local DB-clone");
            return false;
        }
    log(QString("r-DB [%1] loading..").arg(m_remote_clone_db.databaseName()));
    if(!m_remote_clone_db.verifyMetaData())
        {
            log("failed to verify remote DB-clone");
            return false;
        }

    ROmodeRequest = false;
    if(!dbp::guiCheckPassword(ROmodeRequest, &m_remote_clone_db, "Remote DB login"))
        {
            log("failed to login to remote DB");
            return false;   
        }

    if(!m_remote_clone_db.loadTree())
        {
            log("failed to load remote DB-clone root");
            return false;
        }

    log(QString("r-DB-clone opened: %1").arg(m_remote_clone_db.root()->syncInfoAsString())); 

    return true;
};

void SyncPoint::finalize()
{
    syncExtraModules();
    
    if(m_local_clone_db.isOpen()){
        m_local_clone_db.close();
    }
    if(m_remote_clone_db.isOpen()){
        m_remote_clone_db.close();
    }

    if(!gui::isDBAttached()){
        dbp::reopenLast();
    }

#ifdef ARD_OPENSSL
    auto& crypto_cfg = ard::CryptoConfig::cfg();
    if (crypto_cfg.hasTryOldPassword()) {
        crypto_cfg.clearTryOldSyncPassword();
    }
#endif
};


bool SyncPoint::ensureSyncDir() 
{
    QString tmp_sync_dir = tmp_sync_dir_path();
    QDir sync_dir(tmp_sync_dir);
    if (!sync_dir.exists())
    {
        log_error("Can not create sync tmp directory[1] " + tmp_sync_dir);
        return false;
    }
    closeLog();
    bool tmp_Was_deleted = false;
    for (int try_to_remove_possible_locked = 0; try_to_remove_possible_locked < 3 && !tmp_Was_deleted; try_to_remove_possible_locked++)
    {
        if (!removeDir(tmp_sync_dir))
        {
            qWarning() << "Can not remove sync tmp directory" << tmp_sync_dir;
            closeLog();
        }
        else
        {
            tmp_Was_deleted = true;
            break;
        }
        ard::sleep(1000);
    }
    if (!tmp_Was_deleted) {
        log_error("Failed to remove sync tmp directory " + tmp_sync_dir);
        return false;
    }

    /// this command is not 'pure' in will create folder internally
    sync_dir = tmp_sync_dir_path();
    if (!sync_dir.exists()) {
        log_error(QString("Expected Sync directory '%1'").arg(sync_dir.absolutePath()));
        return false;
    };

    return true;
};

ard::aes_result SyncPoint::uncompressRemoteDB()
{
    ard::aes_result rv = { ard::aes_status::err, 0, ""};

    if(!QFile::exists(compressed_remote_sync_area_db_clone()))
        {
            log("compressed file not found " + compressed_remote_sync_area_db_clone());
            rv.status = ard::aes_status::file_not_found;
            return rv;
        }

    if(!m_first_time_remote_db_cloned)
        {
            QFile f(compressed_remote_sync_area_db_clone());
            if(f.size() == 0)
                {
                    m_first_time_remote_db_cloned = true;
                    log(QString("compressed file '%1' size 0 - setting 'first_time_remote_db_cloned'").arg(compressed_remote_sync_area_db_clone()));
                }
        }

    //bool ok = false;
    if(m_first_time_remote_db_cloned){
        if (!copyOverFile(compressed_remote_sync_area_db_clone(), remote_sync_area_db_clone())) {
            rv.status = ard::aes_status::file_access_error;
            return rv;
        }
            log("initiated first time DB clone");
            rv.status = ard::aes_status::ok;
           // ok = true;
        }
    else{
            rv = SyncPoint::uncompress(compressed_remote_sync_area_db_clone(), remote_sync_area_db_clone(), false);
        }
    return rv;
};

bool SyncPoint::compressRemoteDB()
{
    if(!QFile::exists(remote_sync_area_db_clone()))
        {
            log("remote db clone not found: " + remote_sync_area_db_clone());
            return false;
        }
    bool ok = SyncPoint::compress(remote_sync_area_db_clone(), compressed_remote_sync_area_db_clone());
    return ok;
};


QString SyncPoint::compressed_remote_sync_area_db_clone()
{
    QString rv = tmp_sync_dir_path();
    rv += "remote_db_clone.qpk";
    return rv;
};

QString SyncPoint::local_sync_area_db_clone()
{
    QString rv = tmp_sync_dir_path();
    rv += "local_db_clone.sqlite";
    return rv;
};

QString SyncPoint::remote_sync_area_db_clone()
{
    QString rv = tmp_sync_dir_path();
    rv += "remote_db_clone.sqlite";
    return rv;
};

bool SyncPoint::copy_local_db()
{
    if(!copyOverFile(get_db_file_path(), local_sync_area_db_clone()))
        return false;
    log("l-DB " + get_db_file_path() + " -> " + local_sync_area_db_clone());

    return true;
};

bool SyncPoint::upgrade_local_db()
{
    if(!QFile::exists(local_sync_area_db_clone()))
        {
            log("local sync DB clone not found:" + local_sync_area_db_clone());
            return false;
        }

    if(QFile::exists(get_db_file_path()))
        {
            QString db_bak = get_db_file_path() + ".bak";
            if(!renameOverFile(get_db_file_path(), db_bak))
                return false;
        }

    if(!copyOverFile(local_sync_area_db_clone(), get_db_file_path()))
        return false;
    return true;
};

extern QString get_tmp_sync_dir_prefix();

void SyncPoint::archiveSyncLog()
{
    auto log_path = get_sync_log_path(m_composite_local_db_prefix);
    if (QFile::exists(log_path))
    {
        QFileInfo fi(log_path);
        QString def_prefix = get_tmp_sync_dir_prefix();
        QString usr_prefix;
        auto s1 = fi.path();
        auto idx = s1.indexOf(def_prefix);
        if (idx != -1) {
            usr_prefix = s1.mid(idx + def_prefix.size());
        }

        QString suffix = "";
        int i = 0;
        while (i < 1000)
        {
            QString arch_lpath = get_sync_log_archives_path() + usr_prefix + "-" + QDateTime::currentDateTime().toString("yyyyMMdd") +
                "-" + shortName() + "-" + m_composite_local_db_prefix + "sync" + suffix + ".log";
            if (!QFile::exists(arch_lpath))
            {
                if (!QFile::copy(log_path, arch_lpath))
                {
                    ASSERT(0, "failed to archive sync log") << log_path << arch_lpath;
                    return;
                }
                else
                {
                    return;
                }
            }
            i++;
            suffix = QString("-%1").arg(i);
        }
    }
};

void SyncPoint::checkLogFile()
{
    if (!m_logFile)
    {
        QString log_path = get_sync_log_path(m_composite_local_db_prefix);
        m_logFile = new QFile(log_path);
        if (QFile::exists(log_path))
        {
            if (!m_logFile->open(QIODevice::Append | QIODevice::Text))
            {
                ASSERT(0, "Failed to open sync-log (in-append-mode)") << log_path;
                delete(m_logFile);
                m_logFile = nullptr;
                return;
            };
        }
        else
        {
            if (!m_logFile->open(QIODevice::WriteOnly | QIODevice::Text))
            {
                gui::sleep(400);
                ASSERT(0, "Failed to open sync-log (in-trunc-mode)") << log_path;

                /// sleep and try again maybe somebody is doing 'tail' on Windows
                if (!m_logFile->open(QIODevice::WriteOnly | QIODevice::Text))
                {
                    ASSERT(0, "Failed to open sync-log (in-trunc-mode)") << log_path;
                    delete(m_logFile);
                    m_logFile = nullptr;
                    return;
                };
            }

        }

        m_logStream = new QTextStream(m_logFile);
    }
};

void SyncPoint::log(QString s)
{
    checkLogFile();
    if(m_logStream){
        *m_logStream << s << endl;
    }
    qWarning() << "[s]" << s;
    monitorSyncActivity(s, silenceMode());
};

void SyncPoint::log(const STRING_LIST& string_list)
{
    checkLogFile();

    if(m_logStream)
        {
            for(auto& s : string_list){
                *m_logStream << s << endl;
            }
        }

    monitorSyncActivity(string_list, silenceMode());
};

void SyncPoint::stat(QString s)
{
    monitorSyncStatus(s, silenceMode());
};

void SyncPoint::log_error(QString s)
{
    log("ARD_ERROR:" + s);
    //ASSERT(0, s);
};

bool SyncPoint::sync()
{
    m_changes_detected = false;
    m_hash_OK = false;

    RootTopic* sdb = m_local_clone_db.root();
    if(!sdb->ensureRootRecord())
        {
            ASSERT(0, "[local]failed to ensure root record");
            return false;
        };

    RootTopic* sdb_sibling = m_remote_clone_db.root();
    if(!sdb_sibling->ensureRootRecord())
        {
            ASSERT(0, "[remote]failed to ensure root record");
            return false;
        };

    m_local_clone_db.prepareForSync();
    m_remote_clone_db.prepareForSync();

    if(!sync_2SDB(&m_local_clone_db, 
                  &m_remote_clone_db,
                  //m_media_resolve_list, 
                  m_changes_detected,
                  m_hash_OK,
                  m_hint,
                  m_hashCompareResult,
                  m_silence_mode ? nullptr : ard::syncProgressStatus()))
        {
            qWarning() << "sync failed: " << sdb->title();
            return false;
        };

    if(!m_local_clone_db.root()->checkTreeSanity())
        {
            QString s = "sanity check failed on local DB";
            sync_log(s);
            ASSERT(0, s);
            return false;
        }

    if(!m_remote_clone_db.root()->checkTreeSanity())
        {
            QString s = "sanity check failed on remote DB";
            sync_log(s);
            ASSERT(0, s);
            return false;
        }         

    if(!sdb->storeHistoryAfterSync(getPointID(), sdb_sibling)) return false;
    if(!sdb_sibling->storeHistoryAfterSync(syncpDevice, sdb)) return false;

    if(m_changes_detected)
        {
            sync_log(QString("store l-DB %1").arg(m_local_clone_db.databaseName()));
            if(!m_local_clone_db.ensurePersistant())return false;
            sync_log(QString("store r-DB %2").arg(m_remote_clone_db.databaseName()));
            if(!m_remote_clone_db.ensurePersistant())return false;
  /*
            if(!m_media_resolve_list.empty())
                {
                    for(RESOLVE_LIST::iterator i = m_media_resolve_list.begin();i != m_media_resolve_list.end();i++)
                        {
                            SMediaResolveInfo& ri = *i;
                            if(ri.remote_image_owner_id_resolution_req)
                                {
                                    ASSERT(ri.remote_image_owner_id_resolution_req->cit_owner(), "expected owner");
                                    ASSERT(IS_VALID_DB_ID(ri.remote_image_owner_id_resolution_req->cit_owner()->id()), "expected valid owner id");
                                    ri.remote_image_owner_id = ri.remote_image_owner_id_resolution_req->cit_owner()->id();
                                    ri.clearRemoteImageOwnerIDResolutionReq();
                                }

                            if(ri.local_image_id_resolution_req)
                                {
                                    ASSERT(IS_VALID_DB_ID(ri.local_image_id_resolution_req->id()), "expected valid image id");
                                    ri.local_image_id = ri.local_image_id_resolution_req->id();
                                    ri.clearLocalImageIDResolutionReq();
                                }
                        }
                }
                */
        }

    return true;
};

QString SyncPoint::tmp_sync_dir_path()
{
    QString rv = get_tmp_sync_dir_path(m_composite_local_db_prefix);
    return rv;
};

bool SyncPoint::checkSyncAuxCommands()
{
    extern QString syncCommand2String(SyncAuxCommand);

    bool rv = true;

    if (isSyncBroken())
        return false;

    if (m_sync_aux_commands.empty())
        return true;

    /*
    for (SYNC_AUX_COMMANDS::iterator i = m_sync_aux_commands.begin(); i != m_sync_aux_commands.end(); i++)
    {
        SyncAuxCommand c = *i;
        QString msg = QString("sync-aux-command: %1").arg(syncCommand2String(c));
        sync_log(msg);
    }*/

    for (SYNC_AUX_COMMANDS::iterator i = m_sync_aux_commands.begin(); i != m_sync_aux_commands.end(); i++)
    {
        SyncAuxCommand c = *i;
        QString msg = QString("running-aux-command: %1").arg(syncCommand2String(c));
        sync_log(msg);
        switch (c)
        {
        case scmdPrintDataOnHashError:
        {
            aux_PrintDataOnHashError();
        }break;
        default:ASSERT(0, "unsupported command") << c; return false;
        }
    }

    return rv;
};


struct two_cit
{
    two_cit(snc::cit* l, snc::cit* r):local(l), remote(r), flag(0){}

    snc::cit* local;
    snc::cit* remote;
    int       flag;
};



static void printTreeDiff(topic_ptr roolLocal, topic_ptr rootRemote)
{
    typedef std::vector<two_cit> TWO_CITS;

    snc::MemFindAllWithSYIDmap mpLocal;
    roolLocal->memFindItems(&mpLocal);

    snc::MemFindAllWithSYIDmap mpRemote;
    rootRemote->memFindItems(&mpRemote);

    TWO_CITS identity_hash_mismatch;
    TWO_CITS identity_error;
    CITEMS_SET missed_in_remote;
    CITEMS_SET missed_in_local;
    int iflags;

    SYID_SET processed_syid;

    for(CITEMS::iterator k = mpLocal.items().begin(); k != mpLocal.items().end(); k++)
        {
            snc::cit* it = *k;
            SYID_2_ITEM::iterator j = mpRemote.syid2item().find(it->syid());
            if(j == mpRemote.syid2item().end())
                {
                    missed_in_remote.insert(it);
                }
            else
                {
                    processed_syid.insert(it->syid());

                    snc::cit* it_remote = j->second;
                    bool atomicIdentical = it->isAtomicIdenticalTo(it_remote, iflags);
                    QString sh = it->calcContentHashString();
                    QString sh2 = it_remote->calcContentHashString();
                    bool hashIdentical = (sh.compare(sh2) == 0);
                    if((atomicIdentical && !hashIdentical) ||
                       (!atomicIdentical && hashIdentical))
                        {
                            two_cit c2(it, it_remote);
                            identity_hash_mismatch.push_back(c2);
                        }
                    else
                        {
                            if(!atomicIdentical || !hashIdentical)
                                {
                                    two_cit c2(it, it_remote);
                                    c2.flag = iflags;
                                    identity_error.push_back(c2);
                                }
                        }
                }
        }//for locals..


    for(CITEMS::iterator k = mpRemote.items().begin(); k != mpRemote.items().end(); k++)
        {
            snc::cit* it = *k;
            SYID_2_ITEM::iterator j = mpLocal.syid2item().find(it->syid());
            if(j == mpLocal.syid2item().end())
                {
                    missed_in_local.insert(it);
                }
        }//for remote

    int   diffLevel = missed_in_remote.size() + missed_in_local.size() + identity_error.size() + identity_hash_mismatch.size();  
    if(diffLevel > 0)
        {
            qDebug() << "[diff]" << "number:" << diffLevel;

            if(!missed_in_remote.empty())
                {
                    qDebug() << "[diff]" << "missed_in_remote:" << missed_in_remote.size();
                    for(CITEMS_SET::iterator k = missed_in_remote.begin(); k != missed_in_remote.end(); k++)
                        {
                            qDebug() << "[diff]" << (*k)->dbgHint();
                        }
                    qDebug() << "[diff]" << "==end missed_in_remote==";
                }

            if(!missed_in_local.empty())
                {
                    qDebug() << "[diff]" << "missed_in_local:" << missed_in_local.size();
                    for(CITEMS_SET::iterator k = missed_in_local.begin(); k != missed_in_local.end(); k++)
                        {
                            qDebug() << "[diff]" << (*k)->dbgHint();
                        }
                    qDebug() << "[diff]" << "==end missed_in_local==";
                }

            if(!identity_hash_mismatch.empty())
                {
                    qDebug() << "[diff]" << "identity_hash_mismatch:" << identity_hash_mismatch.size();
                    for(TWO_CITS::iterator k = identity_hash_mismatch.begin();k != identity_hash_mismatch.end(); k++)
                        {
                            two_cit& c2 = *k;
                            cit* local = c2.local;
                            cit* remote = c2.remote;

                            topic_ptr it_local = dynamic_cast<topic_ptr>(local);
                            topic_ptr it_remote = dynamic_cast<topic_ptr>(remote);
                            assert_return_void(it_local, "expected item");
                            assert_return_void(it_remote, "expected item");
                            it_local->printIdentityDiff(it_remote);
                        }
                    qDebug() << "[diff]" << "==end identity_hash_mismatch==";
                }

            if(!identity_error.empty())
                {
                    qDebug() << "[diff]" << "identity_error:" << identity_error.size();
                    for(TWO_CITS::iterator k = identity_error.begin();k != identity_error.end(); k++)
                        {
                            two_cit& c2 = *k;
                            qDebug() << "[diff]" << sync_flags2string(c2.flag) << " " << c2.local << "|" << c2.remote;
                        }
                    qDebug() << "[diff]" << "==end identity_error==";
                }
        }
};



void SyncPoint::aux_PrintDataOnHashError()
{
    ASSERT(m_local_clone_db.isOpen(), "expected opened local DB clone");
    ASSERT(m_remote_clone_db.isOpen(), "expected opened remote DB clone");

    //qDebug() << "running aux_PrintDataOnHashError.." << m_hash_OK;
    if(!m_hash_OK)
        {
            //#ifdef _DEBUG
            RootTopic* rootLocal = m_local_clone_db.root();
            RootTopic* rootRemote = m_remote_clone_db.root();
            printTreeDiff(rootLocal, rootRemote);

            //auto res_rootLocal = m_local_clone_db.resource_root();
            //auto res_rootRemote = m_remote_clone_db.resource_root();
            //printTreeDiff(res_rootLocal, res_rootRemote);

            //#endif //_DEBUG
        }
};

/**
    compressed archive format
    |64bits|64bits|..data..|
    |version|reserved|data-zip|
    |1ard|0000|......|
*/
union UC
{
    uint64_t val;
    struct
    {
        uint8_t v1;
        uint8_t v2;
        uint8_t v3;
        uint8_t v4;
        uint32_t rest;
    };
};

static bool compress_v1(QString sourceFile, QString destFile)
{
    if (QFile::exists(destFile))
    {
        if (!QFile::remove(destFile))
        {
            ASSERT(0, "failed to delete while compress") << destFile;
            return false;
        }
    };


    static uint64_t ard_v_tag;
    static UC uc;
    uc.v1 = '1';
    uc.v2 = 'a';
    uc.v3 = 'r';
    uc.v4 = 'd';
    uc.rest = 0;
    ard_v_tag = uc.val;

    QByteArray in_ar, out_ar;
    try {
        QFile sfile(sourceFile);
        if (!sfile.open(QIODevice::ReadOnly))
        {
            ASSERT(0, "Failed to open source file") << sourceFile;
            return false;
        }
        in_ar = sfile.readAll();
        sfile.close();
    }
    catch (std::bad_alloc& ba)
    {
        qWarning() << "bad_alloc-exception/read-on-compress" << ba.what();
        return false;
    }

    QFile dfile(destFile);
    try {
        out_ar = qCompress(in_ar, 9);
        if (!dfile.open(QIODevice::WriteOnly))
        {
            ASSERT(0, "Failed to open dest file") << destFile;
            return false;
        }
    }
    catch (std::bad_alloc& ba)
    {
        qWarning() << "bad_alloc-exception/compress" << ba.what();
        return false;
    }

    dfile.write((char *)&ard_v_tag, sizeof(uint64_t));
    ard_v_tag = 0;
    dfile.write((char *)&ard_v_tag, sizeof(uint64_t));
    dfile.write(out_ar);
    dfile.close();
    return true;
};


extern std::pair<uint64_t, uint64_t> def_encr_key();
static bool compress_v2(QString sourceFile, QString destFile)
{
    if (QFile::exists(destFile))
    {
        if (!QFile::remove(destFile))
        {
            ASSERT(0, "failed to delete while compress") << destFile;
            return false;
        }
    };


    static uint64_t ard_v_tag;
    static UC uc;
    uc.v1 = '2';
    uc.v2 = 'a';
    uc.v3 = 'r';
    uc.v4 = 'd';
    uc.rest = 0;
    ard_v_tag = uc.val;

    QByteArray in_ar, out_ar;
    try {
        QFile sfile(sourceFile);
        if (!sfile.open(QIODevice::ReadOnly))
        {
            ASSERT(0, "Failed to open source file") << sourceFile;
            return false;
        }
        in_ar = sfile.readAll();
        sfile.close();
    }
    catch (std::bad_alloc& ba)
    {
        qWarning() << "bad_alloc-exception/read-on-compress" << ba.what();
        return false;
    }

    QFile dfile(destFile);
    try {       
        auto arr_tmp = qCompress(in_ar, 9);
        out_ar = ard::encryptBarray(def_encr_key(), arr_tmp);
        if (!dfile.open(QIODevice::WriteOnly))
        {
            ASSERT(0, "Failed to open dest file") << destFile;
            return false;
        }
    }
    catch (std::bad_alloc& ba)
    {
        qWarning() << "bad_alloc-exception/compress" << ba.what();
        return false;
    }

    dfile.write((char *)&ard_v_tag, sizeof(uint64_t));
    ard_v_tag = 0;
    dfile.write((char *)&ard_v_tag, sizeof(uint64_t));
    dfile.write(out_ar);
    dfile.close();
    return true;
};

static QByteArray uncompress_v1(const QByteArray& arr)
{
    QByteArray out_arr = qUncompress(arr);
    return out_arr;
};

bool SyncPoint::compress(QString sourceFile, QString destFile, int forceVersion /*= -1*/)
{
    /// we want to convert everything from encrypted state to normal compression for now ///
    forceVersion = 1;

    bool rv = false;
    if (forceVersion == -1) {
        //rv = compress_v2(sourceFile, destFile);
#ifdef ARD_OPENSSL
        auto res = ard::aes_archive_encrypt_file(sourceFile, destFile);
        if (res == ard::aes_status::ok) {
            rv = true;
        }
        else {
            ASSERT(0, QString("Compress failed with error '%1'").arg(ard::aes_status2str(res)));
            return false;
        }
#else
        rv = compress_v1(sourceFile, destFile);
#endif
        /// verify data compressed OK
        if (rv) {
            QString verifyTmp = sourceFile + ".tmp-verify";
            if (QFile::exists(verifyTmp)) {
                if (!QFile::remove(verifyTmp)) {
                    ASSERT(0, "failed to remove temporary verification file (1)") << verifyTmp;
                    return false;
                }
            }
            auto res = uncompress(destFile, verifyTmp, true);
            if (res.status != ard::aes_status::ok) {
                ASSERT(0, "File compress verification failed, giving p..");
                QFile::remove(verifyTmp);
                return false;
            };
            QFile::remove(verifyTmp);
        }
    }
    else if (forceVersion == 1) {
        rv = compress_v1(sourceFile, destFile);
    }
    else {
        ASSERT(0, "unsupoprted version requested") << forceVersion;
    }

    return rv;
};

ard::aes_result SyncPoint::uncompress(QString sourceFile, QString destFile, bool verify_mode /*= false*/)
{
    ard::aes_result rv = { ard::aes_status::err, 0, ""};

    uint64_t ard_v_tag;
    UC uc;

    QFile sfile(sourceFile);
    if (!sfile.open(QIODevice::ReadOnly))
        {
            ASSERT(0, "Failed to open source file") << sourceFile;
            rv.status = ard::aes_status::file_access_error;
            return rv;
        }
    sfile.read((char *)&ard_v_tag, sizeof(uint64_t));
    uc.val = ard_v_tag;
    if(uc.v1 > '3')
        {
            ASSERT(0, "unsupported archive version or corrupted data") << sourceFile << uc.v1 << uc.v2 << uc.v3 << uc.v4;
            rv.status = ard::aes_status::unsupported_archive;
            return rv;
        }
    if (uc.v1 == '3') {
#ifdef ARD_OPENSSL
        sfile.close();
        auto& cfg = ard::CryptoConfig::cfg();
        ard::SyncCrypto enforce_key = cfg.syncCrypto().second;
        if (verify_mode) {          
            if (cfg.hasPasswordChangeRequest()) {
                enforce_key = cfg.syncLimboCrypto().second;
            }
        }
        else {
            if (cfg.hasTryOldPassword()) {
                enforce_key = cfg.tryOldCrypto().second;
            }
            else {
                if (!enforce_key.isValid()) {
                    if(cfg.hasPasswordChangeRequest()){
                        enforce_key = cfg.syncLimboCrypto().second;
                    }
                }
            }
        }
        rv = ard::aes_archive_decrypt_file(sourceFile, destFile, &enforce_key);
        if (rv.status == ard::aes_status::ok) {
            qDebug() << "decr-ok arc-tag:" << rv.key_tag << "encr-date:" << rv.encr_date.toString();
            /// have to store tag# if it's less than our
            auto& cfg = ard::CryptoConfig::cfg();
            auto cr = cfg.syncCrypto();
            if (cr.first) {
                if (cr.second.tag < rv.key_tag) {
                    cfg.promoteSyncPasswordKeyTag(rv.key_tag);
                }
            }
        }
        else {
            qWarning() << QString("uncompress/encrypt error - %1'").arg(ard::aes_status2str(rv.status));
            //ASSERT(0, "encrypt error ") << ard::aes_status2str(rv.status);
        }
        return rv;
#else
        Q_UNUSED(verify_mode);
        ASSERT(0, "unsupported archive version or corrupted data") << sourceFile << uc.v1 << uc.v2 << uc.v3 << uc.v4;
        rv.status = ard::aes_status::unsupported_archive;
        return rv;
#endif
    }
    sfile.read((char *)&ard_v_tag, sizeof(uint64_t));
    QByteArray in_ar = sfile.readAll();//<<<@todo we have to support v3 only from now on..
    sfile.close();
    QByteArray out_ar;
    if (uc.v1 == '1') {
        out_ar = uncompress_v1(in_ar);
    }
    else if (uc.v1 == '2') {
        in_ar = ard::decryptBarray(def_encr_key(), in_ar);
        out_ar = uncompress_v1(in_ar);
    }

    if(out_ar.size() < 1)
        {
            ASSERT(0, "corrupted archive data, failed to uncompress") << sourceFile;
            rv.status = ard::aes_status::corrupted;
            return rv;
        }
    QFile dfile(destFile);
    if (!dfile.open(QIODevice::WriteOnly))
        {
            ASSERT(0, "Failed to open dest file") << destFile;
            rv.status = ard::aes_status::file_access_error;
            return rv;
        }
    dfile.write(out_ar);
    dfile.close();
    rv.status = ard::aes_status::ok;
    return rv;
};





/**
   LocalSyncPoint
*/

LocalSyncPoint::LocalSyncPoint(bool silence_mode)
    :SyncPoint(silence_mode)
{

};


bool LocalSyncPoint::copyRemoteDB()
{
    if(!QFile::exists(m_remote_db_path))
        {
            log("remote DB file not found:" + m_remote_db_path);
            return false;
        }
    if(!copyOverFile(m_remote_db_path, compressed_remote_sync_area_db_clone()))
        return false;

    log("r-DB " + m_remote_db_path + " -> " + compressed_remote_sync_area_db_clone());

    return true;
};

bool LocalSyncPoint::upgradeRemoteDB()
{
    QFileInfo remote_compressed_fi(compressed_remote_sync_area_db_clone());

    if(!remote_compressed_fi.exists())
        {
            log("remote sync DB compressed clone not found:" + compressed_remote_sync_area_db_clone());
            return false;
        }    

    if(QFile::exists(m_remote_db_path))
        {
            QString db_bak = m_remote_db_path + ".bak";
            if(!renameOverFile(m_remote_db_path, db_bak))
                return false;
        }

    if(!copyOverFile(compressed_remote_sync_area_db_clone(), m_remote_db_path))
        return false;

    //m_last_sync_info.invalidate();
    LastSyncInfo si;
    si.syncDbSize = remote_compressed_fi.size();
    si.syncTime = QDateTime::currentDateTime();
    dbp::configLocalSyncStoreLastSyncTime(&m_local_clone_db, si);


    log(QString("remote db updated: %1").arg(m_remote_db_path));
    return true;
};

/**
   SyncController
*/
SyncController::SyncController(bool silence_mode, SYNC_AUX_COMMANDS sync_commands)
    :m_silence_mode(silence_mode)
{
    m_controller_sync_aux_commands = sync_commands;

    g__syncBreak = false;
    g__syncResult = ard::aes_result{ ard::aes_status::err, 0, ""};
    g__syncNoChangesDetected = false;
    static bool firstCall = false;
    if(!firstCall)
        {
            qRegisterMetaType<SyncPointID>("SyncPointID");
            firstCall = true;
        }

    startMonitorSyncActivity(m_silence_mode);
}

SyncController::~SyncController() 
{
    g__syncController = nullptr;
}

void SyncController::afterSync(QString composite_rdb_string)
{
    if(SyncPoint::isSyncBroken())
        {
            sync_log("Sync cancelled");
        }
    else
        {
            sync_log("Sync finished");
            if(m_sp){
                    m_sp->closeLog();
                }
        }

    g__syncController = nullptr;
    stopMonitorSyncActivity(true, composite_rdb_string, m_silence_mode);
    deleteLater();
	ard::asyncExec(AR_DataSynchronized);
};

std::unique_ptr<SyncPoint> SyncController::produceSyncPoint(SyncPointID syncType, bool silence_mode)
{
    std::unique_ptr<SyncPoint> rv;
    //SyncPoint* sp = nullptr;

    switch (syncType) {
    case syncpLocal:
    {
        rv.reset(new LocalSyncPoint(silence_mode));
        //rv = std::make_unique<LocalSyncPoint>(silence_mode);
    }break;
    case syncpGDrive:
    {
#ifdef ARD_GD
        rv.reset(new GDSyncPoint(silence_mode));// = std::make_unique<GDSyncPoint>(silence_mode);
        //sp = new GDSyncPoint(silence_mode);
#endif      
    }break;
    case syncpDBox: ASSERT(0, "NA"); break;
    default:ASSERT(0, "unsupported sync type"); break;
    }

    return rv;
};

void SyncController::initSync(SyncPointID syncType, QString hint)
{
    assert_return_void(gui::isDBAttached(), "expected attached DB");
  
    m_sp = produceSyncPoint(syncType, m_silence_mode);
   // int cloudIdReq = ard::db()->getCloudEmptyCIDImagesCount(m_sp->cloudIdType());

    if(!m_sp->prepare(hint))
        {
            sync_log("Failed to prepare sync procedure."); 
            stopMonitorSyncActivity(false, m_sp->m_composite_local_db_prefix, m_silence_mode);
            m_sp->closeLog();
            return;
        }

    m_sp->setupSyncAuxCommands(m_controller_sync_aux_commands);
    if (m_sp->initSyncPoint()) {
        //auto s_lines = ard::syncProgressStatus()->resulsAsCharMap();
        //for (auto& s : s_lines) {
        //    sync_log(s);
        //}
    };
    m_sp->finalizeSync();
    afterSync(m_sp->m_composite_local_db_prefix);
};

bool SyncPoint::isSyncRunning()
{
    return (g__syncController != nullptr);
};

bool SyncPoint::isSyncBroken()
{
    return g__syncBreak;
};

ard::aes_result SyncPoint::lastSyncResult()
{
    return g__syncResult;
};

bool SyncPoint::isLastSyncDBIdenticalAfterSync()
{
    return g__syncNoChangesDetected;
};

void SyncPoint::breakSync()
{
    assert_return_void(g__syncController != nullptr, "sync is not running");
    assert_return_void(!g__syncBreak, "sync break already (iated");
    g__syncController->sp()->initSyncBreak();
};

bool SyncPoint::checkBreak()const
{
    return isSyncBroken();
};

void SyncPoint::initSyncBreak()
{
    log("Sync break initiated - Sync will be aborted");
    g__syncBreak = true;
};

void SyncPoint::runLocalSync(bool silence_mode, SYNC_AUX_COMMANDS sync_commands, QString hint)
{
    if(isSyncRunning())
        {
            ERR_MSG("Sync procedure already started");
            return;
        }

    assert_return_void(gui::isDBAttached(), "expected attached DB");

    g__syncController = new SyncController(silence_mode, sync_commands);
    g__syncController->initSync(syncpLocal, hint);
    delete(g__syncController);
};

void SyncPoint::tryDboxSync(void* p)
{
    QString s = *(QString*)p;
    g__syncController->initSync(syncpDBox, s);
};

void SyncPoint::tryGDSync(void* p) 
{
    QString s = *(QString*)p;
    g__syncController->initSync(syncpGDrive, s);
};

void SyncPoint::runGdriveSync(bool silence_mode, SYNC_AUX_COMMANDS sync_commands, QString hint)
{
	if (isSyncRunning())
	{
		ERR_MSG("Sync procedure already started");
		return;
	}
	if (!gui::isConnectedToNetwork())
	{
		ERR_MSG("Network connection is not detected. Make sure you are connected to the Internet.");
		return;
	}
	assert_return_void(gui::isDBAttached(), "expected attached DB");
	g__syncController = new SyncController(silence_mode, sync_commands);
	if (!tryCatchAbort(tryGDSync, &hint, "gdriveSync"))
	{
		ard::errorBox(ard::mainWnd(), "Synchronization aborted due to internal error. Could be security or space issue. Try to reboot your machine and clean space.");
	};
	delete(g__syncController);
};

void SyncPoint::incProgress()
{
    m_sync_progress++;
    static qreal maxSyncSteps = 15;
    int percentageCompleted = (int)((100.0 * m_sync_progress) / maxSyncSteps);
    stepMonitorSyncActivity(percentageCompleted, silenceMode());
};


bool LocalSyncPoint::prepare(QString hint)
{    
    log("Local sync");

    m_hint = hint;
    QString db_dir = remoteDBPath4LocalSync();
    QDir d(db_dir);
    if (!d.exists())
    {
        if (!d.mkpath(db_dir))
        {
            log_error("Failed to create folder" + db_dir);
            return false;
        }
    }

    m_remote_db_path = db_dir + "/" + get_compressed_remote_db_file_name(m_composite_remote_db_prefix);

    if (!QFile::exists(m_remote_db_path))
    {
        QFile f(m_remote_db_path);
        if (!f.open(QIODevice::WriteOnly))
        {
            log_error("Failed to create file" + m_remote_db_path);
            return false;
        }
        f.close();

        m_first_time_remote_db_cloned = true;
        log(QString("not found remote_db_path '%1' - setting 'first_time_remote_db_cloned'").arg(m_remote_db_path));
    }

    return true;
};

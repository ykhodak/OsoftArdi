#include <QDebug>
#include "a-db-utils.h"
#include "ansyncdb.h"
#include "ethread.h"
#include "contact.h"

RootTopic::RootTopic()
    :m_osid(0)
{
    m_last_sync_point.sync_point_id = syncpUknown;
};

extern bool isDefaultDBName(QString db_name);

QString RootTopic::title()const
{
    QString rv = "Sortbox";
    QString db_name = gui::currentDBName();
    if(!isDefaultDBName(db_name)){
            rv = db_name;
        }
    return rv;
}

void RootTopic::setTitle(QString, bool)
{
    ASSERT(0, "can't change title of synchronization object");
};
bool RootTopic::canBeMemberOf(topic_cptr)const{return false;};

#ifdef _DEBUG
void debug_print_hist_info(cdb* db, QString );
#endif

void RootTopic::assignSyncAtomicContent(const cit_primitive* _other)
{
    const RootTopic* other = dynamic_cast<const RootTopic*>(_other);
    assert_return_void(other, "expected topic");

//    m_cloudId = other->m_cloudId;
//    m_cloud_id_type = other->m_cloud_id_type;
};

bool RootTopic::isAtomicIdenticalTo(const cit_primitive* _other, int& )const
{
    assert_return_false(_other != nullptr, "expected item");
    const RootTopic* other = dynamic_cast<const RootTopic*>(_other);
    assert_return_false(other != nullptr, QString("expected item %1").arg(_other->dbgHint()));
    return true;
};

void RootTopic::initFromDB_SyncHistory(DB_ID_TYPE osid, COUNTER_TYPE db_id, COUNTER_TYPE mdc, SqlQuery& qDBHistory)
{
    ASSERT(IS_VALID_DB_ID(osid), "expected valid DBID");

    m_last_sync_point.sync_point_id = syncpUknown;
    m_osid = osid;


    bool lastPointIDinitialized = false;

    SYNC_HISTORY h;
    while ( qDBHistory.next() ) 
        {
            COUNTER_TYPE hist_db_id = qDBHistory.value(0).value<COUNTER_TYPE>();

            SHistEntry se;
            se.sync_db_id = hist_db_id;
            se.mod_counter = qDBHistory.value(1).value<COUNTER_TYPE>();
            se.last_sync_time = qDBHistory.value(2).value<time_t>();
            se.sync_point_id = syncpUknown;
            int tmp = qDBHistory.value(3).toInt();
            if(tmp > syncpUknown && tmp <= syncpDBox)
                {
                    se.sync_point_id = (SyncPointID)tmp;

                    if(!lastPointIDinitialized)
                        {
                            lastPointIDinitialized = true;
                            m_last_sync_point = se;
                        }
                }

            h[hist_db_id] = se;
        }

    m_sync_db.setupDb(db_id, mdc, h);
    /*
      #ifdef _DEBUG
      QString s = QString("[h-loaded:%1] lastSyncPoint=%2").arg(syncInfoAsString()).arg(m_last_sync_point.sync_point_id);
      debug_print_hist_info(&m_sync_db, s);
      #endif
    */
    ASSERT(m_sync_db.isValid(), "loaded invalid SDB");
};


bool RootTopic::ensureRootRecord()
{
    if(!db().isValid())
        {
            db().initDB();
            qDebug() << "initialized DB" << syncInfoAsString();
        }

    if(!IS_VALID_DB_ID(m_osid))
        {
            qDebug() << "creating RootRecord..";

            //we just assume one record in sync DB table
            //it's simplification from original design
            //the record ID number doesn't matter
            m_osid = 1;      

            return m_data_db->createRootSyncRecord(this);
        }  

    return true;
};

bool RootTopic::storeHistoryAfterSync(SyncPointID sid, RootTopic* siblingDB)
{
    if(m_data_db == NULL)
        {
            ASSERT(0, "expected valid m_data_db pointer");
            return false;
        }

    assert_return_false(IS_VALID_COUNTER(osid()), QString("expected valid oid: %1").arg(osid()));
    assert_return_false(IS_VALID_COUNTER(siblingDB->osid()), QString("expected valid oid").arg(siblingDB->osid()));
    assert_return_false(IS_VALID_COUNTER(db().db_id()), QString("expected valid SDB id:%1").arg(db().db_id()));
    assert_return_false(IS_VALID_COUNTER(siblingDB->db().db_id()), QString("expected valid SDB id:%1").arg(siblingDB->db().db_id()));

    COUNTER_TYPE sibling_db_id = siblingDB->db().db_id();
    SYNC_HISTORY::const_iterator i = m_sync_db.history().find(sibling_db_id);
    if(i == m_sync_db.history().end())
        {
            ASSERT(0, "history entry for sibling SDB not found.");
            return false;      
        }
    if(i != m_sync_db.history().end())
        {
            const SHistEntry& he = i->second;
            if(!dbp::updateSyncDBInfoAfterSync(sid, this, siblingDB->db().db_id(), he, m_data_db)) return false;
        }

    return true;
};

QString RootTopic::syncInfoAsString()const
{
    QString rv = "####";
    if(m_sync_db.isValid())
        {
            rv = QString("(%1/%2)").arg(m_sync_db.db_id())
                .arg(m_sync_db.db_mod_counter());
        }
    return rv;
};


QPixmap RootTopic::getSecondaryIcon(OutlineContext )const
{
    return getIcon_Sortbox();
}

IMPLEMENT_ROOT(dbp, MyDataRoot, "MyDataRoot", ard::topic);
IMPLEMENT_ROOT(dbp, ExportRoot, "Export", ard::topic);
IMPLEMENT_ROOT(dbp, EThreadsRoot, "Threads", ard::ethread);

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
#include "email_draft.h"
#include "ethread.h"
#include "contact.h"
#include "rule.h"
#include "kring.h"
#include "board.h"
#include "extnote.h"
#include "picture.h"

extern void clear_limbo_file_set();

bool dbp::updateItemAtomicContent(topic_ptr it, ArdDB* db)
{
    assert_return_false(it, "expected item");
    ASSERT(IS_VALID_DB_ID(it->id()), "updateItemAtomicContent - expected valid oid");


    QString sql = QString("UPDATE ard_tree SET todo=%1, todo_done=%2, retired=%3, title=?, annotation=? WHERE oid=%5")
        .arg(it->getToDoPriorityAsInt())
        .arg(it->getToDoDonePercent())
        .arg(it->mflag4serial())
        .arg(it->id());

    sqlPrint() << "upd-atomic:" << sql;

    auto q = db->prepareQuery(sql);
    if (!q) {
        return false;
    }
    q->addBindValue(it->serializable_title().trimmed());
    q->addBindValue(it->annotation().trimmed());
    if(!q->exec())
        {
            dbp::show_last_sql_err(*q, sql);
            return false;      
        };

    it->clear_persistance_request(np_ATOMIC_CONTENT);

    return true;
};

bool dbp::updateAtomicContent(ard::email_draft_ext* d, ArdDB* db)
{
    assert_return_false(d->cit_owner(), "expected owner");
    assert_return_false(IS_VALID_DB_ID(d->id()), "expected valid owner id");
    QString sql = QString("UPDATE ard_ext_draft SET email_eid=?, email_references=?, email_threadid=?, email_to=?, email_cc=?, email_bcc=?, attachments=?, attachments_host=?, labels=?, content_modified=? WHERE oid=%1")
        .arg(d->id());
    auto q = db->prepareQuery(sql);
    if (!q) {
        return false;
    }

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
    q->exec();
    d->clear_persistance_request(np_ATOMIC_CONTENT);
    return true;
};

bool dbp::updateAtomicContent(ard::contact_ext* pe, ArdDB* db)
{
    assert_return_false(pe->cit_owner(), "expected owner");
    auto oid = pe->id();
    assert_return_false(IS_VALID_DB_ID(oid), "expected valid owner id");

    QString sql = QString("UPDATE ard_ext_contact SET xml=? WHERE oid=%1")
        .arg(oid);

    auto q = db->prepareQuery(sql);
    if (!q) {
        return false;
    }
    q->addBindValue(pe->toXml());
    q->exec();
    pe->clear_persistance_request(np_ATOMIC_CONTENT);
    return true;
};

bool dbp::updateAtomicContent(ard::note_ext* pe, ArdDB* db) 
{
    assert_return_false(pe->cit_owner(), "expected owner");
    auto oid = pe->id();
    assert_return_false(IS_VALID_DB_ID(oid), "expected valid owner id");

    time_t mod_time = QDateTime::currentDateTime().toTime_t();
    QString sql = QString("UPDATE ard_ext_note SET note_text=?, note_plain_text=?,mod_time=? WHERE oid=%1")
        .arg(oid);

    auto q = db->prepareQuery(sql);
    if (!q) {
        return false;
    }
    q->addBindValue(pe->html());
    q->addBindValue(pe->plain_text());
    q->addBindValue(static_cast<uint>(mod_time));
    q->exec();
    pe->clear_persistance_request(np_ATOMIC_CONTENT);
    return true;
};

bool dbp::updateSentTime(ard::email_draft_ext* d, ArdDB* db) 
{
    assert_return_false(d->cit_owner(), "expected owner");
    assert_return_false(IS_VALID_DB_ID(d->id()), "expected valid owner id");
    assert_return_false(d->sentTime().isValid(), "expected valid sent time");
    time_t sent_time = d->sentTime().toTime_t();
    QString sql = QString("UPDATE ard_ext_draft SET sent_time=%1 WHERE oid=%2")
        .arg(sent_time)
        .arg(d->id());
    auto q = db->prepareQuery(sql);
    if (!q) {
        return false;
    }
    q->exec();
    return true;
};

/*
bool dbp::updateAtomicContent(anCloudIdExt* e, ArdDB* db) 
{
    assert_return_false(e->owner(), "expected owner");
    assert_return_false(IS_VALID_DB_ID(e->id()), "expected valid owner id");
    QString sql = QString("UPDATE ard_cloudid SET gdive_id=?, local_id=?, gtask_id=?, gtask_etag=?, gdive_id_originals=? WHERE oid=%1")
        .arg(e->id());
    auto q = db->prepareQuery(sql);
    if (!q) {
        return false;
    }
    q->addBindValue(e->gdriveId());
    q->addBindValue(e->localId());
    q->addBindValue(e->gtaskId());
    q->addBindValue(e->gtaskEtag());
    q->addBindValue(e->gdriveIdOriginals());
    q->exec();
    e->clear_persistance_request(np_ATOMIC_CONTENT);
    return true;
};*/

bool dbp::updateAtomicContent(ard::ethread_ext* e, ArdDB* db) 
{
    assert_return_false(e->cit_owner() != NULL, "expected owner");
    assert_return_false(IS_VALID_DB_ID(e->id()), "expected valid owner id");
    QString sql = QString("UPDATE ard_ext_ethread SET account_email='%1', thread_id='%2' WHERE oid=%3")
        .arg(e->accountEmail())
        .arg(e->threadId())
        .arg(e->id());
    auto q = db->prepareQuery(sql);
    if (!q) {
        return false;
    }
    q->exec();
    e->clear_persistance_request(np_ATOMIC_CONTENT);
    return true;
};

bool dbp::updateAtomicContent(ard::anKRingKeyExt* e, ArdDB* db) 
{
    assert_return_false(e->cit_owner(), "expected owner");
    assert_return_false(IS_VALID_DB_ID(e->id()), "expected valid owner id");
    QString sql = QString("UPDATE ard_ext_kring SET payload=? WHERE oid=%1")
        .arg(e->id());
    auto q = db->prepareQuery(sql);
    if (!q) {
        return false;
    }
    q->addBindValue(e->encryptedContent());
    q->exec();
    e->clear_persistance_request(np_ATOMIC_CONTENT);
    e->clearModified();
    return true;
};

bool dbp::updateAtomicContent(ard::board_ext* e, ArdDB* db)
{
    assert_return_false(e->cit_owner(), "expected owner");
    assert_return_false(IS_VALID_DB_ID(e->id()), "expected valid owner id");
    QString sql = QString("UPDATE ard_ext_bboard SET bands=? WHERE oid=%1")
        .arg(e->id());
    auto q = db->prepareQuery(sql);
    if (!q) {
        return false;
    }
    q->addBindValue(e->bandsPayload());
    q->exec();
    e->clear_persistance_request(np_ATOMIC_CONTENT);
    e->clearModified();
    return true;
};

bool dbp::updateAtomicContent(ard::board_item_ext* e, ArdDB* db)
{
    assert_return_false(e->cit_owner(), "expected owner");
    assert_return_false(IS_VALID_DB_ID(e->id()), "expected valid owner id");
    QString sql = QString("UPDATE ard_ext_bboard_item SET ref_syid='%1', bshape=%2, bindex=%3, ypos=%4, ydelta=%5 WHERE oid=%6")
        .arg(e->ref_topic_syid())
        .arg(static_cast<int>(e->bshape()))
        .arg(e->bandIndex())        
        .arg(e->yPos())
        .arg(e->yDelta())
        .arg(e->id());
    auto q = db->prepareQuery(sql);
    if (!q) {
        return false;
    }
    q->exec();
    e->clear_persistance_request(np_ATOMIC_CONTENT);
    return true;
};

bool dbp::updateAtomicContent(ard::picture_ext* e, ArdDB* db)
{
	assert_return_false(e->cit_owner(), "expected owner");
	assert_return_false(IS_VALID_DB_ID(e->id()), "expected valid owner id");
	QString sql = QString("UPDATE ard_ext_picture SET cloud_file_id=?, local_media_time=? WHERE oid=%1")
		.arg(e->id());
	auto q = db->prepareQuery(sql);
	if (!q) {
		return false;
	}
	q->addBindValue(e->cloud_file_id());
	q->addBindValue(static_cast<uint>(e->local_media_time()));
	q->exec();
	e->clear_persistance_request(np_ATOMIC_CONTENT);
	return true;
};

bool dbp::updateAtomicContent(ard::rule_ext* e, ArdDB* db)
{
	assert_return_false(e->cit_owner(), "expected owner");
	assert_return_false(IS_VALID_DB_ID(e->id()), "expected valid owner id");
	QString sql = QString("UPDATE ard_ext_rule SET words_in_subject=?, exact_phrase=?, from_list=?, exclusion_filter=? WHERE oid=%1")
		.arg(e->id());
	auto q = db->prepareQuery(sql);
	if (!q) {
		return false;
	}
	q->addBindValue(e->subjectStr());
	q->addBindValue(e->exact_phrase());
	q->addBindValue(e->fromStr());
	q->addBindValue(e->isExclusionFilter() ? 1 : 0);
	q->exec();
	e->clear_persistance_request(np_ATOMIC_CONTENT);
	return true;
};


bool dbp::updateItemPIndex(topic_ptr it, ArdDB* db)
{
    ASSERT(IS_VALID_DB_ID(it->id()), "dbp::updateItemPIndex - expected valid oid");
    DB_ID_TYPE pidx = it->pindex();
    ASSERT(IS_VALID_DB_ID(pidx), "dbp::updateItemPIndex - expected valid pindex");

    QString sql = QString("UPDATE ard_tree SET pindex=%1 WHERE oid=%2").arg(pidx).arg(it->id());
    //bool rv = db->execQuery(sql);  
    bool rv = false;
    if (db->execQuery(sql)) {
        rv = true;
    };
    it->clear_persistance_request(np_PINDEX);
    return rv;
};



extern QString get_SqlDatabase_info(ArdDB* db);


void dbp::updateBItemsYPos(ArdDB* db, ard::selector_board* b, const std::vector<ard::board_item_ext*>& lst)
{
    assert_return_void(db, "expected DB");

    QString sql = QString("UPDATE ard_ext_bboard_item SET ypos=? ");
    if (b->isSyncDbAttached()) {
        auto mdc = b->syncDb()->db_mod_counter() + 1;
        sql += QString(", mdc = %1").arg(mdc);
    }
    sql += " WHERE oid=?";
    auto q = db->prepareQuery(sql);
    if (!q) {
        qWarning() << "ERROR. Failed to prepare query board_link" << sql;
        return;
    }

    db->startTransaction();
    QVariantList ypos, oid;
    for (const auto& e : lst) {     
        ypos << e->yPos();
        oid << e->owner()->id();
    }

    q->addBindValue(ypos);
    q->addBindValue(oid);

    if (!q->execBatch())
    {
        db->rollbackTransaction();
        auto error = q->lastError().text();
        QString s = QString("%1\n%2").arg(error).arg(sql);
        qWarning() << s;
        return;
    }
    db->commitTransaction();
};

void dbp::updateBItemsBIndex(ArdDB* db, const std::vector<ard::board_item_ext*>& lst) 
{
    assert_return_void(db, "expected DB");
    QString sql = QString("UPDATE ard_ext_bboard_item SET bindex=? ");
    if (db->syncDb()) {
        auto mdc = db->syncDb()->db_mod_counter() + 1;
        sql += QString(", mdc = %1").arg(mdc);
    }
    sql += QString(" WHERE oid=?");
    auto q = db->prepareQuery(sql);
    if (!q) {
        qWarning() << "ERROR. Failed to prepare query board_link" << sql;
        return;
    }
    db->startTransaction();
    QVariantList bindex, oid;
    for (const auto& e : lst) {
        bindex << e->bandIndex();
        oid << e->owner()->id();
    }

    q->addBindValue(bindex);
    q->addBindValue(oid);
    if (!q->execBatch())
    {
        db->rollbackTransaction();
        auto error = q->lastError().text();
        QString s = QString("%1\n%2").arg(error).arg(sql);
        qWarning() << s;
        return;
    }
    db->commitTransaction();
};

void dbp::updateBLinksPIndex(ArdDB* db, const std::vector<ard::board_link*>& lst)
{
    assert_return_void(db, "expected DB");

    QString sql = QString("UPDATE ard_board_links SET pindex=? ");
    if (db->syncDb()) {
        auto mdc = db->syncDb()->db_mod_counter() + 1;
        sql += QString(", mdc = %1").arg(mdc);
    }

    sql += QString(" WHERE linkid=?");
    auto q = db->prepareQuery(sql);
    if (!q) {
        qWarning() << "ERROR. Failed to prepare query board_link" << sql;
        return;
    }

    db->startTransaction();

    QVariantList pindex, linkid;
    for (const auto& lnk : lst) {
        pindex << lnk->linkPindex();
        linkid << lnk->linkid();
    }

    q->addBindValue(pindex);
    q->addBindValue(linkid);

    if (!q->execBatch())
    {
        db->rollbackTransaction();
        auto error = q->lastError().text();
        QString s = QString("%1\n%2").arg(error).arg(sql);
        qWarning() << s;
        return;
    }
    db->commitTransaction();
};

template <class T>
static bool updateSyncInfo(T* obj, ArdDB* db, QString table_name, QString id_field_name)
{
    QString sql = QString("UPDATE %1 SET syid='%2', mdc=%3, mvc=%4 WHERE %5=%6").
        arg(table_name).
        arg(obj->syid()).
        arg(obj->modCounter()).
        arg(obj->moveCounter()).
        arg(id_field_name).
        arg(obj->id());
    db->execQuery(sql);
    obj->clear_persistance_request(np_SYNC_INFO);
  
    return true;
};

bool dbp::updateItemSyncInfo (topic_ptr it, ArdDB* db)
{
    assert_return_false(IS_VALID_DB_ID(it->id()), "expected valid oid");
    return updateSyncInfo(it, db, "ard_tree", "oid");
};

bool dbp::updateItemPOS(topic_ptr it, ArdDB* db)
{
    if(!IS_VALID_DB_ID(it->id()))
        {
            ASSERT(0, "updateItemPOS - expected valid oid");
            return false;
        }
    if(it->parent() == NULL)
        {
            ASSERT(0, "updateItemPOS - expected valid parent") << it->id();
            return false;
        }
    if(it->parent()->id() != 0 && 
       !IS_VALID_DB_ID(it->parent()->id()))
        {
            ASSERT(0, "updateItemPOS - expected valid parent oid") << it->id() << it->parent()->id();
            return false;
        }

    if(it->pindex() == -1)
        {
            ASSERT(0, "invalid index inside the parent") << it->id() << it->pindex();
            return false;
        }

    QString sql = QString("UPDATE ard_tree SET pid=%1, pindex=%2 WHERE oid=%3").arg(it->parent()->id()).arg(it->pindex()).arg(it->id());
    sqlPrint() << "update-item-POS: " << sql;
    db->execQuery(sql);
    it->clear_persistance_request(np_POS);
    it->clear_persistance_request(np_PINDEX);

    return true;
};

bool dbp::updateSyncDBInfoAfterSync(snc::SyncPointID sid, RootTopic* sdb, COUNTER_TYPE sibling_db_id, const SHistEntry& he, ArdDB* db)
{
    //should  store syncdb if NULL-->

    ASSERT(IS_VALID_DB_ID(sdb->osid()), "expected valid OID value of anSyncDB object.");
    ASSERT(IS_VALID_COUNTER(sdb->db().db_id()), "expected valid syncDB db_id");
    ASSERT(IS_VALID_COUNTER(sdb->db().db_mod_counter()), "expected valid syncDB mdc");
 
    time_t now = time(NULL);

    QString sql = QString("UPDATE ard_dbs SET mdc=%1 WHERE oid=%2").arg(sdb->db().db_mod_counter()).arg(sdb->osid());
    db->execQuery(sql);  

    sql = QString("SELECT count(synced_mdc) FROM ard_dbh WHERE db_oid=%1 AND synced_db_id=%2").arg(sdb->osid()).arg(sibling_db_id);
    int r = db->queryInt(sql);
    if(r == 0)
        {
            sql = QString("INSERT INTO ard_dbh(db_oid, synced_db_id, synced_mdc, last_sync_time, sync_point_id) VALUES(%1, %2, %3, %4, %5)").
                arg(sdb->osid()).
                arg(sibling_db_id).
                arg(he.mod_counter).
                arg(now).
                arg(sid);
            if(!db->execQuery(sql))
                {
                    qDebug() << "failed to insert sync history" << sql;
                    return false;
                }
        }
    else
        {
            sql = QString("UPDATE ard_dbh SET synced_mdc=%1, last_sync_time=%2, sync_point_id=%3 WHERE db_oid=%4 AND synced_db_id=%5").
                arg(he.mod_counter).
                arg(now).
                arg(sid).
                arg(sdb->osid()).
                arg(sibling_db_id);

            if(!db->execQuery(sql))
                {
                    qDebug() << "failed to store sync history" << sql;
                    return false;
                }      
        }

    return true;
};






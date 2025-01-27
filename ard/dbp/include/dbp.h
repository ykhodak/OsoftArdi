#pragma once

#include <QVariant>
#include <QSqlQueryModel>
#include "global-containers.h"

class QStandardItemModel;
class RootTopic;
class ArdDB;


/**
   dbp - some DB utility namespace
*/
namespace dbp
{  
    ::ArdDB&             defaultDB();

    ::RootTopic*           root();
    ard::topic*           threads_root();
    bool             create(QString dbName, QString pwd, QString hint);
    void             close(bool keepLastPWDInMemory = true);
    bool             openStandardPath(QString dbName);
    bool             safeGuiOpenStandardPath(QString dbName);
    bool             openAbsolutePath(QString dbPathName);
    QString             currentDBName();
    QString             compositRemoteDbPrefix4CurrentDB(QString& localDbPrefix);
    QString             lastUsedPWD();
    void                setLastUsedPWD(QString s);
    bool                reopenLast();

    bool          updateItemPIndex              (ard::topic* it, ArdDB* db);
    bool          updateItemSyncInfo            (ard::topic* it, ArdDB* db);
    bool          updateItemPOS                 (ard::topic* it, ArdDB* db);
    bool          updateItemAtomicContent       (ard::topic* it, ArdDB* db);

    bool          updateAtomicContent(ard::email_draft_ext* pe,  ArdDB* db);
    bool          updateSentTime(ard::email_draft_ext* pe,       ArdDB* db);
    bool          updateAtomicContent(ard::ethread_ext* pe,     ArdDB* db);
    bool          updateAtomicContent(ard::contact_ext* pe,       ArdDB* db);
    bool          updateAtomicContent(ard::anKRingKeyExt* pe, ArdDB* db);
    bool          updateAtomicContent(ard::board_ext* pe, ArdDB* db);
    bool          updateAtomicContent(ard::board_item_ext* pe, ArdDB* db);
    bool          updateAtomicContent(ard::note_ext* pe, ArdDB* db);
	bool          updateAtomicContent(ard::picture_ext* pe, ArdDB* db);
	bool          updateAtomicContent(ard::rule_ext* pe, ArdDB* db);

    void          updateBItemsYPos(ArdDB* db, ard::selector_board* b, const std::vector<ard::board_item_ext*>& lst);
    void          updateBItemsBIndex(ArdDB* db, const std::vector<ard::board_item_ext*>& lst);
    void          updateBLinksPIndex(ArdDB* db, const std::vector<ard::board_link*>& lst);

    bool                removeTopics            (ArdDB* ard_db, TOPICS_LIST& items_list);
    bool                removeBoards            (ArdDB* ard_db, TOPICS_LIST& blist);
    bool                removeBoardItemEx       (ArdDB* ard_db, TOPICS_LIST& blist);

    bool                removeQsByOID(ArdDB* db, IDS_SET ids);

    bool                removeDraftsByOID(ArdDB* db, IDS_SET ids);

    void                load_note           (ard::note_ext* n);

    bool          updateSyncDBInfoAfterSync (snc::SyncPointID sid, RootTopic* sdb, snc::COUNTER_TYPE sibling_db_id, const snc::SHistEntry& he, ArdDB* db);

    bool                create_board_links(ArdDB* db, QString board_syid, const std::vector<ard::board_link*>&);
    bool                remove_board_links(ArdDB* db, const std::vector<ard::board_link*>&);
    bool                update_board_links(ArdDB* db, const std::vector<ard::board_link*>&);

    void show_last_sql_err(QSqlQuery& q, QString sql);  
};

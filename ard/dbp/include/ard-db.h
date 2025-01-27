#pragma once

#include <functional>
#include <memory>
#include <QSqlDatabase>
#include "snc-cdb.h"
#include "global-enums.h"
#include "global-containers.h"
#include "global-struct.h"
#include "gcontact/GcontactRoutes.h"
#include "gcontact/GcontactCache.h"

class RootTopic;
class ArdDB;




namespace gui
{
    bool isDBAttached();
};


using SYID2TOPIC = std::unordered_map<QString, topic_ptr>;
typedef std::unordered_map<DB_ID_TYPE, topic_ptr>  ID_2_TOPIC;

typedef TOPICS_LIST CHILDREN;
typedef std::unordered_map<DB_ID_TYPE, topic_ptr> ID_2_FOLDER;
typedef std::unordered_map<DB_ID_TYPE, CHILDREN> ID_2_SUBTOPICS;

typedef bool(*FUN_IDS_DB_PROCESSOR)(ArdDB* db, IDS_LIST& nids, DB_ID_TYPE id_param, DB_ID_TYPE param2);
#define MAX_IN_SQL_LIST_STEP  200

class ArdDB :   public snc::persistantDB
{
    friend class QueryDB;
public:

    ArdDB();
    virtual ~ArdDB();

    bool                   openDb(QString dbName, QString dbPath);
    bool                   openAsMainDb(QString dbName, QString dbPath);
    void                   close();

    RootTopic*               root();
    const RootTopic*         root()const;
    bool                     hasRoot()const;
    RootTopic*               detachRoot();

    QSqlQuery*              selectQuery(QString sql);///@todo: see QSqlQuery::setForwardOnly for possible optimization
    QSqlQuery*              execQuery(QString sql);
    QSqlQuery*              prepareQuery(QString sql);
    int                     queryInt(QString sql);
    QString                 queryString(QString sql);
    bool                createTableIfNotExists(QString table_name, QString create_sql);
    bool                isOpen()const;
    bool                isDataLoaded()const {return m_data_loaded;}
    void                startTransaction();
    void                rollbackTransaction();
    void                commitTransaction();
    QString             connectionName()const;
    QString             databaseName()const;
    bool                backupDB(QString compressed_file_path)const;
    static bool         checkRegularBackup(bool force);
    static void         guiBackup();
    ///prepareForSync - function will load all data from DB
    void                      prepareForSync();
    ///findLocusFolder - will locate special singleton folder, can be GTD or other..
	ard::locus_folder*	findLocusFolder(EFolderType type);

    snc::cdb* syncDb();
    const snc::cdb* syncDb()const;

    
    topic_ptr						threads_root(){ return m_ethreads_root; }

    ard::contacts_model*			cmodel() { return   m_contacts_model.get(); };
    ard::kring_model*				kmodel() { return   m_kring_model.get(); };
    ard::boards_model*				boards_model() { return m_boards_model.get(); };
	ard::rules_model*				rmodel() { return m_rules_model.get(); }

    ard::local_search_observer*   local_search() { return m_local_search; }
    ard::rule_runner*				gmail_runner() {return m_gmail_runner;}
    ard::task_ring_observer*          task_ring() { return m_task_ring; }
    ard::comments_observer*          annotated() { return m_annotated; }
    ard::notes_observer*             notes() { return m_notes; }
	ard::bookmarks_observer*         bookmarks() { return m_bookmarks; }
	ard::pictures_observer*			pictures() { return m_pictures; }
    ard::color_observer*             colored() {return m_colored;}

    topic_ptr           lookupLoadedItem   (DB_ID_TYPE oid);
	TOPICS_LIST         lookupAndLockLoadedItemsList(const IDS_LIST& ids);

    void                pipeItems(QString oidSQL, TOPICS_LIST& items_list);
    bool          verifyMetaData     ();
    bool          loadTree           ();
    bool          storeNewTopic       (topic_ptr it);
    bool          createRootSyncRecord(RootTopic* sdb);

    void          insertNewExt(ard::email_draft_ext* d);
    void          insertNewExt(ard::ethread_ext* pe);
    void          insertNewExt(ard::contact_ext* pe);
    void          insertNewExt(ard::anKRingKeyExt* pe);
    void          insertNewExt(ard::board_ext* pe);
    void          insertNewExt(ard::board_item_ext* pe);
    void          insertNewExt(ard::note_ext* pe);
	void          insertNewExt(ard::picture_ext* pe);
	void          insertNewExt(ard::rule_ext* pe);

    bool          registerObj       (topic_ptr it);
    void          unregisterObj     (topic_ptr it);
    void          registerObjId     (topic_ptr it);

    const SYID2TOPIC&   syid2topic()const { return m_syid2topic; }
    bool                registerSyid(topic_ptr)const;

    RootTopic*    createRoot();

    void       calcContentHash(STRING_MAP&)const;
    size_t     lookupMapSize()const;
    ///cleanupDepending - find and cleanup all topics that might depend on given in list
    //void       cleanupDepending(TOPICS_SET depend_targets);
    //  void attachSynchronizable(snc::cit_base* o);
    //template <class T> 
    bool attach_data_db(topic_ptr);

    bool ensurePersistant();
    bool recreateDBIndexes(int& indexes_created);
    bool updateChildrenPIndexesInBatch(topic_ptr parent);
	ard::locus_folder*  createCustomFolderByName(QString name);

  bool sql_process_id_set(const IDS_SET& ids, FUN_IDS_DB_PROCESSOR ids_processor, DB_ID_TYPE id_param, DB_ID_TYPE param2);

  QSqlDatabase&      sqldb() { return m_db; }

  /**
    means it is regular gui-operated DB and not DB opened dyring sync.
    somewhat convoluted context.
  */
  bool          isMainDb()const { return m_mainDB; }

  ///return (dbid, dbid_mod_counter) if DB was nerer synced, we generate dbid but don't make it persistant
  ///this function is need to generate unique syid for new items even for DB that was never synced
  std::pair<snc::COUNTER_TYPE, snc::COUNTER_TYPE>   getSyncCounters4NewRegistration()const;

  int   convertNotes2NewFormat();
#ifdef _DEBUG
  void dbgPrint();
#endif
protected:
    //void            loadProjects();
    void            loadDraftEmails();
    void            loadEThreadsExt();
    void            loadCryptoExt();
    void            loadSingletons();
    void            runLoadSanityRules();
    void            linkThreadWrappers();

    void          loadSyncHistory();
    void          loadRootDetails();
    void          loadAllNotesText();
    void          clear();

    bool           isWeeklyBackupChecked()const{return m_WeeklyBackupChecked;}
    void           setWeeklyBackupChecked(bool val){m_WeeklyBackupChecked = val;}
    void			loadTopics();
    void			loadContacts();
    void			loadKRing();
    void			loadBoards();
    void			loadThreadRootSpace();
    void			loadNotesExt();
	void			loadPicturesExt();
	void			loadQFilters();

    template <class T, class O>
    int           loadExtensions(QString sql, IDS_SET& orfants);

    void            removeFromTableById(QString table_name, IDS_SET& orfants);

    template <class T, class O>
    void          loadAttachables(QString sql, IDS_SET& orfants);

    template <class T, class D>
    void loadReferences(QString sql, IDS_SET& orfant_oid_ref, IDS_SET& orfant_depend_oid_ref, QString referenceType);

    using objProducer = std::function<topic_ptr (EOBJ otype, EFolderType ftype)>;
    using wrapperResolver = std::function<bool (topic_ptr)>;

    void doLoadTree(ArdDB* adb, 
        topic_ptr root, 
        QString sql, 
        ID_2_SUBTOPICS& id2subitems, 
        ID_2_FOLDER& id2folder, 
        objProducer producer,
        wrapperResolver wresolver); 

protected:
    QSqlDatabase            m_db;
    ID_2_TOPIC              m_id2item;
    mutable SYID2TOPIC      m_syid2topic;
    TOPICS_LIST             m_serialize_orphants;
    TOPICS_LIST             m_serialize_duplicate_syid;
    THREAD_VEC              m_serialize_unresolvable_threads;

    bool                    m_data_loaded{ false };
    THREAD_VEC              m_threads_wrappers2link_after_load;

    bool                    m_mainDB;
    bool                    m_WeeklyBackupChecked;
    bool                    m_data_adjusted_during_db_load{false};

    RootTopic*              m_root{nullptr};
    topic_ptr               m_ethreads_root{ nullptr };
    std::unique_ptr<ard::contacts_model>        m_contacts_model;
    std::unique_ptr<ard::kring_model>           m_kring_model;
    std::unique_ptr<ard::boards_model>          m_boards_model;
	std::unique_ptr<ard::rules_model>			m_rules_model;
    ard::rule_runner*							m_gmail_runner{ nullptr };
	ard::local_search_observer*				m_local_search{nullptr};
    ard::task_ring_observer*                      m_task_ring{nullptr};
    ard::notes_observer*                         m_notes{ nullptr };
	ard::bookmarks_observer*                     m_bookmarks{ nullptr };
	ard::pictures_observer*                      m_pictures{ nullptr };
    ard::comments_observer*                      m_annotated{ nullptr };
    ard::color_observer*                         m_colored{ nullptr };
    std::unique_ptr<QSqlQuery>                  m_query{ nullptr };
private:
    ///clean up root so it would contain only special folders
    bool      pullOrdinaryTopicsOutOfRoot();
    ///collect orphants under Sortbox
    bool      adoptOrphants();
    ///move incompatible items into orphant list - sometime only at the end of data load we can
    ///determine that items are incompatible - due to some extra properties loaded - like projects can't be nested
    bool      cleanupIncompatible(cit_ptr parent);
    ///merge duplicate topics, the group merger is selected by TopicMatch
    bool      mergeDuplicates(topic_ptr parent, TopicMatch* m);
};

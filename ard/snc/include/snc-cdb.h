#pragma once


#include "snc.h"

namespace snc
{
  /**
     cdb - sync database object, usually attached
     to an owning topic, called CloudTopic, contains
     synchronization history and utility functions
   */

  class cdb
  {
    friend class cit;
    friend class attachable;
    friend class image;
    friend class comment;
    friend class dlink;
    friend class SyncProcessor;
    friend class SyncProgressStatus;
  public:
    cdb();
    ~cdb();

    COUNTER_TYPE db_mod_counter()const{return m_db_mod_counter;}
    COUNTER_TYPE db_id()const{return m_db_id;}
    void inc_db_mod_counter();
    void checkOnLargestDBModCounter();

    void setupDb(const COUNTER_TYPE& db_id, const COUNTER_TYPE& mod_counter, SYNC_HISTORY& history);
    void initDB();
    COUNTER_TYPE db_hist_mod_counter_of_db(cdb* db_other);
    bool isValid()const;

    const SYNC_HISTORY& history()const{return m_hist;}
    cit_ptr findItem(cit_ptr otherDBItem);
    cit_ptr findItemBySyid(const SYID& syid);
    
    void createSyidMap(SyncProcessor* proc);

    cit_ptr getSingleton(ESingletonType t);
    void registerSingletons(CITEMS& lst);

#ifdef _DEBUG
    bool verifyMaps();
#endif

  protected:
    bool prepareSync(SyncProcessor* proc, bool after_sync_sanity_test);
	bool checkTreeSanity();
    bool prepareSyncStep2(SyncProcessor* proc);
    void cleanupSync();
    bool processSyncModified(SyncProcessor* proc);
    bool processSyncMoved(SyncProcessor* proc);
    bool processSyncCompound(SyncProcessor* proc);
    bool processSyncDelete(SyncProcessor* proc);
    //bool processSyncDeleteAtt(SyncProcessor* proc);
    cit_ptr syncEnsureOtherDBTreeBranch(cit_ptr other_cit);
    void cleanModifications();
    void updateHistory(cdb* other_db);
    bool checkIdentity(SyncProcessor* proc);
    void registerSingleton(cit_ptr);

  protected:
    SYNC_HISTORY      m_hist;
    COUNTER_TYPE      m_db_id;
    COUNTER_TYPE      m_db_mod_counter;
    SYID_2_ITEM       m_syid2item;
    CITEMS            m_roots;
    SNGL_TYPE_2_CIT   m_sngl2topic;
  };

  class persistantDB
  {
  public:
    persistantDB(){};
    virtual ~persistantDB(){};
    virtual cdb* syncDb()=0;
    virtual const cdb* syncDb()const = 0;
    virtual void calcContentHash(STRING_MAP&)const = 0;
  };
};

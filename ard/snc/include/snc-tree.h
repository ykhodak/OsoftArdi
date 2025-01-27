#pragma once

#include "snc-assert.h"
#include "snc-misc.h"
#include "snc-meta.h"

namespace ard 
{
    class board_link;
    using BLINK_LIST = std::vector<board_link*>;
};

namespace snc
{
  class cit;

  class MemFindPipe
  {
  public:
    virtual ~MemFindPipe(){}
    virtual void pipe(cit_ptr ) = 0;
    ///we can skip exploring topic is it's already added as branch
    virtual bool skipTopic(cit_ptr )const{return false;}
  };
  
  class MemFindAllPipe: public MemFindPipe
  {
  public:
      CITEMS& items(){return m_items;}
    const CITEMS& items()const{return m_items;}
    virtual void pipe(cit_ptr it){m_items.push_back(it);}
  protected:
      CITEMS m_items;
  };

  class MemFindAllIndexedPipe: public MemFindAllPipe
  {
  public:
    MemFindAllIndexedPipe();
    void prepare(cit_ptr root);
    void pipe(cit_ptr it);
    CITEMS&  filterByKeyword(QString s);
    CITEMS&  keywordFiltered(){return m_keyword_filtered;}
    bool isFilterOn()const{return m_filterIsOn;}
  protected:
    DB_ID_TYPE  m_idx;
    QString     m_keyword;
    CITEMS      m_keyword_filtered;
    bool        m_filterIsOn;
  };


  class MemFindByStringPipe: public MemFindAllPipe
  {
  public:
    MemFindByStringPipe(QString keyword);

    virtual void pipe(cit_ptr it);
  protected:
    QString m_keyword;
  };

     
  class MemFindAllWithSYIDmap: public MemFindPipe
  {
  public:
      CITEMS& items(){return m_items;}
    const CITEMS& items()const{return m_items;}
    SYID_2_ITEM& syid2item(){return  m_syid2item;}
    const SYID_2_ITEM& syid2item()const{return  m_syid2item;}
    virtual void pipe(cit_ptr it);
  protected:
    CITEMS     m_items;
    SYID_2_ITEM  m_syid2item;
    };

  using LSYNC_INF = std::vector<LinkSyncInfo>;

  class SyncProgressStatus
  {
  public:
    void clearProgressStatus();
    void prepareProgressStatus(cdb* db1, cdb* db2);
    SyncProgressInfo&		find_info(const snc::SYID& syid);

	LSYNC_INF	m_cloned_lnk;
	LSYNC_INF	m_modified_lnk;
	LSYNC_INF	m_deleted_lnk;
  protected:
	std::vector<QString>    resulsAsCharMap()const;
    void flatoutTree(cit* it);
    SYID_2_SPINFO	m_info;
    int				m_idx;
  };

  /**
    CompoundObserver - builds maps & arrays out of cit-item derived
    compound objects for lookup, comparasing, etc.
  */
  struct CompoundInfo 
  {
	  int		size, compound_size;
	  QString	hashStr;
	  QString	toString()const;
  };
  

  class CompoundObserver
  {
  public:
    CompoundObserver(cit_ptr it, bool build_map = true);

    const PRIM_LIST&      prim_list()const{return m_prim_list;}
    const STRING_2_PRIM&  prim_map()const{return m_prim_map;}

  protected:
    bool          m_build_map;
    PRIM_LIST     m_prim_list;
    STRING_2_PRIM m_prim_map;    
  };

  /**
    SummaryBuilder - count number of subtopics, notes, images, etc.
  
  class SummaryBuilder
  {
  public:
      SummaryBuilder(cit_ptr topTopic);

      size_t    totalTopics()const{return m_totalTopics;}
      size_t    totalExtensions()const{return m_totalExtensions;}

  protected:
      size_t    m_totalTopics,
                m_totalExtensions;
  };*/
};


#pragma once

//#include "meta-utils.h"
#include "anfolder.h"

#define DECLARE_ROOT(N, T, S)                   \
  N(ArdDB* db, QString stitle = "");                         \
  bool   canAcceptChild   (const snc::cit* it)const override;    \
  bool   isRootTopic      ()const override{return true;}         \
  EOBJ   otype            ()const override{return T;}            \
  ESingletonType getSingletonType()const override{return S;}        \



#define IMPLEMENT_ROOT(S, T, N, C)                     \
  S::T::T(::ArdDB* db, QString stitle){setupAsRoot(db, stitle + QString(N));};           \
  bool S::T::canAcceptChild(cit_cptr it)const           \
  {                                 \
    bool rv = ard::topic::canAcceptChild(it);              \
  if(rv)                                \
    {                                   \
      const C* c = dynamic_cast<const C*>(it);              \
      if(!c)                         \
    return false;                           \
    }                                   \
  return rv;                                \
  };                                    \


class ArdDB;

namespace ard {
    //class contact;
    //class anContactGroup2;
};

/**
   RootTopic - we are topic that can be synchronized
   and items inside it all should maintain sync
   information. We should be root topic in database
   The "create" should not be overriden
   The RootTopic can't be created explicitely but through factory function when attaching to DB
*/

class RootTopic: public ard::topic
{
  friend class SyncPoint;
  friend class SupportWindow;
  friend RootTopic* ArdDB::createRoot();
public:
  QString               title                     ()const override;
  virtual void          setTitle                  (QString title, bool guiRecalc = false)override;
  QString               objName                   ()const override{return "SyncDB";};

  const snc::cdb&          db                        ()const{return m_sync_db;}
  snc::cdb&                db                        (){return m_sync_db;}

  DB_ID_TYPE			osid              ()const{return m_osid;}

  bool                ensureRootRecord();
  bool                isRootTopic      ()const override{return true;}

  virtual QPixmap     getSecondaryIcon    (OutlineContext c)const override;
  virtual QString     syncInfoAsString          ()const;
  virtual bool        canBeMemberOf             (topic_cptr parent_folder)const override;

  void                initFromDB_SyncHistory       (DB_ID_TYPE osid, COUNTER_TYPE db_id, COUNTER_TYPE mdc, SqlQuery& qDBHistory);
  bool                storeHistoryAfterSync     (SyncPointID sid, RootTopic* siblingDB);
  virtual bool        canBeCloned               ()const override{return false;}

  bool        canRename         ()const override{return false;}

  const SHistEntry&   lastSyncPoint()const{return m_last_sync_point;}
  void assignSyncAtomicContent(const cit_primitive* _other)override;
  bool isAtomicIdenticalTo(const cit_primitive* _other, int& iflags)const override;
//protected:
//    void requestVisibleItemsCalculation()const override;

protected:
  DB_ID_TYPE  m_osid;
  cdb         m_sync_db;
  SHistEntry  m_last_sync_point;
  mutable CITEMS      m_visible_items;

//private:
  RootTopic();
  void queryVisibleItems()const;
};


namespace dbp
{
    class MyDataRoot : public RootTopic
    {
    public:
        DECLARE_ROOT(MyDataRoot, objDataRoot, ESingletonType::dataRoot);
        QString     objName()const override { return "DataRoot"; };
    };

    class ExportRoot: public RootTopic
    {
    public:
        DECLARE_ROOT(ExportRoot, objFolder, ESingletonType::dataRoot);
        QString     objName()const override { return "ExportRoot"; };
    };

    class EThreadsRoot : public RootTopic
    {
    public:
        DECLARE_ROOT(EThreadsRoot, objEThreadsRoot, ESingletonType::etheadsRoot);
        QString     title()const override { return "tspace"; };
        QString     objName()const override { return "ThreadsRoot"; };
    };
};


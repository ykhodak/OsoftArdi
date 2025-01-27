#pragma once

#include <map>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <list>
#include <set>
#include <memory>
#include <stdint.h>
#include <QtGlobal>
#include <QMutex>
#include <QDebug>

#include "snc-enum.h"

typedef uint32_t DB_ID_TYPE;
#define INVALID_DB_ID             0
#define IS_VALID_DB_ID(V)         (V != INVALID_DB_ID)
#define PINDEX_STEP               1000
#define PINDEX_MIN_STEP           5
#define FREE_OBJ(O)               {if(O != NULL)delete(O);O=NULL;}

#ifndef QT_SPECIALIZE_STD_HASH_TO_CALL_QHASH
namespace std {
    template<> struct hash<QString> {
        std::size_t operator()(const QString& s) const {
            return qHash(s);
        }
    };
}
#endif

using STRING_SET		= std::unordered_set<QString>;
using STRING_LIST		= std::vector<QString>;
using STRING_MAP		= std::unordered_map<QString, QString>;
using ORD_STRING_SET	= std::set<QString>;

namespace snc
{
    class smart_counter;
    class image;
    class cit;
    class cit_primitive;
    class cit_base;
    class cit;
    class cdb;
    class reference;
    class extension;
    class SyncProcessor;
};

#define smart_counter_ptr         snc::smart_counter*
#define smart_counter_wptr        snc::smart_counter*
#define cit_ptr                   snc::cit*
#define cit_cptr                  const snc::cit*
#define cit_wptr                  snc::cit*
#define cit_prim_ptr              snc::cit_primitive*
#define cit_prim_cptr             const snc::cit_primitive*
#define cit_base_ptr              snc::cit_base*
#define cit_base_cptr             const snc::cit_base*
#define cit_extension_ptr         snc::extension*



namespace snc
{
  typedef uint32_t COUNTER_TYPE;
  typedef uint32_t TINDEX_TYPE;
  typedef QString SYID;


  /**
    smart_counter - basic smart counter class
    can be locked, released, some debug features
   */

  class smart_counter
  {
  public:
    smart_counter();
    virtual ~smart_counter();


    ///lock object, initial counter is 1
#ifdef _DEBUG
    void lock(QString location);
#else
    void lock();
#endif
    ///release, calls delete operator when counter reaches 0
    void release();

    ///returns counter value
    COUNTER_TYPE counter()const{return m_counter;}
    ///debug object information
    virtual QString dbgHint(QString s = "")const = 0;
    ///unregister - the last function to be called on live object
    ///before it gets deleted, for persistant object it might clean local cache map
    virtual void unregister(){};

#ifdef _DEBUG
    STRING_LIST&        lock_frames(){return m_lock_frames;}
    void                print_lock_frames();
    int                 magic_number()const{return m_magic_num;}
    QString				dbgMark()const { return m_dbg_mark; };
    void				setDbgMark(QString s) { m_dbg_mark = s; }
#endif

  protected:
      virtual void      final_release();

  private:
    COUNTER_TYPE m_counter;
#ifdef _DEBUG
	int         m_magic_num;
    STRING_LIST m_lock_frames;
    QString     m_dbg_mark;
#endif
  };



  struct SHistEntry
  {
    SHistEntry();
    COUNTER_TYPE sync_db_id;
    COUNTER_TYPE mod_counter;
    time_t       last_sync_time;
    SyncPointID  sync_point_id;
  };


  /// SyncProgressInfo - status info on synced item
  class SyncProgressInfo
  {
  public:
	  SyncProgressInfo();
	  SyncProgressInfo(int index);

	  bool isValid()const;
	  void markModLocal();
	  void markModRemote();
	  void markDelLocal();
	  void markDelRemote();
	  void markErr();
	  int  index()const;
	  bool isMarked()const;
	  SYNC_PROCESS_FLAG flags()const { return m_flags; }
  protected:
	  SYNC_PROCESS_FLAG m_flags;
  };

  struct LinkSyncInfo
  {
	  QString	origin_syid,
				target_syid,
				link_syid,
				str;
  };
  
#define INVALID_SYID ""
#define INVALID_COUNTER (uint32_t) (-1)

#define IS_VALID_SYID(V) (V.indexOf('-') != -1)
#define IS_VALID_COUNTER(V) (V != 0 && V != (uint32_t)-1)


  typedef std::unordered_map<COUNTER_TYPE, SHistEntry> SYNC_HISTORY;


  typedef std::set<cit_base_ptr>                CBASE_SET;
  typedef std::set<cit_ptr>                     CITEMS_SET;
  typedef std::vector<cit_ptr>                  CITEMS;
  
  typedef std::vector<cit_prim_ptr>          PRIM_LIST;
  typedef std::unordered_map<QString, cit_prim_ptr>  STRING_2_PRIM;

 
  typedef std::set<cit_extension_ptr>						CEXTENSIONS;
  typedef std::vector<cit_extension_ptr>					CEXTENSIONS_LIST;

  typedef std::unordered_map<SYID, TINDEX_TYPE>				SYID_2_TINDEX;
  typedef std::unordered_map<TINDEX_TYPE, SYID>				TINDEX_2_SYID;
  typedef std::unordered_map<SYID, cit_ptr>					SYID_2_ITEM;
  typedef std::unordered_map<SYID, SyncProgressInfo>		SYID_2_SPINFO;
  typedef std::set<SYID>									SYID_SET;
  typedef std::vector<SYID>									SYID_LIST;
  typedef std::map<ESingletonType, cit_ptr>					SNGL_TYPE_2_CIT;
};


QString formatDBID(DB_ID_TYPE);
QString formatSYID(snc::SYID);

#define tmpDebug() qDebug() << "D<<"

extern bool is_sync_broken();
extern void sync_log(QString s);
extern void sync_log(const STRING_LIST& string_list);
extern void sync_log_error(QString s);
extern QString sync_flags2string(int flags, const snc::cit_base* o = NULL, const snc::cit_base* other = NULL);
extern void on_identity_error(QString s);
extern bool areSyidEqual(snc::SYID id1, snc::SYID id2);


#define COMPARE_ATTR(A, F, I)if(A != other->A){QString s=QString("ident-err:%1 (%2)<=>(%3) [%4]").arg(sync_flags2string(I, this, other)).arg(A).arg(other->A).arg(dbgHint()); sync_log(s); F ^= I;on_identity_error(s); return false;}

#define COMPARE_SYID(O1, O2, F, I)if(!O1->isSyidIdenticalTo(O2)){QString s=QString("syid-ident-err:%1 [%4] [%5]").arg(sync_flags2string(I, O1, O2)).arg(O1->dbgHint()).arg(O2->dbgHint()); sync_log(s); F ^= I;on_identity_error(s); return false;}

#define COMPARE_EXT_ATTR(A)if(A != other->A){QString s=QString("ext-ident-err:%1 (%2)<=>(%3)").arg(extName()).arg(A).arg(other->A); sync_log(s); on_identity_error(s); return false;}


#ifdef _DEBUG
#define LOCK(o) o->lock(QString("%1:%2").arg(__FILE__).arg(__LINE__));
#else
#define LOCK(o) o->lock();
#endif


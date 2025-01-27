#pragma once

#include "snc.h"
#include "dbp.h"
#include "a-db-utils.h"

namespace ard
{
	template <class T, class O>
	class ardiExtension : public snc::extension
	{
		friend class ::ArdDB;
	public:
		ardiExtension() { m_flags.flags = 0; m_flags.is_persistant = 1; };
		O*					owner();
		const O*			owner()const;

		bool                hasDBRecord()const { return (m_flags.has_db_record == 1); }
		DB_ID_TYPE          id()const { return m_owner2 ? m_owner2->id() : 0; };
		bool                isPersistant()const { return m_flags.is_persistant == 1; }
		void                disablePersistance() { m_flags.is_persistant = 0; }
		bool				updateSyncInfo(ArdDB* db);
		virtual QString		dbTableName()const = 0;
		bool				ensureExtPersistant(ArdDB* db)override;
	protected:
		union EXT_FLAGS
		{
			uint8_t flags;
			struct
			{
				unsigned has_db_record : 1;
				unsigned is_persistant : 1;
			};
		} m_flags;
	};
}

template<class T, class O>
O*	ard::ardiExtension<T, O>::owner() 
{
	return dynamic_cast<O*>(cit_owner());
};

template<class T, class O>
const O* ard::ardiExtension<T, O>::owner()const
{
	return dynamic_cast<const O*>(cit_owner());
};

template<class T, class O>
bool ard::ardiExtension<T, O>::updateSyncInfo(ArdDB* db)
{
	DB_ID_TYPE oid = id();
	QString sql = QString("UPDATE %1 SET mdc=%2 WHERE oid=%4").
		arg(dbTableName()).
		arg(modCounter()).
		arg(oid);
	db->execQuery(sql);
	clear_persistance_request(snc::np_SYNC_INFO);
	return true;
};

template<class T, class O>
bool ard::ardiExtension<T, O>::ensureExtPersistant(ArdDB* db)
{
	assert_return_false(db, "expected DB");
	if (!isPersistant())return true;
	if (!hasDBRecord())
	{
		assert_return_false(cit_owner(), QString("expected owner: %1").arg(dbgHint()));
		db->insertNewExt(dynamic_cast<T*>(this));
		m_flags.has_db_record = 1;
	}
	else
	{
		if (need_persistance(snc::np_ATOMIC_CONTENT))
		{
			if (!dbp::updateAtomicContent(dynamic_cast<T*>(this), db))
				return false;
		}
		if (need_persistance(snc::np_SYNC_INFO))
		{
			if (!updateSyncInfo(db))
				return false;
		}
	}
	return true;
};



#define DECLARE_DB_EXTENSION_PERSISTANT(T, N, DBT) protected:           \
public:                                                                 \
 EOBJ_EXT ext_type()const override{return T;};                          \
 QString  extName()const override{return N;}                            \
 QString  dbTableName()const override{return DBT;}						\



/*
 bool ensureExtPersistant(ArdDB* db)override                            \
 {                                                                      \
    assert_return_false(db, "expected DB");                          \
    if (!isPersistant())return true;                                    \
     if(!hasDBRecord())                                                 \
         {                                                              \
             assert_return_false(cit_owner(), QString("expected owner: %1").arg(dbgHint())); \
             db->insertNewExt(this);                                    \
             m_flags.has_db_record = 1;                                 \
         }                                                              \
     else                                                               \
         {                                                              \
             if(need_persistance(np_ATOMIC_CONTENT))                    \
                 {                                                      \
                     if(!dbp::updateAtomicContent(this, db))            \
                         return false;                                  \
                 }                                                      \
             if(need_persistance(np_SYNC_INFO))                         \
                 {                                                      \
                     if(!updateSyncInfo(db))							\
                         return false;                                  \
                 }                                                      \
         }                                                              \
     return true;                                                       \
 };                                                                     \
private:                                                                \


*/

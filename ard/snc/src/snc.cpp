#include <iostream>
#include <time.h>
#include <QDateTime>
#include "snc.h"
#include "snc-cdb.h"
#include "snc-tree.h"

//@todo: we might want to wrap in macross STAT-functions-containers

#ifdef _DEBUG
static QString _checkSID;
#endif

#define SYNC_MAX_REG_COUNTER 16

using namespace snc;

extern bool sync_mod_register_enabled;

/**
   cdb & SyncProcessor
*/
extern bool has_ppos_field(int flags);
//extern bool has_content_field(int flags);



typedef std::map<cit_prim_ptr, int>         PRIM_2_FLAGS;
typedef std::map<cit_base_ptr, int>         CBASE_2_FLAGS;
typedef std::map<cit_ptr, int>              CITEM_2_FLAGS;

#define STAT_LIST(N)        CITEMS				N##_local, N##_remote;
#define STAT_EXTENSIONS(N)  CEXTENSIONS_LIST    N##_local, N##_remote;

//-- start here and finish! on print staff
#define ADD_STAT(C, X) if(it != nullptr){ if(it->syncDb() == m_db1_local) C ## _local.push_back(it); \
    else if(it->syncDb() == m_db2_remote) C ## _remote.push_back(it);   \
        else {ASSERT(0, "ERROR - inconsistend SyncDB attachement") << it->syncDb() << it->dbgHint();return; } \
    LOCK(it);                                                           \
    if(X)m_changes_detected = true;}                                     \
    else{                                                               \
        ASSERT(0, "extected object in ADD_STAT");                       \
    }                                                                   \



#define ADD_STAT_ATTACHABLE(C, a, X)   if(a != nullptr){ASSERT(a->cit_owner(), "expected owner"); \
    if(a->cit_owner()->syncDb() == m_db1_local) C ## _local.push_back(a); \
        else if(a->cit_owner()->syncDb() == m_db2_remote) C ## _remote.push_back(a); \
            else {ASSERT(0, "ERROR - inconsistend SyncDB attachement") << a->cit_owner()->syncDb() <<  a->dbgHint();return; } \
    LOCK(a);															 \
    if(X)m_changes_detected = true;}                                     \
    else{                                                               \
        ASSERT(0, "expected object in ADD_STAT_ATTACHABLE");            \
    }                                                                   \


#define ADD_FLAG(C, O, F)  if(O != nullptr){ASSERT((F != 0), "flags not defined");       \
    if(O->syncDb() == m_db1_local) C ## _local[O] = F;                  \
        else if(O->syncDb() == m_db2_remote) C ## _remote[O] = F;       \
            else {ASSERT(0, "ERROR - inconsistend SyncDB attachement") << O->syncDb() << O->dbgHint();return; } \
    }else{                                                  \
        ASSERT(0, "expected object in ADD_FLAG");           \
    }                                                       \


#define CLEAR_STAT(C)clear_locked_vector(C ## _local);clear_locked_vector(C ## _remote);
#define PRINT_STAT(T, C)print_stat_list(QString("local-") + T, C ## _local);print_stat_list(QString("remote-") + T, C ## _remote);if(is_sync_broken())return;

#define PRINT_STAT2(T, C, D)print_stat_list(QString("local-") + T, C ## _local, D);print_stat_list(QString("remote-") + T, C ## _remote, D);if(is_sync_broken())return;

#define PRINT_STAT3(T1, T2, C, D1, D2)print_stat_list(T1, C ## _local, D1);print_stat_list(T2, C ## _remote, D2);if(is_sync_broken())return;

#define PRINT_ITEMS_STAT_WITH_POS(T, C)print_item_stat_list_with_pos(QString("local-") + T, C ## _local);print_item_stat_list_with_pos(QString("remote-") + T, C ## _remote);if(is_sync_broken())return;

#define PRINT_IDENTITY_STAT(T, M, C)print_identity_list(QString("local-") + T, M##_local, C##_local);print_identity_list(QString("remote-") + T, M##_remote, C##_remote);if(is_sync_broken())return;

#define PRINT_PRIM_IDENTITY_STAT(T, M, C)print_prim_identity_list(QString("local-") + T, M##_local, C##_local);print_prim_identity_list(QString("remote-") + T, M##_remote, C##_remote);if(is_sync_broken())return;

#define PRG(N, O)if(O && m_progress) \
        {   if(O->isSynchronizable()){                    \
            auto si = m_progress->find_info(O->syid()); \
            assert_return_void(si.isValid(), QString("expected progress info [%1]").arg(O->syid())); \
            if(O->syncDb() == m_db1_local)                              \
                {                                                       \
                    si.mark##N##Local();                               \
                }                                                       \
            else                                                        \
                {                                                       \
                    si.mark##N##Remote();                              \
                }                                                       \
        }                                                               \
        }                                                               \


#define PRG_ERR(O) if(O && m_progress)                                  \
        {   if(O->isSynchronizable()){                                  \
                auto si = m_progress->find_info(O->syid()); \
                assert_return_void(si.isValid(), "expected progress info"); \
                si.markErr();                                              \
            }                                                               \
        }                                                                   \


#define PRG_PRIM(N, O)if(O && O->cit_owner())PRG(N, O->cit_owner());

#define PRG_ERR_PRIM(O)if(O && O->cit_owner())PRG_ERR(O->cit_owner());

namespace snc
{
    class SyncProcessor
    {
    public:
        SyncProcessor(bool first_pass, QString hint, snc::SyncProgressStatus* progressStatus)
            :m_db1_local(NULL), m_db2_remote(NULL),  m_changes_detected(false), m_first_pass(first_pass), m_hint(hint), m_progress(progressStatus){}
        ~SyncProcessor(){clear();}
        bool syncDB(snc::persistantDB* p1, snc::persistantDB* p2);
        void print_stat(snc::SyncProgressStatus*);

        cdb* contraDB(const snc::cit_primitive* p);
        COUNTER_TYPE comparatorCounter(const snc::cit_primitive* p)const;
        snc::persistantDB* contraP(const snc::cit_primitive* p);
        bool areContraItems(const cit_ptr it1, const cit_ptr it2);
    
        bool detected_changes()const{return m_changes_detected;}
        bool is_local(const cit* it){return (it->syncDb() == m_db1_local);};

        void process_modified_assigment(cit_ptr otherDBobj, cit_ptr ourDBobj);
        void process_modified_assigment_ext(cit_extension_ptr otherDBobj, cit_extension_ptr ourDBobj);
        bool complete_item_clone(cit_ptr the_clone, cit_ptr it_source);


        bool complete_extension_clone(cit_extension_ptr the_clone, cit_extension_ptr source);
        void stat_on_assigned(cit_ptr);
        void stat_on_cloned(cit_ptr);
        void stat_on_moved(cit_ptr);
        void stat_on_deleted(cit_ptr);
        void stat_on_ambiguous(cit_ptr);
        void stat_on_identity_lost(cit_ptr);
        void stat_on_identity_err(cit_ptr, cit_ptr, int);
        void stat_on_err_dupe_syid(cit_ptr);
        void stat_on_identity_pos_err_resolved(cit_ptr);
        void stat_on_identity_err_assigned(cit_ptr);

        void stat_on_sync_orphant(cit_ptr);


        void stat_on_assigned(cit_extension_ptr);
        void stat_on_ambiguous(cit_extension_ptr);

    protected:
        void clear();
        void cleanupIdentityResolutionMaps();
        bool syncLoop();
        bool sanityTest(bool after_sync);
        void process_local_db_identity_resolution();
        void verifyMaps();

    protected:
        cdb  *m_db1_local, *m_db2_remote;
        snc::persistantDB *m_db1_local_p, *m_db2_remote_p;
        COUNTER_TYPE m_db1_local_hist_mod_counter, m_db2_remote_hist_mod_counter;
        SYID_SET      m_resolve_assign_set,
            m_resolve_del_set, 
            m_resolve_moved_set;
        bool          m_changes_detected, m_first_pass;
        QString       m_hint;
        snc::SyncProgressStatus* m_progress;

        CBASE_SET m_content_assigned;
        CITEM_2_FLAGS   m_localdb_identity_err_4resolution;

        STAT_LIST(m_stat_assigned);
        STAT_LIST(m_stat_cloned);
        STAT_LIST(m_stat_moved);
        STAT_LIST(m_stat_deleted);
        STAT_LIST(m_stat_ambiguous_mod);
        STAT_LIST(m_stat_identity_lost);
        STAT_LIST(m_stat_identity_err);
        STAT_LIST(m_stat_dupe_syid);
        STAT_LIST(m_stat_identity_pos_err_resolved);
        STAT_LIST(m_stat_identity_err_assigned);
        STAT_LIST(m_stat_sync_orphant);

        CBASE_2_FLAGS m_stat_map_identity_err_local, m_stat_map_identity_err_remote;


        STAT_EXTENSIONS(m_stat_assigned_extension);
        STAT_EXTENSIONS(m_stat_ambiguous_extension);

        CBASE_2_FLAGS   m_stat_map_identity_err_img_local, 
            m_stat_map_identity_err_img_remote;

        PRIM_2_FLAGS m_stat_map_identity_err_prim_local,
            m_stat_map_identity_err_prim_remote;
    };
};

cdb* SyncProcessor::contraDB(const snc::cit_primitive* p)
{
    cdb* rv = NULL;
    const cdb* db = p->syncDb();
    if (db == m_db1_local){
        rv = m_db2_remote;
    }
    else if (db == m_db2_remote){
        rv = m_db1_local;
    }
    else
        {
            ASSERT(0, "invalid syncDB pointer") << p->dbgHint();
        }

    return rv;
};

snc::persistantDB* SyncProcessor::contraP(const snc::cit_primitive* p)
{
    snc::persistantDB* rv = NULL;
    const cdb* db = p->syncDb();
    if (db == m_db1_local){
        rv = m_db2_remote_p;
    }
    else if (db == m_db2_remote){
        rv = m_db1_local_p;
    }
    else
        {
            ASSERT(0, "invalid syncDB pointer") << p->dbgHint();
        }
  
    return rv;
};

bool SyncProcessor::areContraItems(const cit_ptr it1, const cit_ptr it2)
{
    bool rv = false;
	assert_valid(it1);
	assert_valid(it2);
    const cdb* db1 = it1->syncDb();
    const cdb* db2 = it2->syncDb();
	assert_return_false(db1 != NULL, "expected DB");
	assert_return_false(db2 != NULL, "expected DB");
	assert_return_false(db1 != db2, "expected different DB");
	assert_return_false(it1->otype() == it2->otype(), QString("expected same otype [%1] [%2]").arg(it1->dbgHint()).arg(it2->dbgHint()));
    bool as_sigleton = it1->isSingleton();
    if(as_sigleton)
        {
			assert_return_false(it2->isSingleton(), QString("expected singleton [%1] [%2]").arg(it1->dbgHint()).arg(it2->dbgHint()));
            rv = (it1->getSingletonType() == it2->getSingletonType());
        }
    else
        {
            rv = areSyidEqual(it1->syid(), it2->syid());
        }  
    return rv;
};

COUNTER_TYPE SyncProcessor::comparatorCounter(const snc::cit_primitive* p)const
{
    COUNTER_TYPE rv = 0;
    const cdb* db = p->syncDb();
    if (db == m_db1_local){
        rv = m_db1_local_hist_mod_counter;
    }
    else if (db == m_db2_remote){
        rv = m_db2_remote_hist_mod_counter;
    }
    else
        {
            ASSERT(0, "invalid syncDB pointer") << p->dbgHint();
        }
    return rv;
};

extern void print_stat_header(QString s);


template <class T>
static void print_stat_list(QString title, std::vector<T*>& lst, QString desc = "")
{
    if(!lst.empty())
        {
            int list_size = lst.size();
            QString s = QString("%1 [%2]").arg(title).arg(list_size);
            print_stat_header(s);

            if(!desc.isEmpty())
                {
                    sync_log(desc);
                }

            STRING_LIST string_list;
            int i = 1;
            int fieldSize = 1;
            if(list_size > 9)fieldSize=2;
            if(list_size > 99)fieldSize=3;
            if(list_size > 999)fieldSize=4;
            typedef typename std::vector<T*>::iterator ITR;
            for(ITR k = lst.begin(); k != lst.end(); k++)
                {
                    T* o = *k;
                    QString s = QString("%1.%2").arg(i, fieldSize).arg(o->dbgHint());
                    string_list.push_back(s);
                    i++;
                }
            sync_log(string_list);
        }
}

extern void print_item_stat_list_with_pos(QString title, CITEMS& lst, QString desc = "");


template <class T>
static void print_identity_list(QString title, CBASE_2_FLAGS b2flags, std::vector<T*>& lst)
{
    if(!lst.empty())
        {
            QString s = QString("%1 [%2]").arg(title).arg(lst.size());
            print_stat_header(s);

            STRING_LIST string_list;
            typedef typename std::vector<T*>::iterator ITR;
            for(ITR k = lst.begin(); k != lst.end(); k++)
                {
                    T* o = *k;
                    QString s = QString("%1").arg(o->dbgHint());

                    CBASE_2_FLAGS::iterator z = b2flags.find(o);
                    if(z != b2flags.end())
                        {
                            s += " flags:";
                            s += sync_flags2string(z->second);
                        }
                    else
                        {
                            ASSERT(0, "identity flags not defined") << b2flags.size() << o->dbgHint();
                        }
                    string_list.push_back(s);
                }//for
            sync_log(string_list);
        }
}

template <class T>
static void print_prim_identity_list(QString title, PRIM_2_FLAGS b2flags, std::vector<T*>& lst)
{
    if(!lst.empty())
        {
            QString s = QString("%1 [%2]").arg(title).arg(lst.size());
            print_stat_header(s);

            STRING_LIST string_list;
            typedef typename std::vector<T*>::iterator ITR;
            for(ITR k = lst.begin(); k != lst.end(); k++)
                {
                    T* o = *k;
                    QString s = QString("%1").arg(o->dbgHint());

                    PRIM_2_FLAGS::iterator z = b2flags.find(o);
                    if(z != b2flags.end())
                        {
                            s += " flags:";
                            s += sync_flags2string(z->second);
                        }
                    else
                        {
                            ASSERT(0, "identity flags not defined") << b2flags.size() << o->dbgHint();
                        }
                    string_list.push_back(s);
                }//for
            sync_log(string_list);
        }
}



#define ADD_STAT_EXT ADD_STAT_ATTACHABLE

void SyncProcessor::stat_on_assigned(cit_ptr it)
{
	PRG(Mod, it);
	ADD_STAT(m_stat_assigned, true);
};
void SyncProcessor::stat_on_cloned(cit_ptr it){ADD_STAT(m_stat_cloned, true);};
void SyncProcessor::stat_on_moved(cit_ptr it){PRG(Mod, it);ADD_STAT(m_stat_moved, true);};
void SyncProcessor::stat_on_deleted(cit_ptr it){PRG(Del, it);ADD_STAT(m_stat_deleted, true);};
void SyncProcessor::stat_on_ambiguous(cit_ptr it){PRG_ERR(it);ADD_STAT(m_stat_ambiguous_mod, true);};
void SyncProcessor::stat_on_identity_lost(cit_ptr it){PRG_ERR(it);ADD_STAT(m_stat_identity_lost, true);};
void SyncProcessor::stat_on_identity_pos_err_resolved(cit_ptr it){PRG_ERR(it);ADD_STAT(m_stat_identity_pos_err_resolved, true);};
void SyncProcessor::stat_on_identity_err_assigned(cit_ptr it){PRG_ERR(it);ADD_STAT(m_stat_identity_err_assigned, true);};

void SyncProcessor::stat_on_identity_err(cit_ptr it, cit_ptr other, int flags)
{
#ifdef _DEBUG
    ASSERT(!it->isOnLooseBranch(), "loose branch item") << it->dbgHint();
    ASSERT(!other->isOnLooseBranch(), "loose branch item") << it->dbgHint();
#else
    Q_UNUSED(other);
#endif

    if(it->syncDb() == m_db1_local)
        {
            m_localdb_identity_err_4resolution[it] = flags;
            LOCK(it);
            m_changes_detected = true;
        }

    ADD_STAT(m_stat_identity_err, false);
    ADD_FLAG(m_stat_map_identity_err, it, flags);
};

void SyncProcessor::stat_on_err_dupe_syid(cit_ptr it){ADD_STAT(m_stat_dupe_syid, true);};

void SyncProcessor::stat_on_sync_orphant(cit_ptr it){ADD_STAT(m_stat_sync_orphant, true);};


void SyncProcessor::stat_on_assigned(cit_extension_ptr e)
{
	ADD_STAT_EXT(m_stat_assigned_extension, e, true);
};

void SyncProcessor::stat_on_ambiguous(cit_extension_ptr e){ADD_STAT_EXT(m_stat_ambiguous_extension, e, true);};

void SyncProcessor::clear()
{
    CLEAR_STAT(m_stat_assigned);
    CLEAR_STAT(m_stat_cloned);
    CLEAR_STAT(m_stat_deleted);
    CLEAR_STAT(m_stat_moved);
    CLEAR_STAT(m_stat_ambiguous_mod);
    CLEAR_STAT(m_stat_identity_lost);
    CLEAR_STAT(m_stat_identity_err);
    CLEAR_STAT(m_stat_assigned_extension);
    CLEAR_STAT(m_stat_ambiguous_extension);

    CLEAR_STAT(m_stat_dupe_syid);
    CLEAR_STAT(m_stat_sync_orphant);

    CLEAR_STAT(m_stat_identity_pos_err_resolved);
    CLEAR_STAT(m_stat_identity_err_assigned);

    cleanupIdentityResolutionMaps();

#ifdef _DEBUG
    _checkSID = "";
#endif
};

void SyncProcessor::cleanupIdentityResolutionMaps()
{
    for(CITEM_2_FLAGS::iterator i = m_localdb_identity_err_4resolution.begin();i != m_localdb_identity_err_4resolution.end();i++)
        {
            cit_ptr it = i->first;
            it->release();
        }
    m_localdb_identity_err_4resolution.clear();

};


void SyncProcessor::print_stat(snc::SyncProgressStatus* p)
{
	print_stat_header("");
	if (!m_changes_detected)
	{
		if (m_first_pass)
		{
			sync_log(QString("    NO CHANGES DETECTED - DB IN SYNC %1     ").arg(m_hint));
		}
		else
		{
			sync_log(QString("    COMPLETED IN 1ST PASS - DB IN SYNC  %1    ").arg(m_hint));
		}
	}

	//  if(m_changes_detected)
	{
		PRINT_STAT("assigned", m_stat_assigned);
		PRINT_STAT3("cloud->local cloned",
			"local->cloud cloned",
			m_stat_cloned,
			"Topics were copied from cloud to local database",
			"Topics were copied from local database to cloud");
		PRINT_ITEMS_STAT_WITH_POS("moved", m_stat_moved);
		PRINT_STAT3("local-deleted",
			"remote-deleted",
			m_stat_deleted,
			"Topics deleted localy",
			"Topics deleted on the cloud");
		PRINT_STAT("ambiguous", m_stat_ambiguous_mod);


		PRINT_STAT("assigned-ext", m_stat_assigned_extension);
		PRINT_STAT("ambiguous-ext", m_stat_ambiguous_extension);

		PRINT_STAT2("identity-lost", m_stat_identity_lost, "The listed items are missing from the opposite SYNCDB. So local-identity-lost means the items are in local DB but not in remote.");
		PRINT_IDENTITY_STAT("identity-err", m_stat_map_identity_err, m_stat_identity_err);
		PRINT_STAT("identity-err-pos-resolved", m_stat_identity_pos_err_resolved);
		PRINT_STAT("identity-err-reassigned", m_stat_identity_err_assigned);

		PRINT_STAT("err-duplicate-syid", m_stat_dupe_syid);
		PRINT_STAT("force-moved-mark", m_stat_sync_orphant);
	}

	std::function<void(QString, snc::LSYNC_INF&)> print_links = [](QString title, snc::LSYNC_INF& lst) 
	{
		if (!lst.empty())
		{
			QString s = QString("%1 [%2]").arg(title).arg(lst.size());
			print_stat_header(s);
			for (auto& i : lst) {
				QString s = QString("%1, %2, %3").arg(i.origin_syid, i.target_syid, i.link_syid);
				sync_log(s);
			}
		}
	};

	if (p) {
		print_links("cloned links", p->m_cloned_lnk);
		print_links("assigned links", p->m_modified_lnk);
		print_links("deleted links", p->m_deleted_lnk);
	}
	print_stat_header("");
};


void SyncProcessor::process_modified_assigment(cit_ptr otherDBobj, cit_ptr ourDBobj)
{
    // COUNTER_TYPE other_hist_mod_counter = other_hist_counter();
    //  bool other_modified = (otherDBobj->modCounter() > other_hist_mod_counter);
    bool other_modified = (otherDBobj->modCounter() > comparatorCounter(otherDBobj));
    if(other_modified)
        {
            ourDBobj->m_fsync.sync_ambiguous_mod = 1;
            otherDBobj->m_fsync.sync_ambiguous_mod = 1;
            ourDBobj->syncStatOnAmbiguous(this, otherDBobj);
        }
    otherDBobj->assignSyncAtomicContent(ourDBobj);
    otherDBobj->m_fsync.sync_modified_processed = 1;
    otherDBobj->m_mod_counter = otherDBobj->syncDb()->db_mod_counter() + 1;
    otherDBobj->ask4persistance(np_SYNC_INFO);
    otherDBobj->syncStatAfterAssigned(this, ourDBobj);
};

void SyncProcessor::process_modified_assigment_ext(cit_extension_ptr otherDBobj, cit_extension_ptr ourDBobj)
{
    //  COUNTER_TYPE other_hist_mod_counter = other_hist_counter();
    //  bool other_modified = (otherDBobj->modCounter() > other_hist_mod_counter);
    bool other_modified = (otherDBobj->modCounter() > comparatorCounter(otherDBobj));
    if(other_modified)
        {
            ourDBobj->m_fsync.sync_ambiguous_mod = 1;
            otherDBobj->m_fsync.sync_ambiguous_mod = 1;
            stat_on_ambiguous(otherDBobj);
        }
    otherDBobj->assignSyncAtomicContent(ourDBobj);
    otherDBobj->m_fsync.sync_modified_processed = 1;
    otherDBobj->m_mod_counter = otherDBobj->syncDb()->db_mod_counter() + 1;
    otherDBobj->ask4persistance(np_SYNC_INFO);
	otherDBobj->syncStatAfterAssigned(this, ourDBobj);
    //stat_on_assigned(otherDBobj);
};


bool SyncProcessor::complete_item_clone(cit_ptr the_clone, cit_ptr it_source)
{
    cdb* db = the_clone->syncDb();
	assert_return_false(db, QString("failed to item clone, expected valid sdb %1").arg(the_clone->dbgHint()));
	assert_return_false(the_clone->cit_parent(), QString("failed to item clone, expected valid parent %1").arg(the_clone->dbgHint()));


    SYID_2_ITEM::iterator k = db->m_syid2item.find(the_clone->syid());
    if(k != db->m_syid2item.end())
        {
            ASSERT(0, "can't create clone - item with the same SYID already exists in destination DB");
            return false;
        }


    it_source->m_fsync.sync_modified_processed = 1;
    it_source->m_fsync.sync_moved_processed = 1;

    db->m_syid2item[the_clone->syid()] = the_clone;
    stat_on_cloned(the_clone);
    ///PRG-macross won't work for new/cloned items as they don't have userPointer as SyncProgressInfo
    if (it_source && m_progress) {
        auto si = m_progress->find_info(it_source->syid());
		assert_return_false(si.isValid(), "expected progress info");
        if (it_source->syncDb() == m_db1_local) {
            si.markModRemote();
        }
        else {
            si.markModLocal();
        }
    }
    return true;
};

bool SyncProcessor::complete_extension_clone(cit_extension_ptr the_clone, cit_extension_ptr source)
{
    the_clone->m_fsync.sync_modified_processed = 1;
    the_clone->m_fsync.sync_moved_processed = 1;
    source->m_fsync.sync_modified_processed = 1;
    source->m_fsync.sync_moved_processed = 1;
   // stat_on_cloned_prim(the_clone);
    return true;
};

#ifdef _DEBUG
void debug_print_hist_info(cdb* db, QString prefix);
#endif

extern bool synchronizeBoardLinks(snc::persistantDB* p1,
	snc::persistantDB* p2,
	COUNTER_TYPE db1_local_hist_mod_counter,
	COUNTER_TYPE db2_remote_hist_mod_counter,
	snc::SyncProgressStatus* progressStatus);

bool do_sync_2SDB(snc::persistantDB* p1_local, 
				snc::persistantDB* p2_remote, 
				bool& changes_detected, 
				bool firstPass, 
				QString hint, 
				snc::SyncProgressStatus* progressStatus)
{
    cdb* db1 = p1_local->syncDb();
    cdb* db2 = p2_remote->syncDb();

    if(db1->db_id() == db2->db_id())
        {
            sync_log_error(QString("can not sync DB with duplicate ids. One DB has to be recreated. [%1] [%2]").arg(db1->db_id()).arg(db2->db_id()));
            return false;
        }

    QString db1title = db1->getSingleton(ESingletonType::dataRoot)->title();
    QString db2title = db2->getSingleton(ESingletonType::dataRoot)->title();

    if(db1title.compare(db2title, Qt::CaseInsensitive) != 0)
        {
            sync_log_error(QString("pairing SYNCDB have different titles '%1' vs '%2'")
                           .arg(db1title)
                           .arg(db2title));
            return false;
        }

    QString s = QString("begin sync on local %1[%2 %3] remote %4[%5 %6]")
        .arg(db1title)
        .arg(db1->db_id())
        .arg(db1->db_mod_counter())
        .arg(db2title)
        .arg(db2->db_id())
        .arg(db2->db_mod_counter());

    sync_log(s);

    sync_mod_register_enabled = false;
    SyncProcessor proc(firstPass, hint, progressStatus);
    bool rv = proc.syncDB(p1_local, p2_remote);
    changes_detected = proc.detected_changes();

    proc.print_stat(progressStatus);
    sync_mod_register_enabled = true;
    return rv;
}

static bool printContentHash(snc::persistantDB* p1, snc::persistantDB* p2, STRING_MAP& compareHash)
{
	bool rv = true;
	ASSERT(p1 != p2, "invalid parameters provided");

	STRING_MAP hmap_db1;
	STRING_MAP hmap_db2;
	p1->calcContentHash(hmap_db1);
	p2->calcContentHash(hmap_db2);
	for (STRING_MAP::iterator i = hmap_db1.begin(); i != hmap_db1.end(); i++)
	{
		STRING_MAP::iterator j = hmap_db2.find(i->first);
		ASSERT(j != hmap_db2.end(), "expected hash map entry") << i->first;
		if (j != hmap_db2.end())
		{
			QString hres;
			if (i->second.compare(j->second) == 0)
			{
				auto s1 = QString("%1-hash-OK").arg(i->first);
				hres = QString("%1%2").arg(s1, -32).arg(i->second.left(32));
			}
			else
			{
				auto s1 = QString("%1-hash-ERROR").arg(i->first);
				hres = QString("%1%2 %3").arg(s1, -32).arg(i->second).arg(j->second);
				rv = false;
			}
			compareHash[i->first] = hres;
			sync_log(hres);
		}
	}

	return rv;
};

bool sync_2SDB(snc::persistantDB* p1_local, 
    snc::persistantDB* p2_remote, 
    bool& changes_detected, 
    bool& hashOK, 
    QString hint, 
    STRING_MAP& hashCompareResult, 
    snc::SyncProgressStatus* progressStatus)
{
    if(progressStatus)
        {
            progressStatus->clearProgressStatus();
        }
  
    hashOK = false;

    cdb* db1_local = p1_local->syncDb();
    cdb* db2_remote = p2_remote->syncDb();

    bool rv = do_sync_2SDB(p1_local, p2_remote, changes_detected, true, hint, progressStatus);
    if(rv)
        {
            if(changes_detected)
                {
                    QString db1title = db1_local->getSingleton(ESingletonType::dataRoot)->title();
                    QString db2title = db2_remote->getSingleton(ESingletonType::dataRoot)->title();
                    QString msg = QString("2nd path sync on local %1[%2 %3] remote %4[%5 %6]")
                        .arg(db1title)
                        .arg(db1_local->db_id())
                        .arg(db1_local->db_mod_counter())
                        .arg(db2title)
                        .arg(db2_remote->db_id())
                        .arg(db2_remote->db_mod_counter());
      
                    sync_log("started " + msg);
                    rv = do_sync_2SDB(p1_local, p2_remote, changes_detected, false, hint, progressStatus);
                    if(rv)
                        {
                            QString changes_res = changes_detected ? "(need-resync)" : "(completed)";
                            sync_log(QString("finished%1 ").arg(changes_res) + msg);
                        }
                    else
                        {
                            sync_log("error " + msg);
                        }

                    changes_detected = true;//we know it's been changed even first time
                }

            hashOK = printContentHash(p1_local, p2_remote, hashCompareResult);
        }
    return rv;
}

bool cdb::processSyncModified(SyncProcessor* proc)
{
	for (auto f : m_roots) {
		if (!f->processSyncModified(proc)) {
			ASSERT(0, "failed cdb::processSyncModified");
			return false;
		}
	}

	return true;
};

void cdb::cleanModifications()
{
    for (auto o : m_roots) {
        o->cleanModifications();
    }
//    util::iterate_sync_clean_modification(m_roots);
};



bool cdb::processSyncMoved(SyncProcessor* proc)
{
    for (auto o : m_roots) {
        if (!o->processSyncMoved(proc)) {
            ASSERT(0, "failed cdb::processSyncMoved");
            return false;
        };
    }
    return true;
};

bool cdb::checkIdentity(SyncProcessor* proc)
{
    for (auto o : m_roots) {
        o->checkIdentity(proc, this);
    }
    return true;
};

bool cdb::processSyncCompound(SyncProcessor* proc)
{
    for (auto o : m_roots) {
        if (!o->processSyncCompound(proc)) {
            ASSERT(0, "failed cdb::processSyncCompound");
            return false;
        }
    }

    return true;
};

bool cdb::processSyncDelete(SyncProcessor* proc)
{
    for (auto o : m_roots) {
        if (!o->processSyncDelete(proc)) {
            ASSERT(0, "failed cdb::processSyncDelete");
            return false;
        }
    }
    return true;
};

void cdb::createSyidMap(SyncProcessor* proc)
{
    m_syid2item.clear();

    for (auto o : m_roots) {
        o->createSyidMap(proc, m_syid2item);
    }    
};

bool cdb::checkTreeSanity()
{
	assert_return_false(m_sngl2topic.size() == SINGLETON_FOLDERS_COUNT, QString("expected %1 singletons, provided %2").arg(SINGLETON_FOLDERS_COUNT).arg(m_sngl2topic.size()));

	int i, Max = m_roots.size();
	for (i = 0; i < Max; i++)
	{
		cit_ptr it = m_roots[i];
		if (!it)
		{
			ASSERT(0, "root topic is NULL at position") << i;
			return false;
		}
		if (!it->isRootTopic())
		{
			ASSERT(0, "expected root topic at position") << i;
			return false;
		}
	}

	for (auto o : m_roots) {
		if (!o->checkTreeSanity()) {
			ASSERT(0, "treeSanity check failed") << o->dbgHint();
			return false;
		}
	}
	return true;
};

bool cdb::prepareSync(SyncProcessor* proc, bool after_sync_sanity_test)
{
	if (!checkTreeSanity())
		return false;

	if (!after_sync_sanity_test) {
		for (auto o : m_roots) {
			o->prepareSync(this);
		}
	}

	createSyidMap(proc);
	return true;
};

bool cdb::prepareSyncStep2(SyncProcessor* proc)
{
    for (auto o : m_roots) {
        if (!o->markSyncOrphantAsMoved(proc)) {
            ASSERT(0, "failed prepareSyncStep2/markSyncOrphantAsMoved") << o->dbgHint();
        }
    }

    return true;
}


#ifdef _DEBUG
void debug_print_hist_info(cdb* db, QString prefix)
{
    tmpDebug() << "[sync] history" << prefix << " dbid=" << db->db_id() << "db-mod" << db->db_mod_counter() << "h-size" << db->history().size();
    for(SYNC_HISTORY::const_iterator i = db->history().begin(); i != db->history().end(); i++)
        {
            QDateTime dt;
            dt.setTime_t(i->second.last_sync_time);
            tmpDebug() << i->first << i->second.mod_counter << dt.toString();
        }
    tmpDebug() << "----- end hist ------";
}
#endif



/**
   cit_primitive
*/
cit_prim_ptr cit_primitive::cloneSyncAtomic(SyncProcessor* proc)const
{
    cit_prim_ptr rv = create();
    rv->attachPdb(proc->contraP(this));
    rv->assignSyncAtomicContent(this);
    rv->m_mod_counter = proc->contraDB(this)->db_mod_counter() + 1;
    rv->m_fsync.sync_modified_processed = 1;
    rv->m_fsync.sync_moved_processed = 1;
    return rv;
};

bool cit_primitive::isSyncModified(SyncProcessor* proc)const
{
    COUNTER_TYPE our_relative_mod_counter = proc->comparatorCounter(this);
    bool first_time_ever_this_db_in_synced = !IS_VALID_COUNTER(our_relative_mod_counter);
    bool rv = (first_time_ever_this_db_in_synced ||
                m_mod_counter > our_relative_mod_counter);
    return rv;
};

bool cit_primitive::isSyncReadyToDelete(SyncProcessor* proc)const
{
    Q_UNUSED(proc);
    bool rv = (m_fsync.sync_delete_requested == 1);
    return rv;
};

/**
   cit_base
*/
cit_prim_ptr cit_base::cloneSyncAtomic(SyncProcessor* proc)const
{
    cit_base_ptr rv = dynamic_cast<cit_base*>(cit_primitive::cloneSyncAtomic(proc));
    rv->m_syid = m_syid;
    rv->m_move_counter = 1;
    return rv;
};

bool cit_base::isSyncModified(SyncProcessor* proc)const
{
    ASSERT(syncDb(), "expected sync DB attached");
    return cit_primitive::isSyncModified(proc);
}

bool cit_base::isSyncMoved(SyncProcessor* proc)const
{
    ASSERT(syncDb(), "expected sync DB attached");
    COUNTER_TYPE our_relative_mod_counter = proc->comparatorCounter(this);
    bool first_time_sync = !IS_VALID_COUNTER(our_relative_mod_counter);
    bool rv = ( first_time_sync ||
                m_move_counter > our_relative_mod_counter);
    return rv;
};

/**
   cit - class
*/
void cit::syncStatAfterCloned(SyncProcessor* p, cit_cptr source)
{
	ASSERT_VALID(this);
	ASSERT_VALID(cit_owner());
	ASSERT_VALID(source);

	for (CEXTENSIONS::iterator i = source->extensions().begin();
		i != source->extensions().end();
		i++)
	{
		auto e = *i;
		auto cloned = e->cloneToOtherDB(p, this);
		if (!cloned) {
			ASSERT(0, "Failed to clone extension") << dbgHint();
		}
	}
};

void cit::syncStatOnAmbiguous(SyncProcessor* p, cit_cptr )
{
    p->stat_on_ambiguous(this);
};

void cit::syncStatAfterAssigned(SyncProcessor* p, cit_cptr)
{
    p->stat_on_assigned(this);
};

bool cit::isSyncCompoundModifiedOrMoved(SyncProcessor* proc)const
{
    if(isSyncModified(proc) || isSyncMoved(proc))
        {
            return true;
        }

    for(CEXTENSIONS::const_iterator i = m_extensions.begin(); 
        i != m_extensions.end();
        i++)
        {
            if((*i)->isSyncModified(proc))
                return true;
        }

    return false;
};


#define REGENERATE_SYID_4_DUPE_SYID(o, o2)       QString s = QString("ERROR. Duplicate SYID (%1 %2 %3) (%4 %5 %6). Regenerating SYID..") \
        .arg(reinterpret_cast<qlonglong>(o))                            \
        .arg(o->dbgHint())                                              \
        .arg(o->title())                                                \
        .arg(reinterpret_cast<qlonglong>(o2))                           \
        .arg(o2->dbgHint())                                             \
        .arg(o2->title());                                              \
    sync_log(s);                                                        \
    o->regenerateSyid();                                                \


void cit::createSyidMap(SyncProcessor* proc, SYID_2_ITEM& syid2item)
{
    if (isSynchronizable())
    {
#ifdef _DEBUG
        ASSERT(!isOnLooseBranch(), QString("item is on loose branch: %1").arg(dbgHint()));
#endif

        ASSERT(!isRootTopic(), "root can't be a synchronizable item") << dbgHint();
		assert_return_void(cit_parent(), QString("expected parent: %1").arg(dbgHint()));

        if (!IS_VALID_SYID(syid())) {
            regenerateSyid();
        }
        else
        {
            SYID_2_ITEM::iterator i = syid2item.find(syid());
            if (i != syid2item.end()) {
                REGENERATE_SYID_4_DUPE_SYID(this, i->second);
                if (proc) {
                    proc->stat_on_err_dupe_syid(this);
                }
            }
        }

        syid2item[syid()] = this;
    }

    for (CITEMS::iterator i = items().begin(); i != items().end(); i++)
    {
        cit_ptr it = *i;
        it->createSyidMap(proc, syid2item);
    }
};

bool cit::isSyncReadyToDelete(SyncProcessor* proc)const
{
    bool rv = cit_base::isSyncReadyToDelete(proc);
    if (rv)
    {
        if (isSyncCompoundModifiedOrMoved(proc))
        {
            return false;
        }

        for (CITEMS::const_iterator i = m_items.begin(); i != m_items.end(); i++)
        {
            const cit_ptr it = *i;
            if (!it->isSyncReadyToDelete(proc))
                return false;
        }
    }
    return rv;
};


cit_ptr cit::syncEnsureOtherDBParent(SyncProcessor* proc, int& insert_pos)
{
    cit_ptr our_parent = cit_parent();
	assert_return_null(our_parent, "expected parent");
    insert_pos = our_parent->indexOf_cit(this);

    cit_ptr other_parent = proc->contraDB(this)->findItem(our_parent);
    if(!other_parent)
        {
            //try to insert clone of our parent
            //into other DB

            int p2_insert_pos = -1;
            cit_ptr other_grand_parent = our_parent->syncEnsureOtherDBParent(proc, p2_insert_pos);
            if(!other_grand_parent){
                    ASSERT(0, "no grandparent recovery, giwing up") << our_parent->dbgHint();
                    return nullptr;
                }

            other_parent = dynamic_cast<cit*>(our_parent->cloneSyncAtomic(proc));
            if(other_parent){
                    other_grand_parent->insert_cit(p2_insert_pos, other_parent);
                    other_parent->syncStatAfterCloned(proc, our_parent);
                }
        }

    if(other_parent){
            if((int)other_parent->items().size() < insert_pos)
                insert_pos = other_parent->items().size();
        }

    return other_parent;
};

bool cit::syncAdjustOtherDBItemPosition(SyncProcessor* snc, cit_ptr OtherDBItem)
{
    cit_ptr OurParent = cit_parent();
	assert_return_false(OurParent, "Invalid parent object in our DB.");

    cit_ptr OtherParent = OtherDBItem->cit_parent();
	assert_return_false(OtherParent, "Invalid parent object in other DB.");

    cit_ptr OtherAdjustedParent = OtherParent;
    int position = 0;

    bool we_are_local = snc->is_local(this);
    QString sinfo = we_are_local ? "we=local" : "we=remote";

    if(!OurParent->isSyidIdenticalTo(OtherAdjustedParent))
        {            
            OtherAdjustedParent = syncEnsureOtherDBParent(snc, position);
			assert_return_false(OtherAdjustedParent, "Failed to ensure parent in other DB.");

            //qDebug() << "pos-adjusted from" << OtherParent->dbgHint() << " to " << OtherAdjustedParent->dbgHint();
        }

	assert_return_false(OtherAdjustedParent, "expected adjuasted parent");

    if(OtherDBItem->isAncestorOf(OtherAdjustedParent))
        {
            qWarning() << "cross-ref-on-pos-adjusted from" << OtherDBItem->dbgHint() << " to " << OtherAdjustedParent->dbgHint();
            //ASSERT(0, "cross-ref-on-pos-adjusted from") << sinfo << dbgHint() << OtherDBItem->dbgHint() << " to " << OtherAdjustedParent->dbgHint();
            return true;//allow processing sync
        }


    if(!OtherAdjustedParent->canAcceptChild(OtherDBItem))
        {
            qWarning() << "incompatible-child-on-pos-adjusted" << OtherDBItem->dbgHint() << " to " << OtherAdjustedParent->dbgHint();
            //ASSERT(0, "incompatible-child-on-pos-adjusted") << sinfo << dbgHint() << OtherDBItem->dbgHint() << " to " << OtherAdjustedParent->dbgHint();
            return true;//allow processing sync     
        }

    if(!OurParent->isSyidIdenticalTo(OtherAdjustedParent))
        {
            ASSERT(0, "failed to ensure other DB parent") << OurParent->dbgHint() << OtherAdjustedParent->dbgHint();
#ifdef _DEBUG
            bool ok = OurParent->isSyidIdenticalTo(OtherParent);
            ASSERT(ok, "parent-syid-check-err") << OurParent->dbgHint() << OtherAdjustedParent->dbgHint();
#endif
        }

    OtherParent->remove_cit(OtherDBItem, false);
    position = OurParent->indexOf_cit(this);
    if((int)OtherAdjustedParent->items().size() < position)
        {
            position = OtherAdjustedParent->items().size();
        }      
    OtherAdjustedParent->insert_cit(position, OtherDBItem);
    OtherDBItem->ask4persistance(np_POS);
    snc->stat_on_moved(OtherDBItem);

    return true;
};

cit_ptr cit::cloneToOtherDB(SyncProcessor* proc)
{
    cit_ptr p = cit_parent();
    ASSERT(p, "Invalid parent pointer");
    int nInsertAt = 0;
    cit_ptr OtherSideParent = syncEnsureOtherDBParent(proc, nInsertAt);
    if(!OtherSideParent)
        {
            ASSERT(0, "Failed to ensure other-DB parent");
            return nullptr;
        }
  
    cit_ptr it = dynamic_cast<cit*>(cloneSyncAtomic(proc));
    if(it)
        {
            OtherSideParent->insert_cit(nInsertAt, it);
            if(!proc->complete_item_clone(it, this))
                return nullptr;
            it->syncStatAfterCloned(proc, this);

			auto mc = proc->contraDB(this)->db_mod_counter() + 1;
			it->postCloneSyncAtomic(this, mc);
        }
    else
        {
            ASSERT(0, "cloneSyncAtomic failed for") << dbgHint();
        }
  
    return it;
};

/**
   markSyncOrphantAsMoved - if item's parent is different than on contra
   adjust - should be done on remote/master so that local will get adjusted
*/
bool cit::markSyncOrphantAsMoved(SyncProcessor* proc)
{ 
    cit_ptr our_parent = cit_parent();

    if(!proc->is_local(this) && !isSingleton() && !our_parent->isRootTopic())
        {
            if(!isSyncMoved(proc))
                {
                    cdb* otherDB = proc->contraDB(this);
                    ASSERT(IS_VALID_SYID(syid()), "expected valid SYID") << syid() << dbgHint();
                    cit_ptr it_other = otherDB->findItem(this);
                    if(it_other && !it_other->isSyncMoved(proc))
                        {
                            cit_ptr other_parent = it_other->cit_parent();
                            if(!other_parent->isSyidIdenticalTo(our_parent))
                                {
                                    //---have to set as moved
                                    proc->stat_on_sync_orphant(this);
                                    m_move_counter = syncDb()->db_mod_counter() + 1;
                                    ask4persistance(np_SYNC_INFO);
                                }
                        }      
                }
        }

    for(CITEMS::iterator i = items().begin(); i != items().end(); i++)
        {
            cit_ptr it = *i;
            if(!it->markSyncOrphantAsMoved(proc))
                return false;
        }  

    return true;
};

bool cit::processSyncModified(SyncProcessor* proc)
{
    if(isSynchronizable())
        {
            ASSERT(!isRootTopic(), "root can't be a synchronizable item") << dbgHint();
            ASSERT(isValidSyid(), "expected valid syid");

            cdb* otherDB = proc->contraDB(this);
            SYID _syid = syid();
            ASSERT(syncDb() != otherDB, "SyncDB switch error") << syncDb() << otherDB;

            if(isSyncModified(proc))
                {
                    if(!m_fsync.sync_modified_processed)
                        {
                            cit_ptr it_other = otherDB->findItem(this);
                            if(it_other)
                                {
                                    proc->process_modified_assigment(it_other, this);
                                }
                            else
                                {
                                    cit_ptr cloned = cloneToOtherDB(proc);
                                    if(!cloned)
                                        {
                                            ASSERT(0, "Failed to clone item") << dbgHint();
                                            return false;
                                        }
          
                                }
                            m_fsync.sync_modified_processed = 1;
                        }//m_fsync.sync_modified_processed
                }
            else
                {
                    if(!isSyncCompoundModifiedOrMoved(proc))
                        {
                            cit_ptr it = otherDB->findItem(this);
                            if(!it)
                                {
                                //...
                                    if (isSyncModified(proc)){
                                        qWarning() << "ykh1";
                                    }
                                    //...

                                    m_fsync.sync_delete_requested = 1;
                                }
                        }
                }
        }//isSynchronizable
    else
        {
            if(isRootTopic())
                {
                    cdb* otherDB = proc->contraDB(this);
                    if(isSyncModified(proc))
                        {
                            if(!m_fsync.sync_modified_processed)
                                {
                                    cit_ptr it_other = otherDB->findItem(this);
                                    if(it_other)
                                        {
                                            proc->process_modified_assigment(it_other, this);
                                        }
                                    else
                                        {
                                            ASSERT(0, "Failed to locate root item. Roots are non-clonable") << dbgHint();
                                            return false;
                                        }
                                }
                        }
                }
        }//non-synchronizable

    if (hasSynchronizableItems()) {
        for (auto& it : items()){
            if (!it->processSyncModified(proc))
                return false;
        }
    }

    return true;
};

bool cit::processSyncMoved(SyncProcessor* proc)
{
    if(isSynchronizable())
        {
            ASSERT(!isRootTopic(), "root can't be a synchronizable item") << dbgHint();
            cdb* otherDB = proc->contraDB(this);

            if(isSyncMoved(proc))
                {
                    if(!m_fsync.sync_moved_processed)
                        {
                            cit_ptr it_other = otherDB->findItem(this);
                            if(!it_other)
                                {
                                    it_other = cloneToOtherDB(proc);
                                    if(!it_other)
                                        {
                                            ASSERT(0, "Failed to clone item") << dbgHint();
                                            return false;
                                        }
                                }
                            else
                                {
                                    if(!syncAdjustOtherDBItemPosition(proc, it_other))
                                        return false;
                                }
                            m_fsync.sync_moved_processed = 1;
                            it_other->m_fsync.sync_moved_processed = 1;
                            it_other->m_move_counter = otherDB->db_mod_counter() + 1;
                            it_other->ask4persistance(np_SYNC_INFO);
                        }
                };
        }//isSynchronizable

    if (hasSynchronizableItems()) {
        for (auto& it : items()){
            if (!it->processSyncMoved(proc))
                return false;
        }
    }
  
    return true;
};

bool cit::processSyncDelete(SyncProcessor* proc)
{  
    ASSERT_VALID(this);
    CITEMS  items2del;

    for(CITEMS::iterator i = m_items.begin(); i != m_items.end();i++)
        {
            cit_ptr it = *i;
            ASSERT_VALID(it);
            if(it->isSynchronizable() && it->isSyncReadyToDelete(proc))
                {
                    items2del.push_back(it);
                }
        }

    for(CITEMS::iterator i = items2del.begin(); i != items2del.end();i++)
        {
            cit_ptr it = *i;
            ASSERT_VALID(it);
            it->syncStatBeforeKilled(proc);
            it->killSilently(false);
        }

    if (hasSynchronizableItems()) {
        for (auto& it : m_items){
            ASSERT_VALID(it);
            if (!it->processSyncDelete(proc))
                return false;
        }
    }

    return true;
};

void cit::syncStatBeforeKilled(SyncProcessor* p)
{
	assert_return_void(isSynchronizable(), QString("expected synchronizable: %1").arg(dbgHint()));
	assert_return_void(!isRootTopic(), QString("root not expected: %1").arg(dbgHint()));
#ifdef _DEBUG
    ASSERT(!isOnLooseBranch(), QString("item is on loose branch: %1").arg(dbgHint()));    
#endif

    syncStatCompoundBeforeKilled(p);  
};

void cit::syncStatCompoundBeforeKilled(SyncProcessor* proc)
{
	proc->stat_on_deleted(this);

	ASSERT(IS_VALID_SYID(syid()), "expected valid SYID") << dbgHint();
	cdb* db = syncDb();
	assert_return_void(db, QString("expected syncDB: %1").arg(dbgHint()));
	SYID_2_ITEM::iterator k = db->m_syid2item.find(syid());
	assert_return_void(k != db->m_syid2item.end(), QString("expected SYID in map: %1").arg(dbgHint()));
	db->m_syid2item.erase(k);


	for (CITEMS::iterator i = items().begin(); i != items().end(); i++)
	{
		cit_ptr it = *i;
		it->syncStatCompoundBeforeKilled(proc);
	}
};

void cit::checkIdentity(SyncProcessor* proc, snc::cdb*)
{
    cdb* otherDB = proc->contraDB(this);

    if(isSynchronizable())
        {
            cit_ptr it_other = otherDB->findItem(this);
            if(it_other != NULL)
                {
                    int flags = 0;
                    if(!isAtomicIdenticalTo(it_other, flags))
                        {
                            proc->stat_on_identity_err(this, it_other, flags);
                            if(proc->is_local(it_other))
                                {         

                                    if(flags & flPSyid)
                                        {             
                                            if(!syncAdjustOtherDBItemPosition(proc, it_other)){
                                                ASSERT(0, "failed to adjust other item pos on ident-err") << dbgHint() << it_other->dbgHint();
                                            }
                                            int flags1 = 0;
                                            if (!isAtomicIdenticalTo(it_other, flags1))
                                                {
                                                    if (flags & flPSyid)
                                                        {
                                                            ASSERT(0, "failed to adjust other DB item pos") << dbgHint() << it_other->dbgHint();
                                                        }
                                                }             
                                        }//flPSyid
                                }
                        }
                }
            else
                {
                    proc->stat_on_identity_lost(this);
                    ///1
                    QString s1;
                    if(proc->is_local(this))
                        {
                            s1 = "to remote DB";
                        }
                    else
                        {
                            s1 = "to local DB";
                        }
                    QString s(QString("cloning-item-with-lost-identity %1 %2").arg(s1).arg(dbgHint()));
                    sync_log(s);
                    cit_ptr cloned = cloneToOtherDB(proc);
                    if(cloned == NULL)
                        {
                            ASSERT(0, "Failed to clone item") << dbgHint();
                        }     
                    ///1
                }
  
        }//isSynchronizable

//    APPLY_ON_PRIM_P2(syncCheckIdentity, proc, otherDB);

    if (hasSynchronizableItems()) {
        for (auto& it : items()){
            it->checkIdentity(proc, otherDB);
        }
    }
};

template<class T> size_t index_of(T* o, std::vector<T*>& arr)
{
    typedef typename std::vector<T*>::iterator ITR;
    size_t idx = 0;
    for(ITR i = arr.begin(); i != arr.end(); i++)
        {
            T* it = *i;
            if(it == o)
                return idx;
            idx++;
        
        }
    return -1;
}

bool cit::processSyncCompound(SyncProcessor* proc)
{
    if(isSynchronizable())
        {
            ASSERT(!isRootTopic(), "root can't be a synchronizable item") << dbgHint();
            cdb* otherDB = proc->contraDB(this);
  
            if(isSyncCompoundModifiedOrMoved(proc))
                {
                    cit_ptr it_other = otherDB->findItem(this);
                    if(it_other == NULL)
                        {
                            it_other = cloneToOtherDB(proc);
                        }
                    if(it_other == NULL){
                        ASSERT(0, "inner error - unexpected NULL");
                        return false;
                    }
                }
        }

//    RUN_BOOL_FUNC_ON_PRIM_P1(syncModifiedMoved, proc);
    for (auto e : m_extensions) {
        if (!e->processSyncModified(proc)) {
            ASSERT(0, "failed processSyncCompound") << dbgHint();
            return false;
        }
    }
  
    if (hasSynchronizableItems()) {
        for (auto& it : items()){
            if (!it->processSyncCompound(proc))
                return false;
        }
    }

    return true;
};

static bool verify_counters_before_sync(COUNTER_TYPE c1, COUNTER_TYPE c2)
{
    bool c1_is_valid = IS_VALID_COUNTER(c1);
    bool c2_is_valid = IS_VALID_COUNTER(c2);
    if(c1_is_valid && c2_is_valid)
        {
            //tmpDebug() << "snc" << "hist sdb counters:" << c1 << c2;
        }
    else if (!c1_is_valid && !c2_is_valid)
        {
            //tmpDebug() << "snc" << "first time syncing";
        }
    else
        {
            ASSERT(0, "sync information is not complete or corrupted, sync history is invalid") << c1 << c2;
            return false;
        }

    return true;
};

bool SyncProcessor::syncDB(snc::persistantDB* p1, snc::persistantDB* p2)
{
	cdb* db1_local = p1->syncDb();
	cdb* db2_remote = p2->syncDb();

	ASSERT(db1_local != db2_remote, "SyncProcessor::syn_db databases are equal..");

	m_db1_local_p = p1;
	m_db2_remote_p = p2;

	m_db1_local = db1_local;
	m_db2_remote = db2_remote;
	m_db1_local_hist_mod_counter = m_db2_remote->db_hist_mod_counter_of_db(m_db1_local);
	m_db2_remote_hist_mod_counter = m_db1_local->db_hist_mod_counter_of_db(m_db2_remote);
	if (!verify_counters_before_sync(m_db1_local_hist_mod_counter, m_db2_remote_hist_mod_counter))
	{
		ASSERT(0, "history information inconsistant") << m_db1_local_hist_mod_counter << m_db2_remote_hist_mod_counter << m_db1_local->m_hist.size() << m_db2_remote->m_hist.size();
		return false;
	};

	if (!sanityTest(false))
	{
		sync_log("sanity test failed before sync. aborted.");
		return false;
	}

	if (m_progress) {
		//m_progress->prepareProgressStatus(m_db1_local->m_syid2item,m_db2_remote->m_syid2item);
		m_progress->prepareProgressStatus(m_db1_local, m_db2_remote);
	}

	bool ok = syncLoop();

	if (ok)
	{
		if (synchronizeBoardLinks(m_db1_local_p,
			m_db2_remote_p,
			m_db1_local_hist_mod_counter,
			m_db2_remote_hist_mod_counter,
			m_progress))
		{
			m_changes_detected = true;
		};

		m_db1_local->inc_db_mod_counter();
		m_db2_remote->inc_db_mod_counter();

		m_db1_local->updateHistory(m_db2_remote);
		m_db2_remote->updateHistory(m_db1_local);

		m_db1_local->cleanModifications();
		m_db2_remote->cleanModifications();
	}

	db1_local->cleanupSync();
	db2_remote->cleanupSync();

	if (ok)
	{
		if (!sanityTest(true))
		{
			ASSERT(0, "sync - failed sanity test");
			return false;
		}
	}

	return ok;
};

bool SyncProcessor::syncLoop()
{
    bool ok = true;

#define SYNC_DB(D, F) if(ok){                   \
        ok = D->F(this);                        \
        ASSERT(ok, #F);                         \
    }                                           \

#define SYNC_LOOP_STEP(F)                       \
    SYNC_DB(m_db1_local, F);                    \
    SYNC_DB(m_db2_remote, F);                   \

    int loop_step = 1;

    bool identityOK = false;
    while(!identityOK && loop_step < 5)
        {
            SYNC_LOOP_STEP(processSyncModified);
            SYNC_LOOP_STEP(processSyncMoved);
            verifyMaps();
            SYNC_LOOP_STEP(processSyncCompound);
            verifyMaps();
            SYNC_LOOP_STEP(processSyncDelete);
            verifyMaps();
            cleanupIdentityResolutionMaps();
            verifyMaps();
            SYNC_LOOP_STEP(checkIdentity);

            if(!identityOK)
                {
                    verifyMaps();
                    process_local_db_identity_resolution();
                }

            cleanupIdentityResolutionMaps();
            loop_step++;
        }

#undef SYNC_DB
#undef SYNC_LOOP_STEP

    return ok;
};

void SyncProcessor::verifyMaps()
{

};

/**
   process_local_db_identity_resolution - on identity errors, when
   items from different DB are not identical after sync, we forcefully assign
   values from remote DB to local. It means cloud (master) storage has preference over local
*/
void SyncProcessor::process_local_db_identity_resolution()
{
	for (CITEM_2_FLAGS::iterator i = m_localdb_identity_err_4resolution.begin(); i != m_localdb_identity_err_4resolution.end(); i++)
	{
		cit_ptr it = i->first;
		cit_ptr our_parent = it->cit_parent();
		if (our_parent == NULL)continue;//the item might have been deleted by sync
		cit_ptr other = m_db2_remote->findItem(it/*->syid()*/);

#ifdef _DEBUG
		ASSERT(!other->isOnLooseBranch(), "loose branch item") << other->dbgHint();
#endif

		assert_return_void(other, it->dbgHint("expected other DB item"));
		int flags = i->second;
		if (has_ppos_field(flags))
		{
			cit_ptr other_parent = other->cit_parent();
			assert_return_void(other_parent, it->dbgHint("expected other DB item parent"));
			cit_ptr our_new_parent = m_db1_local->findItem(other_parent/*->syid()*/);
			if (our_new_parent == NULL)
			{
				ASSERT(0, "YKH - need code to recreate our DB branch") << other_parent->dbgHint();
			}

			if (our_new_parent != NULL)
			{
				if (our_new_parent->canAcceptChild(it))
				{
					int other_it_pos = other_parent->indexOf_cit(other);
					int our_it_pos = our_parent->indexOf_cit(it);
					if (our_it_pos != other_it_pos || our_new_parent != our_parent)
					{
						int insert_pos = other_it_pos;
						our_parent->remove_cit(it, false);
						if ((int)our_new_parent->items().size() < insert_pos)
							insert_pos = our_new_parent->items().size();
						our_new_parent->insert_cit(insert_pos, it);
						stat_on_identity_pos_err_resolved(it);
						it->ask4persistance(np_POS);
					}
				}
				else
				{
					ASSERT(0, "skipped incompatible child") << our_new_parent->dbgHint() << it->dbgHint();
#ifdef _DEBUG
					if (!our_new_parent->canAcceptChild(it)) {
						qDebug() << "skipped incompatible in dbg-mode";
					}
#endif
				}
			}
		}

		//if (has_content_field(flags))
		{
			it->assignSyncAtomicContent(other);
			stat_on_identity_err_assigned(it);
		}
	}
};


bool SyncProcessor::sanityTest(bool after_sync)
{
    if(!m_db1_local->prepareSync(this, after_sync)){
        sync_log("local db failed sanity test");
        return false;
    }
    if(!m_db2_remote->prepareSync(this, after_sync)){
        sync_log("remote db failed sanity test");
        return false;
    }      

    if(!after_sync)
        {
            if(!m_db2_remote->prepareSyncStep2(this)){
                sync_log("remote db failed prepareSyncStep2");
                return false;
            }  
            if(!m_db1_local->prepareSyncStep2(this)){
                sync_log("local db failed prepareSyncStep2");
                return false;
            }
        }

    return true;
};



/*
  extension
*/

QString extension::dbgHint(QString s)const
{
    QString rv = s + " " + extName();//+ m_owner->dbgHint();
    if(m_owner2 != NULL)
        rv += m_owner2->dbgHint();
    return rv;
};

void extension::setSyncModified()
{
    if(sync_mod_register_enabled)
        {
            if(cit_owner()->syncDb()){
                m_mod_counter = cit_owner()->syncDb()->db_mod_counter() + 1;
            }
            else{
                ASSERT(0, "expected syncDB for set-modified") << dbgHint();
            }
        }
};

bool extension::processSyncModified(SyncProcessor* proc)
{
    EOBJ_EXT et = ext_type();
    cdb* otherDB = proc->contraDB(this);
    cit_ptr our_owner = cit_owner();
	assert_return_false(our_owner, QString("expected owner of extension %1 %2").arg(static_cast<int>(et)).arg(extName()));

    if(isSyncModified(proc))
        {      
            if(!m_fsync.sync_modified_processed)
                {
                    cit_ptr it_other = otherDB->findItem(our_owner/*->syid()*/);
					assert_return_false(it_other != NULL, QString("expected other DB owner of extension %1/%2/%3").arg(static_cast<int>(et)).arg(extName()).arg(our_owner->dbgHint()));

                    auto other_ext = it_other->findExtensionByType(et);
                    if(other_ext){
                            proc->process_modified_assigment_ext(other_ext, this);
                        }
                    else
                        {
                            auto cloned = cloneToOtherDB(proc, it_other);
                            if(!cloned){
                                    ASSERT(0, "Failed to clone extension") << dbgHint();
                                    return false;
                                }
                        }      
                    m_fsync.sync_modified_processed = 1;
                }
        }
    else
        {
            if(!m_fsync.sync_modified_processed)
                {
                    cit_ptr it_other = otherDB->findItem(our_owner/*->syid()*/);
                    if(it_other == NULL)
                        {
                            //the other could have been deleted
                            return true;
                        }
                    auto other_ext = it_other->findExtensionByType(et);
                    if(other_ext){
                            //no need to assign since we weren't modified
                        }
                    else
                        {
                            auto cloned = cloneToOtherDB(proc, it_other);
                            if(cloned == NULL)
                                {
                                    ASSERT(0, "Failed to clone extension") << dbgHint();
                                    return false;
                                }
                            m_fsync.sync_modified_processed = 1;
                        }
                }
            // ASSERT(0, "might want to remove ext");
        }

    return true;
};


//void extension::checkIdentity(SyncProcessor* , cdb* )
//{
//    ASSERT(0, "NA");
//};

//bool extension::isSyncReadyToDelete(SyncProcessor* )const
//{
//    ASSERT(0, "NA");
//    return false;  
//};

//bool extension::killSilently(bool )
//{
//    ASSERT(0, "NA");
//    return false;
//};

//bool extension::processSyncMoved(SyncProcessor*)
//{
//    ASSERT(0, "NA");
//    return false;  
//};

//void extension::createSyidMap(SyncProcessor*, SYID_2_ITEM& )
//{
//    ASSERT(0, "NA");
//};

//void extension::syncStatAfterCloned(SyncProcessor*, const extension*) 
//{
//
//};

void extension::syncStatAfterAssigned(SyncProcessor* p, const extension*) 
{
	p->stat_on_assigned(this);
};

cit_prim_ptr extension::cloneSyncAtomic(SyncProcessor* proc)const
{
    cit_extension_ptr rv = dynamic_cast<extension*>(create());
    rv->assignSyncAtomicContent(this);
    rv->m_mod_counter = proc->contraDB(this)->db_mod_counter() + 1;
    rv->m_fsync.sync_modified_processed = 1;
    rv->m_fsync.sync_moved_processed = 1;
    return rv;
};


cit_extension_ptr extension::cloneToOtherDB(SyncProcessor* proc, cit_ptr it_other)
{
    auto p = cloneSyncAtomic(proc);
	assert_return_null(p, QString("Failed to clone atomic extension: %1").arg(dbgHint()));
    cit_extension_ptr e = dynamic_cast<extension*>(p);
	assert_return_null(e, QString("expected extension clone: %1").arg(dbgHint()));
    if(!it_other->addExtension(e))
        {
            ASSERT(0, "extension was not added") << dbgHint();
            return nullptr;
        }

    if(!proc->complete_extension_clone(e, this))
        return nullptr;

    e->syncStatAfterCloned(proc, this);
    return e;
};

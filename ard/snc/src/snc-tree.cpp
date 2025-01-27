#include <iostream>
#include <time.h>
#include <QDateTime>
#include <QCryptographicHash>

#include "snc.h"
#include "snc-tree.h"
#include "snc-cdb.h"
#include "blinks.h"

using namespace snc;

union SYID_union
{
    uint64_t  id;
    struct
    {
        uint32_t db_id:32;
        uint32_t it_id:32;
    };
};

bool sync_mod_register_enabled = true;

static bool hasKeyword(cit_ptr it, QString s)
{
    if (it->IsUtilityFolder()) {
        return false;
    }

    bool ok = (it->title().indexOf(s, 0, Qt::CaseInsensitive) != -1) ||
        (it->annotation().indexOf(s, 0, Qt::CaseInsensitive) != -1);
    return ok;
};


cit_ptr snc::cit::findByTitleNonRecursive(QString title)
{
    for(CITEMS::iterator i = items().begin(); i != items().end(); i++)
        {
            cit_ptr it = *i;
            if(it != NULL && 
               it->title().compare(title, Qt::CaseInsensitive) == 0)
                return it;
        }
    return NULL;
};

cit_ptr snc::cit::findBySyidNonRecursive(QString syid)
{
    for(CITEMS::iterator i = items().begin(); i != items().end(); i++)
        {
            cit_ptr it = *i;
            if(it != NULL && 
               it->syid().compare(syid, Qt::CaseSensitive) == 0)
                return it;
        }
    return NULL;
};

void snc::cit::memFindItems(MemFindPipe* mp)
{
    ASSERT_VALID(this);

    for(CITEMS::iterator i = items().begin(); i != items().end();i++)
        {
            cit_ptr it = *i;
            mp->pipe(it);
        }

    for(CITEMS::iterator i = items().begin(); i != items().end();i++)
        {
            cit_ptr f = *i;
            if(!mp->skipTopic(f))
                {
                    f->memFindItems(mp);
                }
        }
};

/**
   MemFindAllIndexedPipe
*/
snc::MemFindAllIndexedPipe::MemFindAllIndexedPipe():m_idx(0)
{
    m_filterIsOn = false;
}

void snc::MemFindAllIndexedPipe::prepare(cit_ptr root)
{
    m_items.clear();
    m_keyword_filtered.clear();
    m_keyword = "";
    m_idx = 0;

    root->memFindItems(this);
};

void snc::MemFindAllIndexedPipe::pipe(cit_ptr it)
{
    it->setUserData(m_idx);
    m_items.push_back(it);
    m_idx++;
};

CITEMS& snc::MemFindAllIndexedPipe::filterByKeyword(QString s)
{
    s = s.trimmed();
    if(s.isEmpty())
        {
            m_filterIsOn = false;
            return m_keyword_filtered;
        }

    m_filterIsOn = true;

    if(s.compare(m_keyword, Qt::CaseInsensitive) == 0)
        {
            return m_keyword_filtered;
        }

    int subSearch = !m_keyword.isEmpty() && (s.indexOf(m_keyword) == 0);
    if(subSearch)
        {
        CITEMS sub_search;
            for(CITEMS::iterator i = m_keyword_filtered.begin();i != m_keyword_filtered.end();i++)
                {
                    cit_ptr it = *i;
                    if(hasKeyword(it, s))
                        {
                            sub_search.push_back(it);
                        }
                }      
            m_keyword_filtered.clear();
            m_keyword_filtered = sub_search;
        }
    else
        {
            m_keyword_filtered.clear();
            for(CITEMS::iterator i = m_items.begin();i != items().end();i++)
                {
                    cit_ptr it = *i;
                    if(hasKeyword(it, s))
                        {
                            m_keyword_filtered.push_back(it);
                            //          qDebug() << "[s]yes" << s << it->id() << it->title();
                        }
                    else
                        {
                            //          qDebug() << "[s]no" << s << it->id() << it->title();
                        }
                }
        }

    m_keyword = s;

    return m_keyword_filtered;
};


/**
   MemFindByStringPipe
*/
snc::MemFindByStringPipe::MemFindByStringPipe(QString keyword)
    :m_keyword(keyword)
{
  
};

void snc::MemFindByStringPipe::pipe(cit_ptr it)
{
    bool ok = hasKeyword(it, m_keyword);
    if(ok)
        {
            m_items.push_back(it);
        }
};

bool snc::cit::isSyidIdenticalTo(const cit_ptr it2)const
{
    bool rv = (otype() == it2->otype());
    if(rv)
        {
            if(isSingleton() && !it2->isSingleton()){
                rv = false;
            }
            if(!isSingleton() && it2->isSingleton()){
                rv = false;
            }

            if(rv)
                {
                    if(isSingleton())
                        {
                            rv = (getSingletonType() == it2->getSingletonType());
                        }
                    else
                        {
                            rv = areSyidEqual(syid(), it2->syid());     
                        }
                }
        }

    return rv;
};

/**
   cdb
*/
cdb::cdb()
    :m_db_id(INVALID_COUNTER), 
     m_db_mod_counter(INVALID_COUNTER)
{

};

cdb::~cdb()
{
};

void cdb::setupDb(const COUNTER_TYPE& db_id, const COUNTER_TYPE& mod_counter, SYNC_HISTORY& history)
{
    m_db_id = db_id;
    m_db_mod_counter = mod_counter;
    m_hist = history;
};

void cdb::initDB()
{
    if(isValid())
        {
            ASSERT(0, "ERROR - synDB already initialized.");
        }
    else
        {
            static DB_ID_TYPE last_init_db_id = 0;

            m_db_id = time(NULL) - SYNC_EPOCH_TIME;
            if(last_init_db_id == m_db_id)
                {
                    m_db_id++;
                    std::cout << "adjusting db_id to avoid duplication" << std::endl;
                }
            last_init_db_id = m_db_id;
            std::cout << "initializing syncDB instance " << m_db_id << std::endl;

            m_db_mod_counter = 1;
            checkOnLargestDBModCounter();
        }
};


cit_ptr cdb::getSingleton(ESingletonType t)
{
    SNGL_TYPE_2_CIT::iterator k = m_sngl2topic.find(t);
	assert_return_null(k != m_sngl2topic.end(), QString("failed to locate singleton type: %1").arg(static_cast<int>(t)));
    return k->second;
};

void cdb::registerSingletons(CITEMS& lst)
{
	assert_return_void(lst.size() == SINGLETON_FOLDERS_COUNT, QString("expected %1 singletons, provided %2").arg(SINGLETON_FOLDERS_COUNT).arg(lst.size()));
	int scount = 0;
	for (CITEMS::iterator i = lst.begin(); i != lst.end(); i++)
	{
		cit_ptr it = *i;
		assert_return_void(it->isSingleton(), QString("expected singleton: %1").arg(it->dbgHint()));
		if (it->isRootTopic())
		{
			m_roots.push_back(it);
		}

		registerSingleton(it);
		scount++;
		qDebug() << "reg-singleton" << scount << it->dbgHint();
	}
};

void cdb::registerSingleton(cit_ptr it)
{
	assert_return_void(it->isSingleton(), QString("expected singleton: %1").arg(it->dbgHint()));
    SNGL_TYPE_2_CIT::iterator k = m_sngl2topic.find(it->getSingletonType());
	assert_return_void(k == m_sngl2topic.end(), QString("duplicate singleton type:%1").arg(static_cast<int>(it->getSingletonType())));
    m_sngl2topic[it->getSingletonType()] = it;
};


/*
  getLargestDBModCounter - this function can be used only 
  when reset db_mod_counter in initDB procedure, it's once a lifetime
  event or when root DB record id corrupted. The idea is that our db_mod_counter
  value should be bigger and mod_counter value of any of the item inside DB
  otherwise it will be market as "modified/moved" untill reaches db_mod_mod_counter
  Also as a due diligence we can check getLargestDBModCounter after each sync procedure
*/
extern COUNTER_TYPE getLargestDBModCounter(cit_ptr it);

void cdb::checkOnLargestDBModCounter()
{
    COUNTER_TYPE c = getLargestDBModCounter(getSingleton(ESingletonType::dataRoot));
    if(c > m_db_mod_counter && c != INVALID_COUNTER)
        {
            sync_log(QString("checkOnLargestDBModCounter - changed DB-mod-counter from %1 to %2").arg(m_db_mod_counter).arg(c));

            m_db_mod_counter = c;
        }
};

COUNTER_TYPE cdb::db_hist_mod_counter_of_db(cdb* db_other)
{
	assert_return_0(db_other, "expected other-DB");
    COUNTER_TYPE rv = INVALID_COUNTER;

    if(db_other == this)
        throw "cdb::hist_mod_counter_of_db - the other DB equal to our";
    SYNC_HISTORY::iterator i = m_hist.find(db_other->db_id());
    if(i != m_hist.end())
        {
            rv = i->second.mod_counter;
        }
    return rv;
};

void cdb::cleanupSync()
{
    m_syid2item.clear();
};

cit_ptr cdb::findItem(cit_ptr otherDBItem)
{
    ASSERT_VALID(otherDBItem);
	assert_return_null(otherDBItem != NULL, "expected item");

#ifdef _DEBUG
    ASSERT(!otherDBItem->isOnLooseBranch(), "loose branch item");
#endif

    if(otherDBItem->isSingleton())
        {
            if(otherDBItem->isRootTopic())
                {
                    SYID _syid = otherDBItem->syid();
                    if(!_syid.isEmpty())
                        {
                            ASSERT(0, "expected empty SYID for root item") << otherDBItem->dbgHint();
                            return NULL;
                        }
                }

            SNGL_TYPE_2_CIT::iterator k = m_sngl2topic.find(otherDBItem->getSingletonType());
			assert_return_null(k != m_sngl2topic.end(), QString("ivalid root type:%1 %2").arg(static_cast<int>(otherDBItem->getSingletonType())).arg(otherDBItem->dbgHint()));
            return k->second;      
        }

    return findItemBySyid(otherDBItem->syid());
};

cit_ptr cdb::findItemBySyid(const SYID& syid)
{
    ASSERT(IS_VALID_SYID(syid), "expected valid SYID") << syid;

    cit_ptr rv = NULL;
    SYID_2_ITEM::iterator i = m_syid2item.find(syid);
    if(i != m_syid2item.end())
        {
            rv = i->second;
        }
    return rv;
};

void cdb::inc_db_mod_counter()
{
    m_db_mod_counter++;
    checkOnLargestDBModCounter();
}

void cdb::updateHistory(cdb* other_db)
{
    SYNC_HISTORY::iterator i =  m_hist.find(other_db->db_id());
    if(i != m_hist.end())
        {
            i->second.mod_counter = other_db->db_mod_counter();
        }
    else
        {
            m_hist[other_db->db_id()].mod_counter = other_db->db_mod_counter();
        }
};

bool cdb::isValid()const
{
    bool rv = IS_VALID_COUNTER(m_db_id) &&
        IS_VALID_COUNTER(m_db_mod_counter);
    return rv;
};


/**
   cit_primitive
*/
void cit_primitive::attachOwner(cit_ptr c)
{
    ASSERT_VALID(this);
    ASSERT_VALID(c);
    m_owner2 = c;
};
void cit_primitive::detachOwner()
{
    ASSERT_VALID(this);  
    m_owner2 = nullptr;
};

void cit_primitive::ask4persistance(ENEED_PERSISTANCE p)
{
    switch(p)
        {
        case np_ALL:ASSERT(0, "np_ALL - invalid persistance enum to set");break;
        case np_ATOMIC_CONTENT:m_fsync.need_persistance_ATOMIC_CONTENT = 1;break;
        case np_PINDEX:m_fsync.need_persistance_PINDEX = 1;break;
        case np_SYNC_INFO:m_fsync.need_persistance_SYNC_INFO = 1;break;
        case np_POS:m_fsync.need_persistance_POS = 1;break;
        default:ASSERT(0, "provided invalid persistance enum");
        }
};

void cit_primitive::clear_persistance_request(ENEED_PERSISTANCE p)
{
    switch(p)
        {
        case np_ALL:
            m_fsync.need_persistance_PINDEX = 0;
            m_fsync.need_persistance_SYNC_INFO = 0;
            m_fsync.need_persistance_POS = 0;
            m_fsync.need_persistance_ATOMIC_CONTENT = 0;
            break;
        case np_ATOMIC_CONTENT:m_fsync.need_persistance_ATOMIC_CONTENT = 0;break;
        case np_PINDEX:m_fsync.need_persistance_PINDEX = 0;break;
        case np_SYNC_INFO:m_fsync.need_persistance_SYNC_INFO = 0;break;
        case np_POS:m_fsync.need_persistance_POS = 0;break;
        default:ASSERT(0, "provided invalid persistance enum");
        } 
};

bool cit_primitive::need_persistance(ENEED_PERSISTANCE p)const
{
    if(support_persistance(p))
        {
            switch(p)
                {
                case np_ATOMIC_CONTENT:return (m_fsync.need_persistance_ATOMIC_CONTENT == 1);
                case np_PINDEX:return (m_fsync.need_persistance_PINDEX == 1);
                case np_SYNC_INFO:return (m_fsync.need_persistance_SYNC_INFO == 1);
                case np_POS:return (m_fsync.need_persistance_POS == 1);
                default:ASSERT(0, "provided invalid persistance enum");      
                }
        }

    return false;
};

QString cit_primitive::req4persistance()const
{
    QString rv = "[";
    if(m_fsync.need_persistance_ATOMIC_CONTENT)rv += "A";
    if(m_fsync.need_persistance_PINDEX)rv += "P";
    if(m_fsync.need_persistance_SYNC_INFO)rv += "S";
    if(m_fsync.need_persistance_POS)rv += "O";
    rv += "]";
    return rv;
};

cdb* cit_primitive::syncDb()
{
	assert_return_null(m_owner2 != NULL, QString("expected owner: %1").arg(dbgHint()));
    ASSERT_VALID(m_owner2);
    return m_owner2->syncDb();
};

const cdb* cit_primitive::syncDb()const
{
	assert_return_null(m_owner2 != NULL, QString("expected owner: %1").arg(dbgHint()));
    ASSERT_VALID(m_owner2);
    return m_owner2->syncDb();
};

void cit_primitive::cleanModifications()
{
    if(isSynchronizable())
        {
            cdb* s = syncDb();
            if(s){
                    if(m_mod_counter > s->db_mod_counter() || m_mod_counter == INVALID_COUNTER)
                        {
                            m_mod_counter = s->db_mod_counter();
                            ask4persistance(np_SYNC_INFO);
                        }
                }      
        }
}

void cit_primitive::prepareSync(cdb* db)
{
    if(syncDb() == NULL)
        {
            ASSERT(0, "expected sync DB attached before") << dbgHint();
        }
    else
        {
            ASSERT((syncDb() == db), "overattaching different SyncDB") << dbgHint();
        }

    if(!IS_VALID_COUNTER(modCounter()))
        {
            m_mod_counter = syncDb()->db_mod_counter() + 1;
            ask4persistance(np_SYNC_INFO);
        }

    //clear olny sync related flags and leave persistance flags
    m_fsync.sync_modified_processed = 0;
    m_fsync.sync_moved_processed = 0;
    m_fsync.sync_delete_requested = 0;
    m_fsync.sync_ambiguous_mod = 0;
};

/**
   cit_base
*/
void cit_base::setTitle(QString, bool)
{
    ASSERT(0, "NA");
};

QString cit_base::dbgHint(QString s /*= ""*/)const
{
    ASSERT_VALID(this);

    QString syid_s = formatSYID(syid());
    QString s2 = "-";
    QString s3 = "-";

    if(IS_VALID_COUNTER(m_mod_counter))
        {
            s2 = QString("%1").arg(m_mod_counter);
        }
    if(IS_VALID_COUNTER(m_move_counter))
        {
            s3 = QString("%1").arg(m_move_counter);
        }

    QString rv = QString("[%1-%2 L=%3 %4/%5/%6]:%7").arg(objName()).arg(id()).arg(counter()).arg(syid_s).arg(s2).arg(s3).arg(title().left(30));
    if(!s.isEmpty())
        {
            rv = s + ":" + rv;
        }
    return rv;
};


QString cit_base::make_new_syid(DB_ID_TYPE db_id)
{
    static uint32_t register_num = 0;
    static uint64_t  id_last_generated_id = 0;
    static char buff[64] = "";
   // ASSERT(syncDb() != NULL, "cit_base::regenerateSyid - Expected valid sync-DB object");

    SYID_union syid_u;

    syid_u.db_id = db_id;// syncDb()->db_id();
    syid_u.it_id = time(NULL) - SYNC_EPOCH_TIME;

    if(id_last_generated_id == syid_u.id)
        {
            register_num++;
            //qDebug() << "fast id-regeration, adjusted register: " << register_num; 
        }
    else
        {
            register_num = 0;
            //qDebug() << "drop sync register";
        }

    id_last_generated_id = syid_u.id;
    if(register_num == 0)
        {
            unsigned int v1 = syid_u.db_id;
            unsigned int v2 = syid_u.it_id;
#ifdef Q_OS_WIN32
            sprintf_s(buff, sizeof(buff), "%X-%X", v1, v2);
#else
            sprintf(buff, "%X-%X", v1, v2);
#endif
        }
    else
        {
            unsigned int v1 = syid_u.db_id;
            unsigned int v2 = syid_u.it_id;
            unsigned int v3 = register_num;
#ifdef Q_OS_WIN32
            sprintf_s(buff, sizeof(buff), "%X-%X-%d", v1, v2, v3);
#else
            sprintf(buff, "%X-%X-%d", v1, v2, v3);
#endif
        }

    return buff;
    //m_syid = buff;

    //ASSERT(IS_VALID_SYID(buff), "expected valid SYID") << buff << dbgHint();
    //qDebug() << "[snc] generated-syid" << dbgHint();
};

void cit_base::regenerateSyid()
{  
    if(syncDb() != NULL)
        {
            m_syid = make_new_syid(syncDb()->db_id());
            ASSERT(IS_VALID_SYID(m_syid), "expected valid SYID") << m_syid << dbgHint();
            m_mod_counter = syncDb()->db_mod_counter() + 1;
            m_move_counter = 0;
            ask4persistance(np_SYNC_INFO);
            onRegeneratedSyid();
        }
    else
        {
            ASSERT(0, "expected valid sync DB pointer") << dbgHint();
        }
};

bool cit_base::isAtomicIdenticalTo(const cit_primitive* _other, int& iflags)const
{  
    const cit_base* other = dynamic_cast<const cit_base*>(_other);
    COMPARE_ATTR(m_syid, iflags, flSyid);
    return true;
};

void cit_base::prepareSync(cdb*)
{
    if(!isValidSyid() && isSynchronizable())
        {
            regenerateSyid();
        }

    //clear olny sync related flags and leave persistance flags
    m_fsync.sync_modified_processed = 0;
    m_fsync.sync_moved_processed = 0;
    m_fsync.sync_delete_requested = 0;
    m_fsync.sync_ambiguous_mod = 0;
};

void cit_base::cleanModifications()
{
    if(isSynchronizable())
        {
            cdb* s = syncDb();
            if(s != NULL)
                {
                    cit_primitive::cleanModifications();
                    if(m_move_counter > s->db_mod_counter() || m_move_counter == INVALID_COUNTER){
                        m_move_counter = s->db_mod_counter();
                        ask4persistance(np_SYNC_INFO);
                    }
                }
            else
                {
                    ASSERT(0, "NULL SyncDB pointer") << dbgHint();
                }
        }
};

void cit_base::setSyncModified()
{
    if(sync_mod_register_enabled)
        {
            if(syncDb()){
                m_mod_counter = syncDb()->db_mod_counter() + 1;
            }
            else{
                ASSERT(0, "expected syncDB for set-modified") << dbgHint();
            }
        }
};

void cit_base::setSyncMoved()
{
    if(sync_mod_register_enabled)
        {
            if(syncDb()){
                    m_move_counter = syncDb()->db_mod_counter() + 1;
                }
            else
                {
                    ASSERT(0, "expected syncDB for set-moved") << dbgHint();
                }
        }  
};

QString cit_base::compoundSignature()const
{
    QString rv = QString("%1-%2-%3").arg(objName()).arg(syid()).arg(m_bflags.posIndex);
    return rv;
};

/**
   cit 
*/
cit::cit()
{
}

cit::~cit()
{
    clear();
};

QString cit::dbgHint(QString s /*= ""*/)const
{
    ASSERT_VALID(this);

    QString syid_s = formatSYID(syid());
    QString s2 = "-";
    QString s3 = "-";
    QString sparent = "null";

    if(IS_VALID_COUNTER(m_mod_counter))
        {
            s2 = QString("%1").arg(m_mod_counter);
        }
    if(IS_VALID_COUNTER(m_move_counter))
        {
            s3 = QString("%1").arg(m_move_counter);
        }

    QString syid_s_composite = syid_s+"/"+s2+"/"+s3;
  
    if(cit_parent()){
        sparent = "nid";
        if(IS_VALID_DB_ID(cit_parent()->id())){
            sparent = QString("%1").arg(cit_parent()->id());
        }
    }

    QString obj_name = QString("%1%2").arg(objName().left(7)).arg(otype());
    QString id_name = "nid";
    if (IS_VALID_DB_ID(id())) {
        id_name = QString("%1").arg(id());
    }

    QString rv = QString("[%1/%2/%3 %4]%5 ")
        .arg(obj_name)
        .arg(id_name)
		.arg(sparent)
        .arg(syid_s_composite)
        .arg(title().left(30));
    if(!s.isEmpty())
        {
            rv = s + "|" + rv;
        }
    return rv;
};


#ifdef ARD_BETA
void cit::printInfo(QString)const
{
    //empy
};
#endif

bool cit::checkTreeSanity()const
{
	ASSERT_VALID(this);

	for (CITEMS::const_iterator i = m_items.begin(); i != m_items.end(); i++)
	{
		const cit_ptr it = *i;
		ASSERT_VALID(it);
		if (!it->checkItemSanity())
		{
			return false;
		}
	}

	for (CITEMS::const_iterator i = items().begin(); i != items().end(); i++)
	{
		const cit_ptr it = *i;
		if (!it->checkTreeSanity())
		{
			return false;
		}
	}

	return true;
};

snc::cit* cit::cit_parent_raw() 
{
#ifdef USE_SMART_PTR
    snc::cit* rv = nullptr;
    auto p = cit_parent();
    if (p) {
        rv = p.get();
    }
    return rv;
#else
    return cit_parent();
#endif
};

const snc::cit* cit::cit_parent_raw()const 
{
#ifdef USE_SMART_PTR
    const snc::cit* rv = nullptr;
    auto p = cit_parent();
    if (p) {
        rv = p.get();
    }
    return rv;
#else
    return cit_parent();
#endif
};


bool cit::checkItemSanity()const
{
    ASSERT_VALID(this);
    for(CITEMS::const_iterator i = m_items.begin();i != m_items.end();i++)
        {
            auto it = *i;
            ASSERT_VALID(it);
            if(it->cit_parent_raw() != this){
                    ASSERT(0, "sanity error - parent") << it->dbgHint() << dbgHint();
                    return false;
                }
        }

    for (auto e : m_extensions){
		ASSERT_VALID(e);
        if (e->cit_owner() != this){
            ASSERT(0, "check owner error on attachable") << e->dbgHint() << dbgHint();
            return false;
        }
    }

    return true;
};


cit_extension_ptr cit::findExtensionByType(EOBJ_EXT ext)
{
    for(auto& e : extensions()){
            if(e->ext_type() == ext)
                return e;
        }
    return nullptr;
};


bool cit::addExtension(cit_extension_ptr e)
{
    if (!e) {
        ASSERT(0, "expected extension");
        return false;
    }

	ASSERT_VALID(e);

#ifdef _DEBUG
    CEXTENSIONS::iterator k = m_extensions.find(e);
    ASSERT(k == m_extensions.end(), "duplicate extension") << e->dbgHint();
    auto e2 = findExtensionByType(e->ext_type());
    if(e2){
            ASSERT(0, "extension of the type already exists") 
                << static_cast<int>(e->ext_type()) 
                << e->extName();
            return false;
        }
#endif

    m_extensions.insert(e);  
    e->attachOwner(this);
    mapExtensionVar(e);
    return true;
};

void cit::onRegeneratedSyid()
{
    for(auto& e : m_extensions)
        {
            e->m_mod_counter = syncDb()->db_mod_counter() + 1;
            e->ask4persistance(np_SYNC_INFO);
        }
};

void cit::onClearedSyid()
{
    for(auto& e : m_extensions){
            e->m_mod_counter = 0;
            e->ask4persistance(np_SYNC_INFO);
        }
};


void cit::clear()
{
	std::vector<cit_extension_ptr> lst;
	for (auto e : m_extensions) {
		ASSERT_VALID(e);
		lst.push_back(e);
	}

	for (auto e : lst) {
		ASSERT_VALID(e);
		e->detachOwner();
		e->release();
	}
	m_extensions.clear();

    for(CITEMS::iterator i = m_items.begin();i != m_items.end(); i++)
        {
            cit_ptr it = *i;
            ASSERT_VALID(it);
            it->detachParent();
            it->release();
        }
    m_items.clear();
};


void cit::clearSyncInfoForAllSubitems()
{  
    m_mod_counter = m_move_counter = 0;
    m_syid = INVALID_SYID;
    ask4persistance(np_SYNC_INFO);
    onClearedSyid();

    for (CITEMS::iterator i = items().begin(); i != items().end(); i++)
    {
        cit_ptr it = *i;
        it->clearSyncInfoForAllSubitems();
    }    
};


void cit::remove_all_cit(bool kill_it)
{
    ASSERT_VALID(this);
    for(CITEMS::iterator i = items().begin(); i != items().end();i++)
        {
            cit_ptr it = *i;
            it->detachOwner();
            if(kill_it)
                {
                    it->release();
                }
        } 
    m_items.clear();

    //requestVisibleItemsCalculation();
};

void cit::remove_cit(cit_ptr it, bool kill_it)
{
    ASSERT_VALID(this);
    ASSERT_VALID(it);

    int idx = it->m_bflags.posIndex;
    if(idx >= 0 && idx < (int)m_items.size())
        {
            m_items.erase(m_items.begin() + idx);
            int Max = m_items.size();
            for(int i = idx; i < Max; i++)
                {
                    m_items[i]->m_bflags.posIndex = i;
                }
        }
    else
        {
            ASSERT(0, "remove_cit - item index invalid") << it->dbgHint() << dbgHint();
            return;
        }

    it->detachOwner();

    if(kill_it)
        {
            it->release();
        }

    //requestVisibleItemsCalculation();  
}

void cit::rebuildCache()
{
    int Max = m_items.size();
    for(int i = 0; i < Max; i++)
        {
            m_items[i]->m_bflags.posIndex = i;
        }  
};

void cit::insert_cit(int position, cit_ptr it)
{
    ASSERT_VALID(this);
    ASSERT_VALID(it);
#ifdef _DEBUG
    if(!canAcceptChild(it))
        {
            ASSERT(0, "can not accept given child") << dbgHint() << it->dbgHint();
            if(!canAcceptChild(it)){
                ///debug point - how did we get here
                return;
            }
        }
#endif


    if(position > (int)m_items.size() || position < 0)
        {
            ASSERT(0, "invalid insert pos") << position << m_items.size();
            position = (int)m_items.size();
        }

    m_items.insert(m_items.begin() + position, it);
    int Max = m_items.size();
    for(int i = position; i < Max; i++)
        {
            m_items[i]->m_bflags.posIndex = i;
        }

    it->attachOwner(this);
    //requestVisibleItemsCalculation();
};

int cit::indexOf_cit(const cit* it)const
{
    int rv = -1;
    if(it->cit_parent() && 
        it->cit_parent() == this)
        {
            rv = it->m_bflags.posIndex;
            if(rv >= (int)m_items.size())
                rv = -1;
        }
    return rv;
};


size_t cit::totalItemsCount()const
{
    size_t rv = m_items.size();
    for(CITEMS::const_iterator i = m_items.begin();i != m_items.end();i++)
        {
            cit_ptr it = *i;
            rv += it->totalItemsCount();
        }
    return rv;
};

//@todo: query_gui_4_images - need some global call before sync
//the current implementation results in series of SQL calls for
//each topic to load images. It can be done in one shot
void cit::prepareSync(cdb* db)
{
    cit_base::prepareSync(db);

    for(CITEMS::iterator i = m_items.begin(); i != m_items.end();i++)
        {
            cit_ptr it = *i;
            it->prepareSync(db);
        }

    query_gui_4_comments();

//    APPLY_ON_PRIM_P1(syncPrepare, db);
};

#ifdef _DEBUG
bool cit::isOnLooseBranch()const
{
    if(isRootTopic())
        return false;

    typedef std::set<cit_cptr>   CONST_CITEMS_SET;

    CONST_CITEMS_SET parents_set;
    cit_cptr p2 = cit_parent();
    while(p2 != NULL)
        {
            if(p2->isRootTopic())
                return false;
            p2 = p2->cit_parent();

            CONST_CITEMS_SET::const_iterator k = parents_set.find(p2);
            if(k != parents_set.end())
                {
                    ASSERT(0, "cross reference detected in tree") << p2->dbgHint();
                    return true;
                }
            parents_set.insert(p2);
        }
    return true;
};


#endif //_DEBUG


/*
template<class T> void clean_modifications(std::vector<T*>& arr)
{
    typedef typename std::vector<T*>::iterator ITR;
    for(ITR i = arr.begin(); i != arr.end(); i++)
        {
            T* it = *i;
            it->cleanModifications();
        }
}*/

void cit::cleanModifications()
{
    cit_base::cleanModifications();
    for (auto o : m_items) {
        o->cleanModifications();
    }
    //clean_modifications(m_items);
    //APPLY_ON_PRIM(cleanModifications);
};

int cit::calcRankInBranch(cit_ptr b)const
{
    int Pos = 1;
    int rv = m_bflags.posIndex;
    cit_cptr p = cit_parent();
    while(p != NULL && p != b){
            Pos *= 10;
            rv += Pos * p->m_bflags.posIndex;
            p = p->cit_parent();
        }
    return rv;
};

void on_identity_error(QString )
{
#ifdef _DEBUG
    //  ASSERT(0, s);
#endif
};

/*
bool snc::cit::ensurePersistant()
{
    return ensurePersistant(-1);
};*/

bool snc::cit::isAncestorOf(const cit_ptr it)const
{
	assert_return_false(it != NULL, "expected item");
    cit_cptr p2 = it->cit_parent();
    while(p2){
#ifdef USE_SMART_PTR
        if (p2.get() == this)
            return true;
#else
            if(p2 == this)
                return true;
#endif
            p2 = p2->cit_parent();
        }
    return false;
};

/**
   extension
*/

snc::extension::extension()
{

}

snc::extension::~extension()
{

};

//void snc::extension::attachPdb(persistantDB*)
//{
//    ASSERT(0, "NA") << dbgHint();
//};

QString snc::extension::compoundSignature()const
{
    QString rv = QString("%1%2")
        .arg(extName())
        .arg(static_cast<int>(ext_type()));
    return rv;
};

/**
   MemFindAllWithSYIDmap
*/
void snc::MemFindAllWithSYIDmap::pipe(cit_ptr it)
{
    if(IS_VALID_SYID(it->syid()))
        {
            m_items.push_back(it);
#ifdef _DEBUG
            SYID_2_ITEM::iterator k = m_syid2item.find(it->syid());
            ASSERT(k == m_syid2item.end(), "duplicate SYID in map") << it->dbgHint();
#endif
            m_syid2item[it->syid()] = it;
        }
};


QString	snc::CompoundInfo::toString()const 
{
	return QString("%1/%2/%3").arg(size).arg(compound_size).arg(hashStr);
};

/**
   CompoundObserver
*/
CompoundObserver::CompoundObserver(cit_ptr it, bool build_map)
{
    ASSERT_VALID(it);

    for (CEXTENSIONS::iterator i = it->extensions().begin(); i != it->extensions().end(); i++)
    {
        auto e = *i;
        ASSERT_VALID(e);
        m_prim_list.push_back(e);
        if (build_map)
        {
            m_prim_map[e->compoundSignature()] = e;
        }
    }

};


/**
   SummaryBuilder

snc::SummaryBuilder::SummaryBuilder(cit_ptr topTopic) :
    m_totalTopics(0),
    m_totalExtensions(0)
{
    snc::MemFindAllPipe mp;
    topTopic->memFindItems(&mp);
    m_totalTopics = mp.items().size();
    for (CITEMS::iterator i = mp.items().begin(); i != mp.items().end(); i++)
    {
        cit_ptr f = *i;
        m_totalExtensions += f->extensions().size();
    }
};*/

/**
   SyncProgressInfo
*/
snc::SyncProgressInfo::SyncProgressInfo() 
{
	m_flags.flag = UINT32_MAX;
};

bool snc::SyncProgressInfo::isValid()const 
{
	return (m_flags.flag != UINT32_MAX);
};

snc::SyncProgressInfo::SyncProgressInfo(int index)
{
    m_flags.flag = 0;
    m_flags.flatIndex = index;
};

void snc::SyncProgressInfo::markModLocal()
{
    m_flags.modLocal = 1;
};

void snc::SyncProgressInfo::markModRemote()
{
    m_flags.modRemote = 1;
};


void snc::SyncProgressInfo::markDelLocal()
{
    m_flags.delLocal = 1;
};

void snc::SyncProgressInfo::markDelRemote()
{
    m_flags.delRemote = 1;
};

void snc::SyncProgressInfo::markErr()
{
    m_flags.error = 1;
};

int snc::SyncProgressInfo::index()const
{
    return m_flags.flatIndex;
};

bool snc::SyncProgressInfo::isMarked()const
{
    bool rv = m_flags.modLocal || m_flags.modRemote || m_flags.delLocal || m_flags.delRemote || m_flags.error;
    return rv;
};

void SyncProgressStatus::clearProgressStatus()
{
    m_info.clear();
    m_idx = 0;
	m_cloned_lnk.clear();
	m_modified_lnk.clear();
	m_deleted_lnk.clear();
};

void SyncProgressStatus::flatoutTree(cit* it1) 
{
    for(auto it2 : it1->items())
    {
        if (it2->isSynchronizable() && IS_VALID_SYID(it2->syid())) {
            auto i = m_info.find(it2->syid());
            if (i == m_info.end())
            {
                m_info.emplace(it2->syid(), SyncProgressInfo(m_idx++));
            }
        }
    }

    for(auto it2 : it1->items())
    {
        flatoutTree(it2);
    }
};

SyncProgressInfo& SyncProgressStatus::find_info(const snc::SYID& syid)
{
	static SyncProgressInfo not_there;
    auto i = m_info.find(syid);
    if (i != m_info.end()) {
        return i->second;
    }
    return not_there;
};


/*SyncProgressInfo& SyncProgressStatus::ensureBlinkInfo(ard::board_link* lnk) 
{
    QString syid = lnk->origin() + "|" + lnk->target() + "|" + lnk->link_syid();
    auto it = m_updated_blink_info.find(syid);
    if (it == m_updated_blink_info.end()) {
 		auto r = m_updated_blink_info.emplace(std::make_pair(syid, SyncProgressInfo(m_idx++)));
		return r.first->second;
    }
    return it->second;
};


void SyncProgressStatus::get_links_sync_info(int& removed_local, int& removed_remote,
	int& modified_local, int& modified_remote)const
{
	removed_local	= m_links_removed_local;
	removed_remote	= m_links_removed_remote;
	modified_local	= m_links_modified_local;
	modified_remote = m_links_modified_remote;
};*/


void SyncProgressStatus::prepareProgressStatus(cdb* db1, cdb* db2)
{
	//m_links_removed_local = m_links_removed_remote = m_links_modified_local = m_links_modified_remote;

    for (auto o : db1->m_roots) {
        flatoutTree(o);
    }
    for (auto o : db2->m_roots) {
        flatoutTree(o);
    }
};

std::vector<QString> SyncProgressStatus::resulsAsCharMap()const
{
#define MLINE_LEN 80
    std::vector<QString> rv; 
    QString line;
    for (auto& i : m_info) {
        char c = ' ';
        const snc::SyncProgressInfo& spi = i.second;
        if (spi.isMarked()) {
            SYNC_PROCESS_FLAG f = spi.flags();
            if (f.modLocal)c = 'l';
            else if (f.modRemote)c = 'r';
            else if (f.delLocal)c = 'd';
            else if (f.delRemote)c = 'x';
            else if (f.error)c = 'e';
        }
        else {
            c = '.';
        }
        line += c;
        if (line.size() > MLINE_LEN) {
            rv.push_back(line);
            line = "";
        }
    }
    return rv;
#undef MLINE_LEN
}

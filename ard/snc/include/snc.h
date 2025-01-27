#pragma once

#include <QDate>

#include "snc-assert.h"
#include "snc-misc.h"
#include "snc-meta.h"
#include "snc-aes.h"

class ArdDB;

namespace snc
{
    class persistantDB;
    class MemFindPipe;
    class remote_resource_table;
    class SyncProgressStatus;

    /**
       cit_primitive - pice of other synchronizable object
       can't exist by itself, only represent one or group
       of properties of an owner object. Has only "modified"
       sync-property, can't be moved and don't need SYID
       becase is already assigned to an object
    */

    class cit_primitive : public smart_counter
    {
        friend class cit;
        friend class SyncProcessor;
    public:
        cit_primitive():m_mod_counter(INVALID_COUNTER)
        {
            m_fsync.flag = 0;
            m_owner2 = NULL;
        }

        cit_ptr cit_owner(){return m_owner2;}
        const cit_ptr cit_owner()const{return m_owner2;}


        void attachOwner  (cit_ptr c);
        virtual void detachOwner();

        virtual cdb* syncDb();
        virtual const cdb* syncDb()const;

        ///returns true is primitive was modified against gived syncDB
        virtual bool isSyncModified(SyncProcessor* proc)const;
        ///modification counter
        COUNTER_TYPE modCounter()const{return m_mod_counter;}
        ///request to store in DB some properties of the object later
        void ask4persistance(ENEED_PERSISTANCE p);
        ///clears request to store object data
        void clear_persistance_request(ENEED_PERSISTANCE p);
        ///returns true if object need to save particular type of data
        bool need_persistance(ENEED_PERSISTANCE p)const;
        ///text describing type of persistance request
        QString req4persistance()const;
        ///returns true when primitive support type of persistance
        virtual bool support_persistance(ENEED_PERSISTANCE )const{return true;};
        virtual bool isAtomicIdenticalTo(const cit_primitive* other, int& iflags)const = 0;
        ///copy over atomic content from source topic
        virtual void assignSyncAtomicContent(const cit_primitive* other) = 0;
        ///create new instance of given object and initialize it to default value
        virtual cit_prim_ptr create()const = 0;

        ///isSyncReadyToDelete - can kill obj during sync
        virtual bool isSyncReadyToDelete(SyncProcessor* proc)const;
        ///isSynchronizable - means object can synced against remote DB, root items, managers are not synchronizable
        virtual bool isSynchronizable()const{return true;}

        virtual void cleanModifications();
        virtual void prepareSync(cdb* db);

        virtual cit_prim_ptr cloneSyncAtomic(SyncProcessor* proc)const;

        ///kill object without confirmation, during sync or after it was confirmed before
        virtual bool killSilently(bool gui_update) = 0;
        virtual void attachPdb(persistantDB*) = 0;
        virtual QString calcContentHashString()const = 0;
        virtual uint64_t contentSize()const = 0;
        //compoundSignature - uniquely identifies object object inside cit
        virtual QString compoundSignature()const = 0;

    protected:
        virtual bool processSyncModified(SyncProcessor* proc) = 0;
        virtual bool processSyncMoved(SyncProcessor* proc) = 0;
        virtual void createSyidMap(SyncProcessor* proc, SYID_2_ITEM& syid2item) = 0;
        virtual void checkIdentity(SyncProcessor* proc, cdb* otherDB) = 0;
    protected:
        ///modification counter
        COUNTER_TYPE m_mod_counter;
        ///sync process flags & indicators of required persistance
        SYNC_FLAGS m_fsync;
        cit_ptr m_owner2;
    };

    /**
       cit_base - basic atomic synchronizable object.
       Has SYID, mod-counter, move-counter
       also indexPos - position in some owner container,
       maintained by owner. from cit_base ordinary
       outline items are derived as well as attachable 
    */
    class cit_base : public cit_primitive
    {
        friend class cit;
        friend class SyncProcessor;
    public:
        cit_base():m_syid(INVALID_SYID),           
                   m_move_counter(INVALID_COUNTER)
        {m_bflags.flag = 0;}
        virtual ~cit_base(){};

        virtual QString     objName           ()const = 0;
        virtual DB_ID_TYPE  id                ()const = 0;
        virtual QString     title             ()const = 0;
        virtual void        setTitle          (QString title, bool guiRecalc = false);
        virtual QString     annotation()const { return ""; };       

        const SYID& syid()const{return m_syid;}

        virtual bool isSyncModified(SyncProcessor* proc)const override;
        virtual bool isSyncMoved(SyncProcessor* proc)const;
        void setSyncModified();
        void setSyncMoved();
        COUNTER_TYPE moveCounter()const{return m_move_counter;}    
        bool isValidSyid()const{return IS_VALID_SYID(m_syid);};
        static QString make_new_syid(DB_ID_TYPE db_id);

        virtual bool isAtomicIdenticalTo(const cit_primitive* other, int& iflags)const override;

        SYNC_FLAGS         fsync()const{return m_fsync;}
        virtual QString     dbgHint(QString s = "")const override;
        bool isSyncDbAttached()const{return (syncDb() != NULL);}
    
        cit_prim_ptr cloneSyncAtomic(SyncProcessor* proc)const override;
        QString compoundSignature()const override;
        virtual void cleanModifications() override;
    protected:
        //    bool isOtherDBItemSyncMoved(SyncProcessor* proc)const;
        void regenerateSyid();
        virtual void onRegeneratedSyid(){};
        virtual void onClearedSyid(){};        

        virtual void prepareSync(cdb* db)override;
    protected:
        SYID m_syid;
        COUNTER_TYPE m_move_counter;
    private:
        union bflags {
            uint32_t  flag;
            struct {
                uint16_t  userData;
                uint16_t  posIndex;
            };
        } m_bflags;
        //int          m_indexPos;
    };


    /**
       extension - we are additional property of a kind
       for an item, there will be only one extension of a kind
       for an item; extension can't be moved, only modified
       there could be many different type of extension
    */
  
    class extension: public cit_primitive
    {
        friend class cit;
        friend class util;
        friend class SyncProcessor;
    public:
        extension();
        ~extension();
    
		cit_ptr cit_owner() { return m_owner2; }
		const cit_ptr cit_owner()const { return m_owner2; }

        virtual bool        ensureExtPersistant(ArdDB*) = 0;

        virtual QString dbgHint(QString s = "")const override;
        void setSyncModified();
        bool processSyncModified(SyncProcessor* proc)override;
		bool processSyncMoved(SyncProcessor* )override { return false; };
        virtual EOBJ_EXT ext_type()const = 0;
        virtual QString  extName()const = 0;

        cit_prim_ptr cloneSyncAtomic(SyncProcessor* proc)const override;
		bool isSyncReadyToDelete(SyncProcessor*)const override { return false; };
		bool killSilently(bool)override { return false; };
    
		void attachPdb(persistantDB*)override {};
        QString compoundSignature()const override;

    protected:
        virtual cit_extension_ptr cloneToOtherDB(SyncProcessor* proc, cit_ptr it_other);
		void createSyidMap(SyncProcessor*, SYID_2_ITEM&)override {};
		void checkIdentity(SyncProcessor*, cdb* )override {};
		void syncStatAfterCloned(SyncProcessor*, const extension*) {};
		void syncStatAfterAssigned(SyncProcessor* p, const extension* other);

		//COUNTER_TYPE m_mod_counter;
		//cit_ptr m_owner2{nullptr};
    };


    /**
       cit - the base class for every item in outline.
       Can be collection of other cit items, also can
       contain collection of comments, images, links 
       and extensions and basic functions to maintain them
    */

    class cit: public cit_base
    {
        friend class SyncProcessor;
        friend class cdb;
        friend class util;
        friend class CompoundObserver;
    public:
        cit();
        virtual ~cit();
        virtual void clear();

        virtual int         pindex            ()const = 0;
        virtual void        setPindex         (int val)const = 0;
        virtual DB_ID_TYPE  id                ()const = 0;
        virtual EOBJ        otype             ()const{return objFolder;};
        virtual bool        ensurePersistant  (int depth, EPersBatchProcess pbatch = persAll) = 0;
        virtual bool        canAcceptChild    (const cit_ptr)const{return false;};
        virtual bool        canMove           ()const{return (cit_parent() != NULL) && !isSingleton();}
        virtual bool        canRename         ()const{return (cit_parent() != nullptr);}
        virtual bool        canRenameToAnything()const { return canRename(); }
        virtual STRING_SET  allowedTitles()const { STRING_SET st; return st; }
        virtual bool        canDelete         ()const{return (cit_parent() != NULL) && !isSingleton();}

        /***
            isRootTopic - a logical root topic, one table can maintain many roots though,
            similiar to logical hard drive in Windows file system
        */
        virtual bool        isRootTopic     ()const{return false;}
        /**
           isSynchronizable - means atomic (and compound) data of the topic can be replicated
           and synchronized. Even if topic is not synchronizable it's children can be
           root topics & sigleton topics are not synchronizable - they are made to be identical on
           all databases
        */
        virtual bool        isSynchronizable()const{return !isSingleton();}

        /**
            items itself can be synchronizable but it's subitems not,
            for example ethread (can be part of outline) is synchronizable
            but it's direct children - all of email type are not
        */
        virtual bool hasSynchronizableItems()const { return true; }

        /**
           singleton topic is predefined constant topic that can be only one
           per database instance - roots & special GTD sorting folders are singleton
        */
        virtual ESingletonType getSingletonType()const{return ESingletonType::none;}
        bool isSingleton()const{return (getSingletonType() != ESingletonType::none);}
        virtual bool IsUtilityFolder()const { return isSingleton(); };

        /// this will turn topic and it's branch into new steril topics (from sync point of view)
        /// that will be cloned over as new topics during next sync
        void clearSyncInfoForAllSubitems();

        size_t totalItemsCount()const;

        virtual void query_gui_4_comments()const=0;
        QString     dbgHint(QString s = "")const;
#ifdef ARD_BETA
        virtual void printInfo(QString _title)const;
#endif
        virtual bool isSyidIdenticalTo(const cit_ptr it2)const;
    public:
        void remove_cit(cit_ptr it, bool kill_it);
        void insert_cit(int position, cit_ptr it);
        void remove_all_cit(bool kill_it);
        int indexOf_cit(const cit* it)const;
        void rebuildCache();


        cit_ptr             cit_parent (){return cit_owner();}
        cit_cptr            cit_parent ()const{return cit_owner();}
        snc::cit*           cit_parent_raw();
        const snc::cit*     cit_parent_raw()const;

        CITEMS&            items      (){return m_items;}
        const CITEMS&      items      ()const{return m_items;}

        CEXTENSIONS&       extensions (){return m_extensions;}
        const CEXTENSIONS& extensions ()const{return m_extensions;}

        template<class T>
        std::vector<T*>             itemsAs();
        template<class T>
        std::vector<const T*>       itemsAs()const;
        template<class T>
        std::vector<T*>         itemsAs_Conditionally(std::function<bool(T*)>);


        virtual void cleanModifications();
        virtual bool checkTreeSanity()const;
        virtual bool checkItemSanity()const;

        int calcRankInBranch(cit_ptr b)const;
    
        cit_extension_ptr findExtensionByType(EOBJ_EXT ext);
        virtual bool addExtension(cit_extension_ptr);


        cit_ptr    findByTitleNonRecursive(QString title);
        cit_ptr    findBySyidNonRecursive(QString syid);
        void    memFindItems(MemFindPipe* mp);
#ifdef _DEBUG
        bool isOnLooseBranch()const;
#endif //_DEBUG
        uint16_t        userData()const{return m_bflags.userData;}
        void            setUserData(uint16_t v) { m_bflags.userData = v; }
        bool            isAncestorOf(const cit_ptr it)const;

    protected:
		virtual void	postCloneSyncAtomic(cit*, COUNTER_TYPE) {};
        virtual void	mapExtensionVar(cit_extension_ptr e) = 0;
        void			detachParent(){detachOwner();}

        virtual bool isSyncCompoundModifiedOrMoved(SyncProcessor* proc)const;
        virtual bool isSyncReadyToDelete(SyncProcessor* proc)const;

        virtual void prepareSync(cdb* db);
        ///markSyncOrphantAsMoved sync orphant means that our parent SYID 
        ///is different from other DB parent SYID and we are not marked as "moved"
        virtual bool markSyncOrphantAsMoved(SyncProcessor* proc);
        virtual bool processSyncModified(SyncProcessor* proc);
        virtual bool processSyncMoved(SyncProcessor* proc);
        //virtual bool processSyncDeleteAtt(SyncProcessor* proc);
        virtual bool processSyncDelete(SyncProcessor* proc);
        virtual bool processSyncCompound(SyncProcessor* proc);

        cit_ptr cloneToOtherDB(SyncProcessor* proc);
        virtual cit_ptr syncEnsureOtherDBParent(SyncProcessor* proc, int& insert_pos);
        bool syncAdjustOtherDBItemPosition(SyncProcessor* snc, cit_ptr OtherDBItem);

        virtual void checkIdentity(SyncProcessor* proc, cdb* otherDB);
        virtual void createSyidMap(SyncProcessor* proc, SYID_2_ITEM& syid2item);

        void onRegeneratedSyid();
        void onClearedSyid();
        void syncStatBeforeKilled(SyncProcessor* p);
        void syncStatCompoundBeforeKilled(SyncProcessor* p);
        void syncStatAfterCloned(SyncProcessor* p, cit_cptr source);
        void syncStatOnAmbiguous(SyncProcessor* p, cit_cptr otherDBItems);
        void syncStatAfterAssigned(SyncProcessor* p, cit_cptr other);
       // virtual void requestVisibleItemsCalculation()const{};
    protected:
        CITEMS                   m_items;
        CEXTENSIONS              m_extensions;
    };

};

template<class T>
std::vector<T*> snc::cit::itemsAs()
{
    std::vector<T*> rv;
    using ITR = snc::CITEMS::iterator;
    for (ITR it = m_items.begin(); it != m_items.end(); it++) {
        T* f = dynamic_cast<T*>(*it);
        if (f) {
            rv.push_back(f);
        }
    }
    return rv;
};

template<class T>
std::vector<const T*> snc::cit::itemsAs()const
{
    std::vector<const T*> rv;
    using ITR = snc::CITEMS::const_iterator;
    for (ITR it = m_items.begin(); it != m_items.end(); it++) {
        T* f = dynamic_cast<T*>(*it);
        if (f) {
            rv.push_back(f);
        }
    }
    return rv;
};


template<class T>
std::vector<T*> snc::cit::itemsAs_Conditionally(std::function<bool(T*)> cond)
{
    std::vector<T*> rv;
    using ITR = snc::CITEMS::iterator;
    for (ITR it = m_items.begin(); it != m_items.end(); it++) {
        T* f = dynamic_cast<T*>(*it);
        if (f && cond(f)) {
            rv.push_back(f);
        }
    }
    return rv;
};

extern bool sync_2SDB(snc::persistantDB* p1, snc::persistantDB* p2, bool& changes_detected, bool& hashOK, QString hint, STRING_MAP& hashCompareResult, snc::SyncProgressStatus* progressStatus);
#define SYNC_EPOCH_TIME 1351180607


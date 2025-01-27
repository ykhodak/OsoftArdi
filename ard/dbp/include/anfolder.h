#pragma once

#include<QTextLayout>

#include "extension.h"
#include "snc.h"
#include "dbp.h"
#include "a-db-utils.h"
#include "mpool.h"
#include "GoogleClient.h"
#include "gmail/GmailRoutes.h"

using namespace snc;

/**
   topic - collection of items or singleton item in outline
   All items in GUI/outline are topics 
*/
namespace ard 
{
	using topic_function = std::function<void(ard::topic*)>;

	class note_ext;
	class q_param;
	class email_model;

	class topic : public snc::cit
	{
#ifdef _DEBUG
		friend class q_param;
#endif
		DECLARE_IN_ALLOC_POOL(topic);
		friend class ::ArdDB;
	public:
		topic();
		topic(QString title, EFolderType ftype = EFolderType::folderGeneric);
		virtual ~topic();

		///parent topic
		topic*					parent();
		const topic*			parent()const;


		QString             objName()const override;
		QString             folderTypeAsString()const;
		virtual QPixmap     getIcon(OutlineContext)const;
		virtual QPixmap     getSecondaryIcon(OutlineContext)const;
		virtual bool		hasRButton()const { return false; }
		virtual QPixmap     getRButton(OutlineContext)const { return QPixmap(); };
		EFolderType         folder_type()const { return (EFolderType)m_attr.FolderType; }
		///returns extra space after icon in outline, if any, before title
		virtual std::pair<TernaryIconType, int>   ternaryIconWidth()const;
		int                 subType()const{ return static_cast<int>(folder_type()); }
		virtual QString     title()const override;
		void                setTitle(QString title, bool guiRecalc = false)override;
		QString             shortTitle()const;
		virtual QString     serializable_title()const { return title(); }

		QString             annotation()const override;
		virtual QString     annotation4outline()const;
		virtual void        setAnnotation(QString s, bool gui_update = false);
		virtual bool        canHaveAnnotation()const;

		///if we have title, return it, otherwise calculate title out of notes
		virtual QString     impliedTitle()const;
		///same as impliedTitle for regular topics, but can be a short label for Folders
		virtual QString     altShortTitle()const;

		virtual bool        hasText4SearchFilter(const TextFilterContext& fc)const;

		virtual topic_ptr   clone()const;
		///limited atomic clone, take only essentials - title, annotation, note
		virtual topic_ptr   cloneInMerge()const;
		cit_prim_ptr        create()const override { return new topic; }
		virtual topic_ptr   tspaceTopic() { return this; }
		/// label subject - object that can me labeled in gmail, like 'email' or 'thread'
		virtual topic_ptr       produceLabelSubject() { return nullptr; }
		virtual topic_ptr       produceMoveSource(ard::email_model*) { return this; };
		virtual topic_ptr       outlineCompanion() { return nullptr; }

		/// for regular objects it's just this
		/// for shotcut objects it's obj that shortcut is referencing
		virtual topic_ptr    shortcutUnderlying() { return this; };
		virtual topic_cptr   shortcutUnderlying()const { return this; };
		///clicked on some action button in outline,
		///usually opens topic note view
		virtual bool        hasFatFinger()const { return true; }
		virtual void        fatFingerSelect();
		virtual void        fatFingerDetails(const QPoint& p);
		virtual bool        isContentLoaded()const { return true; };

		//virtual void        toggleStarred() {};
		virtual void        setStarred(bool) {};
		virtual void        setImportant(bool set_it);
		void                markAsModified();

		virtual void			setColorIndex(EColor c);
		virtual EColor			colorIndex()const;


		/// if done_percent -1 we ignore it, if prio is unknown we ignore it
		virtual bool			isToDo()const;
		virtual bool			isToDoDone()const;
		virtual int				getToDoDonePercent()const;
		virtual int				getToDoDonePercent4GUI()const { return getToDoDonePercent(); };
		virtual unsigned char	getToDoPriorityAsInt()const;
		virtual ToDoPriority	getToDoPriority()const;
		virtual bool			hasToDoPriority()const;
		virtual void			setToDo(int done_percent, ToDoPriority prio);
		virtual topic_ptr		getToDoContext() { return this; }
		virtual bool			isImportant()const { return hasToDoPriority(); }

		void                query_gui_4_comments()const override;

		virtual bool        isInLocusTab()const;
		virtual void        setInLocusTab(bool val = true);
		virtual bool        canCloseLocusTab()const { return true; };

		///retired - marked are ready to delete
		bool                isRetired()const;
		void                setRetired(bool val = true);
		uint64_t            mflag4serial()const;
		void				serial2mflag(uint64_t v);

		///addItem - add a child item
		virtual bool        addItem(topic_ptr it);
		///insertItem - insert child item at a position
		bool                insertItem(topic_ptr it, int index, bool raw_inner_insert = false);
		///detachItem - the item won't be killed and can be moved to new parent
		void                detachItem(topic_ptr it);

		bool                moveSubItemsFrom(topic_ptr parent);

		virtual FieldParts  fieldValues(EColumnType column_type, QString type_label)const;
		virtual QString     fieldMergedValue(EColumnType column_type, QString type_label)const;
		virtual void        setFieldValues(EColumnType column_type, QString type_label, const FieldParts& parts);
		virtual EColumnType treatFieldEditorRequest(EColumnType column_type)const { return column_type; };
		virtual TOPICS_LIST produceFormTopics(std::set<EColumnType>* include_columns = nullptr,
			std::set<EColumnType>* exclude_columns = nullptr);
		virtual EColumnType formNotesColumn()const { return EColumnType::Uknown; }
		virtual QString     formValue()const { return ""; };
		virtual QString     formValueLabel()const { return ""; };
		virtual InPlaceEditorStyle inplaceEditorStyle(EColumnType)const { return InPlaceEditorStyle::regular; }

		virtual ENoteView   noteViewType()const;
		virtual bool		isValid()const { return true; }

		///merge - grab all items from given topic
		void                takeAllItemsFrom(topic_ptr from);
		///killAllItemsSilently - delete all subtopics from DB permanently
		virtual void        killAllItemsSilently();
		///indexOf - returns index of item or -1 if not found
		int                 indexOf(topic_cptr it)const;
		//virtual bool        isVisibleChild(topic_cptr)const { return true; }
		///isExpanded - true if topic expanded in outline
		bool                isExpanded()const { return (m_attr.isExpanded == 1); }
		///setExpanded - expands topic in outline
		virtual void        setExpanded(bool val = true);
		virtual bool        isExpandedInList()const { return false; }
		///isRecycleBin - recycle bin is special kind of a topic
		bool                isRecycleBin()const { return (folder_type() == EFolderType::folderRecycle); }
		///isGtdSortingFolder - a special folder recycle, maybe, reference etc.
		bool                isGtdSortingFolder()const;
		///hoisted to host outline to locate the topic
		virtual ard::locus_folder*   getLocusFolder();

		/// for regular topics its dbid, for wrappers - ID of underlying obj, it should be considered in context
		virtual DB_ID_TYPE  underlyingDbid()const { return id(); }

		ESingletonType      getSingletonType()const override;

		virtual bool        isWrapper()const { return false; }

		virtual void		applyOnVisible(std::function<void(ard::topic*)> fnc);

		bool                init_from_db(ArdDB* db, QSqlQuery& q);
		virtual void        setupRootDbItem(ArdDB* db, QSqlQuery& q);
		void                setupAsRoot(ArdDB* db, QString title);
		void                detachDB();
		void                setPindex(int val)const override;


		virtual bool        killSilently(bool) override;
		virtual bool        isEmptyTopic()const;
		virtual void        deleteEmptyTopics();
		virtual bool        isReadyToAutoClean()const { return isEmptyTopic(); }

		void                assignSyncAtomicContent(const cit_primitive* other) override;
		bool                canMove()const override;
		virtual bool        canBeMemberOf(const topic_ptr parent_folder)const;
		bool                canAcceptChild(cit_cptr it)const override;
		virtual bool        canBeCloned()const { return (parent() != nullptr); }
		virtual bool        canChangeToDo()const { return !isSingleton(); };
		bool                ensurePersistant(int depth, EPersBatchProcess pbatch = persAll) override;

		TOPICS_LIST          selectAllThreads();
		TOPICS_LIST          selectAllBookmarks();
		DEPTH_TOPICS         selectDepth();

		///current command if command for currently selected in outline item
		virtual bool        hasCurrentCommand(ECurrentCommand c)const;
		//virtual std::vector<ECurrentCommand> currentCommands()const;


		template<class ResContainer, class Sel>
		void                selectItemsBreadthFirst(ResContainer& items_list, Sel&);
		template<class Sel>
		void                selectItemsDepthFirst(DEPTH_TOPICS& items_list, Sel&, int depth);

		///findTopicByTitle - non recursive call
		topic_ptr            findTopicByTitle(QString title);
		///findTopicBySyid - non recursive call
		topic_ptr            findTopicBySyid(QString syid);

		void                emptyRecycle();

		topic_ptr           gtdGetSortingFolder();
		const topic_ptr     gtdGetSortingFolder()const;

		/// utility folders can't be renamed, don't have annotation, todo, can't be deleted or moved
		/// usually they are top hoisted topic
		bool IsUtilityFolder()const override;
		virtual bool isStandardLocusable()const { return true; };

		///returns false if any of item inside topic is not retired
		bool               isTopicRetired();
		///returns false if any of item inside topic is not completed
		bool               isTopicCompleted();

		virtual bool        canHaveCryptoNote()const;
		virtual void        removeExternalStorage() {};
#ifdef ARD_GD
		virtual bool        hasAttachment()const { return false; };
		virtual std::vector<googleQt::mail_cache::label_ptr> getLabels()const { return std::vector<googleQt::mail_cache::label_ptr>(); };
#endif

#ifdef _DEBUG
		void                dbgPrintPrjInfo();
		bool                hasDebugMark1()const;
		void                setDebugMark1(bool bval);
#endif
#ifdef ARD_BETA
		void               printInfo(QString _title)const override;
#endif
		QString            calcContentHashString()const override;
		uint64_t           contentSize()const override;
		///this is hash on title and notes, nothing else extra
		virtual QString     calcMergeAtomicHash()const;
		void                printIdentityDiff(topic_ptr _other);
		bool                skipMergeImport()const;
		void                setSkipMergeImport(bool val);

		static bool        TitleLess(topic_ptr it1, topic_ptr it2);

		virtual bool       canHaveCard()const;
		virtual bool       canHaveCardToolOptButton()const { return true; };
		void               formatNotes(QTextCharFormat& frm);

		///afterBuildParamMap - needed for topics drawn as root in graph
		///the map contains parameters applied to the graph after build
		virtual const PARAM_MAP* afterBuildParamMap()const { return NULL; };

		snc::cdb*        syncDb()override;
		const snc::cdb*  syncDb()const override;
		void attachPdb(snc::persistantDB*)override;

		void               ensure_ExtensionPersistance(ArdDB*);

		bool                isAtomicIdenticalTo(const cit_primitive* other, int& iflags)const override;
		virtual bool        isAtomicIdenticalPropTo(const cit_primitive* other, int& iflags)const;
		virtual void        assignContent(cit_cptr other);

		virtual bool					canAttachNote()const { return !isSingleton(); }
		static topic_ptr                createNewNote(QString _title = "", QString _html = "", QString _plain_text = "");
		virtual ard::note_ext*         mainNote();
		virtual const ard::note_ext*   mainNote()const;
		bool                            hasNote()const;
		virtual topic_ptr               popupTopic() { return this; }
		/// this is for blackboard
		virtual QString         plainNoteText4Blackboard()const;
		virtual topic_ptr       prepareInjectIntoBBoard();
		virtual snc::CITEMS     prepareBBoardInjectSubItems() { return m_items; }
		virtual bool            isExpandableInBBoard()const;
		virtual QSize           calcBlackboardTextBoxSize()const;

		void                    setMainNoteText(QString sPlainText);
		void                    makeTitleFromNote();
		bool                    hasUrl()const { return false; };

		virtual void            onModifiedNote() {};

		void                toJson(QJsonObject& js)const;
		void                fromJson(const QJsonObject& js);

		/// syid needed while applying GUI action, we might not have sync initiated yet
		void                guiEnsureSyid(const ArdDB*, QString prefix = "L");

		virtual DB_ID_TYPE  id()const override { return m_id; }
		virtual ArdDB*      dataDb() { return  m_data_db; }
		virtual const ArdDB* dataDb()const { return  m_data_db; }
		virtual int         pindex()const override { return m_pindex; }
		virtual EOBJ        otype()const override { return objFolder; }
		virtual void        unregister() override;
		void                detachDb() { m_id = 0; m_data_db = nullptr; }

		/// thhumbnail icon in outline
		bool				isThumbDirty()const { return m_attr.isThumbDirty; }
		void				setThumbDirty() { m_attr.isThumbDirty = 1; };
		virtual const QPixmap*	thumbnail()const { return nullptr; }
		virtual bool		hasThumbnail()const { return false; }

		virtual bool        isPersistant()const { return true; }

		bool				rollUpToRoot();
		time_t				mod_time()const { return m_mod_time; }
		void				set_mod_time(time_t v) { m_mod_time = v; }
		virtual QString     dateColumnLabel()const { return ""; };
		virtual QString     tabMarkLabel()const { return ""; };
		///wrappedId - string/cloud emailId or threadId
		virtual QString     wrappedId()const { return ""; }

		std::pair<char, char> getColorHashIndex()const;
		virtual bool		hasColorByHashIndex()const { return false; }
		virtual bool        hasCustomTitleFormatting()const { return false; }
		virtual void        prepareCustomTitleFormatting(const QFont&, QList<QTextLayout::FormatRange>&) {}


		//last outline height/with - cached data of outlined items, if any '0' treat as 'invalied'/need recalculation
		unsigned            lastOutlinePanelWidht()const { return m_attr.lastOutlinePanelWidth; }
		unsigned            lastOutlineHeight()const { return m_attr.lastOutlineHeight; };
		void                setLastOutlinePanelWidht(int v) { m_attr.lastOutlinePanelWidth = v; }
		void                setLastOutlineHeight(int v) { m_attr.lastOutlineHeight = v; }
		void                forceOutlineHeightRecalc() { m_attr.lastOutlinePanelWidth = 0; m_attr.lastOutlineHeight = 0; };

		bool               isTmpSelected()const { return m_attr.isTmpSelected; };
		void               setTmpSelected(bool v);

		void                mark_gui_4_comments_queried();
		bool                isNoteLoadedFomDB()const { return (m_attr.optNoteLoadedFromDB == 1); }
		template<class T>
		void				selecFromTree(std::function<bool(ard::topic*)> selectFunc, std::unordered_set<T*>& selected);
	protected:
		void mapExtensionVar(cit_extension_ptr e)override;
		template <class T>
		void                ensureExtension(T*& m_O);

		virtual ard::note_ext* ensureNote();

		void                select4kill(TOPICS_LIST& items_list);
		void                rebalance_pindex();
		bool                need_pindex_rebalancing()const;
		void                assign_child_pindex(topic_ptr it, int index);
		void                handle_child_retire(topic_ptr it);
		void                doDetachItem(topic_ptr it);
	private:
		void                setup_id_for_new_local_topic(ArdDB* db, DB_ID_TYPE _id);
		bool                ensurePersistantItem(int depth, EPersBatchProcess pbatch = persAll);

	protected:
		ard::note_ext*			m_note_ext{ nullptr };
		DB_ID_TYPE				m_id{ 0 };
		ArdDB*					m_data_db{ nullptr };
		QString					m_title, m_annotation;
		time_t					m_mod_time;
		mutable UObjAttrib      m_attr;
		mutable int				m_pindex{ -1 };
	};

	class thumb_topic : public ard::topic
	{
	public:
		thumb_topic(QString title);

		const QPixmap*          thumbnail()const override;
		void                    setThumbnail(QPixmap pm)const;
		bool					isThumbnailEmpty()const { return m_thumb_flags.isThumbnailEmpty; };
		bool					hasThumbnail()const override { return true; }
		virtual bool			autogenerateThumbnail()const { return true; }
	protected:
		QString					thumbnailFileName()const;
		virtual QString			thumbnailFileNamePrefix()const = 0;
		virtual QPixmap			emptyThumb()const = 0;
		void					removeExternalStorage()override;

		mutable union UBoardAttrib {
			uint8_t flags;
			struct {
				unsigned        isThumbnailQueried : 1;
				unsigned        isThumbnailEmpty : 1;
				unsigned        isImageQueried : 1;
				unsigned        isImageEmpty : 1;

			};
		} m_thumb_flags;
		mutable QPixmap         m_thumbnail;
	};
}


template <class T>
void ard::topic::ensureExtension(T*& e)
{
	if (!e) {
		auto e2 = new T;
		addExtension(e2);
#ifdef _DEBUG
		ASSERT(e == e2, "internal error, probably missing entry in mapExtensionVar") << e2->extName() << dbgHint();
		ASSERT(e2->cit_owner() == this, "expected this as owner");
		ASSERT(extensions().find(e2) != extensions().end(), "internal extension logic error");
#endif
	}
};

template <class T>
void ard::topic::selecFromTree(std::function<bool(ard::topic*)> selectFunc, std::unordered_set<T*>& selected)
{
	if (!isSingleton() && selectFunc(this)) {
		auto o = dynamic_cast<T*>(this);
		if(o)selected.insert(o); 
	}
	for (auto& i : items()) {
		auto it = dynamic_cast<ard::topic*>(i);
		assert_return_void(it != nullptr, "expected topic");
		if (!it->isSingleton() && selectFunc(it)) {
			auto o = dynamic_cast<T*>(it);
			if(o)selected.insert(o);
		}
	};

	for (auto& i : items()) {
		auto it = dynamic_cast<ard::topic*>(i);
		it->selecFromTree(selectFunc, selected);
	}
};


#define COMPATIBLE_PARENT_CHILD(P, C)(P->canAcceptChild(C) && C->canBeMemberOf(P))

#pragma once

#include <memory>
#include <QSqlQuery>
#include "a-db-utils.h"
#include "anfolder.h"
#include "tooltopic.h"
#include "ansyncdb.h"
#include "board_links.h"
#include "rule.h"
#include "locus_folder.h"

namespace ard {
    class boards_root;
    class selector_board;
	class mail_board;
	class folders_board;
    class board_item;
    class board_band_info;
	class rule;	

    using BITEMS        = std::vector<board_item*>;
    using BITEMS_SET    = std::unordered_set<board_item*>;
    using C2BITEMS      = std::vector<BITEMS>;
    using O2LINKS       = std::unordered_map<board_item*, board_link_list*>;
    using O2O2LINKS     = std::unordered_map<board_item*, O2LINKS>;//o->t->[l]
    using SYID2BITEM    = std::unordered_map<QString, board_item*>;
    using BANDS         = std::vector<board_band_info*>;
    using BAND_IDX      = std::vector<int>;
	using RULE2BAND		= std::unordered_map<ard::q_param*, ard::board_band_info*>;
	using FOLDER2BAND	= std::unordered_map<ard::topic*, ard::board_band_info*>;

    enum class BoardItemStatus
    {
        undefined,
        normal,
        unresolved_ref_link,
        removed
    };

    enum class BoardItemShape
    {
        unknown = -1,
        box = 0,
        circle,
        triangle,
        rombus,
        pentagon,
        hexagon,
        text_normal,
        text_italic,
        text_bold
    };

    enum class InsertBranchType 
    {
        none,
        single_topic,
        branch_expanded_to_right,
        branch_expanded_to_left,
        branch_expanded_from_center,
        branch_top_group_expanded_to_right,
        branch_top_group_expanded_to_down,
    };

    struct InsertBranchResult
    {
        ard::BAND_IDX   bands;
        ard::BITEMS     bitems;
        ard::BITEMS     origins;
    };

    struct InsertTopicsListResult
    {
        ard::BAND_IDX   bands;
        ard::BITEMS     new_items;
        ard::BITEMS     located_items;
    };

    class boards_model
    {
    public:
        boards_model(ArdDB* db);
		~boards_model();

        ard::boards_root*           boards_root();
        const ard::boards_root*     boards_root()const;
		ard::mail_board*			mail_board();
		const ard::mail_board*		mail_board()const;
		ard::folders_board*			folders_board();
		const ard::folders_board*	folders_board()const;

        static QPolygon build_shape(BoardItemShape shp, QRect rc);
        static QRect    calc_edit_rect(BoardItemShape shp, QRect rc);
        static QString  shape_name(BoardItemShape shp);
    protected:
		ard::boards_root*   m_boards_root{ nullptr };
		ard::mail_board*	m_mail_board{ nullptr };
		ard::folders_board*	m_folders_board{ nullptr };
    };

    /// holder of blackboards
    class boards_root : public RootTopic
    {
    public:
        DECLARE_ROOT(boards_root, objBoardRoot, ESingletonType::boardsHolder);
		~boards_root();
        QString                 title()const override { return "Boards"; };

		QString                 objName()const override { return "BRoot"; };
        selector_board*         addBoard(TOPICS_LIST* lst = nullptr);
		selector_board*         cloneBoard(selector_board* source);
        ard::board_link_map*    lookupLinkMap(QString syid)const;
        ard::board_link_map*    createLinkMap(QString syid)const;
        ard::board_link_map*    clone_lmap(ard::board_link_map* src, QString clone_board_syid, const SYID2SYID& source2clone)const;
        const S2BL&             allLinkMaps()const { return m_syid2lnk_map; }
        void                    ensurePersistantAllLinkMaps(ArdDB* db);

        void                    loadBoardLinksFromDb(QSqlQuery* q);
        void                    killAllItemsSilently()override;
		void					prepare_link_sync();
		snc::CompoundInfo		compileCompoundInfo()const;
    protected:
        mutable S2BL            m_syid2lnk_map;
    };

    /// band info
    class board_band_info :public snc::smart_counter
    {
    public:
        board_band_info(QString str = "");
 
		int     bandIndex()const {return m_band_index;}
        QString bandLabel()const;
        int     bandWidth()const {return m_band_width;}

        bool    setBandWidth(int w);
        bool    setBandLabel(QString s);

        ///non-persistant properties
        QColor  color()const {return m_color;}
        int     xstart()const {return m_xstart;}
        void    set_color(QColor c)const { m_color = c; }
        void    set_x_start(int x)const { m_xstart = x; }

        void    toJson(QJsonObject& js)const;
        void    fromJson(const QJsonObject& js);

        QString dbgHint(QString s = "")const;
    protected:
        QString m_band_label;///if label is empty we show numbers as label
        int     m_band_index{ 0 };
        int     m_band_width{ BBOARD_BAND_DEFAULT_WIDTH };
        mutable QColor  m_color;
        mutable int     m_xstart{ 0 };
        friend class band_board;
    };

	class band_board : public ard::thumb_topic
	{
	public:
		band_board(QString title = "");
		~band_board();

		const BANDS&            bands()const { return m_bands; }
		board_band_info*        bandAt(int idx)const;
		virtual bool            setBandWidth(int idx, int w);
		bool					setBandLabel(int idx, QString s);
		void					clearBands();
	protected:
		virtual void			markBandsModified() = 0;
		void                    resetBandsIdx();
		QPixmap					emptyThumb()const override;
		BANDS					m_bands;
	};

	template<class T>
	class mapped_topics_board : public ard::band_board 
	{
	public:
		~mapped_topics_board()									{ clearMappedTopicsBoard(); };

		virtual void	rebuildMappedTopicsBoard() = 0;
		void	clearMappedTopicsBoard();

		std::unordered_map<T*, ard::board_band_info*>&			t2b() { return m_t2b; }
		const std::unordered_map<T*, ard::board_band_info*>&	t2b()const { return m_t2b; }
		std::vector<T*>&										topics() { return m_topics; }
		const std::vector<T*>&									topics()const { return m_topics; }
		bool													setBandWidth(int idx, int w)override;
	protected:
		void			markBandsModified()override {};
		std::unordered_map<T*, ard::board_band_info*>		m_t2b;
		std::vector<T*>										m_topics;
	};

	/// mail filters on the board
	class mail_board : public ard::mapped_topics_board<ard::q_param>
	{
	public:
		mail_board();
		void	rebuildMappedTopicsBoard()override;

		ENoteView       noteViewType()const override { return ENoteView::MailBoard; };
		EOBJ            otype()const override { return objMailBoard; };
		//bool			setBandWidth(int idx, int w)override;
		bool			autogenerateThumbnail()const override{ return false; }
	protected:
		QString			thumbnailFileNamePrefix()const override { return "mb"; }

	};

	/// top selector folders	
	class folders_board : public ard::mapped_topics_board<ard::locus_folder>
	{
	public:
		folders_board();
		void	rebuildMappedTopicsBoard()override;

		ENoteView       noteViewType()const override { return ENoteView::FoldersBoard; };
		EOBJ            otype()const override { return objFoldersBoard; };
		//bool			setBandWidth(int idx, int w)override;
	protected:
		QString			thumbnailFileNamePrefix()const override { return "fb"; }
	};

    /// board - collection of topic links and labels
    class selector_board : public ard::band_board
    {
    public:
        selector_board(QString title = "");
        
        void                    insertBand(int idx);
        void                    addBands(int num);
        void                    removeBandAt(int idx);

        const C2BITEMS&         band2items()const { return m_b2items; }
		

        /// put topic into board, returns pair of lists - new and existing(located) items
        std::pair<ard::BITEMS, ard::BITEMS> insertTopicsBList(const TOPICS_LIST& lst, int band_idx, int ypos=0, int ydelta=0, BoardItemShape shp = ard::BoardItemShape::box);
        /// place topcics in separate band - expand horizontally
        ard::InsertTopicsListResult insertTopicsBListInSeparateColumns(const TOPICS_LIST& lst, int start_band_idx, int step_band_index = 1, BoardItemShape shp = ard::BoardItemShape::box);

        void                    removeBItem(board_item*, int yDeltaAdj);
        board_item*             findBItem(topic_ptr);
        InsertBranchResult      insertTopicsWithBranches(InsertBranchType insert_type, const TOPICS_LIST& lst, int band_idx, int ypos = -1, int ydelta = 0, BoardItemShape shp = ard::BoardItemShape::box, int band_space = 1);

        ard::board_link_list*   addBoardLink(board_item* origin, board_item* target);
        void                    removeBoardLink(ard::board_link* lnk);
        void                    removeBoardLinks4Origin(board_item* origin);
        const O2O2LINKS&        adjList()const;
        const O2O2LINKS&        radjList()const;

        boards_root*            broot();
        const boards_root*      broot()const;
        board_ext*              bext();
        const board_ext*        bext()const;
        board_ext*              ensureBExt();
		QString                 objName()const override { return "board"; };
        EOBJ                    otype()const override { return objBoard; };
        ENoteView               noteViewType()const override { return ENoteView::SelectorBoard; };
        cit_prim_ptr            create()const override { return new ard::selector_board; };
        

        void                    moveToBand(ard::BITEMS& bitems, int bidx, int yPos, int yDelta, qreal height);
        board_item*             getNextBItem(ard::board_item* bi, Next2Item n);
        board_item*             getNext2SelectBItem(ard::board_item* bi);
        board_link*             getNext2SelectBLink(ard::board_link* lnk);


        ard::note_ext*         mainNote()override { return nullptr; };
        const ard::note_ext*   mainNote()const override { return nullptr; };
        bool                    canBeMemberOf(const topic_ptr)const override;
        bool                    canAcceptChild(cit_cptr it)const override;
        bool                    ensurePersistant(int depth, EPersBatchProcess pbatch = persAll) override;
        bool                    killSilently(bool) override;
        topic_ptr               clone()const override;
        
        ard::board_item*        findBitemBySyid(QString syid);
        void                    bandsToJson(QJsonObject& js)const;
        void                    bandsFromJson(QJsonObject& js);

        ///new topics created in bboard will be collected in outline in special folder
        topic_ptr               ensureOutlineTopicsHolder();

#ifdef _DEBUG
        void                    debugFunction();
#endif
    protected:
		void					markBandsModified()override;
		void					postCloneSyncAtomic(cit*, COUNTER_TYPE)override;
        void                    mapExtensionVar(cit_extension_ptr e)override;
        void                    resetYPosInBands(ArdDB* db, const std::vector<int>& bands_idx);
        void                    resetLinksPindex(ArdDB* db);
        void                    rebuildBoardFromDb(const ArdDB* db, ard::board_link_map* lmap, IDS_SET& invalid_bitems);
        BITEMS                  doInsertTopicsWithBranches(board_item* parent_bi, const TOPICS_LIST& lst, int band_idx, BAND_IDX& idxset, BITEMS& origins, bool expand2right, BoardItemShape shp);
        void                    register_link_list(ard::board_link_list* lst, ard::board_item* origin, ard::board_item* target);
        void                    reset_lrpos_link_indexes(ard::board_item* origin);
        BITEMS                  getOriginsPointingToTarget(ard::board_item* target);
        void                    doRemoveBoardItemAndLinks(board_item* bi);
        BITEMS                  getItemsInBandRange(const std::vector<int>& band_idx);
        void                    updateBIndexInBandRange(const std::vector<int>& band_idx, int idx_delta);
		QString					thumbnailFileNamePrefix()const override { return "b"; }
    protected:
        board_ext*          m_bext{ nullptr };
        C2BITEMS            m_b2items;
        O2O2LINKS           m_o2t_adj, m_t2o_adj;
        SYID2BITEM          m_syid2bi;
        board_link_map*     m_lmap{nullptr};;
        friend class        ::ArdDB;
    };

    /// boards details
    class board_ext : public ardiExtension<board_ext, selector_board>
    {
        DECLARE_DB_EXTENSION_PERSISTANT(snc::EOBJ_EXT::extBoard, "board-ext", "ard_ext_bboard");
    public:
        ///default constructor
        board_ext();
        ///for recovering from DB
        board_ext(topic_ptr _owner, QSqlQuery& q);

        void   assignSyncAtomicContent(const cit_primitive* other)override;
        snc::cit_primitive* create()const override;
        bool   isAtomicIdenticalTo(const cit_primitive* other, int& iflags)const override;
        QString calcContentHashString()const override;
        uint64_t contentSize()const override;

        QByteArray  bandsPayload()const;

        void        clearModified() { m_modified = false; }
        void        setModified();
        bool        isModified()const { return m_modified; };

    protected:
        bool        m_modified{ false };
        mutable QByteArray  m_bands_payload;

        friend class selector_board;
    };


    /// board item - can be link to a topic or label
    class board_item : public ard::topic
    {
    public:
        board_item();
        board_item(topic_ptr f, ard::BoardItemShape shp);

        topic_ptr               refTopic();
        topic_cptr              refTopic()const;

        selector_board*         getBoard();
        const selector_board*   getBoard()const;
        board_item_ext*         biext();
        const board_item_ext*   biext()const;
        board_item_ext*         ensureBIExt();
        QString                 objName()const override;
        EOBJ                    otype()const override { return objBoardItem; };
        BoardItemShape          bshape()const;
        cit_prim_ptr            create()const override { return new ard::board_item; };
        topic_ptr               clone()const override;

        void                    setBShape(BoardItemShape );

        ard::note_ext*         mainNote()override { return nullptr; };
        const ard::note_ext*   mainNote()const override { return nullptr; };
        bool                    canBeMemberOf(const topic_ptr)const override;
        bool                    killSilently(bool) override;

        int                     bandIndex()const;
        int                     yPos()const;
        int                     yDelta()const;
        void                    setBandIndex(int band_index);
        void                    setYDelta(int y_delta);
        void                    markStatus(BoardItemStatus);

    protected:
        void            mapExtensionVar(cit_extension_ptr e)override;
        topic_ptr       resolveRefTopic(const ArdDB* db);
    protected:
        board_item_ext*     m_biext{ nullptr };
        friend class selector_board;
    };

    /// board item details
    class board_item_ext : public ardiExtension<board_item_ext, board_item>
    {
        DECLARE_DB_EXTENSION_PERSISTANT(snc::EOBJ_EXT::extBoardItem, "bitem-ext", "ard_ext_bboard_item");
    public:
        ///default constructor
        board_item_ext();
        ///for recovering from DB
        board_item_ext(topic_ptr _owner, QSqlQuery& q);

        virtual ~board_item_ext();
        
        BoardItemShape      bshape()const { return m_bshape; }
        QString             ref_topic_syid()const { return m_ref_topic_syid; };
        ///index of band or column
        int                 bandIndex()const { return m_band_index; }
        ///y-pos inside band, order in list
        int                 yPos()const { return m_y_pos; }
        ///delta in units from previous item in band, if '0' - it's minimum (one unit)
        int                 yDelta()const { return m_y_delta; }

        void   assignSyncAtomicContent(const cit_primitive* other)override;
        snc::cit_primitive* create()const override;
        bool   isAtomicIdenticalTo(const cit_primitive* other, int& iflags)const override;
        QString calcContentHashString()const override;
        uint64_t contentSize()const override;

    protected:
        void        setRefTopic(topic_ptr f);
        void        clearRefTopic();
        topic_ptr   resolveRefTopic(const ArdDB* db);


        BoardItemShape  m_bshape{ BoardItemShape::box };
        int             m_band_index{ 0 }, m_y_pos{ 0 }, m_y_delta{ 0 };
        topic_ptr       m_ref_topic{nullptr};
        QString         m_ref_topic_syid;
        BoardItemStatus m_status{ BoardItemStatus::undefined};

        friend class selector_board;
        friend class board_item;
    };


    bool    isBoxlikeShape(ard::BoardItemShape sh);
    bool    isTextlikeShape(ard::BoardItemShape sh);
    QFont*  getTextlikeFont(ard::BoardItemShape sh);
};


template<class T>
void ard::mapped_topics_board<T>::clearMappedTopicsBoard() 
{
	clearBands();
	for (auto& i : m_t2b) {
		i.first->release();
	}
	m_t2b.clear();
	m_topics.clear();
};

template<class T>
bool ard::mapped_topics_board<T>::setBandWidth(int idx, int w)
{
	ard::band_board::setBandWidth(idx, w);
	auto r = m_topics[idx];
	r->setBoardBandWidth(w);
	return true;
};
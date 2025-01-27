#pragma once
#include "workspace.h"
#include "custom-widgets.h"
#include "BlackBoardItems.h"


namespace ard {
	class search_edit;
	class selector_board;
    class BoardScene;
    class BlackBoardToolbar;
    using B2G           = std::unordered_map<ard::board_item*, ard::black_board_g_topic*>;
    using L2G           = std::unordered_map<ard::board_link*, ard::GBLink*>;
    using LNK_LIST      = std::list<ard::GBLink*>;
    using GB_LIST       = std::vector<ard::black_board_g_topic*>;
    using B2LG          = std::unordered_map<ard::board_item*, LNK_LIST>;
 
    class BlackBoard : public board_page<ard::selector_board>
    {
        struct BoardDropPos
        {
            int bandIndex{ -1 };
            int yPos{ -1 };
            int yDelta{ 0 };
        };
        enum class CustomMode 
        {
            none,
            link,
            locate,
            insert,
            template_insert
        };
    public:
        BlackBoard(ard::selector_board* b);

        void                    rebuildBoard()override;
        void                    reloadContent()override;
        void                    setFocusOnContent()override;

        //void					selectCurrent(board_g_topic<ard::selector_board>*);
        /// we look at mselect first if none take current
  		ard::board_item*	firstSelectedBItem();
      //  void                selectNext(Next2Item)override;
        void                resetBand(const std::unordered_set<int>& bands2reset)override;

        void                setCurrentControlLink(ard::board_link*);
		void                removeSelected(bool silently)override;
        void                removeCurrentControlLink();

        void                createBoardTopic(const QPointF& )override;
        void                insertTopic(topic_ptr f, int band = -1, int ypos = -1);

        ard::black_board_g_topic*  findByBItem(ard::board_item*);
        void                processMoved(board_g_topic<selector_board>* gi)override;
        void                onModifiedOutsideTopic(topic_ptr f);


        ///show arrows going out of item
        void                showLinkProperties()override;
 
        /// custom edit mode - insert, remove, link - action on item select
        bool                isInCustomEditMode()const override;
        void                exitCustomEditMode()override;

        /// insert mode
        bool                isInInsertMode()const override;
        void                enterInsertMode()override;
        void                exitInsertMode()override;
        void                completeAddInInsertMode();

        /// locate mode
        bool                isInLocateMode()const override;
        void                enterLocateMode()override;
        void                exitLocateMode()override;
        //void                completeFindInLocateMode();

        /// template mode
        bool                isInCreateFromTemplateMode()const override;
        void                startCreateFromTemplateMode(BoardSample tmpl);      
        void                suggestCreateFromTemplateAtPos(const QPointF&)override;
        void                completeCreateFromTemplate(const QPointF&);
		void				insertFromTemplate(const QPoint& pt)override;

        /// link mode
        bool                isInLinkMode()const override;
        void                enterLinkMode()override;
        void                exitLinkMode()override;
        void                addLink();
        void                editLink(ard::GBLink*);
        ard::board_link*    currrentControlLink() { return m_currrent_control_link; }

		void				selectShape(ard::BoardItemShape sh, const QPoint& pt)override;
		void				pasteFromClipboard()override;
		std::pair<bool, QString>renameCurrent()override;

        void                        editComment();
        void                        renameBand(board_band<ard::selector_board>* g_band);
        void                        onBandControl(board_band_header<ard::selector_board>* g_band)override;
		void						applyNewShape(ard::BoardItemShape sh);

        void                setupAnnotationCard(bool)override {};
        void                zoomView(bool)override {};

		void				debugFunction();

		void				onBandDragEnter(board_band<ard::selector_board>* b, QGraphicsSceneDragDropEvent *e)override;
		void				onBandDragDrop(board_band<ard::selector_board>* b, QGraphicsSceneDragDropEvent *e)override;

    protected:
        /// action button ///
        void                repositionActionButtons(board_g_topic<ard::selector_board>* gi)override;
        void                hideActionButtons();
        void                updateActionButton();
        std::vector<GBActionButton::EType> custom_mode_button_types4mode(CustomMode m);

        void                initCustomEditMode(CustomMode m);
        void                clearCustomEditModeGui();
        void                setupActionButtonInCaseCustomEditMode()override;

        void                clearBoard()override;
        void                makeGrid();
        int                 layoutBand(board_band_info* b, const ard::BITEMS& bitems, int xstart, bool lookup4registered);
        void                removeBItems(GB_LIST& lst);
        void                removeLinksFromOriginToAllTargets(ard::board_item* origin);
        void                removeLinksFromAllOriginsToTarget(ard::board_item* target);

        ard::GBLink*        current_link_g();
        /// (above, below) - function will return pair, can be null-values
        std::pair<gb_ptr, gb_ptr>   findDropSpot(const ard::BITEMS& band_items, const QPointF& ptInScene, ard::black_board_g_topic* gi);

        BoardDropPos        calcTopicDropDestination(const QPointF& ptInScene, const ard::BITEMS& items2move, ard::black_board_g_topic* gi);
        BoardDropPos        calcPointDropDestination(const QPointF& ptInScene);

        black_board_g_topic*        register_bitem(ard::board_item*, board_band_info* band);
        ard::GBLink*        register_blink(ard::board_link_list* link_list, ard::board_link* lnk, black_board_g_topic* origin_g, black_board_g_topic* target_g);
        void                register_blink_list(ard::board_link_list* lnk_list, black_board_g_topic* origin_g, black_board_g_topic* target_g);
        void                updateCurrentRelated(board_g_topic<ard::selector_board>*)override;
		void				clearCurrentRelated()override;
        void                releaseLinkOriginList();        
        void                updateLinkOriginMarks();
        bool                isInLinkOrigins(const ard::board_item*)const;
        void                buildLinks();

        /// link list of preselected origins with a target
        void                doAddLinkFromSelectedOrigins(ard::board_item* target);
        void                doAddLinkFromOrigin(ard::board_item* origin, ard::board_item* target, QString LinkLabel = "");


        /// we need function after moving from one band to another
        void                resetLinksInBands(const std::unordered_set<int>& bands);

        void                applyTextFilter()override;

		void				dropTopics(board_band<ard::selector_board>* b, QGraphicsSceneDragDropEvent *e, const ard::TopicsListMime* m);
		void				dropTextEx(board_band<ard::selector_board>* b, QGraphicsSceneDragDropEvent *e, const QMimeData* m);

    protected:
        B2G                         m_b2g;
        B2LG                        m_target_b2link_list;
        L2G                         m_l2g;
        BITEMS                      m_link_origin_list;
        std::map<ard::board_item*, GCurrLinkOriginMark*> m_link_origin_marks;
        ard::board_link*            m_currrent_control_link{ nullptr };
        std::map<ard::board_item*, int> m_add_progress_in_insert_mode;//we count added topics per base topic to name new items
        LNK_LIST                    m_marked_links;         /// marked up arrows, when current is changed
        ACT_TYPE2BUTTON             m_act2b;
        BoardSample                 m_createFromTemplate{ BoardSample::None };
        CustomMode                  m_custom_mode{ CustomMode::none};
		QTimer						m_board_watcher;
    };

};

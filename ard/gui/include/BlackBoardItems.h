#pragma once
#include <QGraphicsRectItem>
#include "custom-widgets.h"
#include "board.h"
#include "ardmodel.h"
#include "utils.h"
#include "board_page.h"


#define ASSERT_VALID_RECT(R)    if(R.width() > 10000 || R.height() > 10000){ASSERT(0, "possible invalid rect") << R;}

#ifdef _DEBUG
void printBItems(ard::BITEMS& bitems, QString prefix);
#endif

namespace ard {
    class BlackBoard;
    class black_board_g_topic;
	class picture;

    using gb_ptr = black_board_g_topic*;

    ///
    ///bitem with topic attached
    ///
    class black_board_g_topic : public board_g_topic<ard::selector_board>
    {
    public:
        black_board_g_topic(BlackBoard* bb, ard::board_item* b);
        virtual ~black_board_g_topic();

		ard::board_item*		bitem() { return    m_bitem; }
		ard::topic*				refTopic()override;
		const ard::topic*		refTopic()const override;
		ard::BoardItemShape		bshape()const override;
		int						ypos()const override;

		BlackBoard*				selector_board();

 //       void					keyPressEvent(QKeyEvent * e)override;
    protected:
		void					onContextMenu(const QPoint& pt)override;
        ard::board_item*		m_bitem{ nullptr };
    };

    ///
    ///bitem board
    ///
    class GBLinkArrow;
    class GBLinkCurvePoint;
    class GBLinkControl;
    class GBLinkLabel;
    class GBLink : public QGraphicsPathItem
    {
    public:
        enum class ESelectionMode 
        {
            normal,             ///default arrow
            selected,           ///link in selected origin - all links to targets will be selected
            control_selected    ///on link to a target can be control selected
        };


        GBLink(BlackBoard* bb, ard::board_link_list* link_list, ard::board_link* blnk, black_board_g_topic* origin, black_board_g_topic* target);
        ~GBLink();

        black_board_g_topic*        g_origin() { return m_origin; }
        black_board_g_topic*        g_target() { return m_target; }
        BlackBoard*         bb() { return m_bb; }
        ard::board_link*    blink() { return m_blnk; }
        bool                isIdentityLink()const;

        void                resetLink();
        std::vector<QGraphicsItem*> produceLinkItems();
        void                removeLinkFromScene(ArdGraphicsScene*);
        void                markAsNormal();
        void                markAsSelected();
        void                markAsControlSelected();
        ESelectionMode      selectionMode()const { return m_sel_mode; }
        void                onMovedGLinkCurveControl();
        void                processKey(int key);
        void                processDoubleClick();
        void                updateLink();

        void                paint(QPainter * p, const QStyleOptionGraphicsItem * o, QWidget * widget = 0)override;
        void                mousePressEvent(QGraphicsSceneMouseEvent* e)override;
        void                mouseDoubleClickEvent(QGraphicsSceneMouseEvent * e)override;
        void                keyPressEvent(QKeyEvent * e)override;
    protected:
        void                        recalcStartEndPoints();
        std::pair<QPointF, QPointF> recalcControlPoints();
        void                        rebuildPathWithNewCurveControlPoints(const std::pair<QPointF, QPointF>&);
        void                        rebuildLabel();

        BlackBoard*         m_bb{ nullptr };
        ard::board_link_list* m_link_list{ nullptr };
        ard::board_link*    m_blnk{ nullptr };
        black_board_g_topic*        m_origin{ nullptr };
        black_board_g_topic*        m_target{ nullptr };
        GBLinkArrow*        m_arrow{ nullptr };
        GBLinkCurvePoint   *m_cp1{ nullptr }, *m_cp2{ nullptr };
        GBLinkControl*      m_ctrl{nullptr};
        GBLinkLabel*        m_label{ nullptr };
        ESelectionMode      m_sel_mode{ ESelectionMode::normal };
        QPointF             m_originPt, m_targetPt;
        friend class BlackBoard;
    };

    ///
    ///GBLinkArrow
    ///
    class GBLinkArrow : public QGraphicsPathItem
    {
    public:
        GBLinkArrow(GBLink* bl);
        void    paint(QPainter * p, const QStyleOptionGraphicsItem * o, QWidget * widget = 0)override;
        void    recalcArrow(const QPointF& ptBegin, const QPointF& ptEnd);
    protected:
        GBLink*     m_blnk{ nullptr };
        friend class GBLink;
    };

    ///
    ///Bezier control point 
    ///
    class GBLinkCurvePoint : public QGraphicsRectItem
    {
    public:
        GBLinkCurvePoint(GBLink* bl);

        void    paint(QPainter * p, const QStyleOptionGraphicsItem * o, QWidget * widget = 0)override;
        void    mouseReleaseEvent(QGraphicsSceneMouseEvent * e) override;
    protected:
        GBLink*     m_blnk{ nullptr };
    };

    ///
    ///link selection control point 
    ///
    class GBLinkControl : public QGraphicsRectItem
    {
    public:
        GBLinkControl(GBLink* bl);
        void    paint(QPainter * p, const QStyleOptionGraphicsItem * o, QWidget * widget = 0)override;
        void    mousePressEvent(QGraphicsSceneMouseEvent* e)override;
        void    mouseDoubleClickEvent(QGraphicsSceneMouseEvent * e)override;
        void    keyPressEvent(QKeyEvent * e)override;
    protected:
        GBLink*     m_blnk{ nullptr };
    };

    ///
    ///link label
    ///
    class GBLinkLabel : public QGraphicsPathItem
    {
    public:
        GBLinkLabel(GBLink* bl);
        void    paint(QPainter * p, const QStyleOptionGraphicsItem * o, QWidget * widget = 0)override;
        void    mousePressEvent(QGraphicsSceneMouseEvent* e)override;
        void    mouseDoubleClickEvent(QGraphicsSceneMouseEvent * e)override;
        void    keyPressEvent(QKeyEvent * e)override;
        void    resetLabel(const QPointF& pt, qreal angle);
        GBLink* blink() { return m_blnk; };
    protected:
        GBLink*     m_blnk{ nullptr };
        qreal       m_angle{ 0 };
        QPolygon    m_rotatedRect;
    };




    ///
    /// current link origin marker
    ///
    class GCurrLinkOriginMark : public board_g_mark<ard::selector_board>
    {
    public:
        GCurrLinkOriginMark(BlackBoard* bb);
        void  paint(QPainter * p, const QStyleOptionGraphicsItem *, QWidget *)override;
    };


    ///
    /// we can complete link
    ///
    class GBActionButton : public board_g_mark<ard::selector_board>
    {
    public:
        enum EType 
        {
            none,
            add_link,
            insert_template,
            insert_item,
            //remove_item,
            find_item,
            show_item_links,
            properties
        };

        GBActionButton(BlackBoard* bb);
        void    paint(QPainter * p, const QStyleOptionGraphicsItem *, QWidget *)override;
        void    updateMarkPos(board_g_topic<ard::selector_board>* gi)override;
        void    mousePressEvent(QGraphicsSceneMouseEvent* e)override;
        void    mouseReleaseEvent(QGraphicsSceneMouseEvent * e) override;

		BlackBoard*			selector_board();

        void				setType(EType t);

    protected:
        EType m_type;
    };

    using ACT_TYPE2BUTTON = std::map<GBActionButton::EType, GBActionButton*>;



    ///
    /// view for bboard
    ///
	class BoardView : public board_page_view<ard::selector_board> 
	{
	public:
		BoardView(BlackBoard* bb);
		bool        process_keyPressEvent(QKeyEvent *e)override;
	};
 
}

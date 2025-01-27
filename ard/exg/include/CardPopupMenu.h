#pragma once

#include "a-db-utils.h"
#include "custom-widgets.h"

#ifdef ARD_BIG

namespace ard
{
    /**
    * popup card context menu, with some options to change colors, todos etc.
    */
    class CardPopupMenuBase : public QWidget
    {
    public:
        CardPopupMenuBase(topic_ptr f);
        CardPopupMenuBase(TOPICS_LIST lst);
        void showCardMenu(const QPoint& pt);
    protected:
        void changeEvent(QEvent *event)override;
        void paintEvent(QPaintEvent *e)override;
        void closeEvent(QCloseEvent *e)override;
        void mousePressEvent(QMouseEvent *e)override;
		void focusOutEvent(QFocusEvent * e)override;

        virtual void drawMenu(QPainter& ) = 0;
        virtual void processMouseClick(const QPoint& pt) = 0;
        void initCard();
    protected:
        TOPICS_LIST         m_topics;
        int                 bmargin;
        QFont               m_working_font;
        QRect               m_rcPinButton;
        QRect               m_rcLayoutHeader, m_rcLayoutLockSelector, m_rcLayoutCloseAll, m_rcLayoutPinTab, m_rcLayoutArrangeTabs,
                            m_rcComment, m_rcColorButtons, m_rcColorButtonsDef, m_rcSlider, m_rcSliderDef, m_rcLocate, m_rcFind;
        std::vector<QRect>  m_layout_rects;
		bool                m_hasProgressSlider{ false }, m_hasLocate{false};

        const ard::EColor	m_buttons[4]{ ard::EColor::purple, ard::EColor::red, ard::EColor::blue, ard::EColor::green };
        int                 m_progress_width, m_progress_height, m_button_width_height,
                            m_default_btn_width, m_buttons_area_offset;
        ard::EColor         m_selected_color_idx{ ard::EColor::none };
        bool                m_isToDo{ false };
        int                 m_CompletedPercent{ 0 };

        enum EHit
        {
            hitNone,
            hitPinBtn,
            hitNewTabBtn,
            hitColorBtn,
            hitColorDefault,
            hitSliderDefault,
            hitSlider,
            hitLocate,
			hitFindText,
            hitProperties,
            hitMove,
            hitMoveFX,
            hitZoomOut,
            hitZoomIn,
            hitComment,
            hitStar,
            hitImportant,

            hitLayoutBtn,
            hitLayoutCloseAllBtn,
            hitLayoutArrangeTabsBtn         
        };

        struct HitInfo
        {
            EHit		hit;
            int			val;
			ard::EColor color;
        };

        topic_ptr   topic();
        QSize       recalcColorButtonsControls(int startY);
        void        drawColorButtons(QPainter& p);
        void        drawSlider(QPainter& p);
        void        drawPixMap(QPainter& p, const QPixmap& pm, const QRect& rc);
        bool        processColorButtonMenuCommand(const ard::CardPopupMenuBase::HitInfo& h);
        HitInfo     hitTestLayoutControls(const QPoint& pt);
        HitInfo     hitTestColorButtonsControls(const QPoint& pt);
#ifdef _DEBUG
        QString hit2str(EHit);
#endif

    };

    class SelectorTopicPopupMenu : public CardPopupMenuBase
    {
    public:
        static void showSelectorTopicPopupMenu(QPoint pt, TOPICS_LIST lst, bool withLocateOption);
    protected:
        SelectorTopicPopupMenu(TOPICS_LIST lst, bool withLocateOption);
        void drawMenu(QPainter&)override;
        void processMouseClick(const QPoint& pt)override;
	protected:
		bool	m_popup_annotation{true};
    };
}
#endif //ARD_BIG
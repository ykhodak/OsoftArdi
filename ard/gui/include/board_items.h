#pragma once

#include <QGraphicsRectItem>
#include <QToolTip>
#include <QApplication>
#include "board.h"
#include "picture.h"
#include "contact.h"
#include "workspace.h"
#include "custom-widgets.h"
#include "search_edit.h"
#include "utils.h"

#define BOXING_MARGIN 5
#define BBOARD_ZVAL_BITEM           0.0
#define BBOARD_ZVAL_RAISED          0.1
#define BBOARD_ZVAL_ACTIVE_BTN      0.2         
#define BBOARD_ZVAL_EDGE            -0.1
#define BBOARD_ZVAL_BACK_WALL       -1.0
#define BBOARD_FAT_FINGER_WIDTH 3 * gui::lineHeight()
//#define BBOARD_CURR_MARK_COLOR QColor(153, 217, 234)
//#define BBOARD_CURR_MARK_COLOR QColor(157,189,255)
//#define BBOARD_CURR_MARK_COLOR QColor(53,119,255)
#define BBOARD_CURR_MARK_COLOR QColor(86,156,214)
#define BBOARD_CURR_LINK_CTRL_COLOR QColor(255, 127, 39)
#define BBOARD_LINK_CONTROL_WIDTH 10

//#define SHOW_BOARD_DBG_INFO

class TextFilter;

extern QSize calcAnnotationSize(int band_width, QString annotation);
extern void register_no_popup_slide(ard::topic* f);

namespace ard 
{
	enum class BandHit
	{
		none,
		header,
		band_label,
		resize_header,
		control_button
	};

	class selector_board;
	template<class> class board_band;
	template<class> class board_band_header;
	template<class> class board_page;
	template<class> class board_g_topic;
	template<class> class board_g_mark;
	template<class> class selection_g_mark;
	class board_band_splitter;

	using B2TLIST					= std::vector<TOPICS_LIST>;
	template<class B> using BAND2G	= std::unordered_map<ard::board_band_info*, ard::board_band<B>*>;
	template<class B> using BH2G	= std::unordered_map<ard::board_band_info*, ard::board_band_header<B>*>;
	template<class B> using GBLIST	= std::vector<board_g_topic<B>*>;
	template<class B> using T2L		= std::unordered_map<ard::topic*, GBLIST<B>>;
	template<class B> using B2L		= std::vector<GBLIST<B>>;
	template<class B> using MS_LIST	= std::vector<selection_g_mark<B>*>;
	using LINES = std::vector<QGraphicsLineItem*>;
	using DEPTH2LINES = std::unordered_map<int, LINES>;

	template<class BB>
	class board_g_topic : public QGraphicsRectItem
	{
	public:
		board_g_topic(board_page<BB>* b);

		virtual ard::topic*				refTopic() = 0;
		virtual const ard::topic*		refTopic()const = 0;
		int								bandIndex()const { return m_attr.BandIndex; };
		virtual ard::BoardItemShape		bshape()const = 0;
		virtual int						ypos()const { return 0; }

		bool							isMSelected()const { return m_attr.MSelected; }
		void							setMSelected(bool bval) { m_attr.MSelected = bval ? 1 : 0; }

		void							resize2Content(board_band_info* band);
		void							repositionInBand(board_band_info* band, const QPointF& pt);

		const QRectF&					title_rect()const { return m_title_rect; }
		QRectF							box_rect()const { return m_box_rect; };
		const QPolygon&					shape_polygon()const { return   m_shape_polygon; }

		void							setBandIndex(int v) { m_attr.BandIndex = v; }
		void							setOutlineIndex(int v) { m_attr.OutlineIndex = v; }

		void							setDepth(int depth) { m_attr.Depth = depth; }
	protected:
		virtual void					onContextMenu(const QPoint& ) {};
		void							draw_box(QPainter* p, topic_ptr, ard::BoardItemShape);
		void							draw_topic_box(QPainter* p, topic_ptr, ard::BoardItemShape);
		void							draw_contact_box(QPainter* p, contact_ptr, ard::BoardItemShape);
		void							draw_picture_box(QPainter* p, ard::picture*);
		void							draw_annotation(QPainter* p, topic_ptr);
		void							draw_todo(QPainter*, topic_ptr);
		void							draw_color_mark(QPainter*, topic_ptr);
		void							draw_text_line(QPainter*, QString s, QRectF& rc, int lineH);

		void							paint(QPainter * p, const QStyleOptionGraphicsItem *, QWidget *)override;
		void							mousePressEvent(QGraphicsSceneMouseEvent* e)override;
		void							mouseReleaseEvent(QGraphicsSceneMouseEvent * e) override;
		void							mouseDoubleClickEvent(QGraphicsSceneMouseEvent * event)override;
		void							hoverEnterEvent(QGraphicsSceneHoverEvent*)override;
		void							hoverLeaveEvent(QGraphicsSceneHoverEvent*)override;
		void							keyPressEvent(QKeyEvent * e)override;
#ifdef _DEBUG
		void					drawDebugInfo(QPainter* p, topic_ptr);
#endif //_DEBUG

		board_page<BB>*			m_bb{ nullptr };
		QRectF					m_title_rect, m_box_rect, m_annotation_rect;
		QPolygon				m_shape_polygon;
		QPointF					m_ptPress;
		union UATTR
		{
			uint64_t flag;
			struct
			{
				unsigned Hover			: 1;
				unsigned MSelected		: 1;
				unsigned BandIndex		: 6;
				unsigned OutlineIndex	: 20;
				unsigned MaximizeWidth  : 1;
				unsigned Depth			: 5;
			};
		} m_attr;
	};

	/// bord g-topic for particular topic type with box-frame
	template<class BB, class T>
	class board_box_g_topic : public board_g_topic<BB>
	{
	public:
		board_box_g_topic(board_page<BB>* bb, T* f);
		virtual ~board_box_g_topic();

		ard::topic*				refTopic()override			{ return m_topic; }
		const ard::topic*		refTopic()const override	{ return m_topic; }
		ard::BoardItemShape		bshape()const override		{ return ard::BoardItemShape::box; };
		//void					keyPressEvent(QKeyEvent * e)override;
		void					mouseMoveEvent(QGraphicsSceneMouseEvent* e)override;
	protected:
		T*						m_topic{ nullptr };
	};



	///
	/// vertical band in blackboard
	///
	template<class BB>
	class board_band : public QGraphicsRectItem
	{
	public:
		board_band(board_page<BB>* bb, ard::board_band_info* bi);
		~board_band();

		board_band_info*    band() { return m_band; }

		void    updateSize(int band_height);
		void    paint(QPainter * p, const QStyleOptionGraphicsItem *, QWidget *)override;
		
		void    mousePressEvent(QGraphicsSceneMouseEvent* e)override;
		void    mouseReleaseEvent(QGraphicsSceneMouseEvent * e) override;
		void    mouseMoveEvent(QGraphicsSceneMouseEvent *e)override;
		
		void    dragEnterEvent(QGraphicsSceneDragDropEvent *e)override;
		void    dropEvent(QGraphicsSceneDragDropEvent *e)override;
		void	dragMoveEvent(QGraphicsSceneDragDropEvent *e)override;
	protected:
		board_page<BB>*			m_bb{nullptr};
		board_band_info*		m_band{nullptr};
		bool					m_createTopicInit{ false };
		QDateTime				m_mouse_press_time;
	};

	template<class BB>
	class board_band_header : public QGraphicsRectItem
	{
	public:
		board_band_header(board_page<BB>* bb, ard::board_band_info* bi);
		~board_band_header();

		board_band_info*    band() { return m_band; }
		void	setCurrent(bool val) { m_is_current = val; update(); }
		bool	isCurrent()const { return m_is_current; }

		void    paint(QPainter * p, const QStyleOptionGraphicsItem *, QWidget *)override;
		void    mouseMoveEvent(QGraphicsSceneMouseEvent *e)override;
		void    mouseDoubleClickEvent(QGraphicsSceneMouseEvent *e)override;

		BandHit hitTest(QPointF pt);
	protected:
		void    mousePressEvent(QGraphicsSceneMouseEvent* e)override;
		void    mouseReleaseEvent(QGraphicsSceneMouseEvent * e) override;
		void    hoverMoveEvent(QGraphicsSceneHoverEvent * e)override;

		board_page<BB>*			m_bb{ nullptr };
		board_band_info*		m_band{ nullptr };
		bool					m_resize_in_progress{ false };
		QRectF					m_lbl_rect, m_ctrl_rect;
		bool					m_is_current{ false };
	};

	enum class EBoardCmd
	{
		none, colorBtn, colorDefault,
		sliderDefault, slider,
		comment, rename,
		locate, insert, remove, link,
		setShape, setTemplate, copy, paste,
		mark_as_read,
		filter_unread
	};

	struct ToolbarHitInfo
	{
		EBoardCmd	hit;
		int			val;
		ard::EColor	color;
	};

	struct board_page_toolbar_item 
	{
		board_page_toolbar_item() {};
		board_page_toolbar_item(EBoardCmd c, int w) :cmd(c), width(w){}
		EBoardCmd	cmd;
		int			width;
	};

	///
	/// toolbar for bboard
	///
	template<class BB>
	class board_page_toolbar : public QWidget
	{
	public:
		board_page_toolbar(ard::board_page<BB>*, std::vector<EBoardCmd> commands, std::vector<EBoardCmd> rcommands);

		void sync2Board();
	protected:
		void    paintEvent(QPaintEvent *e)override;
		void    mouseMoveEvent(QMouseEvent *e)override;
		void    mousePressEvent(QMouseEvent *e)override;

		void	drawSlider(QPainter& p, const QRect& rc);
	protected:
		board_page<BB>*			m_bb{ nullptr };
		const ard::EColor		m_buttons[4]{ ard::EColor::purple,
										ard::EColor::red,
										ard::EColor::blue,
										ard::EColor::green };

		int				bmargin, bspace;
		int				m_progress_width, m_button_width_height, m_clear_btn_width,
						m_unread_label_width;
		int				m_CompletedPercent{ 0 };
		ard::EColor     m_selected_color_idx{ ard::EColor::none };
		std::vector<board_page_toolbar_item> m_items, m_ritems;;


		ToolbarHitInfo		hitTest(const QPoint& pt);
		void				processTooltip(const QPoint& pt, const QPoint& pt_global);
		QString				hit2Str(const ToolbarHitInfo& hi);
	};


	///
	/// splitter between bands
	///
	class board_band_splitter : public QGraphicsLineItem
	{
	public:
		board_band_splitter();
		void    updateXPos(int);
		void    paint(QPainter * p, const QStyleOptionGraphicsItem *, QWidget *)override;
	};

	///
	/// edit comment, title, band label etc
	///
	enum class BEditorType
	{
		title,
		annotation,
		band_label
	};

	template<class BB>
	class board_editor : public QTextEdit,
		public LocationAnimator
	{

	public:
		static board_editor<BB>* editAnnotation(ard::board_page<BB> *bb, QRect rce);
		static board_editor<BB>* editTitle(ard::board_page<BB> *bb, QRect rce);
		static board_editor<BB>* editBandLabel(ard::board_page<BB> *bb, QRect rce, int band_index);
	protected:
		board_editor<BB>(ard::board_page<BB> *bb, BEditorType etype, QRect rce, int band_index = -1);
		~board_editor<BB>();
		
		void			storeText();
		void			focusOutEvent(QFocusEvent *e)override;
		void			paintEvent(QPaintEvent *e)override;
		QRect			locatorCursorRect()const override;
		void			locatorUpdateRect(const QRect& rc)override;
		void			keyPressEvent(QKeyEvent * e)override;

		board_page<BB>	*	m_bb{ nullptr };
		BEditorType			m_etype{ BEditorType::title };
		int					m_band_index{ -1 };
	};

	///
	/// rectangle to multi select items
	///
	class board_selector_rect : public QGraphicsRectItem
	{
	public:
		board_selector_rect();
		void    updateSelectRect(QRectF);
		void    paint(QPainter * p, const QStyleOptionGraphicsItem *, QWidget *)override;
	};
	///
	/// current selected marker
	///
	template <class BB>
	class board_g_mark : public QGraphicsRectItem
	{
	public:
		board_g_mark(board_page<BB>* bb);
		void  paint(QPainter * p, const QStyleOptionGraphicsItem *, QWidget *)override;

		virtual void updateMarkPos(board_g_topic<BB>* gi);
	protected:
		board_page<BB>* m_bb{ nullptr };
	};

	template <class BB>
	class selection_g_mark : public board_g_mark<BB>
	{
	public:
		selection_g_mark(board_page<BB>* bb, board_g_topic<BB>* gb);
		board_g_topic<BB>*		g() { return m_g; }
		void					updateSelectionMark();

		void  paint(QPainter * p, const QStyleOptionGraphicsItem *, QWidget *)override;
	protected:
		board_g_topic<BB>* m_g{ nullptr };
	};

	template <class BB>
	class board_page_view : public ArdGraphicsView
	{
	public:
		board_page_view(board_page<BB>* bb);
		virtual bool  process_keyPressEvent(QKeyEvent *e);
	protected:
		void keyPressEvent(QKeyEvent *e)override;
		void scrollContentsBy(int dx, int dy)override;
	protected:
		board_page<BB>* m_bpage{ nullptr };
	};

};


template<class BB>
ard::board_g_topic<BB>::board_g_topic(board_page<BB>* b) :m_bb(b)
{
	setRect(QRectF(0, 0, BBOARD_MAX_BOX_WIDTH, gui::lineHeight()));
	setOpacity(DEFAULT_OPACITY);
	setAcceptHoverEvents(true);
	m_attr.flag = 0;
};

template<class BB>
void ard::board_g_topic<BB>::paint(QPainter * p, const QStyleOptionGraphicsItem *, QWidget *)
{
	auto f = refTopic();
	if (f) 
	{
		PGUARD(p);
		p->setRenderHint(QPainter::Antialiasing, true);
		p->setFont(*ard::defaultSmallFont());
		QColor bk;
		bool as_current = isMSelected();
		if (as_current) {
			bk = color::MSELECTED_ITEM_BK;
		}
		else {
			bk = gui::colorTheme_CardBkColor();
		}
		p->setBrush(bk);

		draw_box(p, f, bshape()/*ard::BoardItemShape::box*/);
		draw_annotation(p, f);
		draw_todo(p, f);
		auto clidx = f->colorIndex();
		if (clidx != ard::EColor::none)
		{
			draw_color_mark(p, f);
		}

#ifdef _DEBUG
#ifdef SHOW_BOARD_DBG_INFO
		drawDebugInfo(p, f);
#endif
#endif
	}
};


template<class BB>
void ard::board_g_topic<BB>::draw_topic_box(QPainter* p, topic_ptr f, ard::BoardItemShape shp)
{
	QString stitle = f->impliedTitle();
	if (!stitle.isEmpty())
	{
		auto clidx = f->colorIndex();
		if (ard::isTextlikeShape(shp)) {
			if (clidx != ard::EColor::none) {
				auto clr = ard::cidx2color(clidx);
				p->setPen(QPen(clr));
			}
			else {
				p->setPen(QPen(Qt::gray));
			}

			auto fnt = ard::getTextlikeFont(shp);
			p->setFont(*fnt);
		}


		if (f->hasCustomTitleFormatting())
		{
			QList<QTextLayout::FormatRange> fr;
			f->prepareCustomTitleFormatting(p->font(), fr);
			QString date_label = f->dateColumnLabel();
			m_bb->text_filter()->drawTextMLine(p,
				p->font(),
				m_title_rect,
				stitle,
				false,
				nullptr,
				&fr);
		}
		else
		{
			auto rc_txt = m_title_rect;

			auto px = f->getSecondaryIcon(OutlineContext::normal);
			if (!px.isNull())
			{
				QRect rcpm;// = m_title_rect;
				rectf2rect(m_title_rect, rcpm);
				rcpm.setWidth(gui::lineHeight());
				rcpm.setHeight(gui::lineHeight());
				p->drawPixmap(rcpm, px);

				rc_txt = m_title_rect;
				rc_txt.setLeft(rc_txt.left() + rcpm.width());
			}

			if (ard::isTextlikeShape(shp)) {
				m_bb->text_filter()->drawTextMLine(p, p->font(), rc_txt, stitle, true);
			}
			else {
				m_bb->text_filter()->drawTextMLine(p, p->font(), rc_txt, stitle, true);
			}
		}
	}
};

template<class BB>
void ard::board_g_topic<BB>::draw_annotation(QPainter* p, topic_ptr f)
{
	if (!m_annotation_rect.isEmpty()) {
		PGUARD(p);
		p->setOpacity(0.9);
		QColor cl_btn = color::Yellow;
		QBrush br(cl_btn);
		p->setBrush(br);

		cl_btn = cl_btn.darker(200);
		QPen pn(cl_btn);
		p->setPen(pn);

		int radius = 5;
		p->drawRoundedRect(m_annotation_rect, radius, radius);


		auto annotation = f->annotation4outline();
		if (!annotation.isEmpty()) {
			m_bb->text_filter()->drawTextMLine(p, *ard::defaultSmallFont(), m_annotation_rect, annotation, true);
		}
	}
};

template<class BB>
void ard::board_g_topic<BB>::draw_todo(QPainter* p, topic_ptr f)
{
	QRectF rc = m_title_rect;
	int perc = f->getToDoDonePercent4GUI();
	if (perc > 0 && perc < 100)
	{
		PGUARD(p);
		QRectF rc1(rc.topRight().x() - gui::lineHeight() * 2,
			rc.topRight().y(),
			gui::lineHeight() * 2,
			6);

		QBrush b1(color::Navy);
		p->setBrush(b1);
		p->drawRect(rc1);
		int w = (int)(rc1.width()*((double)perc / 100));
		if (w < 2)w = 2;
		rc1.setWidth(w);
		QBrush b2(color::Green);
		p->setBrush(b2);
		p->drawRect(rc1);
	}
};

template<class BB>
void ard::board_g_topic<BB>::draw_color_mark(QPainter* p, topic_ptr f)
{
	auto clidx = f->colorIndex();
	if (clidx != ard::EColor::none)
	{
		auto pm = utils::colorMarkSelectorPixmap(clidx);
		if (pm)
		{
			QRectF rc = boundingRect();
			QRect rc1;
			rectf2rect(rc, rc1);
			if (rc1.height() > 48) {
				rc1.setBottom(rc1.top() + 48);
			}
			if (rc1.width() > 200) {
				rc1.setRight(rc1.left() + 200);
			}
			p->drawPixmap(rc1, *pm);

			//..
			//qDebug() << "band-color" << rc1 << it->title();
			//p->setBrush(Qt::red);
			//p->drawRect(rc1);

			//..
		}
	}
};

template<class BB>
void ard::board_g_topic<BB>::draw_text_line(QPainter* p, QString s, QRectF& rc, int lineH)
{
	if (!s.isEmpty()) { 
		m_bb->text_filter()->drawTextMLine(p, p->font(), rc, s, false); 
		rc.translate(0, lineH); 
	}
};


template<class BB>
void ard::board_g_topic<BB>::draw_contact_box(QPainter* p, contact_ptr c, ard::BoardItemShape shp)
{
	auto name = c->contactFullName();
	auto email = c->contactEmail();
	auto phone = c->contactPhone();

	auto clidx = c->colorIndex();
	if (ard::isTextlikeShape(shp)) {
		if (clidx != ard::EColor::none) {
			auto clr = ard::cidx2color(clidx);
			p->setPen(QPen(clr));
		}
		else {
			p->setPen(QPen(Qt::gray));
		}

		auto fnt = ard::getTextlikeFont(shp);
		p->setFont(*fnt);
	}

	auto lineH = gui::shortLineHeight();
	QRectF rc = m_title_rect;
	rc.setBottom(rc.top() + lineH);

	draw_text_line(p, name, rc, lineH);
	rc.setLeft(rc.left() + ARD_MARGIN);
	draw_text_line(p, email, rc, lineH);
	draw_text_line(p, phone, rc, lineH);
};

template<class BB>
void ard::board_g_topic<BB>::draw_picture_box(QPainter* p, ard::picture* pic)
{
	auto lineH = gui::shortLineHeight();
	QRectF rc = m_title_rect;
	rc.setBottom(rc.top() + lineH);
	m_bb->text_filter()->drawTextMLine(p, p->font(), rc, pic->title(), false);
	rc = m_title_rect;
	rc.setTop(rc.top() + lineH);
	auto pm = pic->thumbnail();
	if (pm && !pm->isNull())
	{
		QRect rc1;
		rectf2rect(rc, rc1);
		utils::drawImagePixmap(*pm, p, rc1, nullptr);
	}
};

template<class BB>
void ard::board_g_topic<BB>::draw_box(QPainter* p, topic_ptr f, ard::BoardItemShape shp)
{
	QRectF rc = boundingRect();
	PGUARD(p);
	static int radius = 10;
	auto it = f->shortcutUnderlying();
	if (m_attr.Hover)
	{
		QBrush brush(color::HOVER_ITEM_BK);
		p->setBrush(brush);
	}

	switch (shp)
	{
	case ard::BoardItemShape::box:
	{
		QRectF rc2 = box_rect();
		p->drawRoundedRect(rc2, radius, radius);
	}break;
	case ard::BoardItemShape::circle:
	{
		p->drawEllipse(rc);
	}break;

	case ard::BoardItemShape::text_normal:
	case ard::BoardItemShape::text_italic:
	case ard::BoardItemShape::text_bold:
		break;

	default:
	{
		p->drawPolygon(m_shape_polygon);
	}break;
	}
	//...
	auto c = dynamic_cast<contact_ptr>(it);
	if (c) {
		draw_contact_box(p, c, shp);
	}
	else {
		auto pic = dynamic_cast<ard::picture*>(it);
		if (pic)
		{
			draw_picture_box(p, pic);
		}
		else
		{
			draw_topic_box(p, it, shp);
		}
	}
	//..
	//.. draw color-hash bubble
	if (f->hasColorByHashIndex()) {
		auto ch = f->getColorHashIndex();
		if (ch.second != 0) {
			QRect rcpm2clr_hash = QRect((int)((rc.left() + rc.right()) / 2 - ICON_WIDTH / 2),
				(int)rc.top(),
				(int)ICON_WIDTH, (int)ICON_WIDTH);

			switch (shp)
			{
			case ard::BoardItemShape::box:
			case ard::BoardItemShape::text_normal:
			case ard::BoardItemShape::text_italic:
			case ard::BoardItemShape::text_bold:
			{
				rcpm2clr_hash = QRect(m_title_rect.right() - ICON_WIDTH,
					(int)m_title_rect.top(),
					(int)ICON_WIDTH, (int)ICON_WIDTH);
			}break;
			case ard::BoardItemShape::triangle:
			case ard::BoardItemShape::rombus:
			case ard::BoardItemShape::hexagon:
			case ard::BoardItemShape::pentagon:
				rcpm2clr_hash = rcpm2clr_hash.translated(0, ICON_WIDTH); break;
			default:break;
			}

			QColor rgb = color::getColorByHashIndex(ch.first);
			rgb = rgb.darker(150);
			PGUARD(p);
			p->setBrush(QBrush(rgb));
			p->setPen(Qt::NoPen);
			p->setFont(*ard::defaultOutlineLabelFont());
			p->drawEllipse(rcpm2clr_hash);
			p->setPen(Qt::white);
			p->drawText(rcpm2clr_hash, Qt::AlignCenter | Qt::AlignHCenter, QString(ch.second));
		}
	}

	/*
	auto clidx = it->colorIndex();
	if (clidx != ard::EColor::none)
	{
		auto pm = utils::colorMarkSelectorPixmap(clidx);
		if (pm)
		{
			QRect rc1;
			rectf2rect(rc, rc1);
			if (rc1.height() > 48) {
				rc1.setBottom(rc1.top() + 48);
			}
			if (rc1.width() > 200) {
				rc1.setRight(rc1.left() + 200);
			}
			p->drawPixmap(rc1, *pm);

			//..
			qDebug() << "band-color" << rc1 << it->title();
			p->setBrush(Qt::red);
			p->drawRect(rc1);

			//..
		}		
	}*/
};

template<class BB>
void ard::board_g_topic<BB>::repositionInBand(board_band_info* band, const QPointF& pt)
{
	m_attr.BandIndex = band->bandIndex();
	setPos(pt);
};

template<class BB>
void ard::board_g_topic<BB>::resize2Content(ard::board_band_info* band)
{
	auto f = refTopic();
	if (f) 	
	{
		auto shp = bshape();
		//m_attr.BandIndex = band->bandIndex();
		prepareGeometryChange();
		int band_width = band->bandWidth() - m_attr.Depth * gui::lineHeight();
		if (!ard::isBoxlikeShape(shp))
		{
			constexpr int def_non_box_width = BBOARD_BAND_DEFAULT_WIDTH;
			if (band_width > def_non_box_width) {
				band_width = def_non_box_width;
			}
		}

		m_annotation_rect = QRectF();
		auto annotation = f->annotation4outline();
		if (!annotation.isEmpty()) {
			QSize szAnnotation = calcAnnotationSize(band_width, annotation);
			int w = szAnnotation.width();
			int x_offset = 0;
			if (w < band_width) {
				x_offset = (band_width - w) / 4;
			}
			else if (w > band_width) {
				w = band_width;
			}
			m_annotation_rect = QRectF(x_offset, 0, w, szAnnotation.height());
		}

		if (ard::isBoxlikeShape(shp)) {
			int max_box_w = band_width - 2 * BOXING_MARGIN;

			int w = 0;
			if (m_attr.MaximizeWidth) {
				w = band_width - 6 * BOXING_MARGIN;
			}
			int title_height = 0;

			if (true)
			{
				auto szTitle = f->calcBlackboardTextBoxSize();
				auto w2 = szTitle.width();
				title_height = szTitle.height();
				if (w2 > w) {
					w = w2;
				}
			}
			if (w == 0 || w > BBOARD_BAND_MAX_WIDTH) {
				w = BBOARD_BAND_DEFAULT_WIDTH;
			}

			if (title_height == 0) {
				title_height = gui::lineHeight();
			}

			if (w > max_box_w) {
				w = max_box_w;
			}

			int x_text_offset = BOXING_MARGIN;

			m_title_rect = QRectF(x_text_offset, m_annotation_rect.height() + BOXING_MARGIN, w, title_height);
			m_box_rect = m_title_rect.translated(-BOXING_MARGIN, -BOXING_MARGIN);
			m_box_rect.setWidth(m_box_rect.width() + 2 * BOXING_MARGIN);
			m_box_rect.setHeight(m_box_rect.height() + 2 * BOXING_MARGIN);

			//+ m_annotation_rect.width()

			int max_x = std::max(m_annotation_rect.right(), m_box_rect.right());

			QRectF rc = QRectF(0, 0, max_x, title_height + m_annotation_rect.height() + 2 * BOXING_MARGIN);
			setRect(rc);
		}
		else {
			QRectF rcb = QRectF(0, 0, band_width - gui::lineHeight(), band_width - gui::lineHeight());
			setRect(rcb);
			QRect rc1;
			rectf2rect(rcb, rc1);
			m_title_rect = ard::boards_model::calc_edit_rect(shp, rc1);

			if (shp != ard::BoardItemShape::circle) {
				m_shape_polygon = ard::boards_model::build_shape(shp, rc1);
			}
		}
	}
};

template<class BB>
void ard::board_g_topic<BB>::mousePressEvent(QGraphicsSceneMouseEvent* e)
{
	auto prev_sel = m_bb->firstSelected();
	QGraphicsRectItem::mousePressEvent(e);
	setZValue(BBOARD_ZVAL_RAISED);
	m_bb->setCurrentBand(bandIndex());
	if (e->buttons() & Qt::RightButton)
	{
		onContextMenu(e->screenPos());
		return;
	}

	if (e->modifiers() == Qt::ControlModifier)
	{
		std::unordered_set<ard::board_g_topic<BB>*> sel_lst;
		sel_lst.insert(this);
		m_bb->addMSelected(sel_lst);
	}
	else if (e->modifiers() == Qt::ShiftModifier)
	{
		if (prev_sel && prev_sel != this)
		{
			auto bidx = bandIndex();
			if (prev_sel->bandIndex() == bidx)
			{
				auto b = m_bb->bandAt(bidx);
				assert_return_void(b, QString("expected band %1").arg(bidx));		

				auto top = std::min(pos().y(), prev_sel->pos().y());
				auto bottom = std::max(pos().y(), prev_sel->pos().y());
				QRectF rc(b->band()->xstart()+10, top, b->band()->bandWidth()-10, bottom - top);
				auto lst = m_bb->scene()->items(rc);
				std::unordered_set<ard::board_g_topic<BB>*> sel_lst;
				sel_lst.insert(this);
				for (auto i : lst){
					auto gb = dynamic_cast<ard::board_g_topic<BB>*>(i);
					if (gb) {
						if (gb->bandIndex() == bidx) {
							sel_lst.insert(gb);
						}
						else {
							qDebug() << "ykh-not-right-band" << gb->bandIndex() << bidx;
						}
					}
				}
				m_bb->addMSelected(sel_lst);
			}
		}
	}
	else
	{
		if (!m_bb->isMSelected(this)) {
			m_bb->selectAsCurrent(this);
		}
	}
	m_ptPress = e->scenePos();
	//update();
};

template<class BB>
void ard::board_g_topic<BB>::mouseReleaseEvent(QGraphicsSceneMouseEvent * e)
{
	QGraphicsRectItem::mouseReleaseEvent(e);
	auto pt = e->scenePos();

	QLineF ln(m_ptPress, pt);
	auto move_len = ln.length();
	if (move_len > 10) {
		m_bb->processMoved(this);
	}

	setZValue(BBOARD_ZVAL_BITEM);
}

template<class BB>
void ard::board_g_topic<BB>::mouseDoubleClickEvent(QGraphicsSceneMouseEvent* )
{
	auto f = refTopic();
	if (f) {
		ard::open_page(f);
	}
};

template<class BB>
void ard::board_g_topic<BB>::hoverEnterEvent(QGraphicsSceneHoverEvent*)
{
	m_attr.Hover = 1;
	update();
};

template<class BB>
void ard::board_g_topic<BB>::hoverLeaveEvent(QGraphicsSceneHoverEvent*)
{
	m_attr.Hover = 0;
	update();
};

template<class BB>
void ard::board_g_topic<BB>::keyPressEvent(QKeyEvent * e) 
{
	QGraphicsRectItem::keyPressEvent(e);
	auto ref = refTopic();
	if (ref) 
	{
		switch (e->key())
		{
		case Qt::Key_Return:
		case Qt::Key_Enter:			
			ard::open_page(ref);
			break;
		case Qt::Key_Delete:
			m_bb->removeSelected(false);
			break;
		case Qt::Key_F2:			
			m_bb->renameCurrent();
			break;
		case Qt::Key_Down:
			m_bb->selectNext(Next2Item::down);
			break;
		case Qt::Key_Up:
			m_bb->selectNext(Next2Item::up);
			break;
		case Qt::Key_Left:
			m_bb->selectNext(Next2Item::left);
			break;
		case Qt::Key_Right:
			m_bb->selectNext(Next2Item::right);
			break;
		}
	}
}

#ifdef _DEBUG
template<class BB>
void ard::board_g_topic<BB>::drawDebugInfo(QPainter* p, topic_ptr)
{
	PGUARD(p);
	auto s = QString("%1/%2").arg(bandIndex()).arg(m_attr.OutlineIndex);
	auto f1 = QApplication::font("QMenu");
	p->setFont(f1);
	QSize testSize = ard::calcSize(s, &f1);
	auto rcText = rect();
	rcText.setSize(testSize);
	p->fillRect(rcText, QBrush(QColor(qRgb(234, 234, 177))));
	p->drawText(rcText, Qt::AlignLeft, s, &rcText);
	/*
	QRectF rc = boundingRect();
	PGUARD(p);
	p->drawRect(rc);

	QRectF rcs = sceneBoundingRect();
	QString s = "";

	p->setPen(QPen(Qt::black));
	auto f1 = QApplication::font("QMenu");
	p->setFont(f1);
	QSize testSize = ard::calcSize(s, &f1);
	QRectF rcText = rc;
	rcText.setTop(rcText.bottom() - testSize.height());
	rcText.setWidth(testSize.width() + 4);
	p->fillRect(rcText, QBrush(QColor(qRgb(234, 234, 177))));
	p->drawText(rcText, Qt::AlignLeft, s, &rcText);
	*/
};
#endif

///
/// board_box_g_topic
///
template<class BB, class T>
ard::board_box_g_topic<BB, T>::board_box_g_topic(board_page<BB>* bb, T* f) 
	:board_g_topic<BB>(bb), m_topic(f)
{
	LOCK(m_topic);
	board_g_topic<BB>::setFlags(QGraphicsItem::ItemIsSelectable |
		QGraphicsItem::ItemIsFocusable |
		QGraphicsItem::ItemSendsGeometryChanges);

	board_g_topic<BB>::m_attr.MaximizeWidth = 1;
};

template<class BB, class T>
ard::board_box_g_topic<BB, T>::~board_box_g_topic() 
{
	m_topic->release();
};

template<class BB, class T>
void ard::board_box_g_topic<BB, T>::mouseMoveEvent(QGraphicsSceneMouseEvent* e)
{
	QGraphicsRectItem::mouseMoveEvent(e);

	auto can_start_dnd = e->buttons() & Qt::LeftButton &&
		QLineF(e->screenPos(), e->buttonDownScreenPos(Qt::LeftButton)).length() > QApplication::startDragDistance();

	if (can_start_dnd) {
		auto lst = board_g_topic<BB>::m_bb->selectedRefTopics();
		if (!lst.empty()) 
		{
			auto l2 = board_g_topic<BB>::m_bb->selectedGBItems();
			auto bands = board_g_topic<BB>::m_bb->gitems2bands(l2);

			auto *drag = new QDrag(board_g_topic<BB>::m_bb);
			drag->setMimeData(new ard::TopicsListMime(lst.begin(), lst.end()));
			Qt::DropAction dropAction = drag->exec();
			board_g_topic<BB>::m_bb->onMovedInBands(bands);
			//qDebug() << "mouseMoveEvent dropAction=" << static_cast<int>(dropAction);
			Q_UNUSED(dropAction);
		}
	}
};

////
////  board_band - column on board
////
template<class BB>
ard::board_band<BB>::board_band(ard::board_page<BB>* bb, ard::board_band_info* bi/*, int bidx*/)
	:m_bb(bb), m_band(bi)//, m_band_index(bidx)
{
	QRectF rc = QRectF(0, 0, bi->bandWidth(), BBOARD_DEFAULT_HEIGHT);
	setAcceptHoverEvents(true);
	setRect(rc);
	setOpacity(DEFAULT_OPACITY);
	setZValue(BBOARD_ZVAL_BACK_WALL);
	setAcceptDrops(true);
	LOCK(m_band);
}

template<class BB>
ard::board_band<BB>::~board_band()
{
	m_band->release();
};

template<class BB>
void ard::board_band<BB>::paint(QPainter * p, const QStyleOptionGraphicsItem *, QWidget *)
{
	QRectF rc = boundingRect();

	PGUARD(p);
	p->setRenderHint(QPainter::Antialiasing, true);
	p->setPen(m_band->color());
	p->setBrush(m_band->color());
	p->drawRect(rc);
}


template<class BB>
void ard::board_band<BB>::updateSize(int band_height)
{
	if (band_height > BBOARD_DEFAULT_HEIGHT) {
		QRectF rc = QRectF(0, 0, m_band->bandWidth(), band_height);
		setRect(rc);
		update();
	}
};


template<class BB>
void ard::board_band<BB>::mousePressEvent(QGraphicsSceneMouseEvent* e)
{
	///
	/// don't call it, otherwise band resize doesn't work
	/// QGraphicsRectItem::mousePressEvent(e);
	///
	m_bb->setCurrentBand(m_band->bandIndex());
	bool wasEditMode = m_bb->isInEditMode();
	auto pt = e->scenePos();
	m_createTopicInit = false;
	m_mouse_press_time = QDateTime::currentDateTime();

	m_bb->initMSelect(pt);
	if (!wasEditMode) {
		if (!m_bb->isInCreateFromTemplateMode()) {
			m_createTopicInit = true;
		}
	}	
};


template<class BB>
void ard::board_band<BB>::mouseReleaseEvent(QGraphicsSceneMouseEvent * e)
{
	QGraphicsRectItem::mouseReleaseEvent(e);
	//if (e->buttons() & Qt::LeftButton)
	{
		auto pt = e->scenePos();
		if (!m_bb->completeMSelect()) {
			if (m_bb->isInCreateFromTemplateMode())
			{
				m_bb->suggestCreateFromTemplateAtPos(pt);
			}
			else
			{
				if (m_createTopicInit)
				{
					auto curr_time = QDateTime::currentDateTime();
					auto time_diff = m_mouse_press_time.msecsTo(curr_time);
					if (m_bb->isInInsertMode() || time_diff > 70) {
						m_createTopicInit = false;
						m_bb->createBoardTopic(pt);
					}
					else {
						m_createTopicInit = false;
					}
				}
			}
		};
	}
};

template<class BB>
void ard::board_band<BB>::mouseMoveEvent(QGraphicsSceneMouseEvent *e)
{
	QGraphicsRectItem::mouseMoveEvent(e);
	auto pt = e->scenePos();
	auto len = m_bb->resizeMSelect(pt);
	if (len > 10) {
		m_createTopicInit = false;
		//qDebug() << "drop-create-mode" << len;
	}
};



template<class BB>
void ard::board_band<BB>::dragEnterEvent(QGraphicsSceneDragDropEvent *e)
{
	m_bb->onBandDragEnter(this, e);
};

template<class BB>
void ard::board_band<BB>::dropEvent(QGraphicsSceneDragDropEvent *e)
{
	m_bb->onBandDragDrop(this, e);
}

template<class BB>
void ard::board_band<BB>::dragMoveEvent(QGraphicsSceneDragDropEvent *e) 
{
	m_bb->onBandDragMove(this, e);
};

template<class BB>
ard::board_band_header<BB>::board_band_header(ard::board_page<BB>* bb, ard::board_band_info* bi)
	:m_bb(bb), m_band(bi)
{
	QRectF rc = QRectF(0, 0, bi->bandWidth(), BBOARD_DEFAULT_HEIGHT);
	setAcceptHoverEvents(true);
	setRect(rc);
	setOpacity(DEFAULT_OPACITY);
	setZValue(BBOARD_ZVAL_BACK_WALL);
	setAcceptDrops(true);
	LOCK(m_band);
}

template<class BB>
ard::board_band_header<BB>::~board_band_header()
{
	m_band->release();
};

template<class BB>
void ard::board_band_header<BB>::paint(QPainter * p, const QStyleOptionGraphicsItem *, QWidget *)
{
	QRectF rc = boundingRect();

	PGUARD(p);
	p->setRenderHint(QPainter::Antialiasing, true);
	p->setPen(m_band->color());
	p->setBrush(m_band->color());
	p->drawRect(rc);

	QRectF rcText = rc;
	rcText.setBottom(rcText.top() + gui::lineHeight());
	p->setPen(QPen(Qt::gray, 1));
	m_lbl_rect = QRectF(0, 0, 0, 0);
	p->drawText(rcText, Qt::AlignCenter | Qt::AlignVCenter, m_band->bandLabel(), &m_lbl_rect);
	if (m_is_current) {
		rcText.setRight(rcText.right() - 10);
		rcText.setHeight(10);
		p->drawText(rcText, Qt::AlignRight | Qt::AlignVCenter, "...", &m_ctrl_rect);
	}
	int x = rc.right() - 2;
	p->drawLine(x, 0, x, 10);
}

template<class BB>
ard::BandHit ard::board_band_header<BB>::hitTest(QPointF pt)
{
	QRectF rc = boundingRect();
	QRectF rc2(rc.right() - 10, rc.top(), 10, gui::lineHeight());
	if (rc2.contains(pt)) {
		return BandHit::resize_header;
	}
	else {
		if (m_lbl_rect.contains(pt)) {
			return BandHit::band_label;
		}
		else if (m_ctrl_rect.contains(pt)) {
			return BandHit::control_button;
		}
		else {
			QRectF rc2 = rc;
			rc2.setHeight(gui::lineHeight());
			if (rc2.contains(pt)) {
				return BandHit::header;
			}
		}
	}

	return BandHit::none;
};

template<class BB>
void ard::board_band_header<BB>::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *e)
{
	QGraphicsRectItem::mouseDoubleClickEvent(e);
	m_bb->applyBandSplitterSetDefaultSize(band()->bandIndex());
};

template<class BB>
void ard::board_band_header<BB>::hoverMoveEvent(QGraphicsSceneHoverEvent * e)
{
	QGraphicsRectItem::hoverMoveEvent(e);
	auto vp = m_bb->header_view()->viewport();

	auto h = hitTest(e->pos());
	if (h == BandHit::resize_header) {
		vp->setCursor(Qt::SizeHorCursor);
	}
	else {
		vp->setCursor(Qt::ArrowCursor);
	}
};

template<class BB>
void ard::board_band_header<BB>::mouseMoveEvent(QGraphicsSceneMouseEvent *e)
{
	QGraphicsRectItem::mouseMoveEvent(e);
	auto pt = e->scenePos();
	if (m_resize_in_progress) {
		m_bb->resizeBandSplitter(pt.x());
	}
	else {
		m_bb->resizeMSelect(pt);
	}
};

template<class BB>
void ard::board_band_header<BB>::mousePressEvent(QGraphicsSceneMouseEvent* e)
{
	///
	/// don't call it, otherwise band resize doesn't work
	/// QGraphicsRectItem::mousePressEvent(e);
	///

	auto pt = e->scenePos();

	auto h = hitTest(e->pos());
	if (h == BandHit::resize_header) {
		m_resize_in_progress = true;
		m_bb->initBandSplitter(pt.x());
	}
	else if (h == BandHit::control_button)
	{
		m_bb->onBandControl(this);
	}
	else 
	{
		m_bb->setCurrentBand(m_band->bandIndex());
	}
};

template<class BB>
void ard::board_band_header<BB>::mouseReleaseEvent(QGraphicsSceneMouseEvent * e)
{
	QGraphicsRectItem::mouseReleaseEvent(e);
	auto pt = e->scenePos();
	if (m_resize_in_progress)
	{
		m_resize_in_progress = false;
		m_bb->applyBandSplitter(band()->bandIndex(), pt.x());
	}
};


template<class BB>
ard::board_g_mark<BB>::board_g_mark(ard::board_page<BB>* bb) :m_bb(bb)
{
	setZValue(BBOARD_ZVAL_RAISED);
};

template<class BB>
void ard::board_g_mark<BB>::paint(QPainter * p, const QStyleOptionGraphicsItem *, QWidget *)
{
	QRectF rc = boundingRect();
	PGUARD(p);

	const int m = ARD_MARGIN;
	auto rc1 = rc.marginsRemoved(QMarginsF(m, m, m, m));

	if (m_bb->isInLinkMode()) {
		p->setPen(QPen(BBOARD_CURR_MARK_COLOR, 6, Qt::DotLine));
		p->drawRect(rc1);
	}
	else {
		p->setPen(QPen(BBOARD_CURR_MARK_COLOR, 6));
		const int d = 10;

		qreal top = rc1.top();
		qreal left = rc1.left();
		qreal right = rc1.right();
		qreal bottom = rc1.bottom();

		qreal y1 = rc1.topLeft().y() + d;
		qreal y2 = rc1.bottomLeft().y() - d;
		qreal x1 = rc1.topLeft().x() + d;
		qreal x2 = rc1.topRight().x() - d;

		p->drawLine(rc1.topLeft(), QPointF(left, y1));
		p->drawLine(rc1.topRight(), QPointF(right, y1));

		p->drawLine(rc1.bottomLeft(), QPointF(left, y2));
		p->drawLine(rc1.bottomRight(), QPointF(right, y2));

		p->drawLine(rc1.topLeft(), QPointF(x1, top));
		p->drawLine(rc1.topRight(), QPointF(x2, top));

		p->drawLine(rc1.bottomLeft(), QPointF(x1, bottom));
		p->drawLine(rc1.bottomRight(), QPointF(x2, bottom));
	}
};

template<class BB>
void ard::board_g_mark<BB>::updateMarkPos(board_g_topic<BB>* gi)
{
	if (gi) {
		auto rc = gi->boundingRect();
		int dy = gui::lineHeight() / 2;

		//auto bi = gi->bitem();
		switch (gi->bshape()) {
		case ard::BoardItemShape::box:
		case ard::BoardItemShape::triangle:
		case ard::BoardItemShape::text_normal:
		case ard::BoardItemShape::text_italic:
		case ard::BoardItemShape::text_bold:
			break;
		default:
			dy = -gui::lineHeight() / 2;
		}

		auto pt = gi->pos();
		auto new_y = pt.y() - dy;
		auto new_x = rc.left() - dy;
		rc.setRect(new_x, rc.top(), rc.width() + 2 * dy, rc.height() + 2 * dy);
		if (new_y < 0)new_y = 0;
		pt.setY(new_y);

		setRect(rc);
		setPos(pt);
		show();
		update();
	}
	else {
		hide();
	}
};

template<class BB>
ard::selection_g_mark<BB>::selection_g_mark(board_page<BB>* bb, board_g_topic<BB>* g) 
	:board_g_mark<BB>(bb), m_g(g)
{
};

template<class BB>
void  ard::selection_g_mark<BB>::paint(QPainter * p, const QStyleOptionGraphicsItem *, QWidget *) 
{
	PGUARD(p);
	QRectF rc = board_g_mark<BB>::boundingRect();
	static int linew = 6;
	const int m = ARD_MARGIN;
	auto rc1 = rc.marginsRemoved(QMarginsF(m, m, m, m));
	//rc1.setTop(rc1.top() + linew);

	p->setPen(QPen(BBOARD_CURR_MARK_COLOR, linew, Qt::DotLine));
	//p->setPen(QPen(Qt::red, linew, Qt::DotLine));
	p->drawRect(rc1);

	//board_g_mark<BB>::paint(p, s, w);
	/*
	static int radius = 10;
	QRectF rc = QGraphicsRectItem::boundingRect();
	PGUARD(p);
    QGraphicsRectItem::setOpacity(0.1);
	p->setPen(QPen(BBOARD_CURR_MARK_COLOR, 3, Qt::DotLine));
	QBrush brush(QColor(211, 239, 245));
	p->setBrush(brush);
	p->drawRoundedRect(rc, radius, radius);
	*/
};

template<class BB>
void  ard::selection_g_mark<BB>::updateSelectionMark()
{
	if (m_g) {
        board_g_mark<BB>::updateMarkPos(m_g);
	}
};

template<class BB>
ard::board_page_toolbar<BB>::board_page_toolbar(ard::board_page<BB>* bb, std::vector<EBoardCmd> commands, std::vector<EBoardCmd> rcommands):m_bb(bb)
{
	auto f1 = QApplication::font("QMenu");
	QSize testSize = ard::calcSize("Completed: 100%");

	bmargin = ARD_MARGIN * 2;
	bspace = 2 * bmargin;
	m_progress_width = static_cast<int>(testSize.width());
	m_button_width_height = static_cast<int>(testSize.height());
	m_clear_btn_width = m_button_width_height *1.5;
	testSize = ard::calcSize("unread only", &f1);
	m_unread_label_width = static_cast<int>(testSize.width());
	setMinimumHeight(bmargin + m_button_width_height + bmargin);
	//setToolTip("Use toolbar to modify object in blackboard");
	setMouseTracking(true);

	for (auto c : commands) 
	{
		int w = m_clear_btn_width;
		switch (c) 
		{
			case EBoardCmd::colorDefault:
			case EBoardCmd::sliderDefault:
				w = m_clear_btn_width;
				break;
			case EBoardCmd::colorBtn:
				w = 4 * m_button_width_height + 4 * bmargin + m_clear_btn_width;
				break;
			case EBoardCmd::slider:
				w = m_progress_width + bspace + m_clear_btn_width;
				break;
        default:break;
		}

		board_page_toolbar_item it(c, w);
		m_items.push_back(it);
	}

	for (auto c : rcommands)
	{
		int w = m_clear_btn_width;
		switch (c)
		{
		case EBoardCmd::filter_unread:
			w = m_unread_label_width + bspace + m_button_width_height;
			break;
		default:break;
		}
		board_page_toolbar_item it(c, w);
		m_ritems.push_back(it);
	}
};

template<class BB>
void ard::board_page_toolbar<BB>::sync2Board() 
{
	m_CompletedPercent = 0;
	m_selected_color_idx = ard::EColor::none;

	auto f = m_bb->firstSelectedRefTopic();	
	if (f) 
	{
		ASSERT_VALID(f);
		m_CompletedPercent = f->getToDoDonePercent();
		m_selected_color_idx = f->colorIndex();
	}
	update();
};

template<class BB>
void ard::board_page_toolbar<BB>::paintEvent(QPaintEvent *)
{
	QPainter p(this);
	QRect rc3(0, 0, width() - 1, height() - 1);
	auto rc = rc3;

	QPen pen(gui::colorTheme_BkColor());
	QBrush brush(gui::colorTheme_BkColor());
	p.setBrush(brush);
	p.setPen(pen);
	p.drawRect(rc);

	auto f = QApplication::font("QMenu");
	p.setFont(f);
	p.setPen(QPen(Qt::gray));

	p.setBrush(Qt::NoBrush);
	
	std::function<void(QPainter&, const QRect&)> mark_selection = [](QPainter& p, const QRect& rc)
	{
		PGUARD(&p);
		p.setPen(QPen(QColor(color::Gray), 3));
		p.setBrush(Qt::NoBrush);
		auto rc1 = rc;
		rc1.setTop(rc1.top() + 1);
		p.drawRect(rc1);
	};

	std::function<void(const QPixmap&, const QRect&)> draw_pixmap = [&](const QPixmap& pm, const QRect& rc)
	{
		int dx = (rc.width() - m_button_width_height) / 2;
		int w = m_button_width_height;// rc.width() - 2 * dx;
		QRect rcIcon = rc;
		rcIcon.setLeft(rcIcon.left() + dx);
		rcIcon.setWidth(w);
		rcIcon.setHeight(w);
		p.drawPixmap(rcIcon, pm);
	};

	int xpos = 0;
	for (auto& c : m_items) {
		rc.setLeft(xpos);
		rc.setWidth(c.width);

		switch (c.cmd) 
		{
		case EBoardCmd::comment: 
			draw_pixmap(getIcon_Annotation(), rc); break;
		case EBoardCmd::rename:
			draw_pixmap(getIcon_Rename(), rc); break;
		case EBoardCmd::remove:
			draw_pixmap(getIcon_Recycle(), rc); break;
		case EBoardCmd::insert: 
			draw_pixmap(getIcon_Insert(), rc);
			if (m_bb->isInInsertMode())mark_selection(p, rc); 
			break;			 
		case EBoardCmd::link:
			draw_pixmap(getIcon_LinkArrow(), rc);
			if (m_bb->isInLinkMode())mark_selection(p, rc);
			break;
		case EBoardCmd::setShape:
			draw_pixmap(getIcon_BoardShape(), rc); break;
		case EBoardCmd::setTemplate:
			draw_pixmap(getIcon_BoardTemplate(), rc); break;
		case EBoardCmd::copy:
			draw_pixmap(getIcon_Copy(), rc); break;
		case EBoardCmd::paste:
			draw_pixmap(getIcon_Paste(), rc); break;
		case EBoardCmd::mark_as_read:
			draw_pixmap(getIcon_EmailAsRead(), rc); break;
		case EBoardCmd::colorBtn: 
		{
			PGUARD(&p);
			p.setPen(Qt::gray);
			QRect rc1(rc.left(), rc.top()+bmargin, m_button_width_height, m_button_width_height);
			for (auto& idx : m_buttons) {
				p.setBrush(QBrush(ard::cidx2color(idx)));
				p.drawRect(rc1);
				if (idx == m_selected_color_idx) {
					PGUARD(&p);
					p.setPen(Qt::black);
					p.drawText(rc1, Qt::AlignCenter | Qt::AlignVCenter, "[x]");
				}
				rc1.translate(m_button_width_height + bmargin, 0);
			}
			//rc1.translate(bspace - bmargin, 0);
			rc1.setWidth(m_clear_btn_width);
			p.setBrush(QBrush(Qt::white));
			p.drawRect(rc1);
			if (m_selected_color_idx == ard::EColor::none) {
				p.drawText(rc1, Qt::AlignCenter | Qt::AlignVCenter, "[x]");
			}
		}break;
		case EBoardCmd::slider:
		{
			drawSlider(p, rc);
		}break;
		default:break;
		}

		xpos += (c.width + bspace);
	}

	//xpos = 0;
	rc = rc3;
	for (auto& c : m_ritems) {
		//rc.setLeft(xpos);		
		rc.setLeft(rc.right() - c.width);
		rc.setWidth(c.width);

		switch (c.cmd)
		{            
		case EBoardCmd::filter_unread:
		{
			auto rc_icon = rc;
			rc_icon.setTop(rc_icon.top() + bmargin);
			rc_icon.setWidth(m_button_width_height);
			draw_pixmap(dbp::configFileMailBoardUnreadFilterON() ? getIcon_CheckedBox() : getIcon_UnCheckedBox(), rc_icon);
			auto rc_text = rc;
			rc_text.setLeft(rc_icon.right() + bmargin);
			PGUARD(&p);
			p.setPen(Qt::black);
			p.drawText(rc_text, Qt::AlignLeft | Qt::AlignVCenter, "unread only");
		}break;
        default:break;
		}
	}
};

template<class BB>
void ard::board_page_toolbar<BB>::drawSlider(QPainter& p, const QRect& rc)
{
	PGUARD(&p);
	p.setPen(Qt::gray);

	QRect rcSlider = rc;
	rcSlider.setTop(bmargin);
	rcSlider.setWidth(m_progress_width);
	rcSlider.setHeight(m_button_width_height);

	QColor clBkDef = gui::colorTheme_BkColor();
	QBrush brush(clBkDef, isEnabled() ? Qt::SolidPattern : Qt::CrossPattern);
	p.setBrush(brush);
	p.drawRect(rcSlider);

	qreal px_per_val = (qreal)rcSlider.width() / 100;

	int draw_flags = Qt::AlignHCenter | Qt::AlignVCenter;
	int done_perc = m_CompletedPercent;
	if (done_perc > 0)
	{
		int xcompl = (int)(px_per_val * done_perc);
		QRect rc_prg = rcSlider;
		rc_prg.setRight(rc_prg.left() + xcompl);
		if (rc_prg.right() >= rcSlider.right())
			rc_prg.setRight(rcSlider.right() - 1);

		QColor cl = color::LightGreen;

		QBrush brush(cl, isEnabled() ? Qt::SolidPattern : Qt::CrossPattern);
		p.setBrush(brush);
		p.drawRect(rc_prg);
	}

	QString s = QString("Completed:%1%").arg(done_perc);

	p.drawText(rcSlider, draw_flags, s);

	QPoint pt1 = rcSlider.bottomLeft();
	QPoint pt2 = pt1;
	pt2.setY(pt2.y() - 5);

	for (int i = 1; i < 10; i++)
	{
		pt1.setX((int)(rcSlider.left() + 10 * i * px_per_val));
		pt2.setX(pt1.x());
		p.drawLine(pt1, pt2);
	}

	auto rc1 = rcSlider;
	rc1.setLeft(rc1.right() + bmargin);
	rc1.setWidth(m_clear_btn_width);
	p.setBrush(QBrush(Qt::white));
	p.drawRect(rc1);
	if (m_CompletedPercent == 0) {
		p.drawText(rc1, Qt::AlignCenter | Qt::AlignVCenter, "[x]");
	}
};


template<class BB>
void ard::board_page_toolbar<BB>::mouseMoveEvent(QMouseEvent *e)
{
	QWidget::mouseMoveEvent(e);
	setCursor(Qt::ArrowCursor);
	processTooltip(e->pos(), e->globalPos());
};

template<class BB>
void ard::board_page_toolbar<BB>::mousePressEvent(QMouseEvent *e)
{
	auto h = hitTest(e->pos());
	switch (h.hit) {
	case EBoardCmd::none:break;
	case EBoardCmd::slider:
	{
		m_CompletedPercent = h.val;
		m_bb->applySelectionCommand([&](topic_ptr f)
		{
			f->setToDo(m_CompletedPercent, ToDoPriority::unknown);
		});
		m_bb->board()->setThumbDirty();
	}break;
	case EBoardCmd::colorBtn:
	{
		m_selected_color_idx = h.color;
		m_bb->applySelectionCommand([&](topic_ptr f)
		{
			f->setColorIndex(m_selected_color_idx);
		});
		m_bb->board()->setThumbDirty();
	}break;
	case EBoardCmd::colorDefault:
	{
		m_selected_color_idx = ard::EColor::none;
		m_bb->applySelectionCommand([&](topic_ptr f)
		{
			f->setColorIndex(m_selected_color_idx);
		});
		m_bb->board()->setThumbDirty();
	}break;
	case EBoardCmd::sliderDefault:
	{
		m_CompletedPercent = 0;
		m_bb->applySelectionCommand([&](topic_ptr f)
		{
			f->setToDo(m_CompletedPercent, ToDoPriority::unknown);
		});
		m_bb->board()->setThumbDirty();
	}break;
	case EBoardCmd::comment:
	{
		auto g = m_bb->firstSelected();
		if (g) {
			m_bb->editComment(g);
			m_bb->board()->setThumbDirty();
		}
	}break;
	case EBoardCmd::rename:
	{
		auto g = m_bb->firstSelected();
		if (g)
		{
			auto f = g->refTopic();
			if (f) {
				if (!f->canRename()) {
					m_bb->editComment(g);
					m_bb->board()->setThumbDirty();
					return;
				}
			}

			auto r = m_bb->renameCurrent();
			if (!r.first)
			{
				ard::asyncExec(AR_ShowMessage, r.second);
			}
			m_bb->board()->setThumbDirty();
			return;
		}
	}break;
	case EBoardCmd::locate:
	{
		if (m_bb->isInLocateMode()) {
			m_bb->exitLocateMode();
		}
		else {
			m_bb->enterLocateMode();
		}
		update();
	}break;
	case EBoardCmd::insert:
	{
		if (m_bb->isInInsertMode()) {
			m_bb->exitInsertMode();
		}
		else {
			m_bb->enterInsertMode();
		}
		m_bb->board()->setThumbDirty();
		update();
	}break;
	case EBoardCmd::remove:
	{
		m_bb->removeSelected(false);
		return;
	}break;
	case EBoardCmd::link:
	{
		if (m_bb->isInLinkMode()) {
			m_bb->exitLinkMode();
		}
		else {
			m_bb->enterLinkMode();
		}
		m_bb->board()->setThumbDirty();
		update();
	}break;
	case EBoardCmd::setTemplate:
	{
		if (m_bb->isInCreateFromTemplateMode()) {
			m_bb->exitCustomEditMode();
		}
		else {
			QPoint pt = gui::lastMouseClick();
			m_bb->insertFromTemplate(pt);
		}
		m_bb->board()->setThumbDirty();
	}break;
	case EBoardCmd::setShape:
	{
		QPoint pt = gui::lastMouseClick();
		ard::BoardItemShape sh = ard::BoardItemShape::unknown;
		auto g = m_bb->firstSelected();
		if (!g) {
			ard::messageBox(this,"Select topic on board to change it's shape.");
			return;
		}
		sh = g->bshape();
		m_bb->selectShape(sh, pt);
		//BoardShapeSelector::editShape(m_bb->board()->id(), sh, pt);
		m_bb->board()->setThumbDirty();
	}break;
	case EBoardCmd::copy:
	{
		m_bb->copyToClipboard();
	}break;
	case EBoardCmd::paste:
	{
		m_bb->pasteFromClipboard();
		m_bb->board()->setThumbDirty();
	}break;
	case EBoardCmd::mark_as_read: 
	{
		m_bb->markAsRead(true);
		m_bb->board()->setThumbDirty();
		m_bb->updateHeader();
	}break;
	case EBoardCmd::filter_unread: 
	{
		dbp::configFileSetMailBoardUnreadFilterON(!dbp::configFileMailBoardUnreadFilterON());
		m_bb->rebuildBoard();
		update();
		return;
	}break;
	};

	update();
	m_bb->updateMSelected();
}

template<class BB>
void ard::board_page_toolbar<BB>::processTooltip(const QPoint& pt, const QPoint& ptg)
{
	auto h = hitTest(pt);
	auto s = hit2Str(h);
	if (s.isEmpty()) {
		QToolTip::hideText();
	}else {
		QToolTip::showText(ptg, s);
		//qDebug() << "hit" << s;
	}
};


template<class BB>
QString ard::board_page_toolbar<BB>::hit2Str(const ard::ToolbarHitInfo& hi)
{
	if (hi.hit == EBoardCmd::colorBtn)
	{
		auto s1 = QString("Set Color - %1").arg(static_cast<int>(hi.color));
		return s1;
	}
	else if (hi.hit == EBoardCmd::slider)
	{
		auto s1 = QString("Set Progress - %1").arg(hi.val);
		return s1;
	}

#define CASE_H(H, S) case H:return S;
	switch (hi.hit) 
	{
		CASE_H(EBoardCmd::slider, "Set Progress");
		CASE_H(EBoardCmd::colorBtn, "Set Color");
		CASE_H(EBoardCmd::colorDefault, "Clear Color");
		CASE_H(EBoardCmd::sliderDefault, "Clear Progress");
		CASE_H(EBoardCmd::comment, "Set Comment");
		CASE_H(EBoardCmd::rename, "Rename");
		CASE_H(EBoardCmd::locate, "Locate/Details Mode");
		CASE_H(EBoardCmd::insert, "Insert Mode");
		CASE_H(EBoardCmd::remove, "Delete");
		CASE_H(EBoardCmd::link, "Link Mode");
		CASE_H(EBoardCmd::setShape, "Change Node Shape");
		CASE_H(EBoardCmd::setTemplate, "Insert From Template");
		CASE_H(EBoardCmd::copy, "Copy");
		CASE_H(EBoardCmd::paste, "Paste");
		CASE_H(EBoardCmd::mark_as_read, "Mark as read");
		CASE_H(EBoardCmd::filter_unread, "Check to show only unread messages");
	default:break;
	}

	return "";
#undef CASE_H
};

template<class BB>
ard::ToolbarHitInfo ard::board_page_toolbar<BB>::hitTest(const QPoint& pt)
{
	ToolbarHitInfo h;
	h.hit = EBoardCmd::none;
	h.val = 0;

	QRect rc = rect();
	int xpos = 0;
	for (auto& c : m_items) {
		rc.setLeft(xpos);
		rc.setWidth(c.width);
		if (rc.contains(pt)) {
			h.hit = c.cmd;
			if (h.hit == EBoardCmd::colorBtn) 
			{
				QRect rc1 = rc;
				rc1.setWidth(m_button_width_height + bmargin);
				for (auto& idx : m_buttons) {
					if (rc1.contains(pt)) {
						h.color = idx;
						return h;
					}
					rc1.translate(m_button_width_height + bmargin, 0);
				}				
				h.hit = EBoardCmd::colorDefault;
			}
			else if (h.hit == EBoardCmd::slider)
			{
				if (pt.x() > rc.left() + m_progress_width) {
					h.hit = EBoardCmd::sliderDefault;
					return h;
				}
				qreal px_per_val = (qreal)m_progress_width / 100;
				qreal per_val = 10 + (pt.x() - rc.left()) / px_per_val;
				h.val = 10 * (int)(per_val / 10.0);
			}
			return h;
		}

		xpos += (c.width + bspace);
	}

	rc = rect();
	for (auto& c : m_ritems) {				
		rc.setLeft(rc.right() - c.width);
		rc.setWidth(c.width);
		if (rc.contains(pt)) {
			h.hit = c.cmd;
			return h;
		}
	}

	return h;
};


template <class BB>
ard::board_editor<BB>* ard::board_editor<BB>::editAnnotation(ard::board_page<BB> *bb, QRect rce)
{
	return new ard::board_editor<BB>(bb, BEditorType::annotation, rce);
};

template <class BB>
ard::board_editor<BB>* ard::board_editor<BB>::editTitle(ard::board_page<BB> *bb, QRect rce)
{
	return new ard::board_editor<BB>(bb, BEditorType::title, rce);
};

template <class BB>
ard::board_editor<BB>* ard::board_editor<BB>::editBandLabel(ard::board_page<BB> *bb, QRect rce, int band_index)
{
	return new ard::board_editor<BB>(bb, BEditorType::band_label, rce, band_index);
};

template<class BB>
ard::board_editor<BB>::board_editor(ard::board_page<BB> *bb, BEditorType etype, QRect rce, int band_index)
	:m_bb(bb), m_etype(etype), m_band_index(band_index)
{
	setAttribute(Qt::WA_DeleteOnClose);
	m_bb->enterEditMode(this);
	setParent(m_bb->view());
	switch (m_etype) {
	case BEditorType::title:
	{
		setStyleSheet(QString("QTextEdit {border: 1px solid rgb(255,255,192);background-color:rgb(%1)}")
			.arg("172,172,172"));
	}break;
	case BEditorType::annotation:
	{
		setStyleSheet(QString("QTextEdit {border: 1px solid rgb(255,255,192);background-color:rgb(%1)}")
			.arg(color::getColorStringByClrIndex(11)));
	}break;
	case BEditorType::band_label:
	{
		setStyleSheet(QString("QTextEdit {border: 1px solid rgb(255,255,192);background-color:rgb(%1)}")
			.arg(color::getColorStringByClrIndex(7)));
	}break;
	};

	setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	setFont(*ard::defaultFont());

	switch (m_etype) {
	case BEditorType::title:
	{
		auto f = m_bb->firstSelectedRefTopic();
		if (f) {
			setPlainText(f->title().trimmed());
//			qDebug() << "edit-topic" << f->title();
		}
	}break;
	case BEditorType::annotation:
	{
		auto f = m_bb->firstSelectedRefTopic();
		if (f) {
			setPlainText(f->annotation().trimmed());
		}

	}break;
	case BEditorType::band_label:
	{
		auto b = m_bb->board()->bandAt(m_band_index);
		if (b) {
			setPlainText(b->bandLabel());
		}
	}break;
	};

	installLocationAnimator();

	setGeometry(rce);
	setEnabled(true);
	show();
	setFocus();
	animateLocator();

	if (m_etype == BEditorType::band_label) {
		selectAll();
	}
};

template<class BB>
ard::board_editor<BB>::~board_editor() 
{
	if (m_bb)m_bb->detachEditor();
};

template<class BB>
void ard::board_editor<BB>::storeText()
{
	auto str = toPlainText().trimmed();

	switch (m_etype) {
	case BEditorType::title:
	{
		auto f = m_bb->firstSelectedRefTopic();
		if (f) {
			if (str != f->title() ||
				(str.isEmpty() && f->isEmptyTopic()))
			{
				f->setTitle(str);
				bool killed_empty = false;
				if (f->isEmptyTopic())
				{
					auto g1 = m_bb->firstSelected();
					if (g1) {
						if (g1->refTopic() == f) {
							m_bb->removeSelected(true);
						}
						else {
							ASSERT(0, "expected current edited bitem");
						}
					}
					f->killSilently(true);
				}
				gui::rebuildOutline();
				if (!killed_empty) {
					m_bb->resizeGBItems(f);
				}
			}
		}
	}break;
	case BEditorType::annotation:
	{
		auto f = m_bb->firstSelectedRefTopic();
		if (f) {
			if (str != f->annotation()) {
				f->setAnnotation(str);
				gui::rebuildOutline();
				m_bb->resizeGBItems(f);
			}
		}
	}break;
	case BEditorType::band_label:
	{
		assert_return_void(m_band_index != -1, "expected valid band index");
		auto b = m_bb->board()->bandAt(m_band_index);
		if (b) {
			m_bb->board()->setBandLabel(m_band_index, str);
		}
	}break;
	};
};

template<class BB>
void ard::board_editor<BB>::focusOutEvent(QFocusEvent *e)
{
	storeText();
	close();
	QTextEdit::focusOutEvent(e);
}

template<class BB>
void ard::board_editor<BB>::paintEvent(QPaintEvent *e)
{
	QPainter pd(viewport());
	drawLocator(pd);
	QTextEdit::paintEvent(e);
}

template<class BB>
void ard::board_editor<BB>::keyPressEvent(QKeyEvent * e)
{
	switch (e->key())
	{
	case Qt::Key_Return:
	{
		storeText();
		close();
		return;
	}break;
	case Qt::Key_Escape:
	{
		close();
		return;
	}
	}

	QTextEdit::keyPressEvent(e);
};

template<class BB>
QRect ard::board_editor<BB>::locatorCursorRect()const
{
	QRect rc(0, 0, gui::lineHeight(), gui::lineHeight());
	return rc;
}

template<class BB>
void ard::board_editor<BB>::locatorUpdateRect(const QRect& rc)
{
	viewport()->update(rc);
};

///board_page_view
template<class BB>
ard::board_page_view<BB>::board_page_view(board_page<BB>* bb) :ArdGraphicsView(bb), m_bpage(bb)
{
};

template<class BB>
bool ard::board_page_view<BB>::process_keyPressEvent(QKeyEvent *e)
{
	bool rv = false;
	switch (e->key())
	{
	case Qt::Key_Delete:
	{
		auto g1 = m_bpage->firstSelected();
		if (g1) {
			m_bpage->removeSelected(false);
			rv = true;
		}
	}break;
	}
	return rv;
};

template<class BB>
void ard::board_page_view<BB>::keyPressEvent(QKeyEvent *e)
{
	if (process_keyPressEvent(e)) {
		e->accept();
		return;
	}
	ArdGraphicsView::keyPressEvent(e);
};

template<class BB>
void ard::board_page_view<BB>::scrollContentsBy(int dx, int dy)
{
	ArdGraphicsView::scrollContentsBy(dx, dy);
	if (m_bpage->isInEditMode()) {
		m_bpage->exitEditMode();
	}
};


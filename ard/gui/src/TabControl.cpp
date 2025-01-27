#include <QtGui>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QScrollBar>
#include <QMenu>
#include <QPushButton>
#include <QApplication>
#include "TabControl.h"
#include "ardmodel.h"
#include "MainWindow.h"
#include "custom-g-items.h"
#include "NoteEdit.h"
#include "email.h"
#include "extnote.h"

#define T_ICO_W ICON_WIDTH
#define MARK_W T_ICO_W

#define TSPACE_H (utils::outlineSmallHeight() + ARD_MARGIN)
#define TLOCUS_H (gui::lineHeight() + ARD_MARGIN)

static TabControl::tab* __tb = nullptr;
#define ADD_I_TAB(T, I, L, P) __tb = T->addTab(QPixmap(QString(":ard/images/unix/%1.png").arg(I)), L, P);
#define ADD_T_TAB(T, L, P) __tb = T->addTab(L, P);
#define ADD_I_SECONDARY_TAB(T, I, L, P) __tb = T->addSecondaryTab(QPixmap(QString(":ard/images/unix/%1.png").arg(I)), L, P);
#define ADD_I_SECONDARY_TAB_CLR(T, C, L, P){QPixmap _pm(T_ICO_W, T_ICO_W);_pm.fill(C);  __tb = T->addSecondaryTab(_pm, L, P);}
#define ADD_I_SECONDARY_TAB_PM(T, M, L, P, C){__tb = T->addSecondaryTab(M, L, P);} \
        if(C)__tb->setChecked(true);                                    \

#define ADD_I_CHECKED_SECONDARY_TAB(T, I, L, P, C)ADD_I_SECONDARY_TAB(T, I, L, P) \
    if(C)__tb->setChecked(true);                                        \


TabControl::tab::~tab()
{
	if (m_topic) {
		m_topic->release();
	}
};

QRectF TabControl::tab::calcBoundingRect()const
{
    QRectF rcT;

    switch (tc()->ttype())
        {
        case TabControl::EType::Left:
        case TabControl::EType::Right:
        {
            if (tc()->vert_text_by_symbol())
            {
                QFontMetrics fm(*ard::defaultFont());
                auto h = fm.ascent() * label().size() + ARD_MARGIN;
                if (!icon().isNull())
                    h += (T_ICO_W /*+ T_ICO_MARGIN*/);
                rcT = QRectF(0,
                    0,
                    tc()->tabWidth(),
                    h);
            }
            else
            {
                QSize sz = utils::calcSize(label());
                int h = sz.width() + 5 * ARD_MARGIN;
                if (!icon().isNull())
                    h += (T_ICO_W + T_ICO_MARGIN);
                rcT = QRectF(0,
                    0,
                    tc()->tabWidth(),
                    h);
            }
        }break;
        case TabControl::EType::RLocusedToobar:
        {
			const int def_dy = 8 * ARD_MARGIN;
            QSize sz = utils::calcSize(label());
			auto sz2 = utils::calcSize("1", ard::defaultSmall2Font());
			auto yd = sz2.height();
			if (yd > def_dy) {
				yd = def_dy;
			}
			int h = sz.width() + yd;//8 * ARD_MARGIN;// +tc()->tabWidth() / 2;//5 * ARD_MARGIN;
            rcT = QRectF(0,
                0,
                tc()->tabWidth(),
                h);
        }break;
        case TabControl::EType::ABC:
        {
            auto w = utils::calcWidth("www", utils::defaultSmall2Font());
            rcT = QRectF(0,
                0,
                w,
                TSPACE_H);
        }break;
    }//switch    

    return rcT;
};

QString TabControl::tab::mark_label()const 
{
	if (m_topic) {
		return m_topic->tabMarkLabel();
	}
	return "";
};

void TabControl::tab::attach_topic(ard::topic* f) 
{
	if (m_topic) {
		m_topic->release();
	}
	m_topic = f;
	if (m_topic) {
		LOCK(m_topic);
	}
}


class TabGMark : public QGraphicsPolygonItem {};

/**
   TabG
*/
//class TabG: public QGraphicsPolygonItem//QGraphicsRectItem
class TabG : public QGraphicsRectItem
{
public:

    enum class HitTest 
    {
        hitTab
    };

    TabG(TabControl::tab* );

    TabControl::tab* tab(){return m_t;}

    void  paint ( QPainter * p, const QStyleOptionGraphicsItem * o, QWidget * widget = 0);
    void  drawIcon(QPainter * p);
    void  drawAsSideTabs(QPainter * p);
    void  drawAsABC(QPainter * p);
    void  drawAsLocusToolbarTab(QPainter * p);
    void  drawAsRLocusToolbarTab(QPainter * p);
    void requestGeometryChange() { prepareGeometryChange(); };


protected:

    HitTest hitTest(const QPointF& )const
    {
        return HitTest::hitTab;
    }

    void mousePressEvent(QGraphicsSceneMouseEvent *)
    { 
        if(m_t->tc()->isCurrent(m_t)){
            m_t->tc()->onCurrentTabClicked(m_t);
            return;
        }
        

        m_t->tc()->setCurrentTab(m_t);
        m_t->tc()->onTabSelected(m_t);
    }

protected:
    TabControl::tab*    m_t;
    int                 m_fh;
};

TabG::TabG(TabControl::tab* t):m_t(t)
{
    setOpacity(DEFAULT_OPACITY);

    QRectF rcT = m_t->calcBoundingRect();
    switch (m_t->tc()->ttype()) {
        case TabControl::EType::Left:
        case TabControl::EType::Right:
        case TabControl::EType::RLocusedToobar:
        {
            QSize sz = utils::calcSize(m_t->label());
            m_fh = sz.height();
        }break;
        default:break;
    }

    setRect(rcT);
};


void TabG::drawIcon(QPainter * p)
{  
    if(!m_t->icon().isNull())
        {      
            QRectF rc = boundingRect();
            QRect rci;
            rectf2rect(rc, rci);
            int dx = (rci.width() - T_ICO_W) / 2;
            rci.setLeft(rci.left() + dx);
            rci.setTop(rci.top() + ARD_MARGIN);
            rci.setBottom(rci.top() + T_ICO_W);
            rci.setRight(rci.left() + T_ICO_W);
            p->drawPixmap(rci, m_t->icon());     
        }

};

void TabG::drawAsSideTabs(QPainter * painter)
{
    QRectF rc = boundingRect();
    bool asCurr = m_t->tc()->isCurrent(m_t);
    int dx = (int)(rc.width() - m_fh);

    PGUARD(painter);
    painter->setPen(Qt::NoPen);
    QColor bk = gui::darkSceneBk();
    if (asCurr)
    {
        bk = gui::colorTheme_CardBkColor();
        painter->setBrush(bk);
    }
    else
    {
        painter->setBrush(Qt::NoBrush);
    }


    qreal d = ARD_MARGIN;
    QRectF rcBk(rc.left() + d, rc.top() + d,
        rc.width() - 2 * d, rc.height() - 2 * d);
    painter->drawRect(rcBk);

    if (asCurr)
    {
        QPen penText(Qt::black);
        painter->setPen(penText);

        painter->drawLine(rcBk.topLeft(), rcBk.topRight());

        painter->drawLine(QPointF(rcBk.bottomLeft().x(), rcBk.bottomLeft().y()),
            QPointF(rcBk.bottomRight().x(), rcBk.bottomLeft().y()));
    }
    else
    {
        QPen pen(color::invert(bk.rgb()));
        painter->setPen(pen);
    }

    switch (m_t->tc()->ttype())
    {
    case TabControl::EType::Right:
    {
        int x = (int)rc.left();
        int y = (int)(rc.top() + 2 * ARD_MARGIN);

        if (!m_t->icon().isNull())
        {
            drawIcon(painter);
            x += T_ICO_W + T_ICO_MARGIN;
        }
        else
        {
            x += T_ICO_MARGIN;
        }
        painter->setFont(*ard::defaultFont());
        if (m_t->isVertical()) {
            painter->rotate(90);
            painter->drawText(x, y - dx, m_t->label());
        }
        else {
            painter->drawText(rc, Qt::AlignVCenter | Qt::AlignHCenter, m_t->label());
        }

    }break;
    case TabControl::EType::Left:
    {
        if (m_t->tc()->vert_text_by_symbol())
        {
            auto str = m_t->label().toUpper();
            painter->setFont(*ard::defaultFont());
            QFontMetrics fm(*ard::defaultFont());
            //QRectF brct;
            auto sz = fm.size(Qt::TextSingleLine, "W");
            auto h = fm.ascent();
            //qWarning() << "font-calc:" << sz.height() << fm.ascent() << fm.descent();
            auto rc1 = rc;

            if (!m_t->icon().isNull())
            {
                drawIcon(painter);
                rc1.setTop(rc1.top() + T_ICO_W /*+ T_ICO_MARGIN*/);
            }

            rc1.setLeft((rc1.width() - sz.width()) / 2);
            rc1.setWidth(sz.width());
            rc1.setHeight(sz.height());
            for (auto& s : str) {
                //painter->drawText(rc1, 0, s, &brct);
                painter->drawText(rc1, Qt::AlignHCenter, s);
                rc1.translate(0, h);
            }
        }
        else
        {
            int x = (int)rc.left();
            int y = (int)(rc.top() + ARD_MARGIN);

            if (!m_t->icon().isNull())
            {
                drawIcon(painter);
                x += T_ICO_W + T_ICO_MARGIN;
            }
            painter->rotate(90);
            painter->setFont(*ard::defaultFont());
            painter->drawText(x, y - dx, m_t->label());
        }
    }break;
    case TabControl::EType::ABC:
    //case TabControl::EType::LocusedToobar:
    case TabControl::EType::RLocusedToobar:
        ASSERT(0, "NA");
        break;
    }
}

void TabG::paint(QPainter * painter, const QStyleOptionGraphicsItem *, QWidget *)
{
    PGUARD(painter);
    painter->setRenderHint(QPainter::Antialiasing, true);
    switch (m_t->tc()->ttype())
    {
    case TabControl::EType::ABC:			drawAsABC(painter); break;
    case TabControl::EType::RLocusedToobar:	drawAsRLocusToolbarTab(painter); break;
    default:drawAsSideTabs(painter);
    }
};


void TabG::drawAsABC(QPainter * p)
{
    QRectF rc = boundingRect();
    bool asCurr = m_t->tc()->isCurrent(m_t);
    PGUARD(p);

    p->setPen(Qt::NoPen);
    QColor bk = gui::darkSceneBk();
    if (asCurr)
    {
        bk = gui::colorTheme_CardBkColor();
        p->setBrush(bk);
    }
    else
    {
        p->setBrush(Qt::NoBrush);
    }

    QRectF rc2 = rc;
    rc2.setTop(rc2.top());
    rc2.setBottom(rc2.bottom());
    p->drawRect(rc2);

    QPen pen(color::invert(bk.rgb()));
    p->setPen(pen);
    int flags = Qt::AlignCenter;
    p->drawText(rc, flags, m_t->label());
};

void TabG::drawAsRLocusToolbarTab(QPainter * painter)
{
    QRectF rc = boundingRect();
    bool asCurr = m_t->tc()->isCurrent(m_t);
    int dx = (int)(rc.width() - m_fh);

    PGUARD(painter);
    painter->setPen(Qt::NoPen);
    QColor bk = gui::darkSceneBk();
    if (asCurr)
    {
        bk = gui::colorTheme_CardBkColor();
        painter->setBrush(bk);
    }
    else
    {
        painter->setBrush(Qt::NoBrush);
    }

    qreal d = ARD_MARGIN;
    QRectF rcBk(rc.left() + d, rc.top() + d,
        rc.width() - 2 * d, rc.height() - 2 * d);
    painter->drawRect(rcBk);

    if (asCurr)
    {
        QPen penText(Qt::black);
        painter->setPen(penText);

        painter->drawLine(rcBk.topLeft(), rcBk.topRight());

        painter->drawLine(QPointF(rcBk.bottomLeft().x(), rcBk.bottomLeft().y()),
            QPointF(rcBk.bottomRight().x(), rcBk.bottomLeft().y()));
    }
    else
    {
		auto clr = color::invert(bk.rgb());
		{
			PGUARD(painter);			
			QPen p2(QColor(clr), 1);
			auto y2 = rcBk.bottom();
			painter->setPen(p2);
			painter->drawLine(1, y2, 5, y2);
		}
        QPen pen(clr);
        painter->setPen(pen);
    }

	//...
	int mh = 0;
	auto mlbl = m_t->mark_label();
	if (!mlbl.isEmpty())
	{
		if (mlbl.size() == 1) {
			mlbl = QString(" %1 ").arg(mlbl);
		}
		PGUARD(painter);
		auto fnt = ard::defaultSmall2Font();
		QFontMetrics fm(*fnt);
		QRect rc2 = fm.boundingRect(mlbl);
		mh = rc2.height();
		auto ydelta = 0;

		painter->setFont(*fnt);
		QRectF rcText = rc;
		rcText.setLeft(rcText.right() - rc2.width());
		rcText.setBottom(rcText.top() + rc2.height() - ydelta);

		QBrush br(color::Olive);
		painter->setBrush(br);
		QPen pn(color::Gray_1);
		painter->setPen(pn);

		int radius = 5;
		painter->drawRoundedRect(rcText, radius, radius);
		painter->drawText(rcText, Qt::AlignVCenter | Qt::AlignHCenter, mlbl);
	}
	//....

    int x = (int)rc.left() + mh/2;
	int y = (int)(rc.top()+ 2 * ARD_MARGIN);
    painter->setFont(*ard::defaultFont());
    painter->rotate(90);
    painter->drawText(x + ARD_MARGIN, y - dx, m_t->label());
};

void TabG::drawAsLocusToolbarTab(QPainter * p) 
{
    PGUARD(p);
    
    bool asCurr = m_t->tc()->isCurrent(m_t);
    if (asCurr) {
        auto pn = model()->penGray();
        pn.setWidth(3);     
        QColor clbk = color::ReutersChart;
        pn.setColor(clbk);
        p->setPen(pn);
    }
    else {
        p->setPen(model()->penGray());
    }
    

    QRectF rc = boundingRect();
    rc.setTop(rc.top() + ARD_MARGIN);
    rc.setBottom(rc.bottom() - ARD_MARGIN);
    if (asCurr) {
        rc.setTop(rc.top() + ARD_MARGIN);
    }

    QPainterPath r_path;
    r_path.addRoundedRect(rc, 15, 15);
    
    bool black_bg = true;
    QColor clr = color::getColorByClrIndex(0);
    if (m_t->colorIndex() != ard::EColor::none) {
		clr = ard::cidx2color(m_t->colorIndex());//color::getColorByClrIndex(m_t->colorIndex());
        black_bg = false;
    }   

    
    QBrush brush(clr);
    p->setBrush(brush);
    p->drawPath(r_path);    
    
    if (black_bg) {
        auto pn = QPen(Qt::gray);
        p->setPen(pn);
    }
    else {
        auto pn = QPen(Qt::black);
        p->setPen(pn);
    }

    p->setFont(*ard::defaultFont());
    QRectF rcText = rc;
    p->drawText(rcText, Qt::AlignVCenter | Qt::AlignHCenter, m_t->label());
};

/**
   TabControl
*/
TabControl::TabControl(TabControl::EType t, QString hint)
    : m_tab_wh(0.0),
      m_ttype(t),
      m_hint(hint)
{
    m_note_edit = nullptr;
    m_scene = new ArdGraphicsScene;
    m_view = new TabView(this);
    m_view->setScene(m_scene);
    m_view->setFrameShape(QFrame::NoFrame);
    utils::setupCentralWidget(this, m_view);

    resetTabControl();
};

void TabControl::initAsVerticalTab()
{
    QSizePolicy sp(QSizePolicy::Fixed, QSizePolicy::Expanding);
    setSizePolicy(sp);
    setSideTabDefaultTabWidth();
    qreal w = m_tab_wh;

    QPointF pt1(0,0);
    QPointF pt2(w,0);
    QColor clBkDef = gui::darkSceneBk();
    QColor clBkDef2 = clBkDef.lighter(150);
    QLinearGradient lgrad(pt1,
                          pt2);

    lgrad.setColorAt(0.0, clBkDef);
    lgrad.setColorAt(1.0, clBkDef2);

    m_scene->setBackgroundBrush(lgrad);
	m_view->setupKineticScroll(ArdGraphicsView::scrollKineticVerticalOnly);
};

void TabControl::initAsABC() 
{
    ASSERT(m_ttype == EType::ABC, "expected toolbar type");
    QSizePolicy sp(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setSizePolicy(sp);
    setTabHeight(TSPACE_H);

    QPointF pt1(0, 0);
    QPointF pt2(300, 0);
    QColor clBkDef = gui::darkSceneBk();
    QColor clBkDef2 = clBkDef.lighter(150);
    QLinearGradient lgrad(pt1,
        pt2);
    lgrad.setColorAt(0.0, clBkDef2);
    lgrad.setColorAt(1.0, clBkDef);
    m_scene->setBackgroundBrush(lgrad);
	m_view->setupKineticScroll(ArdGraphicsView::scrollKineticHorizontalOnly);
};


/**
   TabView
*/
TabView::TabView(TabControl* tc)
    :m_tc(tc)
{
    setNoScrollBars();
//    setup(true);
};


void TabView::resetGItemsGeometry()
{
    ASSERT(0, "NA");
};

/*
void TabView::scrollContentsBy (int dx, int dy )
{
    switch(kscroll())
        {
        case ArdGraphicsView::scrollKineticVerticalOnly:
            {
                ArdGraphicsView::scrollContentsBy(0, dy);
            }break;
        case ArdGraphicsView::scrollKineticHorizontalOnly:
            {
                ArdGraphicsView::scrollContentsBy(dx, 0);
            }break;
        case scrollKineticVerticalAndHorizontal:
            {
                ArdGraphicsView::scrollContentsBy(dx, dy);
            }break;
        default:break;
        }
  
    //m_tc->updateHudItemsPos();
};

void TabView::swipeX(int dx)
{
    bool proceed_swipe = !m_tc->isRolledDown();
    if (proceed_swipe)
    {
        switch (m_tc->ttype())
        {
        case TabControl::EType::Left:
        {
            proceed_swipe = (dx < 0);
        }break;
        case TabControl::EType::Right:
        {
            proceed_swipe = (dx > 0);
        }break;
        default:proceed_swipe = false;
        }
    }

    if (proceed_swipe)
    {
        if (m_lastSwipeTime.isValid())
        {
            proceed_swipe = (m_lastSwipeTime.msecsTo(QDateTime::currentDateTime()) > 800);
        }
    }

    if (proceed_swipe)
    {
        m_lastSwipeTime = QDateTime::currentDateTime();
        gui::hideExterior();
    }
};*/



void TabView::mousePressEvent(QMouseEvent * e)
{
    m_ptMousePress = e->globalPos();

	auto tt = m_tc->ttype();
	if (tt == TabControl::EType::RLocusedToobar)
	{
		auto s = scene();
		auto pt = mapToScene(e->pos());
		auto lst = s->items(pt);
		if (lst.empty())
		{
			m_splitter_move_request = true;
		}
		else
		{
			auto i = lst.first();
			auto g = dynamic_cast<TabG*>(i);
			if (g && g->tab() == m_tc->currentTab())
			{
				m_splitter_move_request = true;
			}
		}
	}
    ArdGraphicsView::mousePressEvent(e);
}

void TabView::mouseMoveEvent(QMouseEvent* e)
{
	if (m_splitter_move_request) 
	{
		QPoint delta = e->globalPos() - m_ptMousePress;
		auto w = main_wnd();
		if (w)
		{
			w->moveSplitter(delta.x());
			m_ptMousePress = e->globalPos();
			return;
		}
	}
    ArdGraphicsView::mouseMoveEvent(e);
};

void TabView::mouseReleaseEvent(QMouseEvent * e) 
{
    ArdGraphicsView::mouseReleaseEvent(e);
	m_splitter_move_request = false;
};

void TabControl::setTabWidth(qreal w)
{
    m_tab_wh = w;
    setMaximumWidth((int)w);
    setMinimumWidth((int)w);
}

void TabControl::setTabHeight(qreal h)
{
    m_tab_wh = h;
    setMaximumHeight((int)h);
    setMinimumHeight((int)h);
};

TabControl::~TabControl()
{
    //detachGui();
};

void TabControl::detachGui()
{
    removeAllTabs();
};


TabControl::tab* TabControl::addTab(QString _label, int _data)
{
    tab* t = new tab(this, _label, _data);
    m_tabs.push_back(t);
    return t;
};

TabControl::tab* TabControl::addTab(QPixmap _icon, QString _label, int _data)
{
    tab* t = new tab(this, _label, _data);
    t->setIcon(_icon);
    m_tabs.push_back(t);
    return t;
};

TabControl::tab* TabControl::doAddLocusTopic(topic_ptr f)
{
    ASSERT(/*m_ttype == EType::LocusedToobar ||*/ m_ttype == EType::RLocusedToobar, "expected locus bar");
	m_has_locused = true;
    tab* t = new tab(this, f->altShortTitle(), f->id());
	t->attach_topic(f);
    auto color_idx = f->colorIndex();
    t->setColorIndex(color_idx);
    m_tabs.push_back(t);
    //m_locked_t2t[f] = t;
    LOCK(f);
    return t;
};

bool TabControl::hasLocusTopics()const 
{
	return m_has_locused;
};

TabControl::tab* TabControl::addLocusTopic(topic_ptr f)
{
    auto idx = indexOfTopic(f);
    if (idx != -1) {
        ASSERT(0, "NA");
        return nullptr;
    }
    auto t = doAddLocusTopic(f);
    if (t) {//??
        rebuildTabs();
    }
    return t;
};

void TabControl::setLocusTopics(const TOPICS_LIST& lst)
{    
    IDS_LIST tabs2remove;
    IDS_SET tset;
    for (const auto& f : lst) {
        auto idx = indexOfTopic(f);
        if (idx == -1) {
            doAddLocusTopic(f);
        }
        else {
            auto t = m_tabs[idx];
            QString stitle = f->altShortTitle();
            if (stitle != t->label()) {
                t->setLabel(stitle);
            }
            auto color_idx = f->colorIndex();
            if (color_idx != t->colorIndex()) {
                t->setColorIndex(color_idx);
            }
        }
        tset.insert(f->id());
    }//for

    for (auto t : m_tabs) {
        auto fid = t->data();
        if (tset.find(fid) == tset.end()) {
            tabs2remove.push_back(fid);
        }
    }

    for (auto& fid : tabs2remove) {
        auto idx = indexOfTopic(fid);
        if (idx != -1) {
            m_tabs.erase(m_tabs.begin() + idx);
            //removeTab(idx);
            //m_tab_colors.erase(idx);
        }
        else {
            ASSERT(0, "failed to locate topic in tab") << fid;
        }
    }

    ASSERT(static_cast<int>(lst.size()) == count(), "expected same containers size for topic tabs") << lst.size() << count();
    rebuildTabs();
};

int TabControl::indexOfTopic(topic_cptr f)const 
{
    auto fid = f->id();
    return indexOfTopic(fid);
};

int TabControl::indexOfTopic(DB_ID_TYPE fid)const 
{
    int i, Max = static_cast<int>(m_tabs.size());
    for (i = 0; i < Max; i++) {
        if (m_tabs[i]->data() == static_cast<int>(fid)) {
            return i;
        }
    }
    return -1;
};

#ifdef _DEBUG
void TabControl::debug_print()
{
    qDebug() << "====== tabs" << m_tabs.size() << "=======";
    for (auto i : m_tabs) {
        qDebug() << i->label() << i->data();    
    }
    qDebug() << "====== sec-tabs" << m_secondary_tabs.size() << "=======";
    for (auto i : m_secondary_tabs) {
        qDebug() << i->label() << i->data();
    }
};
#endif


void TabControl::ensureVisible(DB_ID_TYPE fid)
{
    auto idx = indexOfTopic(fid);
    if (idx != -1) {
        auto t = m_tabs[idx];

        TABS2G::iterator k = m_tabs2gitem.find(t);
        if (k != m_tabs2gitem.end())
        {
            TabG* g = k->second;
            g->ensureVisible();
        }
    }
};

TabControl::tab* TabControl::addSecondaryTab(QPixmap _icon, QString _label, int _data)
{
    tab* t = new tab(this, _label, _data);
    t->setIcon(_icon);
    m_secondary_tabs.push_back(t);
    return t;
};

struct delete_tab
{
    void operator()(TabControl::tab* t){delete t;}
};

void TabControl::removeAllTabs()
{
    clearTabControl();

    std::for_each(m_tabs.begin(), m_tabs.end(), delete_tab());
    std::for_each(m_secondary_tabs.begin(), m_secondary_tabs.end(), delete_tab());

    m_tabs.clear();
	/*if (!m_locked_t2t.empty())
	{
		for (auto& i : m_locked_t2t) {
			auto f = i.first;
			f->release();
		}
		m_locked_t2t.clear();
	}*/
    m_secondary_tabs.clear();
    m_curr_tab = nullptr;

    if (m_tspace_vlayout) {
        
        int Max = m_tspace_vlayout->count();
        using WGT = std::set<QWidget*>;
        WGT widgets_in_layout;
        for (int i = 0; i < Max; i++) {
            QLayoutItem* it = m_tspace_vlayout->itemAt(i);
            auto w = it->widget();
            if (w && w != m_empty_tab_spacer) {
                widgets_in_layout.insert(w);
            }
        }

        if (!widgets_in_layout.empty()) {
            for (auto w : widgets_in_layout) {
                m_tspace_vlayout->removeWidget(w);
                delete w;
            }
        }        
                
        checkSpacer();
    }
};

int TabControl::selectNextTab(tab* t) 
{
    auto idx = indexOfTab(t);
    if (idx != -1) {
        int next_idx = idx;
        if (next_idx == static_cast<int>(m_tabs.size()) - 1) {
            next_idx--;
        }
        else {
            next_idx++;
        }

        tab* next_tab = nullptr;
        if (m_tabs.size() > 0 && next_idx >= 0 && next_idx < static_cast<int>(m_tabs.size())) {
            next_tab = m_tabs[next_idx];
        }

        if (next_tab) {
            selectTab(next_tab);
        }
        else {
            selectTab(nullptr);
        }
    }
    return idx;
};


bool TabControl::isEmpty()const 
{ 
    return m_tabs.empty(); 
}

int TabControl::count()const 
{
    return static_cast<int>(m_tabs.size()); 
}

TabControl::tab* TabControl::tabAt(int pos)
{
    if (pos >= 0 && pos < static_cast<int>(m_tabs.size())) {
        return m_tabs[pos];
    }
    ASSERT(0, "invalid tab index");
    return nullptr;
};

void TabControl::updateGItem(tab* t)
{
    TABS2G::iterator k = m_tabs2gitem.find(t);
    if(k != m_tabs2gitem.end())
        {
            TabG* g = k->second;
            g->update();
        }
    else
        {
            ASSERT(0, "failed to locate g-item");
        }  
};

bool TabControl::hasCurrent()const 
{
    return (m_curr_tab != nullptr);
};

TabControl::tab* TabControl::currentTab()
{
    return m_curr_tab;
};

int TabControl::currentData()const 
{
    int rv = -1;
    if (m_curr_tab) {
        rv = m_curr_tab->data();
    }
    return rv;
};

void TabControl::setCurrentTab(tab* t)
{
    if (!t) {
        m_curr_tab = nullptr;
        return;
    }

    if (t && t != m_curr_tab)
    {
        if (m_curr_tab)
        {
            auto i = m_tabs2gitem.find(m_curr_tab);
            if (i != m_tabs2gitem.end())
            {
                i->second->update();
            }
        }

        m_curr_tab = t;

        TABS2G::iterator i = m_tabs2gitem.find(t);
        if (i != m_tabs2gitem.end())
        {
            auto g = i->second;
            g->update();

        }
    }
};

bool TabControl::isCurrent(const tab* t)const
{
    bool rv = (m_curr_tab == t);
    return rv;
};

void TabControl::clearTabControl()
{
    m_tabs2gitem.clear();
    if (m_scene) {
        m_scene->clearArdScene();
    }
};

void TabControl::rebuildTabs()
{
    clearTabControl();

    switch(m_ttype)
        {
        case EType::ABC:            rebuildAsABC(); break;
        //case EType::LocusedToobar:  rebuildAsLocusedToobar();
            break;
        default:
            rebuildAsVerticalTab();
        }

    m_view->emit_onResetSceneBoundingRect();
};

void TabControl::asyncRebuild() 
{
    if (!m_async_rebuild_request) {
        m_async_rebuild_request = true;

        QTimer::singleShot(100, this, [=]() {
            m_async_rebuild_request = false;
            rebuildTabs();
        });
    }
};

int TabControl::calcBoundingHeight()const 
{
    int rv = 0;
    for (auto t : m_tabs) {
        auto r = t->calcBoundingRect();
        rv += (static_cast<int>(r.height()) + 3 * ARD_MARGIN);
    }
    return rv;
};

TabG* TabControl::registerTab(tab* t)
{
    TabG* g = new TabG(t);
    m_scene->addItem(g);
    m_tabs2gitem[t] = g;
    return g;
};

void TabControl::rebuildAsThumbnailer()
{
    qreal x_pos = 0.0;
    rebuildXDimensionToolbar(x_pos, 0.0);
};

void TabControl::rebuildAsVerticalTab()
{  
  qreal y_pos = 0.0;
  for (TABS::iterator i = m_tabs.begin(); i != m_tabs.end(); i++)
  {
      tab* t = *i;
      if (!t->isEnabledTab())
          continue;
      TabG* g = registerTab(t);
      g->setPos(0.0, y_pos);
      y_pos += g->boundingRect().height();
      switch (m_ttype)
      {
      case TabControl::EType::RLocusedToobar:
          break;
      default:
          y_pos += 3 * ARD_MARGIN;
          break;
      }
  }
};

void TabControl::rebuildXDimensionToolbar(qreal x_pos, qreal y_pos)
{
    for(auto t : m_tabs)
    {
        TabG* g = registerTab(t);
        g->setPos(x_pos, y_pos);
        x_pos += g->boundingRect().width();
    }
};


void TabControl::rebuildAsABC() 
{
    qreal y_pos = 0.0;
    qreal x_pos = 0.0;
    rebuildXDimensionToolbar(x_pos, y_pos);
};

void TabControl::update()
{
    if(m_view)
        {
            m_view->viewport()->update();
        }
};

#define RETURN_TAB_BY_DATA(C, D) for(auto& t : C){if(t->data() == D)return t;}

TabControl::tab* TabControl::findByData(int d)
{
    RETURN_TAB_BY_DATA(m_tabs, d);

    return nullptr;
};

TabControl::tab* TabControl::findSecondaryByData(int d)
{
    RETURN_TAB_BY_DATA(m_secondary_tabs, d);
    return nullptr;
};


int TabControl::indexOfTab(tab* t)
{
    int rv = -1;
    size_t i = 0, Max = m_tabs.size();
    for (; i < Max; i++) {
        if (m_tabs[i] == t) {
            return i;
        }
    }
    return rv;
};

void TabControl::setCurrentTabByData(int d)
{
    TabControl::tab* t = findByData(d);
    if(t){
        if (t != m_curr_tab) {
            setCurrentTab(t);
        }
    }
};

void TabControl::selectTab(tab* t) 
{
    setCurrentTab(t);
    onTabSelected(t);
};

bool TabControl::selectTabByData(int d) 
{
    auto idx = indexOfTopic(d);
    if (idx != -1) {
        auto t = m_tabs[idx];
        //ykh+if
        if (t && !isCurrent(t)) {
            setCurrentTab(t);
            onTabSelected(t);           
        }
        return true;
    }
    return false;
};

void TabControl::setTabData(int idx, int data)
{
    if(idx >= 0 && idx < (int)m_tabs.size())
        {
            tab* t = m_tabs[idx];
            t->m_data = data;
        }
    else
        {
            ASSERT(0, "invalid tab index") << idx << data;
        }
};

void TabControl::onTabSelected(tab* t)
{
    emit tabSelected(t->data());
};

void TabControl::onCurrentTabClicked(tab* )
{
    emit currentTabClicked();
};


TabControl* TabControl::createABCTabControl() 
{
    TabControl* tc = new TabControl(EType::ABC, "abc");
    STRING_LIST slst;
    slst.push_back("*");
    slst.push_back("abc");
    slst.push_back("def");
    slst.push_back("ghi");
    slst.push_back("jkl");
    slst.push_back("mno");
    slst.push_back("opq");
    slst.push_back("rst");
    slst.push_back("uvw");
    slst.push_back("xyz");

    int idx = '*';
    for (auto& i : slst) {
        tc->addTab(i, idx);
        idx++;
    }
    tc->rebuildTabs();

    auto t = tc->findByData('*');
    if (t) {
        tc->setCurrentTab(t);
    }
    else {
        ASSERT(0, "failed to locate tab");
    }
    return tc;
};

TabControl* TabControl::createLocusedToobar(EOutlinePolicy pol)
{
    TabControl* tc = new TabControl(EType::RLocusedToobar);
    tc->m_policy = pol;
    return tc;
};

TabControl* TabControl::createMainTabControl()
{
    TabControl* tc= new TabControl(TabControl::EType::Left);
	ADD_I_TAB(tc, "email", "Emails", outline_policy_PadEmail);
    ADD_I_TAB(tc, "note", "Notes", outline_policy_Pad);
	ADD_I_TAB(tc, "new-todo", "ToDo", outline_policy_TaskRing);
	ADD_I_TAB(tc, "pencil", "Grouped", outline_policy_Colored);
    ADD_I_TAB(tc, "board", "Boards", outline_policy_BoardSelector);
    ADD_I_TAB(tc, "view", "Misc..", outline_policy_2SearchView);    
    if (dbp::configFileSupportCmdLevel() > 0){
        ADD_I_TAB(tc, "key", "Passwords", outline_policy_KRingTable);
    }

    tc->rebuildTabs();
    return tc;
};

TabControl* TabControl::createGrepTabControl()
{
    TabControl* tc= new TabControl(TabControl::EType::Right, "grep");
    ADD_T_TAB(tc, "Search", outline_policy_2SearchView);
    ADD_T_TAB(tc, "Notes List", outline_policy_Notes);
    ADD_T_TAB(tc, "Comments", outline_policy_Annotated);
	ADD_T_TAB(tc, "Bookmarks", outline_policy_Bookmarks);
	ADD_T_TAB(tc, "Pictures", outline_policy_Pictures);
    tc->rebuildTabs();
    return tc;
};

TabControl* TabControl::createKRingTabControl()
{
    TabControl* tc = new TabControl(TabControl::EType::Right, "kring");
    ADD_T_TAB(tc, "List", outline_policy_KRingTable);
    ADD_T_TAB(tc, "Edit", outline_policy_KRingForm);
    tc->rebuildTabs();
    return tc;
};

void TabControl::setSideTabDefaultTabWidth()
{
    switch(m_ttype)
        {
        case EType::Right:
        case EType::Left:
        case EType::RLocusedToobar:
            break;
        default:{ASSERT(0, "NA");return;}
        }

    unsigned w = utils::calcWidth("AAA", utils::defaultBoldFont());
    setTabWidth(w);
    rebuildTabs();
};

void TabControl::resetTabControl()
{
    assert_return_void(m_scene, "expected scene");

    switch (m_ttype)
    {
    case EType::ABC:        initAsABC(); break;
    default:
        initAsVerticalTab();
    }
};


void TabControl::checkSpacer()
{
    assert_return_void(m_tspace_vlayout, "expected layout");

    bool showSpacer = false;
    int w_count = m_tspace_vlayout->count();
    if (w_count == 0) {
        showSpacer = true;
    }
    else if (w_count == 1) {
        QLayoutItem* it = m_tspace_vlayout->itemAt(0);
        auto w = it->widget();
        if (w == m_empty_tab_spacer) {
            showSpacer = true;
        }
    }

    //bool showSpacer = m_tspace_vlayout->isEmpty();

    if (showSpacer) {
        if (!m_empty_tab_spacer) {
            m_empty_tab_spacer = new QWidget();
            m_empty_tab_spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
            m_tspace_vlayout->addWidget(m_empty_tab_spacer);
        }
        ENABLE_OBJ(m_empty_tab_spacer, true);
    }
    else {
        if (m_empty_tab_spacer) {
            ENABLE_OBJ(m_empty_tab_spacer, false);
        }
    }
};

/*
QWidget* spacer = new QWidget();
spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
q_act_layout->addWidget(spacer);

*/

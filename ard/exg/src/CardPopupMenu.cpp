#include <QTimer>
#include "CardPopupMenu.h"
#include "anfolder.h"
#include "custom-widgets.h"
#include "registerbox.h"
#include "gmail/GmailRoutes.h"
#include "email.h"
#include "ethread.h"
#include <QApplication>
#include "custom-menus.h"
#include "board.h"

#ifdef ARD_BIG

ard::CardPopupMenuBase::CardPopupMenuBase(topic_ptr f)
{
    ASSERT(f, "expected topic");
    LOCK(f);
    m_topics.push_back(f);
    initCard();
};

ard::CardPopupMenuBase::CardPopupMenuBase(TOPICS_LIST lst)
{
    m_topics = lst;
    for (auto& f : m_topics){
        LOCK(f);
    }
    initCard();
};

void ard::CardPopupMenuBase::initCard()
{
	//..............
	setParent(gui::mainWnd());
	setFocus();
	//..............

    auto f = m_topics[0];
    ASSERT(f, "expected topic");

    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowSystemMenuHint | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_DeleteOnClose);
    bmargin = ARD_MARGIN * 2;
    m_working_font = QApplication::font("QMenu");
    m_isToDo = f->isToDo();
    m_selected_color_idx = f->colorIndex();

    QSize testSize = ard::calcSize("Completed: 100%");
    m_progress_width = static_cast<int>(testSize.width());
    m_progress_height = static_cast<int>(testSize.height());
    m_button_width_height = m_progress_height;
    m_default_btn_width = m_button_width_height *1.5;
};

void ard::CardPopupMenuBase::showCardMenu(const QPoint& pt)
{
    QPoint pt2 = pt;
    
	auto p = parent();
	if (p) {
		auto pw = dynamic_cast<QWidget*>(p);
		if (pw) {
			pt2 = pw->mapFromGlobal(pt2);

			auto rcDesk = pw->rect();
			auto w = width();
			if (pt2.x() > rcDesk.right() - w) {
				pt2.setX(rcDesk.right() - w);
			}
			if (pt2.y() < rcDesk.top() + 10) {
				pt2.setY(rcDesk.top() + 10);
			}
		}
	}
	move(pt2);
    show();
};

void ard::CardPopupMenuBase::changeEvent(QEvent *e) 
{
    if (e->type() == QEvent::ActivationChange) {
        if (!isActiveWindow()) {
            close();
        }
    }
};

void ard::CardPopupMenuBase::focusOutEvent(QFocusEvent * ) 
{
	close();
};

void ard::CardPopupMenuBase::paintEvent(QPaintEvent *) 
{
    if (topic()) {
        QPainter p(this);		
        QRect rc(0, 0, width() - 1, height() - 1);

        QPen pen(Qt::gray);
        QBrush brush(gui::colorTheme_BkColor());
        p.setBrush(brush);
        p.setPen(pen);
        p.drawRect(rc);

        p.setFont(m_working_font);
        drawMenu(p);
    }
};

void ard::CardPopupMenuBase::closeEvent(QCloseEvent *e) 
{
    for (auto& i : m_topics) {
        i->release();
    }
    m_topics.clear();

    QWidget::closeEvent(e);
};

void ard::CardPopupMenuBase::mousePressEvent(QMouseEvent *e)
{
    /// On operation that would cause losing focus
    /// window will be destroyed
    if (!topic()) {
        ///it can happen on close event
        return;
    }
    processMouseClick(e->pos());
    close();
};

topic_ptr ard::CardPopupMenuBase::topic() 
{
    topic_ptr rv = nullptr;
    if (!m_topics.empty()) 
    {
        rv = m_topics[0];
    }
    return rv;
};


QSize ard::CardPopupMenuBase::recalcColorButtonsControls(int startY)
{   
    int buttons_count = sizeof(m_buttons) / sizeof(int);
    int buttons_area_width = buttons_count * m_button_width_height + (buttons_count - 1) * bmargin;

    m_rcComment = QRect(bmargin, startY, m_default_btn_width, m_button_width_height);
    m_buttons_area_offset = m_rcComment.width() + bmargin + ((m_progress_width - bmargin - m_rcComment.width()) - buttons_area_width) / 2;

    m_rcColorButtons = QRect(bmargin + m_buttons_area_offset, startY, buttons_area_width, m_button_width_height);
    m_rcSlider = QRect(bmargin, startY + m_button_width_height + 2 * bmargin, m_progress_width, m_progress_height);

    m_rcColorButtonsDef = QRect(m_rcSlider.right() + 2 * bmargin, startY, m_default_btn_width, m_button_width_height);
    m_rcSliderDef = m_rcColorButtonsDef.translated(0, 2 * bmargin + m_button_width_height);
    int menu_height = m_rcColorButtons.bottom() - startY;

	if (m_hasLocate) {
		m_rcLocate = m_rcSlider;
		m_rcLocate.translate(0, m_rcSlider.height() + bmargin);
		m_rcFind = m_rcLocate;
		m_rcFind.translate(0, m_rcFind.height() + bmargin);
	}

    if (m_hasProgressSlider) 
    {
        menu_height = m_rcSliderDef.bottom() - startY;
		if (m_hasLocate) {
			menu_height += 2*(m_button_width_height + bmargin);
		}
    }

    QSize rv(m_rcSliderDef.right() + 2 * bmargin, menu_height);

    return rv;
};

void ard::CardPopupMenuBase::drawColorButtons(QPainter& p) 
{
    PGUARD(&p);
    p.setPen(Qt::gray);
    /// color buttons ///
    QRect rc(m_rcColorButtons.left(), m_rcColorButtons.top(), m_button_width_height, m_button_width_height);
    for (auto& idx : m_buttons) {
        QBrush brush(ard::cidx2color(idx), isEnabled() ? Qt::SolidPattern : Qt::CrossPattern);
        p.setBrush(brush);
        p.drawRect(rc);

        if (idx == m_selected_color_idx) {
            PGUARD(&p);
            p.setPen(Qt::black);
            p.drawText(rc, Qt::AlignCenter | Qt::AlignVCenter, "[x]");
        }

        rc.translate(m_button_width_height + bmargin, 0);
    }

    p.setBrush(Qt::white);
    p.drawRect(m_rcColorButtonsDef);
    if (m_selected_color_idx == ard::EColor::none) {
        //PGUARD(&p);
        p.drawText(m_rcColorButtonsDef, Qt::AlignCenter | Qt::AlignVCenter, "[x]");
    }
    if (m_hasProgressSlider)
    {
        p.drawRect(m_rcSliderDef);
        if (!m_isToDo) {
            //PGUARD(&p);
            p.drawText(m_rcSliderDef, Qt::AlignCenter | Qt::AlignVCenter, "[x]");
        }
    }

	if (m_hasLocate) 
	{
		PGUARD(&p);
		p.setRenderHint(QPainter::Antialiasing, true);
		p.setPen(Qt::black);
		QPixmap pm = getIcon_EmailLocate();
		drawPixMap(p, pm, m_rcLocate);
		auto rctext = m_rcLocate;
		rctext.setLeft(rctext.left() + m_rcLocate.height() + 3*bmargin);
		p.drawText(rctext, Qt::AlignLeft | Qt::AlignVCenter, "Locate Email Thread");

		pm = getIcon_Find();
		drawPixMap(p, pm, m_rcFind);
		rctext = m_rcFind;
		rctext.setLeft(rctext.left() + m_rcFind.height() + 3 * bmargin);
		p.drawText(rctext, Qt::AlignLeft | Qt::AlignVCenter, "Find Text");
	}
};

void ard::CardPopupMenuBase::drawSlider(QPainter& p)
{
    PGUARD(&p);
    p.setPen(Qt::gray);

    QColor clBkDef = gui::colorTheme_BkColor();
    QBrush brush(clBkDef, isEnabled() ? Qt::SolidPattern : Qt::CrossPattern);
    p.setBrush(brush);
    p.drawRect(m_rcSlider);

    qreal px_per_val = (qreal)m_rcSlider.width() / 100;

    int draw_flags = Qt::AlignHCenter | Qt::AlignVCenter;
    int done_perc = m_CompletedPercent;
    if (done_perc > 0)
    {
        int xcompl = (int)(px_per_val * done_perc);
        QRect rc_prg = m_rcSlider;
        rc_prg.setRight(rc_prg.left() + xcompl);
        if (rc_prg.right() >= m_rcSlider.right())
            rc_prg.setRight(m_rcSlider.right() - 1);

        QColor cl = color::LightGreen;

        QBrush brush(cl, isEnabled() ? Qt::SolidPattern : Qt::CrossPattern);
        p.setBrush(brush);
        p.drawRect(rc_prg);
    }

    QString s = QString("Completed:%1%").arg(done_perc);

    p.drawText(m_rcSlider, draw_flags, s);

    QPoint pt1 = m_rcSlider.bottomLeft();
    QPoint pt2 = pt1;
    pt2.setY(pt2.y() - 5);

    for (int i = 1; i < 10; i++)
    {
        pt1.setX((int)(m_rcSlider.left() + 10 * i * px_per_val));
        pt2.setX(pt1.x());
        p.drawLine(pt1, pt2);
    }
};

void ard::CardPopupMenuBase::drawPixMap(QPainter& p, const QPixmap& pm, const QRect& rc)
{
    int dx = (m_rcComment.width() - m_rcComment.height()) / 2;
    QRect rcIcon = rc;
    rcIcon.setLeft(rcIcon.left() + dx);
    rcIcon.setWidth(m_rcComment.height());
    p.drawPixmap(rcIcon, pm);
};


ard::CardPopupMenuBase::HitInfo ard::CardPopupMenuBase::hitTestLayoutControls(const QPoint& pt) 
{
    HitInfo h{ hitNone, 0 };

    if (m_rcLayoutCloseAll.contains(pt)) {
        h.hit = hitLayoutCloseAllBtn;
        h.val = 0;
        return h;
    }
	/*
    else if (m_hasTabspaceMenu) 
    {
        if (m_rcLayoutArrangeTabs.contains(pt)) {
            h.hit = hitLayoutArrangeTabsBtn;
            return h;
        }
    }*/

    int idx = 0;
    for (auto& r : m_layout_rects) 
    {
        if (r.contains(pt)) {
            h.hit = hitLayoutBtn;
            h.val = idx;
            return h;
        }
        idx++;
    }

    return h;
};

ard::CardPopupMenuBase::HitInfo ard::CardPopupMenuBase::hitTestColorButtonsControls(const QPoint& pt)
{
    HitInfo h{ hitNone, 0 };

    if (m_rcColorButtonsDef.contains(pt)) {
        h.hit = hitColorDefault;
        return h;
    }
    else if (m_rcColorButtons.contains(pt)) {
        h.hit = hitColorBtn;
        QRect rc(m_rcColorButtons.left(), m_rcColorButtons.top(), m_button_width_height, m_button_width_height);
        for (auto& idx : m_buttons) {
            if (rc.contains(pt)) {
                h.color = idx;
                return h;
            }
            rc.translate(m_button_width_height + bmargin, 0);
        }
        return h;
    }

    if (m_hasProgressSlider)
    {
        if (m_rcSlider.contains(pt)) {
            qreal px_per_val = (qreal)m_rcSlider.width() / 100;
            qreal per_val = 10 + (pt.x() - m_rcSlider.x()) / px_per_val;
            h.val = 10 * (int)(per_val / 10.0);
            h.hit = hitSlider;
            return h;
        }
        else if (m_rcSliderDef.contains(pt)) {
            h.hit = hitSliderDefault;
            return h;
        }
    }

	if (m_hasLocate)
	{
		if (m_rcLocate.contains(pt)) {
			h.hit = hitLocate;
			return h;
		}
		else if (m_rcFind.contains(pt)) {
			h.hit = hitFindText;
			return h;
		}
	}
    return h;
};

#ifdef _DEBUG
QString ard::CardPopupMenuBase::hit2str(ard::CardPopupMenuBase::EHit h)
{
#define CASE_HIT(H) case H:rv = #H;break
    QString rv;
    switch (h) {
        CASE_HIT(hitNone);
        CASE_HIT(hitPinBtn);
        CASE_HIT(hitNewTabBtn);
        CASE_HIT(hitColorBtn);
        CASE_HIT(hitColorDefault);
        CASE_HIT(hitSliderDefault);
        CASE_HIT(hitSlider);
        CASE_HIT(hitMove);
        CASE_HIT(hitMoveFX);
        CASE_HIT(hitLocate);
		CASE_HIT(hitFindText);
        CASE_HIT(hitZoomOut);
        CASE_HIT(hitZoomIn);
        CASE_HIT(hitComment);
        CASE_HIT(hitStar);
        CASE_HIT(hitImportant);
        CASE_HIT(hitLayoutBtn);
        CASE_HIT(hitLayoutCloseAllBtn);
        CASE_HIT(hitLayoutArrangeTabsBtn);
    }
#undef CASE_HIT
    return rv;
};
#endif


bool ard::CardPopupMenuBase::processColorButtonMenuCommand(const ard::CardPopupMenuBase::HitInfo& h) 
{
    auto f = topic();
    assert_return_false(f, "expected topic");
    bool isModified = false;
    bool todo_modified = false;
    bool processed = true;

    switch (h.hit) {
    case hitSlider:
    {
        m_CompletedPercent = h.val;
        isModified = true;
        m_isToDo = true;
        todo_modified = true;
        update();
    }break;
    case hitColorBtn:
    {
        m_selected_color_idx = h.color;
        isModified = true;
        update();
    }break;
    case hitColorDefault:
    {
        m_selected_color_idx = ard::EColor::none;
        isModified = true;
        update();
    }break;
    case hitSliderDefault:
    {
        isModified = true;
        m_isToDo = false;
        todo_modified = true;
        update();
    }break;
    case hitComment:
    {
       // ard::edit_popup_annotation(f);
    }break;
	case hitLocate: 
	{
		gui::ensureVisibleInOutline(f);
	}break;
	case hitFindText: 
	{
		asyncExec(AR_FindText);
		//qDebug() << "hitFindText";
	}break;
    default:
        processed = false; break;
    }

    if (isModified) 
    {
        for (auto& it : m_topics)
        {
            if (m_selected_color_idx != it->colorIndex()) {
                it->setColorIndex(m_selected_color_idx);
            }
            if (todo_modified) {
                if (m_isToDo) {
                    it->setToDo(m_CompletedPercent, ToDoPriority::unknown);
                }
                else {
                    it->setToDo(0, ToDoPriority::notAToDo);
                }
            }

            ard::asyncExec(AR_UpdateGItem, it);
            ard::update_topic_card(it);
        }
    }

    return processed;
};



/**
    SelectorTopicPopupMenu
*/
void ard::SelectorTopicPopupMenu::showSelectorTopicPopupMenu(QPoint pt, TOPICS_LIST lst, bool withLocateOption)
{
    auto pc = new ard::SelectorTopicPopupMenu(lst, withLocateOption);
    pc->showCardMenu(pt);
};

ard::SelectorTopicPopupMenu::SelectorTopicPopupMenu(TOPICS_LIST lst, bool withLocateOption)
    :CardPopupMenuBase(lst)
{
    auto f = m_topics[0];
    ASSERT(f, "expected topic");

    m_hasProgressSlider = true;
	m_hasLocate = withLocateOption;
    m_CompletedPercent = f->getToDoDonePercent();
    //auto menu_size = recalcLayoutControls();
    int startY = bmargin;// menu_size.height() + bmargin;
    auto menu_size = recalcColorButtonsControls(startY);
    menu_size.setHeight(menu_size.height() + 2 * bmargin);
    //menu_size.setHeight(menu_size.height() + btn_size.height() + 6 * bmargin);
    /*if (btn_size.width() > menu_size.width()) {
        menu_size.setWidth(btn_size.width());
    }*/
    setMinimumSize(menu_size);
    setMaximumSize(menu_size);
};

void ard::SelectorTopicPopupMenu::drawMenu(QPainter& p)
{
    drawColorButtons(p);
    QPixmap pm = getIcon_Annotation();
    drawPixMap(p, pm, m_rcComment);
    drawSlider(p);
};

void ard::SelectorTopicPopupMenu::processMouseClick(const QPoint& pt) 
{
    auto h = hitTestColorButtonsControls(pt);
    if (h.hit != hitNone) {
        if (processColorButtonMenuCommand(h)) {
            return;
        }
    }

    if (m_rcComment.contains(pt)) {
		if (m_popup_annotation) {
			auto f = m_topics[0];
			assert_return_void(f, "expected topic");
			ard::popup_annotation(f);
		}
		else {
			ard::edit_selector_annotation();
		}
        return ;
    }
};

#endif //ARD_BIG

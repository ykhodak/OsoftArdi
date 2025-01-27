#include <QHBoxLayout>
#include <QToolBar>
#include <QCheckBox>
#include <QPushButton>
#include <QApplication>
#include <QDesktopWidget>
#include <QTabWidget>
#include <QStylePainter>
#include <QStyle>
#include <QTabBar>
#include <QMenu>
#include <QStyleOptionTab>
#include "MainWindow.h"
#include "workspace.h"
#include "anfolder.h"
#include "fileref.h"
#include "ardmodel.h"
#include "a-db-utils.h"
#include "TabControl.h"
#include "utils-img.h"
#include "EmailPreview.h"
#include "CardPopupMenu.h"
#include "NoteFrameWidget.h"
#include "popup-widgets.h"
#include "BlackBoard.h"
#include "picture.h"
#include "ethread.h"
#include "mail_board_page.h"
#include "folders_board_page.h"
#include "locus_folder.h"

extern double net_traffic_progress;
static ard::workspace* _wspace = nullptr;

/**
workspace_tab_bar
*/
namespace ard
{
	enum class ETabType 
	{
		mail_board,
		selector_board,
		folders_board,
		pinned_topic,
		current_topic
	};

	enum class ETabHit 
	{
		text,
		close_button
	};

	class workspace_tab_bar_gitem;
	class workspace_tab_bar_select_gitem;
	using TAB_GITEMS = std::vector<workspace_tab_bar_gitem*>;

	class workspace_tab_bar : public QWidget
	{
		workspace_tab_bar(ard::workspace* w);

		void			rebuildBar();
		void			setCurrentTab(ard::topic* f);
		ard::topic*		currentTab();

		workspace_tab_bar_gitem* findTab(ard::topic* f);
		void					updateNetTrafficProgressBar();

		ard::workspace*					m_ws{ nullptr };
		ArdGraphicsScene*				m_scene{ nullptr };
		ArdGraphicsView*				m_view{ nullptr };
		QHBoxLayout*					m_context_box{nullptr};
		TAB_GITEMS						m_tabs;
		workspace_tab_bar_select_gitem* m_sel;
		QFont							m_fnt;
		int								m_tab_height;
		friend class workspace;
		friend class workspace_tab_bar_gitem;
		friend class workspace_tab_bar_select_gitem;
		friend void ::update_net_traffic_progress();
	};

	class workspace_tab_bar_gitem : public QGraphicsRectItem 
	{
		workspace_tab_bar_gitem(workspace_tab_bar* bar, ard::topic* f, ETabType ttype, QFont& fnt);
		~workspace_tab_bar_gitem();

		void		resize2context(int h);
		ETabHit		hitTest(QPointF pt);

		void		paint(QPainter * p, const QStyleOptionGraphicsItem *, QWidget *)override;
		void		mousePressEvent(QGraphicsSceneMouseEvent* e)override;

		workspace_tab_bar*	m_tbar;
		ard::topic*			m_topic;
		ETabType			m_ttype;
		bool				m_is_current{ false };
		QFont&				m_fnt;
		QPixmap				m_pxmap;
		friend class workspace_tab_bar;
	};

	class workspace_tab_bar_select_gitem : public QGraphicsRectItem
	{
		workspace_tab_bar_select_gitem(workspace_tab_bar* bar);
		void	paint(QPainter * p, const QStyleOptionGraphicsItem *, QWidget *)override;
		void	updateCurrPos(ard::workspace_tab_bar_gitem* g);
		workspace_tab_bar*	m_tbar;
		QRectF				m_rc_curr;
		friend class workspace_tab_bar;
	};
};

ard::workspace_tab_bar::workspace_tab_bar(ard::workspace* w):m_ws(w)
{
	m_scene = new ArdGraphicsScene;
	m_view = new ArdGraphicsView;
	m_view->setScene(m_scene);
	m_view->setFrameShape(QFrame::NoFrame);
	m_view->setNoScrollBars();
	m_view->setupKineticScroll(ArdGraphicsView::scrollKineticHorizontalOnly);

	m_context_box = new QHBoxLayout();
	utils::setupBoxLayout(m_context_box);
	setLayout(m_context_box);
	m_context_box->addWidget(m_view);

	m_scene->setBackgroundBrush(QBrush(qRgb(223,223,223)));
	m_fnt = QApplication::font();

	QFontMetrics fm(m_fnt);
	auto sz = fm.boundingRect("W");
	m_tab_height = sz.height() + 6 * ARD_MARGIN;
	setMaximumHeight(m_tab_height);
};

ard::workspace_tab_bar_gitem* ard::workspace_tab_bar::findTab(ard::topic* f) 
{
	for (auto& i : m_tabs) {
		if (i->m_topic == f)
			return i;
	}
	return nullptr;
};

void ard::workspace_tab_bar::updateNetTrafficProgressBar()
{
	if (m_sel) {
		m_sel->update();
	}
};


ard::topic*	ard::workspace_tab_bar::currentTab() 
{
	for (auto& i : m_tabs) {
		if (i->m_is_current)
			return i->m_topic;
	}
	return nullptr;
};

void ard::workspace_tab_bar::setCurrentTab(ard::topic* f)
{
	assert_return_void(f, "expected topic");
	auto curr_g = findTab(f);
	if (curr_g) {
		if (curr_g->m_is_current) {
			curr_g->ensureVisible(QRectF(0, 0, 1, 1));
			return;
		}

		curr_g->m_is_current = true;
		curr_g->resize2context(m_tab_height);
		curr_g->update();

		for (auto& i : m_tabs) {
			if (i != curr_g) {
				i->m_is_current = false;
				i->resize2context(m_tab_height);
				i->update();
			}
		}
	}

	QPointF pt(0, 0);
	for (auto& g : m_tabs) {
		g->setPos(pt);
		pt.setX(pt.x() + g->rect().width());
	}

	if (curr_g) {
		m_sel->updateCurrPos(curr_g);
		curr_g->ensureVisible(QRectF(0, 0, 1, 1));
	}
};

void ard::workspace_tab_bar::rebuildBar()
{
	auto f_current = currentTab();
	m_tabs.clear();
	m_scene->clearArdScene();
	m_sel = new workspace_tab_bar_select_gitem(this);
	m_scene->addItem(m_sel);

	std::unordered_set<ard::topic_tab_page*> processed_pages;

	QPointF pt(0, 0);

	std::function<void(ETabType, ard::topic_tab_page*)> add_tab = [&](ETabType tt, ard::topic_tab_page* p)
	{
		auto k = processed_pages.find(p);
		if (k == processed_pages.end()) {
			auto f = p->topic();
			auto g = new workspace_tab_bar_gitem(this, f, tt, m_fnt);
			if (f_current == f) {
				g->m_is_current = true;
			}
			g->resize2context(m_tab_height);
			m_scene->addItem(g);
			g->setPos(pt);
			pt.setX(pt.x() + g->rect().width());
			m_tabs.push_back(g);
			processed_pages.insert(p);
		}
	};

	for (auto& i : m_ws->m_pages) {
		auto mb = dynamic_cast<ard::mail_board_page*>(i);
		if (mb) {
			add_tab(ETabType::mail_board, i);
			break;
		}
	}

	for (auto& i : m_ws->m_pages) {
		auto mb = dynamic_cast<ard::folders_board_page*>(i);
		if (mb) {
			add_tab(ETabType::folders_board, i);
			break;
		}
	}


	for (auto& i : m_ws->m_pages) {
		auto b = dynamic_cast<ard::BlackBoard*>(i);
		if (b)add_tab(ETabType::selector_board, i);
	}

	for (auto& i : m_ws->m_pages) {
		if (i->isSlideLocked())add_tab(ETabType::pinned_topic, i);
	}

	for (auto& i : m_ws->m_pages) {
		add_tab(ETabType::current_topic, i);
	}

	//auto p = m_ws->currentPage();
	//if (p) {
	//	setCurrentTab(p->topic());
	//}
};


/**
	workspace_tab_bar_gitem
*/
ard::workspace_tab_bar_gitem::workspace_tab_bar_gitem(workspace_tab_bar* bar, ard::topic* f, ETabType ttype, QFont& fnt)
	:m_tbar(bar), m_topic(f), m_ttype(ttype), m_fnt(fnt)
{
	LOCK(m_topic);
	ASSERT_VALID(m_topic);
	setZValue(BBOARD_ZVAL_BITEM);

	switch (m_ttype)
	{
	case ETabType::mail_board:		m_pxmap = getIcon_MailBoard();break;
	case ETabType::folders_board:	m_pxmap = getIcon_Pad(); break;
	case ETabType::selector_board:	m_pxmap = getIcon_SelectorBoard();break;
	case ETabType::pinned_topic:	m_pxmap = getIcon_TabPin(); break;
	case ETabType::current_topic:	m_pxmap = getIcon_NotLoadedEmail(); break;
	}

};

ard::workspace_tab_bar_gitem::~workspace_tab_bar_gitem() 
{
	m_topic->release();
};

void ard::workspace_tab_bar_gitem::resize2context(int h) 
{
	QString str = m_topic->impliedTitle();
	if (str.isEmpty()) {
		str = "---------";
	}
	int xdelta = 0;
	auto fnt = m_fnt;
	//if (m_is_current) {
		fnt.setBold(true);
		xdelta = 6*ARD_MARGIN;
	//}
	QFontMetrics fm(fnt);
	auto sz =  fm.boundingRect(str);
	auto w = sz.width();
	if (w > 300) {
		w = 300;
	}	
	int buttons = m_is_current ? 2 : 1;
	QRectF rcb = QRectF(0, 0, w + buttons*(h+ARD_MARGIN) + xdelta, h);
	setRect(rcb);
};


ard::ETabHit ard::workspace_tab_bar_gitem::hitTest(QPointF pt) 
{
	ard::ETabHit rv = ard::ETabHit::text;
	auto rc = rect();
	rc.setLeft(rc.right() - rc.height());
	if (rc.contains(pt) && m_ttype != ETabType::mail_board) {
		rv = ard::ETabHit::close_button;
	}
	return rv;
};


void ard::workspace_tab_bar_gitem::paint(QPainter * p, const QStyleOptionGraphicsItem *, QWidget *) 
{
	QRectF rc = boundingRect();

	QString str = m_topic->impliedTitle();
	if (str.isEmpty()) {
		str = "---------";
	}

	PGUARD(p);	
	p->setRenderHint(QPainter::Antialiasing, true);
	QColor bk = gui::darkSceneBk();
	if (m_is_current)
	{
		auto fnt = m_fnt;
		fnt.setBold(true);
		p->setFont(fnt);
		bk = gui::colorTheme_CardBkColor();
		p->setBrush(bk);
	}
	else
	{
		p->setFont(m_fnt);
		p->setBrush(Qt::NoBrush);
	}
	
	p->setPen(Qt::NoPen);
	p->drawRect(rc);

	QPen pen(Qt::black);
	p->setPen(pen);

	QRectF rcText = rc;
	QRect rc_icon(0,0, rc.height(), rc.height());
	p->drawPixmap(rc_icon, m_pxmap);
	rcText.setLeft(rcText.left() + rc_icon.width() + ARD_MARGIN);
	if (m_is_current && m_ttype != ETabType::mail_board){
		rcText.setRight(rc.right() - (rc_icon.width() + ARD_MARGIN));
	}
	p->drawText(rcText, Qt::AlignLeft | Qt::AlignVCenter, str);

	if (m_is_current && m_ttype != ETabType::mail_board)
	{
		rc_icon.setRect(rc.right() - rc.height(), 0, rc.height(), rc.height());
		p->drawPixmap(rc_icon, getIcon_CloseBtn());
	}	
};


void ard::workspace_tab_bar_gitem::mousePressEvent(QGraphicsSceneMouseEvent* e) 
{
	auto h = hitTest(e->pos());
	if (h == ETabHit::close_button && m_is_current) {
		m_tbar->m_ws->closePage(m_topic);
	}
	else {	
		m_tbar->m_ws->selectPage(m_topic);
	}
};

ard::workspace_tab_bar_select_gitem::workspace_tab_bar_select_gitem(workspace_tab_bar* bar) 
	:m_tbar(bar)
{
	setZValue(BBOARD_ZVAL_RAISED);
};

void ard::workspace_tab_bar_select_gitem::paint(QPainter * p, const QStyleOptionGraphicsItem *, QWidget *) 
{
	static int radius = 10;
	QRectF rc = boundingRect();
	PGUARD(p);
	p->setPen(QPen(Qt::gray));
	p->setBrush(Qt::NoBrush);
	auto y_1 = m_rc_curr.top() + 2;
	auto y_2 = m_rc_curr.bottom() - 2;
	auto x_1 = m_rc_curr.left();
	auto x_2 = m_rc_curr.right();
	p->drawLine(x_1, y_2, x_1, y_1);
	p->drawLine(x_1, y_1, x_2, y_1);
	p->drawLine(x_2, y_1, x_2, y_2);
	p->drawLine(rc.left(), y_2, x_1, y_2);
	p->drawLine(x_2, y_2, rc.right(), y_2);

	if (net_traffic_progress > 0.0)
	{
		PGUARD(p);
//		int pheight = 10;
		auto rc2 = rc;
		//p.setPen(Qt::blue, 2);
		//rc2.setLeft(rc2.left() + 1);
		rc2.setTop(rc2.bottom() - 2);
		//rc2.setWidth(rc2.width() - 2);
		//rc2.setHeight(pheight/* - 3*/);
		//p.setBrush(Qt::NoBrush);
		//p.drawRect(rc2);

		int w = (int)(rc2.width() * net_traffic_progress);
		if (w < 2)w = 2;
		rc2.setWidth(w);
		QBrush b2(qRgb(53, 110, 233));
		p->setBrush(b2);
		p->drawRect(rc2);
	}
};

void ard::workspace_tab_bar_select_gitem::updateCurrPos(ard::workspace_tab_bar_gitem* g)
{
	if (g) {
		auto vp = m_tbar->m_view->viewport();
		if (vp)
		{
			QRect rc = vp->contentsRect();
			QRect rc1(0,0,rc.width(),rc.height());
			setRect(rc1);
		}
		auto rc1 = g->sceneBoundingRect();
		m_rc_curr = rc1;
		update();
	}
};

/**
* workspace
*/
class PopupTabBarButtons : public QWidget
{
public:
	PopupTabBarButtons(ard::workspace* c, const std::vector<ard::TopicWidget::ECommand>& commands) :m_popup_card(c)
	{
		for (auto& c : commands)
		{
			switch (c)
			{
			case ard::TopicWidget::ECommand::add_page:
			{
				button b{ c, QPixmap(":ard/images/unix/add-tab.png"), QRect() };
				m_buttons.push_back(b);
			}break;
			case ard::TopicWidget::ECommand::view_net_traffic:
			{
				button b{ c, QPixmap(), QRect() };
				m_buttons.push_back(b);
			}break;
			case ard::TopicWidget::ECommand::lock_page:
			{
				//button b{ c, QPixmap(":ard/images/unix/arrange-tab.png"), QRect() };
				button b{ c, getIcon_PopupUnlocked(), QRect() };
				m_buttons.push_back(b);
			}break;
			case ard::TopicWidget::ECommand::close:
			{
				button b{ c, QPixmap(":ard/images/unix/close-tab.png"), QRect() };
				m_buttons.push_back(b);
			}break;
			}
		}

		m_bar_height = ARD_TOOLBAR_HEIGHT;

		//setMinimumHeight(m_bar_height);
		//setMaximumHeight(m_bar_height);
		//setMinimumWidth(m_bar_height * m_buttons.size());
		//setMaximumWidth(m_bar_height * m_buttons.size());
	};

	void update_net_traffic_progress()
	{
		update(m_progress_btn_rect);
	}

protected:
	void paintEvent(QPaintEvent *)override
	{
		QPainter p(this);
		p.setRenderHint(QPainter::Antialiasing, true);
		QRect rc = rect();
		auto h = rc.height();
		QRect rcb = rc;
		rcb.setLeft(rcb.right() - h);// m_bar_height);
		auto wh = std::min(rcb.width(), rcb.height());
		rcb.setSize(QSize(wh, wh));
		for (auto i = m_buttons.rbegin(); i != m_buttons.rend(); i++)
		{
			auto& b = *i;
			if (b.command == ard::TopicWidget::ECommand::view_net_traffic) 
			{
				m_progress_btn_rect = rcb;

				if (net_traffic_progress > 0.0)
				{
					PGUARD(&p);
					int pheight = 10;
					auto rc2 = rcb;
					p.setPen(Qt::gray);
					rc2.setLeft(rc2.left() + 1);
					rc2.setTop(rc2.top() + (rc2.height() - pheight) / 2);
					rc2.setWidth(rc2.width() - 2);
					rc2.setHeight(pheight/* - 3*/);
					p.setBrush(Qt::NoBrush);
					p.drawRect(rc2);

					int w = (int)(rc2.width() * net_traffic_progress);
					if (w < 2)w = 2;
					rc2.setWidth(w);
					QBrush b2(qRgb(53, 110, 233));
					p.setBrush(b2);
					p.drawRect(rc2);
				}
				//if (!m_popup_card->hasDetailsToolbarButton())
				//	continue;
			}
			else if (b.command == ard::TopicWidget::ECommand::lock_page) {
				if (m_popup_card->currentPage()) {
					gui::drawArdPixmap(&p,
						m_popup_card->currentPage()->isSlideLocked() ? getIcon_PopupLocked() : getIcon_PopupUnlocked(),
						rcb);
				}
			}
			else {
				gui::drawArdPixmap(&p, b.pixmap, rcb);
				p.setPen(QPen(Qt::gray));
				p.setBrush(Qt::NoBrush);
				auto r2 = rcb.translated(1,1);
				r2.setWidth(r2.width() - 2);
				r2.setHeight(r2.height() - 3);
				//r2.setLeft(r2.left());
				p.drawRect(r2);
			}
			b.rect = rcb;
			rcb.translate(-h, 0);
		}
	}

	void  mousePressEvent(QMouseEvent * e)override
	{
		QWidget::mousePressEvent(e);

		QPoint pt = e->pos();
		QRect rc = rect();
		for (auto i = m_buttons.rbegin(); i != m_buttons.rend(); i++)
		{
			auto b = *i;
			//if (!m_popup_card->hasCommand(b.command))
			//  continue;

			if (b.rect.contains(pt))
			{
				switch (b.command)
				{
				case ard::TopicWidget::ECommand::add_page:
				{
					m_popup_card->new_page();
					return;
				}break;
				
				case ard::TopicWidget::ECommand::lock_page:
				{
					auto pg = m_popup_card->currentPage();
					if (pg) {
						pg->setSlideLocked(!pg->isSlideLocked());
						//m_popup_card->updateCardHeader();
						update();
					}
					return;
				}break;
				case ard::TopicWidget::ECommand::view_net_traffic: 
				{
					///..
				}break;
				case ard::TopicWidget::ECommand::close:
				{
					m_popup_card->hideAndSave();
					return;
				}break;
				default:break;
				}
				//return;
			}///contains            
		}
	};

	struct button
	{
		ard::TopicWidget::ECommand  command;
		QPixmap     pixmap;
		QRect       rect;
	};

	std::vector<button> m_buttons;
	int					m_bar_height{ 0 };
	QRect				m_progress_btn_rect;
	ard::workspace* m_popup_card{ nullptr };
};


ard::workspace::workspace()
{
	m_content_box = new QVBoxLayout();
	utils::setupBoxLayout(m_content_box);
	setLayout(m_content_box);

	setObjectName("cardHolderMain");
	setAccessibleName("Workspace");

	m_bar_buttons = new PopupTabBarButtons(this, {
		//TopicWidget::ECommand::view_net_traffic,
		//TopicWidget::ECommand::lock_page,
		TopicWidget::ECommand::add_page		
	});

	m_wbar = new workspace_tab_bar(this);
	auto hb1 = new QHBoxLayout();
	auto h = m_wbar->maximumHeight();
	m_bar_buttons->setMinimumWidth(h);
	utils::setupBoxLayout(hb1);
	hb1->addWidget(m_wbar);
	hb1->addWidget(m_bar_buttons);
	m_content_box->addLayout(hb1);
	_wspace = this;
};

ard::workspace::~workspace() 
{
	_wspace = nullptr;
};

ard::workspace* ard::wspace()
{
	return _wspace;
}

void update_net_traffic_progress()
{
	if (_wspace && _wspace->m_bar_buttons) {
		_wspace->m_bar_buttons->update_net_traffic_progress();
	}
	if (_wspace && _wspace->m_wbar)
		_wspace->m_wbar->updateNetTrafficProgressBar();
}


void ard::workspace::hideAndSave()
{
	saveModified();
	storeTabs();
	hide();
	gui::rebuildOutline();
	ard::focusOnOutline();
};

void ard::workspace::storeTabs()
{
	QJsonObject js_out;
	auto ts = ard::wspace();
	if (ts) {
		ts->toJson(js_out);
	}

	QJsonDocument doc(js_out);
	QString s(doc.toJson(QJsonDocument::Compact));
	dbp::configStorePopupIDs(s);
};

void ard::workspace::restoreTabs() 
{
	auto tt = dbp::loadPopupTopics();
	if (!tt.tab_topics.empty())
	{
		ard::trail(QString("wspace-cfg-loaded [%1]").arg(tt.tab_topics.size()));
		if (!tt.tab_topics.empty()) 
		{
			for (auto i : tt.tab_topics)
			{
				auto pg = createPage(i.topic);
				if (pg)
				{
					do_addPage(pg);
					if (i.locked_selector) {
						pg->m_slide_locked = true;
					}
				}
			}
		}
		setVisible(true);
	}
	
	auto mb = mailBoard();
	if (!mb) {
		auto d = ard::db();
		if (d && d->isOpen()) {
			auto mb = d->boards_model()->mail_board();
			if (mb) {
				auto pg = createPage(mb);
				if(pg)replacePage(pg);
			}
		}
	}

	if (!m_pages.empty()) {
		auto p = *(m_pages.rbegin());
		if (p->isRegularSlideLocked()) {
			if (p->m_slide_locked)p->m_slide_locked = false;
		}
	}

	m_wbar->rebuildBar();
	if (m_pages.size() > 0) {
		selectPage(m_pages.size()-1);
	}
};

void ard::workspace::new_page()
{
	auto sb = ard::Sortbox();
	if (sb)
	{
		auto f = new ard::topic();
		sb->addItem(f);
		auto pg = createPage(f);
		addPage(pg);
	}
};

void ard::workspace::do_addPage(topic_tab_page* p) 
{
	assert_return_void(p, "expected page");
	if (m_pages.size() > MAX_PAGES_NUMBER) {
		return;
	}
	auto title = p->topic()->impliedTitle();
	if (title.isEmpty()) {
		title = "---------";
	}
	p->m_tabcard = this;

	for (auto& p : m_pages)
	{
		if (p->isEnabled())
		{
			p->setEnabled(false);
			p->setVisible(false);
		}
		p->m_slide_locked = true;
	}

	m_content_box->addWidget(p);
	m_pages.push_back(p);
	rebuildTabStyleMap();
	selectPage(m_pages.size() - 1);
};

void ard::workspace::addPage(topic_tab_page* p)
{
	do_addPage(p);
	
};


void ard::workspace::replacePage(topic_tab_page* p)
{
	if (m_pages.size() == 0)
	{
		addPage(p);
		return;
	}

	auto idx = -1;// m_tbar->currentIndex();
	auto bb = dynamic_cast<ard::BlackBoard*>(p);
	if (bb) {
		auto res = firstBlackboard();
		if (res.first) {
			idx = res.second;
		}
	}

	if (idx == -1) {
		auto res = firstUnlocked();
		if (res.first) {
			idx = res.second;
		}
	}

	if (idx != -1)
	{
		auto title = p->topic()->impliedTitle();
		if (title.isEmpty()) {
			title = "---------";
		}

		m_content_box->addWidget(p);
		ASSERT(m_pages.size() > 0, "expected non empty pages container 0");
		auto old_p = m_pages[idx];
		old_p->saveModified();
		m_content_box->removeWidget(old_p);
		put_page2cache(old_p);
		assert_return_void(!m_pages.empty(), "expected non empty container 1");
		m_pages[idx] = p;
		p->m_tabcard = this;
		rebuildTabStyleMap();
		selectPage(idx);
	}
	else
	{
		addPage(p);
	}
};

void ard::workspace::closePage(int index)
{
	do_close_page(index);
	rebuildTabStyleMap();


	if (m_pages.size() == 0)
	{
		//@todo: ykh - do something else here
		close();
	}
	else 
	{
		auto next_index = index;
		if (next_index >= static_cast<int>(m_pages.size())) {
			next_index--;
		}
		if (next_index < static_cast<int>(m_pages.size()) && next_index > -1) {
			auto p = m_pages[next_index];
			selectPage(p->topic());
			//next_index--;
		}
	}
};

void ard::workspace::detachGui()
{
	while (m_pages.size() > 0)
	{
		do_close_page(0);
	}
	close();
};

//bool ard::workspace::empty()const
//{
//	return (m_pages.size() == 0);
//};

int ard::workspace::count()const
{
	return m_pages.size();
};

void ard::workspace::do_close_page(int index)
{
	auto p = m_pages[index];
	p->detachCardOwner();
	m_pages.erase(m_pages.begin() + index);
	//m_tbar->removeTab(index);
	m_content_box->removeWidget(p);
	put_page2cache(p);
};

void ard::workspace::put_page2cache(topic_tab_page* p)
{
	auto f = p->topic();
	if (f) {
		auto o = f->otype();
		auto i = m_pages_cache.find(o);
		if (i == m_pages_cache.end())
		{
			p->m_slide_locked = false;
			//p->setSlideLocked(false);
			m_pages_cache[o] = p;
			p->detachCardOwner();
			p->hideAnnotationCard();
			p->hide();
			p->setEnabled(false);
		}
		else {
			p->close();
		}
	}
	else {
		p->close();
	}
};

ard::topic_tab_page* ard::workspace::selectPage(int index)
{
	auto p = pageAt(index);
	if (p) 
	{
	//	m_tbar->setCurrentIndex(index);

		auto Max = m_pages.size();
		for (size_t i = 0; i < Max; i++) {
			auto p = m_pages[i];
			if (i == index)
			{
				m_wbar->setCurrentTab(p->topic());
				if (!p->isEnabled())
				{
					p->setEnabled(true);
					p->setVisible(true);
					p->setFocusOnContent();
				}
			}
			else {
				if (p->isEnabled())
				{
					p->setEnabled(false);
					p->setVisible(false);
				}
			}
		}
		if (m_bar_buttons)
			m_bar_buttons->update();
	}
	return p;
};

void ard::workspace::updatePageTab(int idx)
{
	auto p = pageAt(idx);
	if (p)
	{
		p->refreshTopicWidget();
	}
//	qDebug() << "need implementation update_page_tab";
	/*
	assert_return_void(index < m_tbar->count(), "invalid tab index");
	assert_return_void(index < static_cast<int>(m_pages.size()), "invalid tab index");
	auto str = m_tbar->tabText(index);
	auto p = m_pages[index];
	auto f = p->topic();
	assert_return_void(f, "expected topic");
	auto s2 = f->impliedTitle();
	if (s2 != str) {
		m_tbar->setTabText(index, s2);
	}*/
};

ard::topic_tab_page* ard::workspace::pageAt(int idx)
{
	if (idx >= 0 && idx < static_cast<int>(m_pages.size()))
	{
		return m_pages[idx];
	}

	return nullptr;
};

void ard::workspace::rebuildTabStyleMap()
{
	m_wbar->rebuildBar();
};

void ard::workspace::updateCardHeader()
{
	rebuildTabStyleMap();
};

bool ard::workspace::toJson(QJsonObject& js_out)const
{
	QJsonArray jarr;
	for (auto p : m_pages)
	{
		QJsonObject js;
		auto f = p->topic();
		if (!f) {
			ASSERT(f, "expected topic");
			continue;
		}
		auto fp = f->parent();
		if (!fp) {
			auto mb = dynamic_cast<mail_board*>(f);
			auto fb = dynamic_cast<folders_board*>(f);
			if (mb || fb) {
			
			}
			else {
				/// it must be empty topic ///
				continue;
			}
		}
		if (f->isWrapper())
		{
			auto e = dynamic_cast<ard::email*>(f);
			assert_return_false(e, "expected email ptr");
			js["id"] = f->wrappedId();
			if(fp)js["pid"] = fp->wrappedId();
		}
		else
		{
			js["id"] = QString("%1").arg(f->id());
			if (fp)js["pid"] = QString("%1").arg(fp->id());
		}
		auto ot = f->otype();
		js["otype"] = QString("%1").arg(ot);
		js["locked_selector"] = QString("%1").arg(p->isSlideLocked() ? 1 : 0);
		jarr.append(js);
	}

	js_out["wspace"] = jarr;
	//js_out["tab-index"] = QString("%1").arg(m_tbar->currentIndex());

	/*
	auto pt = pos();
	auto sz = frameGeometry().size();
	js_out["ts_l"] = QString("%1").arg(pt.x());
	js_out["ts_t"] = QString("%1").arg(pt.y());
	js_out["ts_w"] = QString("%1").arg(sz.width());
	js_out["ts_h"] = QString("%1").arg(sz.height());
	*/
	return true;
};


template<class O, class P>
ard::topic_tab_page* ard::workspace::pickup_or_createPage(topic_ptr f)
{
	topic_tab_page* pg = nullptr;
	auto b = dynamic_cast<O*>(f);
	assert_return_null(b, "expected board");
	auto i = m_pages_cache.find(f->otype());
	if (i != m_pages_cache.end())
	{
		pg = i->second;
		auto pgb = dynamic_cast<P*>(pg);
		if (pgb)
		{
			m_pages_cache.erase(i);
			pgb->attachTopic(b);
			pgb->setEnabled(true);
			pgb->show();
		}
		else
		{
			pg = new P(b);
		}
	}
	else
	{
		pg = new P(b);
	}
	pg->setupAnnotationCard(false);
	return pg;
}


ard::topic_tab_page* ard::workspace::createPage(topic_ptr f)
{
	topic_tab_page* pg = nullptr;
	auto nt = f->noteViewType();
	switch (nt)
	{
	case ENoteView::SelectorBoard:
	{
		pg = pickup_or_createPage<ard::selector_board, ard::BlackBoard>(f);
	}break;
	case ENoteView::MailBoard:
	{
		pg = pickup_or_createPage<ard::mail_board, ard::mail_board_page>(f);
	}break;
	case ENoteView::FoldersBoard:
	{
		pg = pickup_or_createPage<ard::folders_board, ard::folders_board_page>(f);
	}break;
	case ENoteView::View:
	{
		pg = pickup_or_createPage<ard::email, ard::EmailTabPage>(f);
	}break;
	case ENoteView::Edit:
	{
		pg = pickup_or_createPage<ard::topic, ard::NoteTabPage>(f);
	}break;
	case ENoteView::Picture:
	{
		pg = pickup_or_createPage<ard::picture, ard::PicturePage>(f);
	}break;
	case ENoteView::EditEmail:
	{
		auto e = dynamic_cast<ard::email_draft*>(f);
		assert_return_null(e, "expected email draft");
		pg = new ard::EmailDraftTabPage(e);
	}break;
	case ENoteView::TDAView:
	{
		pg = pickup_or_createPage<ard::fileref, ard::TdaPage>(f);
	}break;
	//default:ASSERT(0, "invalid view type") << static_cast<int>(nt);
	}
	return pg;
};

ard::topic_tab_page* ard::workspace::currentPage()
{
	auto f = m_wbar->currentTab();
	if (f) {
		auto idx = indexOfPage(f);
		if (idx >= 0 && idx < static_cast<int>(m_pages.size())) {
			return m_pages[idx];
		}
	}
	return nullptr;

}

const ard::topic_tab_page* ard::workspace::currentPage()const
{
	auto ThisNonConst = (ard::workspace*)this;
	return ThisNonConst->currentPage();
	/*
	const topic_tab_page* rv = nullptr;
	auto idx = m_tbar->currentIndex();
	if (idx != -1) {
		if (idx >= 0 && idx < static_cast<int>(m_pages.size())) {
			rv = m_pages[idx];
		}
		else {
			ASSERT(0, "invalid page index");
		}
	}
	return rv;
	*/
};

std::vector<ard::BlackBoard*> ard::workspace::allBlackboard()
{
	std::vector<ard::BlackBoard*> rv;
	for (auto& p : m_pages)
	{
		if (p->isEnabled())
		{
			auto bb = dynamic_cast<ard::BlackBoard*>(p);
			if (bb)
			{
				rv.push_back(bb);
			}
		}
	}
	return rv;
};

std::pair<ard::BlackBoard*, int> ard::workspace::firstBlackboard()
{
	std::pair<ard::BlackBoard*, int> rv{nullptr, -1};
	int idx = 0;
	for (auto& p : m_pages)
	{
		//if (p->isEnabled())
		{
			auto bb = dynamic_cast<ard::BlackBoard*>(p);
			if (bb)
			{
				rv.first = bb;
				rv.second = idx;
				return rv;
			}
		}
		idx++;
	}
	return rv;
};

std::pair<ard::topic_tab_page*, int> ard::workspace::firstUnlocked() 
{
	std::pair<ard::topic_tab_page*, int> rv{ nullptr, -1 };
	if (!m_pages.empty())
	{
		int idx = m_pages.size() - 1;
		while (idx > -1) 
		{
			auto p = m_pages[idx];
			if (p->isRegularSlideLocked()) {
				if (!p->isSlideLocked()) {
					rv.first = p;
					rv.second = idx;
					return rv;
				}
			}
			idx--;
		}
	}
	return rv;
};

void ard::workspace::updateBlackboards(topic_ptr f)
{
	auto lst = allBlackboard();
	for (auto i : lst) {
		i->onModifiedOutsideTopic(f);
	}
};

ard::mail_board_page* ard::workspace::mailBoard()
{
	return findPage<ard::mail_board_page>();
};

ard::folders_board_page* ard::workspace::foldersBoard() 
{
	return findPage<ard::folders_board_page>();
};

void ard::rebuildMailBoard(ard::q_param* q)
{
	ard::trail("rebuild-mail-board");
	if (ard::isGmailConnected()) {
		qDebug() << "gmail connected";
	}
	else {
		qDebug() << "gmail NOT connected";
	}
	if (ard::isDbConnected()) 
	{
		auto d = ard::db();
		if (d && d->rmodel()) 
		{
			auto w = ard::wspace();
			if (w) {
				auto mb = w->mailBoard();
				if (mb) {
					if (q) {
						mb->rebuildBoardBand(q);
					}
					else {
						mb->rebuildBoard();
					}
				}
			}
		}
	}
};

void ard::rebuildFoldersBoard(ard::locus_folder* f)
{	
	if (ard::isDbConnected())
	{
		auto d = ard::db();
		if (d && d->rmodel())
		{
			auto w = ard::wspace();
			if (w) {
				auto mb = w->foldersBoard();
				if (mb)
				{
					if (f) {
						ard::trail(QString("rebuild-folders-board [%1]").arg(f->title()));						
						mb->rebuildBoardBand(f);
					}
					else {
						ard::trail("rebuild-folders-board-ALL");
						mb->rebuildBoard();						
					}
				}
			}
		}
	}
};

topic_ptr ard::workspace::topic()
{
	topic_ptr rv = nullptr;
	auto p = currentPage();
	if (p) {
		rv = p->topic();
	}
	return rv;
};

bool ard::workspace::hasDetailsToolbarButton()const
{
	auto p = currentPage();
	if (p) {
		auto f = p->topic();
		if (f)
		{
			auto b = dynamic_cast<const ard::selector_board*>(f);
			if (b) {
				return false;
			}

			return true;
		}
	}

	return false;
};

void ard::workspace::resetAnnotationCardPos()
{
	auto p = currentPage();
	if (p) {
		p->resetAnnotationCardPos();
	}
};

void ard::workspace::saveModified()
{
	for (auto& p : m_pages)
	{
		p->saveModified();
	}
	/*
	auto p = currentPage();
	if (p) {
	p->saveModified();
	}*/
};

void ard::workspace::reloadContent()
{
	for (auto& p : m_pages)
	{
		p->reloadContent();
	}
};

void ard::workspace::locateFilterText()
{
	auto p = currentPage();
	if (p) {
		p->locateFilterText();
	}
};

void ard::workspace::closeTopicWidget(TopicWidget* tp)
{
	auto p = dynamic_cast<topic_tab_page*>(tp);
	assert_return_void(p, "expected tab page");

	auto idx = indexOfPage(p);
	if (idx != -1)
	{
		closePage(idx);
	}
};

void ard::workspace::selectNote(topic_ptr)
{
	ASSERT(0, "NA");
};

void ard::workspace::selectPage(topic_ptr f) 
{
	auto idx = indexOfPage(f);
	if (idx != -1) {
		selectPage(idx);
	}
};

void ard::workspace::setFocusOnContent()
{
	auto pg = currentPage();
	if (pg) {
		pg->setFocusOnContent();
	}
};

void ard::workspace::setupAnnotationCard(bool edit_it)
{
	auto p = currentPage();
	if (p) {
		p->setupAnnotationCard(edit_it);
	}
};

void ard::workspace::zoomView(bool zoom_in)
{
	auto p = currentPage();
	if (p) {
		p->zoomView(zoom_in);
	}
};

void ard::workspace::detachCardOwner()
{
	for (auto& p : m_pages)
	{
		p->detachCardOwner();
	}
};


ard::TopicWidget* ard::workspace::findTopicWidget(ard::topic* t)
{
	for (auto& p : m_pages)
	{
		auto r = p->findTopicWidget(t);
		if (r) {
			return r;
		}
	}
	return nullptr;
};

int ard::workspace::indexOfPage(const topic_tab_page* p)const
{
	int idx = 0;
	for (auto i : m_pages)
	{
		if (i == p)
			return idx;
		idx++;
	}
	return -1;
};

int ard::workspace::indexOfPage(topic_ptr f)const
{
	int idx = 0;
	for (auto i : m_pages)
	{
		if (i->topic() == f)
			return idx;
		idx++;
	}
	return -1;
};

void ard::workspace::closePage(topic_ptr f)
{
	auto idx = indexOfPage(f);
	if (idx != -1)closePage(idx);
};

/**
* topic_tab_page
*/
ard::topic_tab_page::topic_tab_page(QIcon icon) :m_icon(icon)
{
	m_page_context_box = new QVBoxLayout();
	utils::setupBoxLayout(m_page_context_box);
	setLayout(m_page_context_box);
};

ard::topic_tab_page::~topic_tab_page()
{

};


void ard::topic_tab_page::updateCardHeader()
{
	if (m_tabcard) {
		auto idx = m_tabcard->indexOfPage(this);
		if (idx != -1) {
			m_tabcard->updatePageTab(idx);
		}
	}
};

void ard::topic_tab_page::makeActiveInPopupLayout()
{
	if (m_tabcard) {
		m_tabcard->makeActiveInPopupLayout();
	}
};

void ard::topic_tab_page::closeWidget()
{
	auto idx = m_tabcard->indexOfPage(this);
	assert_return_void(idx != -1, "ivalid page index");
	m_tabcard->closePage(idx);
};

void ard::topic_tab_page::setSlideLocked(bool val)
{ 
	m_slide_locked = val;
	m_tabcard->updateCardHeader();
}



#include <QMenu>
#include <QWidgetAction>
#include "mail_board_page.h"
#include "custom-widgets.h"
#include "custom-menus.h"
#include "rule.h"
#include "email.h"
#include "MainWindow.h"
#include "rule_dlg.h"
#include "ard-algo.h"

ard::mail_board_page::mail_board_page(ard::mail_board* b) 
{
	m_view = new ard::board_page_view<ard::mail_board>(this);
	std::vector<EBoardCmd> cmd, rcmd;
	cmd.push_back(EBoardCmd::mark_as_read);
	cmd.push_back(EBoardCmd::comment);
	cmd.push_back(EBoardCmd::remove);
	cmd.push_back(EBoardCmd::colorBtn);
	cmd.push_back(EBoardCmd::slider);	
	
	rcmd.push_back(EBoardCmd::filter_unread);
	constructBoard(b, cmd, rcmd);
};


void ard::mail_board_page::onBandControl(board_band_header<ard::mail_board>*)
{
	//auto bindex = h->bandIndex();

	QMenu m(this);
	ard::setup_menu(&m);
	QAction* a = nullptr;
	ADD_MENU_ACTION("Edit Mail Rules", 1);
	//ADD_MCMD("Edit Rule", 1);
	connect(&m, &QMenu::triggered, [=](QAction* a)
	{
		auto d = a->data().toInt();
		switch (d) 
		{
		case 1: 
		{
			ard::q_param* r2select = nullptr;
			auto bindex = currentBand();
			if (bindex != -1) {
				auto lst = board()->topics();
				if (bindex < static_cast<int>(lst.size())) {
					r2select = lst[bindex];
				}
			}
			ard::rules_dlg::run_it(r2select);
		}break;
		}
	});

	m.exec(QCursor::pos());
};

int ard::mail_board_page::layoutBand(ard::board_band_info* b, ard::q_param* q, int xstart) 
{
	int ystart = 0;

	static uint64_t lbl_unread = googleQt::mail_cache::unread_label_mask();
	auto only_unread = dbp::configFileMailBoardUnreadFilterON();

	q->applyOnVisible([&](ard::ethread* f) 
	{
		auto o = f->optr();
		if (!o)return;
		if (only_unread) {			
			if (!o->hasLabel(lbl_unread)) {
				return;
			}
		}
		auto g = register_topic(f, b);
		if (g) {
			repositionInBand(b, g, xstart, ystart);
			ystart += g->boundingRect().height();
		}
	});
	return ystart;
};


/*void ard::mail_board_page::applyTextFilter() 
{
	auto key = m_filter_edit->text().trimmed();
	bool refilter = true;
	if (m_text_filter->isActive()) {
		auto s2 = m_text_filter->fcontext().key_str;
		if (key == s2) {
			refilter = false;
		}
	}

	if (refilter) 
	{
		clearSelectedGBItems();

		TextFilterContext fc;
		fc.key_str = key;
		fc.include_expanded_notes = false;
		m_text_filter->setSearchContext(fc);
		bool active_filter = !key.isEmpty();

		for(size_t idx = 0; idx < m_band2glst.size(); idx++)
		{
			auto band = m_bb->bandAt(idx);
			assert_return_void(band, "expected band");
			int xstart = band->xstart();
			int ystart = 0;
			auto& lst = m_band2glst[idx];
			for (auto& j : lst) 
			{				
				auto f = j->refTopic();
				if (f)
				{
					if (active_filter)
					{
						if (f->hasText4SearchFilter(fc))
						{
							j->show();
							repositionInBand(band, j, xstart, ystart);
							ystart += j->boundingRect().height();
						}
						else {
							j->hide();
						}
					}
					else 
					{
						j->show();
						j->update();
						repositionInBand(band, j, xstart, ystart);
						ystart += j->boundingRect().height();
					}
				}	
			}
		}
	}

	m_view->verticalScrollBar()->setValue(0);
};*/

ard::mail_board_g_topic* ard::mail_board_page::register_topic(ard::ethread* f, board_band_info* band)
{
	auto rv = new ard::mail_board_g_topic(this, f);
	registerBoardTopic(rv, band);
	return rv;
};


void ard::mail_board_page::removeSelected(bool silently)
{
	auto lst = selectedGBItems();
	if (!lst.empty()) 
	{
		if (!silently) {
			if (!ard::confirmBox(ard::mainWnd(), QString("Please confirm removing %1 email(s).").arg(lst.size()))) {
				return;
			}
		}

		auto lst = selectedRefTopics();
		if (!lst.empty()) {
			//auto l2 = ard::reduce2ethreads(lst.begin(), lst.end());
			auto m = ard::gmail_model();
			if (m)
			{
				ard::killTopicsSilently(lst);
				/*m->trashThreadsSilently(l2);
				for (auto& t : l2) {
					t->killSilently(true);
				}*/
				removeTopicGBItems(lst);
				clearCurrentRelated();
				clearSelectedGBItems();
				m_toolbar->sync2Board();
			}
		}
	}
};

void ard::mail_board_page::markAsRead(bool bval)
{
	ESET lst;
	for (auto& i : m_selected_marks) 
	{
		auto f = i->g()->refTopic();
		auto t = dynamic_cast<ard::ethread*>(f);
		if (t) 
		{			
			auto e = t->headMsg();
			if (e) {
				if (e) lst.insert(e);
				auto l2 = findGBItems(f);
				for (auto j : l2)j->update();
			}
		}
	}

	if (!lst.empty()) 
	{
		auto m = ard::gmail_model();
		if (m)m->markUnread(lst, !bval);
	}
};

#ifdef _DEBUG
void ard::mail_board_page::debugFunction() 
{
	connect(&m_animation_timer, &QTimer::timeout, [=]() 
	{
		for (auto& i : m_selected_marks) {
			auto g = i->g();
			auto p = g->pos();
			if (p.x() > 0) p.setX(p.x() - 10);
			if (p.y() > 0) p.setY(p.y() - 5);
			g->setPos(p);
		}
		//if (m_bb && m_bb->isThumbDirty())make_thumbnail();
	});
	m_animation_timer.start(50);
};
#endif


ard::board_band_header<ard::mail_board>* ard::mail_board_page::produceBandHeader(ard::board_band_info* band) 
{
	ard::q_param* q = nullptr;
	auto bindex = band->bandIndex();
	if (bindex != -1) {
		auto lst = board()->topics();
		if (bindex < static_cast<int>(lst.size())) {
			q = lst[bindex];
			return new mail_board_band_header(this, band, q);
		}
	}
	return nullptr;
};

ard::mail_board_band_header::mail_board_band_header(mail_board_page* bb, ard::board_band_info* bi, ard::q_param* q)
	:board_band_header<ard::mail_board>(bb, bi), m_rule(q)
{
	LOCK(m_rule);
};

ard::mail_board_band_header::~mail_board_band_header() 
{
	m_rule->release();
};

void ard::mail_board_band_header::paint(QPainter * p, const QStyleOptionGraphicsItem *, QWidget *)
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
	
	if (m_rule)
	{
		auto mlbl = m_rule->tabMarkLabel();
		if (!mlbl.isEmpty())
		{
			//QRectF rcmark = rcText;
			//rcmark.setLeft(m_lbl_rect.right());
			//rcmark.setRight(rcText.right() - 10);

			PGUARD(p);
			if (mlbl.size() == 1) {
				mlbl = QString(" %1 ").arg(mlbl);
			}
			
			auto fnt = ard::defaultSmall2Font();
			QFontMetrics fm(*fnt);
			QRect rc2 = fm.boundingRect(mlbl);
//			auto mh = rc2.height();
			auto ydelta = (rcText.height() - rc2.height())/2;

			p->setFont(*fnt);
			QRectF rcmark = rcText;
			rcmark.setLeft(m_lbl_rect.right() + ARD_MARGIN);
			rcmark.setRight(rcmark.left() + rc2.width());
			rcmark.setTop(rcmark.top() + ydelta);
			rcmark.setHeight(rc2.height());
			//QRectF rcText = rc;
			//rcText.setLeft(rcText.right() - rc2.width());
			//rcText.setBottom(rcText.top() + rc2.height() - ydelta);

			QBrush br(color::Olive);
			p->setBrush(br);
			QPen pn(color::Gray_1);
			p->setPen(pn);

			int radius = 5;
			p->drawRoundedRect(rcmark, radius, radius);
			p->drawText(rcmark, Qt::AlignVCenter | Qt::AlignHCenter, mlbl);
		}
		//....
	}
	if (m_is_current) {
		rcText.setRight(rcText.right() - 10);
		rcText.setHeight(10);
		p->drawText(rcText, Qt::AlignRight | Qt::AlignVCenter, "...", &m_ctrl_rect);
	}
	int x = rc.right() - 2;
	p->drawLine(x, 0, x, 10);
};

/**
	mail_board_g_topic
*/
ard::mail_board_g_topic::mail_board_g_topic(mail_board_page* bb, ard::ethread* f):
	ard::board_box_g_topic<ard::mail_board, ard::ethread>(bb, f)
{
};

void ard::mail_board_g_topic::onContextMenu(const QPoint& pt)
{
	QAction* a = nullptr;
	QMenu m;
	ard::setup_menu(&m);
	ADD_TEXT_MENU_ACTION("Mark As 'Read'", [&]() {m_bb->markAsRead(true); });
	ADD_TEXT_MENU_ACTION("Mark As 'UnRead'", [&]() {m_bb->markAsRead(false); });
	ADD_CONTEXT_MENU_IMG_ACTION("target-mark", "Locate", [&]() {m_bb->locateInSelector(); });
	ADD_CONTEXT_MENU_IMG_ACTION("x-trash", "Del", [&]() {m_bb->removeSelected(false); });
	ADD_TEXT_MENU_ACTION("Properties", [&]() {m_bb->showProperties(); });
	m.exec(pt);
};



#include <QMenu.h>
#include <QWidgetAction>
#include "folders_board_page.h"
#include "small_dialogs.h"

ard::folders_board_page::folders_board_page(ard::folders_board* b) 
{
	m_center_items_in_band = false;
	m_view = new ard::board_page_view<ard::folders_board>(this);
	std::vector<EBoardCmd> cmd, rcmd;
	cmd.push_back(EBoardCmd::comment);
	cmd.push_back(EBoardCmd::rename);
	cmd.push_back(EBoardCmd::remove);
	cmd.push_back(EBoardCmd::colorBtn);
	cmd.push_back(EBoardCmd::slider);
	cmd.push_back(EBoardCmd::copy);
	cmd.push_back(EBoardCmd::paste);
	constructBoard(b, cmd, rcmd);
	m_make_thumb_on_rebuild = true;
};


int	ard::folders_board_page::layoutBand(ard::board_band_info* b, ard::locus_folder* t, int xstart)
{
	std::vector<folders_board_g_topic*> parent_gitems;

	int ystart = 0;
	auto lst = t->selectDepth();
	for (auto& dt : lst) 
	{
		auto g = register_topic(dt.topic, b, dt.depth);
		if (g) {
			repositionInBand(b, g, xstart + dt.depth * gui::lineHeight(), ystart);
			ystart += g->boundingRect().height();
			if (!dt.topic->items().empty()) {
				parent_gitems.push_back(g);
			}
		}
	}

	for (auto& g1 : parent_gitems)
	{
		auto& l2 = g1->refTopic()->items();
		if (!l2.empty()) 
		{
			auto f2 = dynamic_cast<ard::topic*>(*l2.rbegin());
			auto glst = findGBItems(f2);
			if (!glst.empty()) 
			{
				auto g2 = *glst.begin();
				auto rc1 = g1->sceneBoundingRect();
				auto rc2 = g2->sceneBoundingRect();
				auto y1 = rc1.bottom();
				auto y2 = (rc2.top() + rc2.bottom()) / 2;
				auto x1 = rc2.left() - gui::lineHeight() / 2.0;
				auto x2 = rc2.left();
				qDebug() << "ctrl-p" << x1 << y1 << y2 << "p=" << g1->refTopic()->title() << "c=" << g2->refTopic()->title();

				auto gl1 = new QGraphicsLineItem(x1, y1, x1, y2);
				gl1->setPen(QPen(Qt::white));
				m_scene->addItem(gl1);

				auto gl2 = new QGraphicsLineItem(x1, y2, x2, y2);
				gl2->setPen(QPen(Qt::white));
				m_scene->addItem(gl2);

				auto i = m_depth2lines.find(g1->bandIndex());
				if (i == m_depth2lines.end())
				{
					LINES lst;
					lst.push_back(gl1);
					lst.push_back(gl2);
					m_depth2lines[g1->bandIndex()] = lst;
				}
				else 
				{
					auto& lst = i->second;
					lst.push_back(gl1);
					lst.push_back(gl2);
				}
			}
		}
	}
	return ystart;
};

ard::folders_board_g_topic* ard::folders_board_page::register_topic(ard::topic* f, board_band_info* band, int depth)
{
	auto rv = new ard::folders_board_g_topic(this, f);
	rv->setDepth(depth);
	registerBoardTopic(rv, band);
	return rv;
};

void ard::folders_board_page::applyTextFilter() 
{	
	//ard::trail(QString("fboard/apply-text-filter [%1][%2]").arg(m_text_filter->isActive()?"Y":"N").arg(m_text_filter->fcontext().key_str));

	auto key = m_filter_edit->text().trimmed();

	if (key.isEmpty()) {
		TextFilterContext fc;
		fc.key_str = "";
		fc.include_expanded_notes = false;
		m_text_filter->setSearchContext(fc);

		rebuildBoard();
	}
	else 
	{
		mapped_board_page<ard::folders_board, ard::locus_folder>::applyTextFilter();
		for (auto& i : m_depth2lines) {
			for (auto& j : i.second) {
				j->hide();
			}
		}
	}
};

void ard::folders_board_page::removeSelected(bool silently)
{
	auto lst = selectedGBItems();
	if (!lst.empty())
	{
		if (!silently) {
			if (!ard::confirmBox(ard::mainWnd(), QString("Please confirm removing %1 topic(s).").arg(lst.size()))) {
				return;
			}
		}

		auto lst = selectedRefTopics();
		if (!lst.empty()) {
			ard::killTopicsSilently(lst);
			/*auto l2 = ard::reduce2ethreads(lst.begin(), lst.end());
			auto m = ard::gmail_model();
			if (m)
			{
				m->trashThreadsSilently(l2);
			}				
			for (auto& t : lst) {
				t->killSilently(true);
			}*/
			removeTopicGBItems(lst);
			clearCurrentRelated();
			clearSelectedGBItems();
			m_toolbar->sync2Board();

			gui::rebuildOutline();
		}
	}

};

void ard::folders_board_page::pasteFromClipboard() 
{
	auto bboard = board();
	assert_return_void(bboard, "expected board");

	const QMimeData *mm = nullptr;
	if (qApp->clipboard()) {
		mm = qApp->clipboard()->mimeData();
	}
	if (!mm)return;
	auto& bls = m_bb->bands();
	auto& tlst = m_bb->topics();
	assert_return_void(tlst.size() == bls.size(), QString("inconsistent columns arr [%1][%2]").arg(tlst.size()).arg(bls.size()));
	assert_return_void(!tlst.empty(), "empty folders board");

	int dest_band_idx = 0;
	ard::topic* destination_parent = nullptr;
	int destination_pos = -1;

	auto g = firstSelected();
	if (g)
	{
		dest_band_idx = g->bandIndex();
		auto f = g->refTopic();
		destination_parent = f->parent();
		assert_return_void(destination_parent, "expected parent of ref topic");
		destination_pos = destination_parent->indexOf(f);
		destination_pos++;
	}
	else 
	{
		dest_band_idx = 0;
		destination_parent = tlst[dest_band_idx];
		destination_pos = 0;
	}

	if (destination_parent)
	{
		assert_return_void(dest_band_idx > 0 && dest_band_idx < static_cast<int>(bls.size()), "invalid band index");
		clearSelectedGBItems();
		int xstart = 0;
		for (int i = 0; i < dest_band_idx; i++)
		{
			auto band = bls[i];
			auto band_width = band->bandWidth();
			xstart += band_width;
		}
		auto folder = tlst[dest_band_idx];
		auto band = bls[dest_band_idx];

		auto lst = ard::insertClipboardData(destination_parent, destination_pos, mm, false);
		if (!lst.empty())
		{
			auto old_num = removeBandGBItems(dest_band_idx);
			auto yband = layoutBand(band, folder, xstart);
			if (yband > m_max_ybottom)
				m_max_ybottom = yband;
			auto new_num = m_band2glst[dest_band_idx].size();
			ard::trail(QString("rebuild-folder-band [%1] [%2][%3]->[%4]").arg(folder->title()).arg(dest_band_idx).arg(old_num).arg(new_num));
			m_max_ybottom += BBOARD_DELTA_EXPAND_HEIGHT;
			alignBandHeights();

			//if (f2)
			//{
				//TOPICS_LIST lst;
				//lst.push_back(f2);
				selectTopics(lst);
			//}
		}
	}
};

std::pair<bool, QString> ard::folders_board_page::renameCurrent() 
{
	std::pair<bool, QString> rv{ false, "Select topic to rename." };
	auto g = firstSelected();
	if (g)
	{
		return renameGItem(g);
	}
	return rv;
};

void ard::folders_board_page::onBandControl(board_band_header<ard::folders_board>* h)
{
	QMenu m(this);
	ard::setup_menu(&m);
	QAction* a = nullptr;
	ADD_MENU_ACTION("Rename folder", 1);
	ADD_MENU_ACTION("Edit folders", 2);
	connect(&m, &QMenu::triggered, [=](QAction* a)
	{
		auto d = a->data().toInt();
		switch (d)
		{
		case 1:
		{
			ard::topic* t2select = nullptr;
			auto bindex = currentBand();
			if (bindex != -1) 
			{
				auto lst = board()->topics();
				if (bindex < static_cast<int>(lst.size())) {
					t2select = lst[bindex];
				}

			}

			if (!t2select->canRenameToAnything()) 
			{
				STRING_SET st = t2select->allowedTitles();
				if (!st.empty()) {
					ard::messageBox(this, QString("Can't rename reserved folder '%1'").arg(t2select->title()));
					return;
				}				
			}

			if (ard::guiEditUFolder(t2select)) {
				auto b = h->band();
				b->setBandLabel(t2select->title());
				if (m_toolbar)m_toolbar->update();
			}
		}break;
		case 2: 
		{
			ard::folders_dlg::showFolders();
		}break;
		}
	});

	m.exec(QCursor::pos());
};


void ard::folders_board_page::clearBoard() 
{
	m_depth2lines.clear();
	m_dnd_mark = nullptr;
	mapped_board_page<ard::folders_board, ard::locus_folder>::clearBoard();
};

int	ard::folders_board_page::removeBandGBItems(int bindex) 
{
	auto i = m_depth2lines.find(bindex);
	if (i != m_depth2lines.end()) 
	{
		for (auto j : i->second)m_scene->removeItem(j);
		m_depth2lines.erase(i);
	}
	return mapped_board_page<ard::folders_board, ard::locus_folder>::removeBandGBItems(bindex);
};


void ard::folders_board_page::onBandDragEnter(board_band<ard::folders_board>*, QGraphicsSceneDragDropEvent *e)
{
	ard::dragEnterEvent(e);
	if (!e->isAccepted())
	{
		onDragDisabled();
	}
};

void ard::folders_board_page::onBandDragMove(board_band<ard::folders_board>* b, QGraphicsSceneDragDropEvent *e)
{
	auto lst = m_bb->topics();
	auto idx = b->band()->bandIndex();
	assert_return_void(idx >= 0 && idx < static_cast<int>(lst.size()), QString("invalid band index [%1]").arg(idx));
	auto f = lst[idx];
	if (ard::dragMoveEvent(f, e))
	{
		auto rc = b->sceneBoundingRect();
		folders_board_page::drag_target dt;
		dt.dest = f;
		dt.pos = f->items().size();
		dt.pt = rc.topLeft();
		dt.sz = QSizeF(rc.width(), gui::lineHeight());

		if (!f->items().empty()) 
		{
			auto f2 = dynamic_cast<ard::topic*>(*f->items().rbegin());
			auto l2 = findGBItems(f2);
			if (!l2.empty()) 
			{
				auto g2 = *l2.rbegin();
				auto bidx2 = g2->bandIndex();
				assert_return_void(idx == bidx2, QString("invalid band index [%1][%2]").arg(idx).arg(bidx2));
				auto rc2 = g2->sceneBoundingRect();
				dt.pt = rc2.bottomLeft();
			}
		}

		onDragMove(dt);
	}
	else {
		onDragDisabled();
	}
};

void ard::folders_board_page::onBandDragDrop(board_band<ard::folders_board>* b, QGraphicsSceneDragDropEvent *e)
{
	auto lst = m_bb->topics();
	auto idx = b->band()->bandIndex();
	assert_return_void(idx >= 0 && idx < static_cast<int>(lst.size()), QString("invalid band index [%1]").arg(idx));
	auto f = lst[idx];
	if (ard::dragMoveEvent(f, e))
	{
		auto rc = b->sceneBoundingRect();
		folders_board_page::drag_target dt;
		dt.dest = f;
		dt.pos = f->items().size();
		dt.pt = rc.topLeft();
		dt.sz = QSizeF(rc.width(), gui::lineHeight());
		onDragDrop(dt, e);
	}
	else {
		onDragDisabled();
	}
};

void ard::folders_board_page::onDragMove(const drag_target& dt)
{
	//qDebug() << QString("drag-move [%1][%2]").arg(dt.dest->title()).arg(dt.pos);

	if (!m_dnd_mark) {
		m_dnd_mark = new dnd_g_mark(this);
		m_scene->addItem(m_dnd_mark);
	}
	else {
		if (!m_dnd_mark->isVisible()) {
			m_dnd_mark->show();
		}
	}

	m_dnd_mark->updateDndPos(dt.pt, dt.sz);
};

void ard::folders_board_page::onDragDrop(const drag_target& dt, QGraphicsSceneDragDropEvent *e)
{
	const QMimeData* md = e->mimeData();
	if (!md)
		return;

	assert_return_void(dt.dest, "expected destination topic");
	assert_return_void(dt.pos != -1, "expected destination index");
	ard::insertClipboardData(dt.dest, dt.pos, md, true);
	gui::rebuildOutline();

	if (m_dnd_mark)m_dnd_mark->hide();
};

void ard::folders_board_page::onDragDisabled() 
{
	if (m_dnd_mark)m_dnd_mark->hide();
};

/**
folders_board_g_topic
*/
ard::folders_board_g_topic::folders_board_g_topic(folders_board_page* bb, ard::topic* f):
	ard::board_box_g_topic<ard::folders_board, ard::topic>(bb, f)
{
	setAcceptDrops(true);
};

ard::folders_board_page* ard::folders_board_g_topic::fboard()
{
	return dynamic_cast<ard::folders_board_page*>(m_bb);
};

void ard::folders_board_g_topic::onContextMenu(const QPoint& pt)
{
	QAction* a = nullptr;
	QMenu m;
	ard::setup_menu(&m);
	//ADD_TEXT_MENU_ACTION("Mark As 'Read'", [&]() {m_bb->markAsRead(true); });
	//ADD_TEXT_MENU_ACTION("Mark As 'UnRead'", [&]() {m_bb->markAsRead(false); });
	ADD_CONTEXT_MENU_IMG_ACTION("target-mark", "Locate", [&]() {m_bb->locateInSelector(); });
	ADD_CONTEXT_MENU_IMG_ACTION("x-trash", "Del", [&]() {m_bb->removeSelected(false); });
	ADD_TEXT_MENU_ACTION("Properties", [&]() {m_bb->showProperties(); });
	m.exec(pt);
};

void ard::folders_board_g_topic::dragEnterEvent(QGraphicsSceneDragDropEvent *e) 
{
	ard::dragEnterEvent(e);
	if (!e->isAccepted()) 
	{
		fboard()->onDragDisabled();
	}
};

void ard::folders_board_g_topic::dropEvent(QGraphicsSceneDragDropEvent *e) 
{
	auto dt = calcDropTarget(e);
	if (dt.dest) {
		fboard()->onDragDrop(dt, e);
	}
	else {
		fboard()->onDragDisabled();
	}
};

void ard::folders_board_g_topic::dragMoveEvent(QGraphicsSceneDragDropEvent *e) 
{
	auto dt = calcDropTarget(e);
	if (dt.dest) {
		fboard()->onDragMove(dt);
	}
	else {
		fboard()->onDragDisabled();
	}
};

ard::folders_board_page::drag_target ard::folders_board_g_topic::calcDropTarget(QGraphicsSceneDragDropEvent *e)
{
	folders_board_page::drag_target dt;

	auto f = refTopic();
	if (!f) {
		ASSERT(0, "expected ref topic");
		return dt;
	}
	//assert_return_void(f, "expected ref topic");
	auto p = f->parent();
	if (!p) {
		ASSERT(0, "expected ref topic parent");
		return dt;
	}
	//assert_return_void(p, "expected ref topic parent");
	auto idx = p->indexOf(f);
	if (idx == -1) 
	{
		ASSERT(0, "invalid ref topic index");
		return dt;
	}
	//assert_return_void(idx != -1, "invalid ref topic index");

	if (ard::dragMoveEvent(p, e))
	{
		auto mouse_pos = e->scenePos();
		auto rc = sceneBoundingRect();
		
		dt.dest = p;
		dt.pt = rc.topLeft();
		dt.sz = QSizeF(rc.width(), gui::lineHeight());

		if (mouse_pos.y() > (rc.top() + rc.height() / 2.0))
		{
			idx++;
			dt.pt.setY(dt.pt.y() + rc.height());
		}

		dt.pos = idx;
	}
	return dt;
};


/**
	dnd_g_mark
*/
ard::dnd_g_mark::dnd_g_mark(folders_board_page* bb) :board_g_mark<ard::folders_board>(bb)
{

};

void ard::dnd_g_mark::paint(QPainter * p, const QStyleOptionGraphicsItem *, QWidget *) 
{
	auto rc = rect();
	rc.setTop(rc.top() + ARD_MARGIN);
	rc.setLeft(rc.left() + ARD_MARGIN);
	rc.setHeight(2 * ARD_MARGIN);
	rc.setWidth(rc.width() - 2 * ARD_MARGIN);
	p->setBrush(Qt::NoBrush);	
	p->setPen(QPen(BBOARD_CURR_MARK_COLOR, 6));
	p->drawRect(rc);
};

void ard::dnd_g_mark::updateDndPos(const QPointF& pt, const QSizeF& sz) 
{
	setRect(QRectF(QPointF(0,0), sz));
	setPos(pt);
};

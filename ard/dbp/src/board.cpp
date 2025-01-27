#include "board.h"
#include "snc-tree.h"
#include <time.h>
#include "ethread.h"
#include "locus_folder.h"

int approximateDefaultHeightInBoardBand(topic_ptr f, int band_width);

bool ard::isBoxlikeShape(ard::BoardItemShape sh) 
{
    bool rv = false;
    switch (sh) {
    case ard::BoardItemShape::box:
    case ard::BoardItemShape::text_normal:
    case ard::BoardItemShape::text_italic:
    case ard::BoardItemShape::text_bold:
        rv = true;
        break;
    default:
        rv = false;
    }
    return rv;
};

bool ard::isTextlikeShape(ard::BoardItemShape sh)
{
    bool rv = false;
    switch (sh) {
    case ard::BoardItemShape::text_normal:
    case ard::BoardItemShape::text_italic:
    case ard::BoardItemShape::text_bold:
        rv = true;
        break;
    default:
        rv = false;
    }
    return rv;
};

IMPLEMENT_ROOT(ard, boards_root, "Boards", ard::selector_board);

ard::boards_model::boards_model(ArdDB* db) 
{
    m_boards_root = new ard::boards_root(db);
	m_mail_board = new ard::mail_board;
	m_folders_board = new ard::folders_board;
};

ard::boards_model::~boards_model() 
{
	if (m_boards_root)m_boards_root->release();
	if (m_mail_board)m_mail_board->release();
	if (m_folders_board)m_folders_board->release();
};

ard::boards_root*		ard::boards_model::boards_root() { return m_boards_root; };
const ard::boards_root* ard::boards_model::boards_root()const { return m_boards_root; };
ard::mail_board*		ard::boards_model::mail_board() { return m_mail_board; };
const ard::mail_board*	ard::boards_model::mail_board()const { return m_mail_board;};
ard::folders_board*		  ard::boards_model::folders_board() { return m_folders_board; };
const ard::folders_board* ard::boards_model::folders_board()const { return m_folders_board; };


QPolygon ard::boards_model::build_shape(BoardItemShape shp, QRect rc) 
{
    QPolygon rv;
    if (rc.isEmpty())
        return rv;

    switch (shp) 
    {
    case BoardItemShape::rombus: 
    {
        int x = (rc.left() + rc.right()) / 2;
        int y = (rc.top() + rc.bottom()) / 2;
        rv  << QPoint(x, rc.top()) 
            << QPoint(rc.right(), y)
            << QPoint(x, rc.bottom())
            << QPoint(rc.left(), y);
    }break;
    case BoardItemShape::triangle: 
    {
        int x = (rc.left() + rc.right()) / 2;
        rv << rc.bottomLeft() << QPoint(x, rc.top()) << rc.bottomRight();
    }break;
    case BoardItemShape::hexagon: 
    {
//      int r = 0.0833 * rc.width();
        //int x = (rc.left() + rc.right()) / 2;
        int x_mid = (rc.left() + rc.right()) / 2;
        int y_mid = (rc.top() + rc.bottom()) / 2;
        int dx = rc.width() * 0.433;
        int dy = rc.width() * 0.25;
        int x_left = x_mid - dx;
        int x_right = x_mid + dx;
        int y_top = y_mid - dy;
        int y_bottom = y_mid + dy;
//      int y1 = rc.top() + rc.height() / 3;
//      int y2 = y1 + rc.height() / 3;
        rv << QPoint(x_mid, rc.top()) << QPoint(x_right, y_top) << QPoint(x_right, y_bottom)
            << QPoint(x_mid, rc.bottom()) << QPoint(x_left, y_bottom) << QPoint(x_left, y_top);
    }break;
    case BoardItemShape::pentagon: 
    {
        int x_mid = (rc.left() + rc.right()) / 2;
        int y_mid = (rc.top() + rc.bottom()) / 2;

        int dy1 = rc.width() * 0.1545;
        int dx1 = rc.width() * 0.475;
        int dy2 = rc.width() * 0.404;
        int dx2 = rc.width() * 0.294;

        int y1 = y_mid - dy1;
        int x1_left = x_mid - dx1;
        int x1_right = x_mid + dx1;
        int y2 = y_mid + dy2;

        int x1 = x_mid - dx2;
        int x2 = x_mid + dx2;
        //int x1 = rc.left() + rc.width() / 3;
        //int x2 = x1 + rc.width() / 3;

        rv << QPoint(x_mid, rc.top()) 
            << QPoint(x1_right, y1)
            << QPoint(x2, y2)
            << QPoint(x1, y2)
            << QPoint(x1_left, y1);

    }break;
    default: break;
    }
    return rv;
};

QRect ard::boards_model::calc_edit_rect(BoardItemShape shp, QRect rc)
{
    QRect rv;
    switch (shp) 
    {
    case BoardItemShape::circle: 
    {
        int x_mid = (rc.left() + rc.right()) / 2;
        int y_mid = (rc.top() + rc.bottom()) / 2;
        int dx = rc.width() * 0.3535;
        int w = 2 * dx;
        rv = QRect(x_mid - dx, y_mid - dx, w, w);
    }break;
    case BoardItemShape::triangle: 
    {
        static double tang_60_time2 = 3.4642;
        int x_mid = (rc.left() + rc.right()) / 2;
        int y_mid = (rc.top() + rc.bottom()) / 2;
        int dx = rc.height() / tang_60_time2;
        int x = x_mid - dx;

        rv = QRect(x, y_mid, 2 * dx, 50);

    }break;
    case BoardItemShape::rombus: 
    {
        int dx = rc.width() / 4;
        int w = 2 * dx;
        rv = QRect(rc.left() + dx, rc.top() + dx, w, w);
    }break;
    case BoardItemShape::pentagon: 
    {
        ///tang(54) = 1.3764
        static double tang_54 = 1.3764;
        static double tang_18 = 0.3249;
        //static double tang_82 = 7.1154;

        int x_mid = (rc.left() + rc.right()) / 2;
        int y_mid = (rc.top() + rc.bottom()) / 2;

//      int dy1 = rc.width() * 0.1545;
        int dx1 = rc.width() * 0.475;
        int dy2 = rc.width() * 0.404;
        int dx2 = rc.width() * 0.294;

//      int y1 = y_mid - dy1;
        int x1_left = x_mid - dx1;
//      int x1_right = x_mid + dx1;
        int y2 = y_mid + dy2;

        int x1 = x_mid - dx2;
//      int x2 = x_mid + dx2;

        int xr_left = (x1 + x1_left) / 2;
        int wr_2 = x_mid - xr_left;
        int wr = (wr_2) * 2;

        int dx_m = x1 - xr_left;// x1_left;
        int dy_m = dx_m / tang_18;      
        int r_bottom = y2 - dy_m;
        int hr_d = wr_2 / tang_54;
        int r_top = rc.top() + hr_d;
        int hr = (r_bottom - r_top);

        rv = QRect(xr_left, r_top, wr, hr);

    }break;
    case BoardItemShape::hexagon:   
    {
        int x_mid = (rc.left() + rc.right()) / 2;
        int y_mid = (rc.top() + rc.bottom()) / 2;
        int dy = rc.width() * 0.25;
        int dx = rc.width() * 0.433;
        int y_top = y_mid - dy;
//      int y_bottom = y_mid + dy;
        int x_left = x_mid - dx;

        rv = QRect(x_left, y_top, 2 * dx, 2 * dy);

        //QPoint(x_left, y_bottom) << QPoint(x_left, y_top);
        //QPoint(x_right, y_top) << QPoint(x_right, y_bottom)
    }break;
    default:break;
    }
    return rv;
};

QString ard::boards_model::shape_name(BoardItemShape shp) 
{
    QString rv;
#define ADD_CASE(N) case ard::BoardItemShape::N: rv = #N;break;
    switch (shp)
    {
        ADD_CASE(unknown)
        ADD_CASE(box)
        ADD_CASE(circle)
        ADD_CASE(triangle)
        ADD_CASE(rombus)
        ADD_CASE(pentagon)
        ADD_CASE(hexagon)
        ADD_CASE(text_normal);
        ADD_CASE(text_italic);
        ADD_CASE(text_bold);
    }
    return rv;
#undef ADD_CASE
};

/**
    boards_root
*/
ard::boards_root::~boards_root() 
{

};

ard::board_link_map* ard::boards_root::lookupLinkMap(QString syid)const
{
    if (!IS_VALID_SYID(syid)) {
        ASSERT(0, "expected valid syid");
        return nullptr;
    }

    ard::board_link_map* rv = nullptr;
    auto i = m_syid2lnk_map.find(syid);
    if (i != m_syid2lnk_map.end()) {
        rv = i->second.get();
    }
    return rv;
};

ard::board_link_map* ard::boards_root::createLinkMap(QString syid)const
{
    if (!IS_VALID_SYID(syid)) {
        ASSERT(0, "expected valid syid");
        return nullptr;
    }

    auto lm = lookupLinkMap(syid);
    if (lm) {
        ASSERT(0, "lmap already exist for syid") << syid;
        return nullptr;
    }

    ard::board_link_map* rv = new board_link_map(syid);
    std::unique_ptr<ard::board_link_map> ml(rv);
    m_syid2lnk_map[syid] = std::move(ml);
    return rv;
};

ard::board_link_map* ard::boards_root::clone_lmap(ard::board_link_map* src, QString clone_board_syid, const SYID2SYID& source2clone)const 
{
    if (!IS_VALID_SYID(clone_board_syid)) {
        ASSERT(0, "expected valid syid");
        return nullptr;
    }

    auto lm = lookupLinkMap(clone_board_syid);
    if (lm) {
        ASSERT(0, "lmap already exist for clone syid") << clone_board_syid;
        return nullptr;
    }

    ard::board_link_map* rv = src->clone_links(clone_board_syid, source2clone);
    if (rv) {
        std::unique_ptr<ard::board_link_map> ml(rv);
        m_syid2lnk_map[clone_board_syid] = std::move(ml);
    }
    return rv;
};

ard::selector_board* ard::boards_root::addBoard(TOPICS_LIST* lst)
{
    /// select some unique name for new board ///
    auto name_index = items().size();
    QString b_title = QString("board %1").arg(name_index);
    int loop_count = 0;
    while (findTopicByTitle(b_title)) {
        loop_count++;
        name_index++;
        b_title = QString("board %1").arg(name_index);
        if (loop_count > 100) {
            ASSERT(0, "name index loop break") << items().size();
            break;
        }
    }

    auto b = new ard::selector_board(b_title);
    b->addBands(7);
    addItem(b);
    ensurePersistant(1);

    if (lst) {
        b->insertTopicsBList(*lst, 0);
    }
    auto e = b->bext();
    if (e) {
        e->clearModified();
    }
    return b;
};

ard::selector_board* ard::boards_root::cloneBoard(selector_board* source)
{
    auto f = source->clone();
    if (!f) {
        return nullptr;
    }

    auto b = dynamic_cast<ard::selector_board*>(f);
    addItem(b);
    return b;
};

void ard::boards_root::loadBoardLinksFromDb(QSqlQuery* q)
{
    board_link_map* curr_lmap{nullptr};
	QString board = "";

	if (q->next()) 
	{
		board = q->value(1).toString();
		curr_lmap = new board_link_map(board);
		std::unique_ptr<board_link_map> p(curr_lmap);
		m_syid2lnk_map[board] = std::move(p);
		curr_lmap->add_link_from_db(q);
	}

    while (q->next())
    {
        board = q->value(1).toString();
        if (curr_lmap->board_syid() != board) 
		{
#ifdef _DEBUG
            auto i = m_syid2lnk_map.find(board);
            if (i != m_syid2lnk_map.end()) {
				ard::error(QString("board-links-db-load logical error, unexpected board in map [%1]").arg(board));
            }
#endif 
            curr_lmap = new board_link_map(board);
            std::unique_ptr<board_link_map> p(curr_lmap);
            m_syid2lnk_map[board] = std::move(p);
        }

		curr_lmap->add_link_from_db(q);
    }

    /// sort by pindex
    for (auto& i : m_syid2lnk_map) {
        auto* lmap = i.second.get();
        for (auto& i : lmap->m_o2targets) {
            for (auto& j : i.second) {
                j.second->sortByPIndex();
            }
        }
    }
};

void ard::boards_root::ensurePersistantAllLinkMaps(ArdDB* db)
{
    for (auto& i : m_syid2lnk_map) {
        i.second->ensurePersistantLinksMap(db);
    }
};

void ard::boards_root::killAllItemsSilently() 
{
    ///copy list
    snc::CITEMS lst = items();
    for (auto i : lst) {
        i->killSilently(false);
    }
};

void ard::boards_root::prepare_link_sync() 
{
	auto& syid2m = allLinkMaps();
	for (auto& i : syid2m) {
		auto& o2t = i.second->o2targets();
		for (auto& j : o2t) 
		{
			for (auto& k : j.second) {
				k.second->prepare_link_sync();
			};
		}
	}
};

snc::CompoundInfo ard::boards_root::compileCompoundInfo()const 
{
	snc::CompoundInfo rv{0,0,""};
	for (auto& i : m_syid2lnk_map) 
	{
		auto& lm = i.second;
		for (auto& i : lm->m_o2targets) 
		{			
			for (auto& j : i.second) 
			{
				auto blst = j.second;
				rv.size++;
				rv.compound_size += blst->size();
				for (size_t i = 0; i < blst->size(); i++) {
					rv.hashStr += blst->getAt(i)->toHashStr();
					rv.hashStr = QCryptographicHash::hash((rv.hashStr.toUtf8()), QCryptographicHash::Md5).toHex();
				}
			}
		}
	}
	return rv;
};

ard::band_board::band_board(QString title) :ard::thumb_topic(title)
{

};

ard::band_board::~band_board() 
{
	clearBands();
};

ard::board_band_info* ard::band_board::bandAt(int idx)const
{
	if (idx < 0 || idx >= static_cast<int>(m_bands.size())) {
		ASSERT(0, "invalid band index") << idx;
		return nullptr;
	}
	return m_bands[idx];
};


bool ard::band_board::setBandWidth(int idx, int w)
{
	bool rv = false;
	auto b = bandAt(idx);
	if (b) {
		rv = b->setBandWidth(w);
		if (rv)markBandsModified();
	}
	return rv;
};

bool ard::band_board::setBandLabel(int idx, QString s)
{
	bool rv = false;
	auto b = bandAt(idx);
	if (b) {
		rv = b->setBandLabel(s);
		if (rv)markBandsModified();
	}

	return rv;
};

void ard::band_board::clearBands() 
{
	for (auto b : m_bands) {
		b->release();
	}
	m_bands.clear();
};

void ard::band_board::resetBandsIdx()
{
	int idx = 0;
	for (auto& b : m_bands) {
		b->m_band_index = idx;
		idx++;
	}
};

QPixmap	ard::band_board::emptyThumb()const
{
	return getIcon_EmptyBoard();
};


/**
    board
*/
ard::selector_board::selector_board(QString title):ard::band_board(title)
{
};


ard::boards_root* ard::selector_board::broot()
{
    auto p = parent();
    if (!p) {
        ASSERT(0, "expected broot");
        return nullptr;
    }
    auto rv = dynamic_cast<ard::boards_root*>(p);
    return rv;
};

const ard::boards_root* ard::selector_board::broot()const
{
    auto p = parent();
    if (!p) {
        ASSERT(0, "expected broot");
        return nullptr;
    }
    auto rv = dynamic_cast<const ard::boards_root*>(p);
    return rv;
};

ard::board_ext* ard::selector_board::bext()
{
    return ensureBExt();
};

const ard::board_ext* ard::selector_board::bext()const
{
    auto ThisNonCost = (ard::selector_board*)this;
    return ThisNonCost->ensureBExt();
};

ard::board_ext* ard::selector_board::ensureBExt()
{
    ASSERT_VALID(this);
    if (m_bext)
        return m_bext;

    ensureExtension(m_bext);
    return m_bext;
};

void ard::selector_board::mapExtensionVar(cit_extension_ptr e)
{
    ard::topic::mapExtensionVar(e);
    if (e->ext_type() == snc::EOBJ_EXT::extBoard) {
        ASSERT(!m_bext, "duplicate board ext");
        m_bext = dynamic_cast<ard::board_ext*>(e);
        ASSERT(m_bext, "expected board ext");
    }
};

bool ard::selector_board::canBeMemberOf(const topic_ptr f)const
{
    ASSERT_VALID(this);
    bool rv = false;
    if (dynamic_cast<const ard::boards_root*>(f) != nullptr) {
        rv = true;
    }
    return rv;
};

bool ard::selector_board::canAcceptChild(cit_cptr it)const
{
    auto f = dynamic_cast<const board_item*>(it);
    if (!f) {
        return false;
    }

    return ard::topic::canAcceptChild(it);
};

bool ard::selector_board::ensurePersistant(int depth, EPersBatchProcess pbatch)
{
    bool res = ard::topic::ensurePersistant(depth, pbatch);
    if (res) {
        if (m_lmap) {
            ArdDB* d = dataDb();
            if (!d) {
                if (!parent()) {
                    ASSERT(0, "board::ensurePersistant - expected valid parent object ") << dbgHint();
                    return false;
                }
                if (!parent()->dataDb()) {
                    ASSERT(0, "board::ensurePersistant - expected valid parent dataDB object") << parent()->dbgHint();
                    return false;
                }

                d = parent()->dataDb();
            }

            m_lmap->ensurePersistantLinksMap(d);
        }
    }
	//setThumbDirty(false);
    return res;
};


bool ard::selector_board::killSilently(bool gui_update)
{
    ASSERT_VALID(this);

    TOPICS_LIST lst;
    lst.push_back(this);
    if (gui_update) {
        ard::close_popup(lst);
    }

    if (!dataDb()){
        if (!m_owner2){
            release();
            return true;
        }
    }

    bool rv = true;
    auto db = dataDb();
    if (db) {
        rv = dbp::removeBoards(db, lst);
    }

    if (rv) {
        for (auto i : items()) {
            auto b = dynamic_cast<board_item*>(i);
            if (b) {
                b->unregister();
            }
        }
        unregister();
    }

    auto t = parent();
    assert_return_false(t, "expected topic");
    parent()->remove_cit(this, false);
    this->release();
    return rv;
};

topic_ptr ard::selector_board::clone()const
{   
    auto db = dataDb();
    assert_return_null(db, "expected DB");

    SYID2SYID sy2sy;

    auto rv = dynamic_cast<ard::selector_board*>(create());
    rv->assignContent(this);
    rv->m_title = "clone of " + rv->m_title;
    auto e = bext();
    if (e) {
        auto e2 = rv->bext();
        if (e2) {
            e2->assignSyncAtomicContent(e);
        }
    }
    rv->guiEnsureSyid(db, "BC");

    for (auto& tp : items()) {
        auto it = dynamic_cast<ard::board_item*>(tp);
        if (it) {
            topic_ptr it_copy = it->clone();
            rv->insert_cit(rv->items().size(), it_copy);

            auto source_syid = it->syid();
            if (!source_syid.isEmpty()) {
                it_copy->guiEnsureSyid(db, "BC");
                sy2sy[source_syid] = it_copy->syid();
            }
        }
    }

    ard::board_link_map* lmap = nullptr;
    if (m_lmap) {
        const boards_root* r = broot();
        assert_return_null(r, "expected broot");
        lmap = r->clone_lmap(m_lmap, rv->syid(), sy2sy);
    }

    IDS_SET invalid_bitems;
    rv->rebuildBoardFromDb(db, lmap, invalid_bitems);

    return rv;
};

void ard::selector_board::markBandsModified()
{
	auto ex = bext();
	if(ex)ex->setModified();
};


void ard::selector_board::postCloneSyncAtomic(cit* source, COUNTER_TYPE mod_cou) 
{
	auto b_src = dynamic_cast<ard::selector_board*>(source);
	assert_return_void(b_src, "expected board source");
	if (!b_src->m_lmap) {
		return;/// not sure how it can be null, maybe no links..
	}
	auto r_src = b_src->broot();
	assert_return_void(r_src, "expected board source root");
	//assert_return_void(b_src->m_lmap, "expected board source map");
	auto p = parent();
	assert_return_void(p, "expected cloned board parent");
	auto r = dynamic_cast<ard::boards_root*>(p);
	assert_return_void(p, "expected cloned board root");


	auto lm_target = r->createLinkMap(source->syid());
	assert_return_void(lm_target, "failed to create cloned board map");
	if (lm_target) {
		for (auto& i : b_src->m_lmap->o2targets()) {
			for (auto& j : i.second) {
				auto lnk_list = j.second;
				for (auto lnk : lnk_list->m_links) {
					auto l2 = lnk->cloneInSync(mod_cou);
					lm_target->addNewBLink(l2);
				}
			}
		}
	}
};

#ifdef _DEBUG
void printBItems(ard::BITEMS& bitems, QString prefix)
{
    qDebug() << "<<< ====" << prefix << bitems.size() << "items";
    for (auto& i : bitems) {
        qDebug() << i->refTopic()->title();
    }
}
#endif

/// if yDelta = -1 we are not changing position
void ard::selector_board::moveToBand(ard::BITEMS& bitems, int bidx, int yPos, int yDelta, qreal total_height)
{
    if (bitems.empty()) {
        return;
    }

    auto top2move = *(bitems.begin());
    auto bottom2move = *(bitems.rbegin());
    BITEMS_SET bitems_set;
    for (auto i : bitems) {
        bitems_set.insert(i);
    }

//#ifdef _DEBUG
//    printBItems(bitems, QString("moving total-h=%1 ypos=%2").arg(total_height).arg(yPos));
//#endif

    auto old_bidx = top2move->bandIndex();
    if (old_bidx < 0 || old_bidx >= static_cast<int>(m_b2items.size())) {
        ASSERT(0, "invalid band index") << old_bidx;
        return;
    }

    for (auto i : bitems) {
        if (i->bandIndex() != old_bidx) {
            ASSERT(0, "expected same band index for moved group") << old_bidx;
            return;
        }
    }

    int old_ypos = top2move->yPos();
    if (old_ypos < -1) {
        ASSERT(0, "invalid band ypos") << old_ypos;
        return;
    }

    if (yPos == -1) {
        //... maybe this is not right
        /// same ypos
        yPos = old_ypos;
    }

    if (total_height <= 0) {
        ASSERT(0, "invalid total height") << total_height << "of" << bitems.size() << "items";
        return;
    }

    if (yDelta < 0) {
        yDelta = 0;
    }

    if (yDelta > BBOARD_MAX_Y_DELTA) {
        yDelta = BBOARD_MAX_Y_DELTA;
    }

    bool was_moved = false;
    bool same_band = (old_bidx == bidx);
    bool same_ypos = (yPos == old_ypos);
    std::vector<int> bands2resetYPos;
    bands2resetYPos.push_back(bidx);
    if (!same_band) {
        bands2resetYPos.push_back(old_bidx);
    }

    if (!same_band || !same_ypos) {     
        /// move out of a band and create a gap
        auto& lst = m_b2items[old_bidx];    
        for (size_t i = 0; i < lst.size(); i++) {
            if (lst[i] == bottom2move) {
                was_moved = true;
                i = i + 1;
                if (i < lst.size()) {
                    /// creatig gap in old place
                    /// we moved item from one position to another - we want to put 'hole' in old position
                    /// so items below would not shift up

                    auto bi_next = lst[i];
                    auto oldYdelta = bi_next->yDelta();
                    auto oldMovedYpos = top2move->yPos();
                    int need_delta = oldMovedYpos + total_height;
                    int newYdelta = oldYdelta + need_delta;
                    if (newYdelta <= 0)
                        newYdelta = 0;

                    qDebug() << "gap-creator next/old-d" << oldYdelta << "next/new-d:" << newYdelta << "this/delta:" << yDelta<<"need_delta:" << need_delta;

                    bi_next->setYDelta(newYdelta);
                }
                break;
            }
        }//gap created

        auto i = lst.begin();
        for (; i != lst.end();) {
            /// remove all selected from band
            auto k = bitems_set.find(*i);
            if (k != bitems_set.end()) {
                i = lst.erase(i);
            }
            else {
                i++;
            }
        }
    }///move-out


    //int resetYPosRequest = -1;
    int appliedYPos = yPos;
    auto& new_lst = m_b2items[bidx];
    if (/*yPos < 0 || */yPos >= static_cast<int>(new_lst.size())) {
        ///.... here is something wrong
        ASSERT(was_moved, "expected moved-bi");
        appliedYPos = new_lst.size();
        new_lst.insert(new_lst.end(), bitems.begin(), bitems.end());
    }
    else 
    {      
        int nextYIndex = yPos;
        if (same_band && same_ypos) {
            nextYIndex++;
            if (nextYIndex < static_cast<int>(new_lst.size())) {
               // qDebug() << "<< same_band_same_ypos";
                auto bi_below = new_lst[nextYIndex];
                auto k = bitems_set.find(bi_below);
                if (k == bitems_set.end()) 
                {
                    ///if next bitem is not mselected we have to adjust it's pos
                    int change_delta = yDelta - top2move->yDelta();
                    auto oldYdelta = bi_below->yDelta();
                    int newYdelta = oldYdelta - change_delta;
                    if (newYdelta < 0)
                        newYdelta = 0;
                    bi_below->setYDelta(newYdelta);
                    qDebug() << "gap-change next/old-d" << oldYdelta << "next/new-d:" << newYdelta << "this/delta:" << yDelta << "change-delta:" << change_delta;
                }
            }
        }else 
        {
            if (nextYIndex < static_cast<int>(new_lst.size())) {
                ///we moved item into new positition, if there are existing items below, they should
                //gap removed from below

                auto bi_below = new_lst[nextYIndex];
                auto oldYdelta = bi_below->yDelta();

                int dont_need_delta = (total_height - top2move->yDelta() + yDelta);
                int newYdelta = oldYdelta - dont_need_delta;
                if (newYdelta < 0)
                    newYdelta = 0;
                bi_below->setYDelta(newYdelta);
                qDebug()<<"gap-removal next/old-d"<<oldYdelta<<"next/new-d:"<<newYdelta<< "this/delta:"<<yDelta<<"change-delta:"<<dont_need_delta;
            }
        }

        appliedYPos = yPos;
        if (was_moved) {
            new_lst.insert(new_lst.begin() + yPos, bitems.begin(), bitems.end());
        }
    }

    for (auto i : bitems) {
        i->setBandIndex(bidx);
    }
    //top2move->setBandIndex(bidx);
    top2move->setYDelta(yDelta);
    //have to reset ypos in all items in band, starting with resetYPosRequest
    if (!bands2resetYPos.empty()) {
        auto db = dataDb();
        if (db) {
            resetYPosInBands(db, bands2resetYPos);
        }
    }
};

ard::board_item* ard::selector_board::getNextBItem(ard::board_item* bi, Next2Item n)
{
    if (!bi) {
        ASSERT(0, "expected bitem");
        return nullptr;
    }

    auto bidx = bi->bandIndex();
    if (bidx < 0 || bidx >= static_cast<int>(m_b2items.size())) {
        ASSERT(0, "invalid band index") << bidx;
        return nullptr;
    }

    int yidx = bi->yPos();
    if (yidx < 0) {
        ASSERT(0, "invalid y-index") << yidx << bi->dbgHint();
        return nullptr;
    }


    ard::board_item* rv = nullptr;
    auto& lst = m_b2items[bidx];
    if (yidx >= static_cast<int>(lst.size())) {
        ASSERT(0, "invalid y-index") << yidx << bi->dbgHint();
        return nullptr;
    }

    switch (n)
    {
    case Next2Item::up:
    {
        if (yidx > 0) {
            yidx--;
            rv = lst[yidx];
        }
    }break;
    case Next2Item::down:
    {
        if (yidx < static_cast<int>(lst.size()) - 1) {
            yidx++;
            rv = lst[yidx];
        }
    }break;
    case Next2Item::left: 
    {
        if (bidx > 0) {
            bidx--;
            auto& lst2 = m_b2items[bidx];
            if (!lst2.empty()) {
                if (yidx >= static_cast<int>(lst2.size())) {
                    yidx = lst2.size() - 1;
                }
                rv = lst2[yidx];
            }
        }
    }break;
    case Next2Item::right:
    {
        if (bidx < static_cast<int>(m_b2items.size()) - 1) {
            bidx++;
            auto& lst2 = m_b2items[bidx];
            if (!lst2.empty()) {
                if (yidx >= static_cast<int>(lst2.size())) {
                    yidx = lst2.size() - 1;
                }
                rv = lst2[yidx];
            }
        }
    }break;
    }

    return rv;
};

ard::board_item* ard::selector_board::getNext2SelectBItem(ard::board_item* bi) 
{
    auto bi_next2select = getNextBItem(bi, Next2Item::down);
    if (!bi_next2select) {
        bi_next2select = getNextBItem(bi, Next2Item::up);
        if (!bi_next2select) {
            bi_next2select = getNextBItem(bi, Next2Item::left);
            if (!bi_next2select) {
                bi_next2select = getNextBItem(bi, Next2Item::right);
            }
        }
    }
    return bi_next2select;
};

ard::board_link* ard::selector_board::getNext2SelectBLink(ard::board_link* lnk)
{
    auto bi_origin = findBitemBySyid(lnk->origin());
    auto bi_target = findBitemBySyid(lnk->target());
    assert_return_null(bi_origin, "expected link origin");
    assert_return_null(bi_target, "expected link target");
    auto link_pindex = lnk->linkPindex();

    ard::board_link* rv = nullptr;
    if (m_lmap) {
        auto i = m_lmap->m_o2targets.find(lnk->origin());
        if (i != m_lmap->m_o2targets.end()) {
            auto j = i->second.find(lnk->target());
            if (j != i->second.end()) {
                auto blinks = j->second;
                if (blinks->size() > 1 && link_pindex <= static_cast<int>(blinks->size() - 2)) {
                    rv = blinks->getAt(link_pindex + 1);
                    return rv;
                }
                else {
                    auto next_index = link_pindex - 1;
                    if (next_index > 0 && next_index < static_cast<int>(blinks->size())) {
                        rv = blinks->getAt(next_index);
                        return rv;
                    }
                }
            }
        }
    }

    return nullptr;
};


std::pair<ard::BITEMS, ard::BITEMS> ard::selector_board::insertTopicsBList(const TOPICS_LIST& lst, int band_idx, int ypos, int ydelta, BoardItemShape shp)
{
    std::pair<BITEMS, BITEMS> rv;
    auto db = dataDb();
    if (!db) {
        ASSERT(0, "expected DB");
        return rv;
    }

    if (m_bands.empty()) {
        addBands(7);
    }

    if (band_idx < 0 || band_idx >= static_cast<int>(m_bands.size())) {
        ASSERT(0, "Invalid band index") << band_idx << m_bands.size();
        band_idx = 0;
    }

    auto& blst = m_b2items[band_idx];
    if (ypos == -1 || ypos > static_cast<int>(blst.size())) {
        ypos = blst.size();
    }

    ///...
    if (ypos < static_cast<int>(blst.size())) {
        auto band = bandAt(band_idx);

        auto bi_below = blst[ypos];
        auto oldYdelta = bi_below->yDelta();        
        int dropped_height = 0;
        for (auto f : lst) {
            dropped_height += approximateDefaultHeightInBoardBand(f, band->bandWidth());
        }
        auto newYdelta = oldYdelta - ydelta - dropped_height;
        if (newYdelta < 0) {
            newYdelta = 0;
        }
        bi_below->setYDelta(newYdelta);
        qDebug() << "insertTopicsBList/gap-removal next/old-d" << oldYdelta << 
            "next/new-d:" << newYdelta << 
            "dropped-h" << dropped_height << 
            "new-delta:" << ydelta <<
            "on" << bi_below->refTopic()->title();
    }
    //...

    bool firstInList = true;

    for (auto& f : lst) {
        //auto f1 = f->shortcutUnderlying();
        auto f2 = f->prepareInjectIntoBBoard();
        if (f2) {
            auto existing_bi = findBItem(f2);
            if (!existing_bi) 
            {
                auto bi = new ard::board_item(f2, shp);
                bi->guiEnsureSyid(db, "B");
                if (ydelta > 0) {
                    if (firstInList) {
                        bi->setYDelta(ydelta);
                        firstInList = false;
                    }
                }
                if (!addItem(bi)) {
                    ASSERT(0, "failed to add board topic item");
                }
                else {
                    rv.first.push_back(bi);
                    blst.insert(blst.begin() + ypos, bi);
                    bi->setBandIndex(band_idx);
                    ypos++;

                    m_syid2bi[bi->syid()] = bi;
                }
            }
            else 
            {
                rv.second.push_back(existing_bi);
            }
        }
    }

    std::vector<int> bands2resetYPos;
    bands2resetYPos.push_back(band_idx);
    resetYPosInBands(db, bands2resetYPos);
    return rv;
};

ard::InsertTopicsListResult ard::selector_board::insertTopicsBListInSeparateColumns(const TOPICS_LIST& lst, int start_band_idx, int step_band_index, BoardItemShape shp)
{
    ard::InsertTopicsListResult rv;
    
    int band_idx = start_band_idx;
    for (auto i : lst) 
    {
        TOPICS_LIST lst1;
        lst1.push_back(i);
        rv.bands.push_back(band_idx);

        while (band_idx >= static_cast<int>(m_bands.size())) {
            addBands(1);
        }

        auto blst1 = insertTopicsBList(lst1, band_idx, -1, 0, shp);
        band_idx += step_band_index;

        rv.new_items.insert(rv.new_items.end(), blst1.first.begin(), blst1.first.end());
        rv.located_items.insert(rv.located_items.end(), blst1.second.begin(), blst1.second.end());
    }

    return rv;
};

ard::BITEMS ard::selector_board::doInsertTopicsWithBranches(board_item* parent_bi,
    const TOPICS_LIST& lst, 
    int band_idx,
    BAND_IDX& idxset, 
    BITEMS& origins,
    bool expand2right,
    BoardItemShape shp)
{
    ard::BITEMS rv;
    if (band_idx > 16) {
        return rv;
    }

    if (expand2right) {
        while (band_idx >= static_cast<int>(m_bands.size())) {
            addBands(1);
        }
    }
    else {
        if (band_idx < 0) {
            insertBand(0);
            band_idx = 0;
            for (auto& i : idxset) {
                i++;
            }
        }
    }

    idxset.push_back(band_idx);

    auto blst1 = insertTopicsBList(lst, band_idx, -1, 0, shp);
    rv.insert(rv.end(), blst1.first.begin(), blst1.first.end());
    if (parent_bi) {
        origins.push_back(parent_bi);
        for (auto& b : blst1.first) {
            addBoardLink(parent_bi, b);
        }
        for (auto& b : blst1.second) {//ykh+ block
            addBoardLink(parent_bi, b);
        }
    }

    auto next_band = band_idx;
    if (expand2right) {
        next_band += 1;
    }
    else {
        next_band -= 1;
    }

    for(auto& i : blst1.first){
        auto top_ref = i->refTopic();
        if (top_ref) {
            TOPICS_LIST sub_topics;
            for (auto& j : top_ref->items()) {
                auto f = dynamic_cast<ard::topic*>(j);
                sub_topics.push_back(f);
            }

            if (sub_topics.size() > 0) {
                auto blst2 = doInsertTopicsWithBranches(i, sub_topics, next_band, idxset, origins, expand2right, shp);
                rv.insert(rv.end(), blst2.begin(), blst2.end());
            }
        }
    }

    return rv;
};

ard::InsertBranchResult ard::selector_board::insertTopicsWithBranches(InsertBranchType insert_type, 
    const TOPICS_LIST& dropped_topics,
    int band_idx, 
    int ypos, 
    int ydelta, 
    BoardItemShape shp, 
    int band_space)
{
    InsertBranchResult rv;
    rv.bands.push_back(band_idx);
    
    std::pair<ard::BITEMS, ard::BITEMS> blst1;
    if (insert_type == ard::InsertBranchType::branch_top_group_expanded_to_right ||
        insert_type == ard::InsertBranchType::branch_top_group_expanded_to_down){
        auto res = insertTopicsBListInSeparateColumns(dropped_topics, band_idx, band_space + 1);
        blst1.first = res.new_items;
        blst1.second = res.located_items;
        rv.bands.insert(rv.bands.begin(), res.bands.begin(), res.bands.end());
    }
    else {
        blst1 = insertTopicsBList(dropped_topics, band_idx, ypos, ydelta, shp);
    }
    rv.bitems.insert(rv.bitems.end(), blst1.first.begin(), blst1.first.end());

    if (insert_type == ard::InsertBranchType::branch_top_group_expanded_to_right || 
        insert_type == ard::InsertBranchType::branch_top_group_expanded_to_down)
    {
        for (auto& i : blst1.first) {
            auto top_ref = i->refTopic();
            if (top_ref)
            {
                auto next_band = i->bandIndex();
                if (insert_type == ard::InsertBranchType::branch_top_group_expanded_to_right) 
                {
                    next_band += 1;
                }

                auto sub_items = top_ref->prepareBBoardInjectSubItems();
                TOPICS_LIST sub_topics;
                for (auto& j : sub_items) {
                    auto f = dynamic_cast<ard::topic*>(j);
                    sub_topics.push_back(f);
                }

                if (sub_topics.size() > 0) {
                    auto blst2 = doInsertTopicsWithBranches(i, sub_topics, next_band, rv.bands, rv.origins, true, shp);
                    rv.bitems.insert(rv.bitems.end(), blst2.begin(), blst2.end());
                }
            }
        }
    }
    else if (insert_type == ard::InsertBranchType::branch_expanded_to_right ||
        insert_type == ard::InsertBranchType::branch_expanded_to_left)
    {
        bool expand2right = (insert_type == ard::InsertBranchType::branch_expanded_to_right);
        auto next_band = band_idx;
        if (expand2right) {
            next_band += 1;
        }
        else {
            next_band -= 1;
        }

        for (auto& i : blst1.first) {
            auto top_ref = i->refTopic();
            if (top_ref) 
            {
                auto sub_items = top_ref->prepareBBoardInjectSubItems();
                TOPICS_LIST sub_topics;
                for (auto& j : sub_items) {
                    auto f = dynamic_cast<ard::topic*>(j);
                    sub_topics.push_back(f);
                }

                if (sub_topics.size() > 0) {
                    auto blst2 = doInsertTopicsWithBranches(i, sub_topics, next_band, rv.bands, rv.origins, expand2right, shp);
                    rv.bitems.insert(rv.bitems.end(), blst2.begin(), blst2.end());
                }
            }
        }
    }
    else if (insert_type == ard::InsertBranchType::branch_expanded_from_center)
    {
        for (auto& i : blst1.first) 
        {
            auto top_ref = i->refTopic();
            if (top_ref) {
                TOPICS_LIST left_sub_topics, right_sub_topics;
                auto sub_items = top_ref->prepareBBoardInjectSubItems();//top_ref->items();
                int Max = sub_items.size();
                int midval = Max / 2;
                int k = 0;
                for (; k < midval; k++) {
                    auto f = dynamic_cast<ard::topic*>(sub_items[k]);
                    left_sub_topics.push_back(f);
                }
                for (; k < Max; k++) {
                    auto f = dynamic_cast<ard::topic*>(sub_items[k]);
                    right_sub_topics.push_back(f);
                }

                if (left_sub_topics.size() > 0) {
                    auto next_band = band_idx - 1;
                    auto blst2 = doInsertTopicsWithBranches(i, left_sub_topics, next_band, rv.bands, rv.origins, false, shp);
                    rv.bitems.insert(rv.bitems.end(), blst2.begin(), blst2.end());
                }

                if (right_sub_topics.size() > 0) {
                    auto next_band = band_idx + 1;
                    auto blst2 = doInsertTopicsWithBranches(i, right_sub_topics, next_band, rv.bands, rv.origins, true, shp);
                    rv.bitems.insert(rv.bitems.end(), blst2.begin(), blst2.end());
                }
            }
        }
    }
    
    return rv;
};

void ard::selector_board::removeBItem(board_item* bi, int yDeltaAdj)
{
    ASSERT_VALID(bi);

    auto bidx = bi->bandIndex();
    if (bidx < 0 || bidx >= static_cast<int>(m_b2items.size())) {
        ASSERT(0, "invalid band index") << bidx;
        return;
    }

    /// remove item from band ///
    int idx_removed = -1;
    auto& lst = m_b2items[bidx];
    for (size_t i = 0; i < lst.size(); i++) {
        if (lst[i] == bi) {
            lst.erase(lst.begin() + i);
            idx_removed = i;
            break;
        }
    }

    /// increase y-delta for items below///
    if (yDeltaAdj > 0) {
        if (idx_removed >= 0 && idx_removed < static_cast<int>(lst.size())) {
            auto bi_below = lst[idx_removed];
            auto oldYdelta = bi_below->yDelta();
            int newYdelta = oldYdelta + yDeltaAdj;
            bi_below->setYDelta(newYdelta);
            qDebug() << "gap-creator next/old-d" << oldYdelta << "next/new-d:" << newYdelta << "adj_delta:" << yDeltaAdj;
            //auto 
        }
        //auto band = bandAt(bidx);
    }

    doRemoveBoardItemAndLinks(bi);

    auto db = dataDb();
    if (db) {
        std::vector<int> bands2resetYPos;
        bands2resetYPos.push_back(bidx);
        resetYPosInBands(db, bands2resetYPos);
    }
};

void ard::selector_board::doRemoveBoardItemAndLinks(board_item* bi)
{
    STRING_LIST origins_syid;
    BITEMS origins_pointing_to_target = getOriginsPointingToTarget(bi);
    for (auto i : origins_pointing_to_target) {
        origins_syid.push_back(i->syid());
    }
    /*
    qDebug() << "<<< removing" << bi->title();
    qDebug() << bi->syid();
    qDebug() << "----------";
    for (auto i : origins_syid) {
        qDebug() << i;
    }*/

    /// remove links that point to us ///   
    for (auto& o : origins_pointing_to_target) {
        auto i = m_o2t_adj.find(o);
        if (i != m_o2t_adj.end()) {
            auto j = i->second.find(bi);
            if (j != i->second.end()) {
                i->second.erase(j);
            }
        }
    }

    auto k = m_t2o_adj.find(bi);
    if (k != m_t2o_adj.end()) {
        m_t2o_adj.erase(k);
    }

    auto i = m_o2t_adj.find(bi);
    if (i != m_o2t_adj.end()) {
        m_o2t_adj.erase(i);
    }

    if (m_lmap) {
        m_lmap->removeMapLinks(origins_syid, bi->syid());
    }

    auto db = dataDb();
    if (db) {
        if (m_lmap) {
            m_lmap->ensurePersistantLinksMap(db);
        }
    }

    ///remove from syid map
    auto kk = m_syid2bi.find(bi->syid());
    if (kk != m_syid2bi.end()) {
        m_syid2bi.erase(kk);
    }

    bi->killSilently(false);
};

ard::board_item* ard::selector_board::findBItem(topic_ptr f)
{
    board_item* rv = nullptr;
    for (auto& lst : m_b2items) {
        for (auto& b : lst) {
            if (b->refTopic() == f) {
                return b;
            }
        }
    }
    return rv;
};

void ard::selector_board::addBands(int num)
{
	for (int i = 0; i < num; i++) {
		BITEMS v1;
		m_b2items.push_back(v1);
		m_bands.push_back(new board_band_info());
	}

	auto ex = bext();
	ex->setModified();
	resetBandsIdx();
};

void ard::selector_board::insertBand(int idx) 
{
//    qDebug() << "<<..insert band at" << idx << m_bands.size();

    if (idx < 0 || idx > static_cast<int>(m_bands.size())) {
        ASSERT(0, "invalid band index") << idx;
        return;
    }

    std::vector<int> band_idx_list;
    for (int i = idx; i < static_cast<int>(m_bands.size()); i++) {
        band_idx_list.push_back(i);
    }

    updateBIndexInBandRange(band_idx_list, 1);

    BITEMS v1;
    m_b2items.insert(m_b2items.begin() + idx, v1);
    m_bands.insert(m_bands.begin() + idx, new board_band_info());
    auto ex = bext();
    ex->setModified();
    resetBandsIdx();
};

void ard::selector_board::removeBandAt(int idx) 
{
    if (idx < 0 || idx >= static_cast<int>(m_bands.size())) {
        ASSERT(0, "invalid band index") << idx;
        return;
    }

    /// remove all items inside with all related links ///
    auto& lst = m_b2items[idx];
    if (!lst.empty()) {
        for (auto b : lst) {
            doRemoveBoardItemAndLinks(b);
        }
    }

    m_b2items.erase(m_b2items.begin() + idx);

    auto bi = m_bands[idx];
    bi->release();
    m_bands.erase(m_bands.begin() + idx);

    auto ex = bext();
    ex->setModified();

    std::vector<int> band_idx_list;
    for (int i = idx; i < static_cast<int>(m_bands.size()); i++) {
        band_idx_list.push_back(i);
    }

    updateBIndexInBandRange(band_idx_list, -1);
    resetBandsIdx();
};


ard::BITEMS ard::selector_board::getItemsInBandRange(const std::vector<int>& band_idx) 
{
    ard::BITEMS rv;
    for (auto i : band_idx) {
        if (i >= 0 && i < static_cast<int>(m_b2items.size())) {
            auto& lst = m_b2items[i];
            rv.insert(rv.end(), lst.begin(), lst.end());
        }
    }
    return rv;
};

void ard::selector_board::updateBIndexInBandRange(const std::vector<int>& band_idx, int idx_delta) 
{
    std::vector<ard::board_item_ext*> updated_bext;
    ArdDB* d = dataDb();
    if (d) {
        auto blist = getItemsInBandRange(band_idx);
        if (!blist.empty()) {
//            qDebug() << "<<< updateBIndexInBandRange total" << blist.size() << "in" << band_idx.size() << "bands idx_delta=" << idx_delta;
            for (auto bi : blist) {
//                qDebug() << "updateBIndexInBandRange" << bi->id() << bi->syid();
                auto e = bi->biext();
                if (e) {
                    if (e->m_band_index >= 0) {
                        if (IS_VALID_DB_ID(e->owner()->id())) {
                            e->setSyncModified();
                        }
                        e->m_band_index += idx_delta;
                        updated_bext.push_back(e);
                    }
                }
            }
            dbp::updateBItemsBIndex(d, updated_bext);
        }
    }
	setThumbDirty();
};



void ard::selector_board::bandsToJson(QJsonObject& js)const
{
    QJsonArray jarr;
    for (auto b : m_bands) {
        QJsonObject js2;
        b->toJson(js2);
        jarr.append(js2);
    }
    js["bands"] = jarr;
};

void ard::selector_board::bandsFromJson(QJsonObject& js)
{
    auto arr = js["bands"].toArray();
    int Max = arr.size();
    for (int i = 0; i < Max; ++i) {
        auto js2 = arr[i].toObject();
        auto b = new board_band_info();
        b->fromJson(js2);
        m_bands.push_back(b);
    }
    resetBandsIdx();
};

ard::board_item* ard::selector_board::findBitemBySyid(QString syid)
{
    ard::board_item* rv = nullptr;
    auto i = m_syid2bi.find(syid);
    if (i != m_syid2bi.end()) {
        rv = i->second;
    }
    return rv;
};

topic_ptr ard::selector_board::ensureOutlineTopicsHolder()
{
    topic_ptr rv = nullptr;
    QString stitle = title().trimmed() + " " + "topics";

    auto hr = ard::BoardTopicsHolder();//ard::Sortbox();
    assert_return_null(hr, "expected holders root");
    if (hr) {
        rv = hr->findTopicByTitle(stitle);
    }

    if (!rv) {
        rv = new ard::topic(stitle);
        hr->addItem(rv);
        hr->ensurePersistant(-1);
    }

    return rv;
};

void ard::selector_board::register_link_list(ard::board_link_list* link_list, ard::board_item* origin, ard::board_item* target)
{
    bool o2t = true;
    bool t2o = true;

    if (o2t) {
        auto k = m_o2t_adj.find(origin);
        if (k == m_o2t_adj.end()) {
            O2LINKS t2links;
            t2links[target] = link_list;
            m_o2t_adj[origin] = t2links;
        }
        else {
            auto& tmap = k->second;
            auto m = tmap.find(target);
            if (m == tmap.end()) {
                k->second[target] = link_list;
            }
        }
    }

    if (t2o) {
        auto k = m_t2o_adj.find(target);
        if (k == m_t2o_adj.end()) {
            O2LINKS t2links;
            t2links[origin] = link_list;
            m_t2o_adj[target] = t2links;
        }
        else {
            auto m = k->second.find(origin);
            if (m == k->second.end()) {
                k->second[origin] = link_list;
            }
        }
    }
};

void ard::selector_board::reset_lrpos_link_indexes(ard::board_item* origin) 
{
    auto k = m_o2t_adj.find(origin);
    if (k != m_o2t_adj.end()) {
        auto& tmap = k->second;
        int rpos_count = 0;
        int lpos_count = 0;

        for (auto i : tmap) {
            auto t = i.first;
            auto link_list = i.second;
            if (t->bandIndex() > origin->bandIndex()) {
                rpos_count++;
            }
            if (t->bandIndex() < origin->bandIndex()) {
                lpos_count++;
            }
            link_list->m_rpos_index = rpos_count;
            link_list->m_lpos_index = lpos_count;
        }
    }
};

ard::BITEMS ard::selector_board::getOriginsPointingToTarget(ard::board_item* target)
{
    ard::BITEMS rv;
    auto i = m_t2o_adj.find(target);
    if (i != m_t2o_adj.end()) {
        for (auto j : i->second) {
            rv.push_back(j.first);
        }
    }
    return rv;
};

void ard::selector_board::rebuildBoardFromDb(const ArdDB* db, ard::board_link_map* lmap, IDS_SET& invalid_bitems)
{
    auto ex = bext();
    assert_return_void(ex, "expected board ext");
    QJsonDocument doc = QJsonDocument::fromJson(ex->m_bands_payload);
    QJsonObject js = doc.object();
    bandsFromJson(js);

    if (m_bands.empty()) {
        addBands(7);
    }

    m_b2items.clear();
    m_syid2bi.clear();
    m_b2items.resize(m_bands.size());   
    m_o2t_adj.clear();
    m_t2o_adj.clear();

    BITEMS bitems2clear;

    for (auto& i : m_items) {
        auto bi = dynamic_cast<board_item*>(i);
        if (bi) {
            auto res = bi->resolveRefTopic(db);
            ///have to mark as 'deleted' if link is unresolved
            if (res) {
                /*
                auto th = dynamic_cast<ard::ethread*>(res);
                if (th) {
                    auto e = th->getThreadExt();
                    if (e) {
                        qDebug() << "thread-in-board" << e->threadId() << th->id() << th->syid() << th;
                    }
                }*/

                auto bidx = bi->bandIndex();
                if (bidx < 0 || bidx >= static_cast<int>(m_bands.size())) {
                    ASSERT(0, "invalid band index");
                    bidx = 0;
                }
                BITEMS& items = m_b2items[bidx];
                items.push_back(bi);
                
                auto syid = bi->syid();
                if (!syid.isEmpty()) {
                    m_syid2bi[syid] = bi;
                }
            }
            else {
                bi->markStatus(BoardItemStatus::removed);
                bitems2clear.push_back(bi);
                invalid_bitems.insert(bi->id());
            }
        }
    }

    for (auto bi : bitems2clear) {
        qWarning() << "invalid bitem scheduled to delete" << bi->id();
        bi->unregister();
        remove_cit(bi, true);       
    }

    m_lmap = lmap;
    if (m_lmap) {
        ///rebuild adj from m_lmap
        ///it's a good place to clean up lmap also  
        for (auto& i : m_lmap->m_o2targets) {
            auto origin = i.first;
            auto bi_origin = findBitemBySyid(origin);
            if (bi_origin) {
                for (auto& j : i.second) {
                    auto link_list = j.second;
                    auto target = j.first;
                    auto bi_target = findBitemBySyid(target);
                    if (bi_target) {
                        register_link_list(link_list, bi_origin, bi_target);
                    }
                }
            }
        }
        /// reset order of left&right //
        for (auto i : m_o2t_adj) {
            reset_lrpos_link_indexes(i.first);
        }
    }

    /// reset y-pos based item
    for (auto& lst : m_b2items) {
        std::sort(lst.begin(), lst.end(), [](board_item* b1, board_item* b2) 
        {
            return (b1->yPos() < b2->yPos());
        });
    }
};

void ard::selector_board::resetYPosInBands(ArdDB* db, const std::vector<int>& bands_idx)
{
#ifdef _DEBUG
    std::unordered_map<ard::board_item*, int> items_in_board;
#endif 

    snc::COUNTER_TYPE mdc = 0;
    if (syncDb()) {
        mdc = syncDb()->db_mod_counter() + 1;
    }

    std::vector<board_item_ext*> updated_item_ext;
    for (auto idx : bands_idx){
        auto& lst = m_b2items[idx];
        int ypos = 0;
        for (auto& i : lst) {
#ifdef _DEBUG
            auto k = items_in_board.find(i);
            if (k != items_in_board.end()) {
                ASSERT(0, "duplicate b-item in board") << idx << k->second;
            }
            items_in_board[i] = idx;
#endif

            auto e = i->biext();
            if (e) {
                if (e->yPos() != ypos) {
                    e->m_y_pos = ypos;
                    if (mdc != 0) {
                        e->m_mod_counter = mdc;
                    }
                    updated_item_ext.push_back(e);
                }
            }
            ypos++;
        }
    }
    
    if (updated_item_ext.size() > 0) {
        //qDebug() << "<<<<< resetYPosInBands item-ex" << updated_item_ext.size();
        dbp::updateBItemsYPos(db, this, updated_item_ext);
    }
};

void ard::selector_board::resetLinksPindex(ArdDB* db)
{
    snc::COUNTER_TYPE mdc = 0;
    if (syncDb()) {
        mdc = syncDb()->db_mod_counter() + 1;
    }

    if (m_lmap) 
    {
        std::vector<board_link*> updated_links;
        for (auto& i : m_lmap->m_o2targets) {
            for (auto& j : i.second) {
                auto blinks = j.second;
                int pindex = 0;
                for (int k = 0; k < static_cast<int>(blinks->size()); k++) {
                    auto lnk = blinks->getAt(k);
                    if (lnk) {
                        if (pindex != lnk->linkPindex()) {
                            lnk->setLinkPindex(pindex);
                            updated_links.push_back(lnk);
                        }
                        pindex++;
                    }
                }
            }
        }

        if (!updated_links.empty()) {
            //qDebug() << "updated_links" << updated_links.size();
            dbp::updateBLinksPIndex(db, updated_links);
        }
    }
};

/*
void ard::selector_board::resetLinksPindex4Target(ArdDB* db, ard::board_item* origin, ard::board_item* target) 
{
    if (m_lmap)
    {
        std::vector<board_link*> updated_links;
        auto i = m_lmap->m_o2targets.find(origin->syid());
        if (i != m_lmap->m_o2targets.end()) {
            auto j = i->second.find(target->syid());
            if (j != i->second.end()) {
                auto blinks = j->second;
                auto u2 = blinks->resetPIndex();
                if (!u2.empty()) {
                    updated_links.insert(updated_links.end(), u2.begin(), u2.end());
                }
            }
        }
        if (!updated_links.empty()) {
            qDebug() << "updated_links" << updated_links.size();
            dbp::updateBLinksPIndex(db, updated_links);
        }
    }
};*/

ard::board_link_list* ard::selector_board::addBoardLink(board_item* origin, board_item* target)
{
    auto db = dataDb();
    assert_return_null(db, "expected DB");
    cdb* sync_db = db->syncDb();
    assert_return_null(sync_db, "expected SyncDB");

    if (!IS_VALID_SYID(syid())) {
        guiEnsureSyid(db, "B");
    }

    if (!IS_VALID_SYID(origin->syid())) {
        origin->guiEnsureSyid(db, "B");
    }

    if (!IS_VALID_SYID(target->syid())) {
        target->guiEnsureSyid(db, "B");
    }

    if (!m_lmap) {
        auto r = broot();
        assert_return_null(r, "expected broot");
        m_lmap = r->lookupLinkMap(syid());
        if (!m_lmap) {
            m_lmap = r->createLinkMap(syid());
            assert_return_null(m_lmap, "expected board lmap");
        }
    }

    ard::board_link* link = new ard::board_link(origin->syid(), target->syid(), 0, ""/*, db_mod_counter + 1*/);
    auto links = m_lmap->addNewBLink(link);
    if (!links) {
        ASSERT(0, "failed to register entry in mlink");
        return nullptr;
    }

    std::function<QString()> generate_link_syid = []() 
    {
		static uint32_t register_num = 0;
		static uint64_t  id_last_generated_id = 0;
        QString rv;
        static char buff[64] = "";
        unsigned int v2 = time(NULL) - SYNC_EPOCH_TIME;

		if (id_last_generated_id == v2)
		{
			register_num++;
		}
		else
		{
			register_num = 0;
		}

		id_last_generated_id = v2;
		if (register_num == 0)
		{
#ifdef Q_OS_WIN32
			sprintf_s(buff, sizeof(buff), "%X", v2);
#else
			sprintf(buff, "%X", v2);
#endif
		}
		else 
		{
			unsigned int v3 = register_num;
#ifdef Q_OS_WIN32
			sprintf_s(buff, sizeof(buff), "%X-%d", v2, v3);
#else
			sprintf(buff, "%X-%d", v2, v3);
#endif		
		}
        rv = buff;
        return rv;
    };

    if (!IS_VALID_SYID(link->link_syid())) 
	{
        auto used_syids = m_lmap->selectUsedSyid();
        auto rdb = db->getSyncCounters4NewRegistration();
        auto syid = generate_link_syid();
        auto i = used_syids.find(syid);
        if (i != used_syids.end()) 
		{
            int idx = 1;
            while (i != used_syids.end()) 
			{
				if (idx > 5) 
				{
					qWarning() << "duplicate syid on link generation" << syid << idx;
				}
                syid = generate_link_syid() + QString("D%1").arg(idx);
                i = used_syids.find(syid);
                idx++;

				assert_return_null(idx > 1000, QString("exhausted attempts to generate unique linkId [%1][%2]").arg(idx).arg(syid));
            }
        }

        link->m_link_syid = syid;
        link->m_mod_counter = rdb.second;
    }

    register_link_list(links, origin, target);
    reset_lrpos_link_indexes(origin);
    auto updated_links = links->resetPIndex();
    if (!updated_links.empty()) {
        //qDebug() << "updated_links" << updated_links.size();
        dbp::updateBLinksPIndex(db, updated_links);
    }

	setThumbDirty();

    return links;
};


void ard::selector_board::removeBoardLink(ard::board_link* lnk)
{
    auto db = dataDb();

    auto bi_origin = findBitemBySyid(lnk->origin());
    auto bi_target = findBitemBySyid(lnk->target());

    /// remove from adj list - we can operate only on o->t adjacent list
    if (bi_origin && bi_target) {
        auto i = m_o2t_adj.find(bi_origin);
        if (i != m_o2t_adj.end()) {
            auto k = i->second.find(bi_target);
            if (k != i->second.end()) {
                auto lst = k->second;
                std::unordered_set<ard::board_link*> links2remove;
                links2remove.insert(lnk);
                lst->removeBLinks(links2remove);

                auto updated_links = lst->resetPIndex();
                if (!updated_links.empty()) {
                    qDebug() << "updated_links" << updated_links.size();
                    if (db) {
                        dbp::updateBLinksPIndex(db, updated_links);
                    }
                }
            }
        }
    }

    if (m_lmap) {
        if (db) {
            m_lmap->ensurePersistantLinksMap(db);
        }
    }

	setThumbDirty();
};

void ard::selector_board::removeBoardLinks4Origin(board_item* origin) 
{
    auto db = dataDb();
    auto i = adjList().find(origin);
    if (i != adjList().end())
    {       
        auto& targets = i->second;
        for (auto j : targets) {
            auto link_lst = j.second;
            if (link_lst->size() > 0)
            {
                std::unordered_set<ard::board_link*> links2remove;
                for (int i = 0; i < static_cast<int>(link_lst->size()); i++) {
                    auto ll = link_lst->getAt(i);
                    links2remove.insert(ll);
                }
                link_lst->removeBLinks(links2remove);
            }
        }
    }

    if (m_lmap) {
        if (db) {
            m_lmap->ensurePersistantLinksMap(db);
        }
    }

	setThumbDirty();
};

const ard::O2O2LINKS& ard::selector_board::adjList()const
{
    return m_o2t_adj;
};

const ard::O2O2LINKS& ard::selector_board::radjList()const
{
    return m_t2o_adj;
};


#ifdef _DEBUG
void ard::selector_board::debugFunction() 
{
    for (auto b : m_bands) {
        b->setBandLabel("");
    }
    auto ex = bext();
    ex->setModified();
};
#endif


/**
    board_ext
*/
ard::board_ext::board_ext()
{

};

ard::board_ext::board_ext(topic_ptr _owner, QSqlQuery& q)
{
    attachOwner(_owner);
    m_mod_counter = q.value(1).toInt();
    m_bands_payload = q.value(2).toByteArray();
    _owner->addExtension(this);
};

void ard::board_ext::assignSyncAtomicContent(const cit_primitive* _other)
{
    assert_return_void(_other, "expected board extension");
    auto* other = dynamic_cast<const ard::board_ext*>(_other);
    assert_return_void(other, QString("expected board extension %1").arg(_other->dbgHint()));
    m_bands_payload = other->bandsPayload();
    ask4persistance(np_ATOMIC_CONTENT);
};

snc::cit_primitive* ard::board_ext::create()const
{
    return new ard::board_ext;
};

bool ard::board_ext::isAtomicIdenticalTo(const cit_primitive* _other, int&)const
{
    assert_return_false(_other, "expected item [1]");
    auto* other = dynamic_cast<const ard::board_ext*>(_other);
    assert_return_false(other, "expected item [2]");
    if (m_bands_payload != other->m_bands_payload)
    {
        QString s = QString("ext-ident-err:%1").arg(extName());
        sync_log(s);
        on_identity_error(s);
        return false;
    }
    return true;
};

QString ard::board_ext::calcContentHashString()const
{
    QString rv;
    return rv;
};

uint64_t ard::board_ext::contentSize()const
{
    uint64_t rv = 0;
    return rv;
};

void ard::board_ext::setModified()
{
    m_modified = true;
    ask4persistance(np_ATOMIC_CONTENT);
	owner()->setThumbDirty();
};

QByteArray ard::board_ext::bandsPayload()const
{
    if (m_modified || m_bands_payload.isEmpty())
    {
        auto b = owner();
        if (!b) {
            ASSERT(0, "expected board_ext owner");
            return QByteArray();
        }

        QJsonObject js;
        b->bandsToJson(js);
        QJsonDocument doc(js);
        m_bands_payload = doc.toJson(QJsonDocument::Indented);
    }

    return m_bands_payload;
};

/**
    board_item
*/
ard::board_item::board_item() 
{

};

ard::board_item::board_item(topic_ptr f, ard::BoardItemShape shp)
{
    auto e = ensureBIExt();
    e->m_bshape = shp;
    e->setRefTopic(f);  
};

ard::selector_board* ard::board_item::getBoard() 
{
    auto p = parent();
    if (!p) {
        ASSERT(0, "expected board parent");
        return nullptr;
    }
    auto rv = dynamic_cast<ard::selector_board*>(p);
    return rv;
};

const ard::selector_board* ard::board_item::getBoard()const 
{
    auto p = parent();
    if (!p) {
        ASSERT(0, "expected board parent");
        return nullptr;
    }
    auto rv = dynamic_cast<const ard::selector_board*>(p);
    return rv;

};

ard::board_item_ext* ard::board_item::biext() 
{
    return ensureBIExt();
};

const ard::board_item_ext*  ard::board_item::biext()const 
{
    ard::board_item* ThisNonCost = (ard::board_item*)this;
    return ThisNonCost->ensureBIExt();
};

ard::board_item_ext* ard::board_item::ensureBIExt()
{
    ASSERT_VALID(this);
    if (m_biext)
        return m_biext;

    ensureExtension(m_biext);
    return m_biext;
};

QString ard::board_item::objName()const
{
    return "bitem";
};

bool ard::board_item::canBeMemberOf(const topic_ptr f)const
{
    ASSERT_VALID(this);
    bool rv = false;
    if (dynamic_cast<const ard::selector_board*>(f) != nullptr) {
        rv = true;
    }
    return rv;
};

bool ard::board_item::killSilently(bool gui_update) 
{
    auto db = dataDb();
    if (db) {
        TOPICS_LIST lst;
        lst.push_back(this);
        dbp::removeBoardItemEx(db, lst);
    }

    return ard::topic::killSilently(gui_update);
};

void ard::board_item::mapExtensionVar(cit_extension_ptr e)
{
    ard::topic::mapExtensionVar(e);
    if (e->ext_type() == snc::EOBJ_EXT::extBoardItem) {
        ASSERT(!m_biext, "duplicate board item ext");
        m_biext = dynamic_cast<ard::board_item_ext*>(e);
        ASSERT(m_biext, "expected bitem ext");
    }
};

ard::BoardItemShape ard::board_item::bshape()const
{
    ard::BoardItemShape rv = ard::BoardItemShape::box;
    if (m_biext) {
        rv = m_biext->bshape();
    }
    return rv;
};

void ard::board_item::setBShape(BoardItemShape shp) 
{
    auto e = ensureBIExt();
    if (e) {
        if (shp != e->m_bshape) 
        {
            e->m_bshape = shp;

            if (e->hasDBRecord()) {
                if (dataDb() && isSyncDbAttached()) {
                    e->setSyncModified();
                    e->ask4persistance(np_SYNC_INFO);
                }
                e->ask4persistance(np_ATOMIC_CONTENT);
                e->ensureExtPersistant(dataDb());
            }
        }
    }

	auto b = getBoard();
	if (b) {
		b->setThumbDirty();
	}
};

topic_ptr ard::board_item::refTopic() 
{
    topic_ptr rv = nullptr;
    if (m_biext) {
        rv = m_biext->m_ref_topic;
    }
    return rv;
};

topic_cptr ard::board_item::refTopic()const 
{
    topic_cptr rv = nullptr;
    if (m_biext) {
        rv = m_biext->m_ref_topic;
    }
    return rv;
};

int ard::board_item::bandIndex()const 
{
    int rv = -1;
    if (m_biext) {
        rv = m_biext->bandIndex();
    }
    return rv;
};

int ard::board_item::yPos()const 
{
    int rv = 0;
    if (m_biext) {
        rv = m_biext->yPos();
    }
    return rv;
};

int ard::board_item::yDelta()const
{
    int rv = 0;
    if (m_biext) {
        rv = m_biext->yDelta();
    }
    return rv;
};

void ard::board_item::setBandIndex(int band_index)
{
    auto e = ensureBIExt();
    if (e) {
        if (e->m_band_index != band_index) 
        {
            e->m_band_index = band_index;

            if (e->hasDBRecord()) {
                if (dataDb() && isSyncDbAttached()) {
                    e->setSyncModified();
                    e->ask4persistance(np_SYNC_INFO);
                }
                e->ask4persistance(np_ATOMIC_CONTENT);
                e->ensureExtPersistant(dataDb());
            }

			auto b = getBoard();
			if (b) {
				b->setThumbDirty();
			}
        }
    }
};

void ard::board_item::setYDelta(int y_delta) 
{
    assert_return_void(y_delta >= 0, QString("expected non-negative ydelta: ").arg(y_delta));

    auto e = ensureBIExt();
    if (e) {
        if (e->m_y_delta != y_delta)
        {
            e->m_y_delta = y_delta;

            if (e->hasDBRecord()) {
                if (dataDb() && isSyncDbAttached()) {
                    e->setSyncModified();
                    e->ask4persistance(np_SYNC_INFO);
                }
                e->ask4persistance(np_ATOMIC_CONTENT);
                e->ensureExtPersistant(dataDb());
            }

			auto p = parent();
			if (p) {
				auto b = getBoard();
				if (b) {
					b->setThumbDirty();
				}
			}
        }
    }
};


topic_ptr ard::board_item::resolveRefTopic(const ArdDB* db)
{
    topic_ptr rv = nullptr;
    auto e = ensureBIExt();
    if (e) {
        rv = e->resolveRefTopic(db);
    }
    return rv;
};

void ard::board_item::markStatus(ard::BoardItemStatus st)
{
    auto e = biext();
    if (e) {
        e->m_status = st;
    }
};

topic_ptr ard::board_item::clone()const
{
    auto rv = dynamic_cast<ard::board_item*>(create());
    rv->assignContent(this);
    auto e = biext();
    if (e) {
        auto e2 = rv->biext();
        if (e2) {
            e2->assignSyncAtomicContent(e);
        }
    }
    return rv;
};

/**
    board_item_ext
*/
ard::board_item_ext::board_item_ext()
{

};

ard::board_item_ext::board_item_ext(topic_ptr _owner, QSqlQuery& q)
{
    attachOwner(_owner);
    m_mod_counter = q.value(1).toInt();
    m_ref_topic_syid = q.value(2).toString();
    int tmp = q.value(3).toInt();
    if (tmp > static_cast<int>(BoardItemShape::text_bold)) {
        ASSERT(0, "invalid bshape") << tmp;
        tmp = 0;
    }
    m_bshape = static_cast<BoardItemShape>(tmp);

    if (!m_ref_topic_syid.isEmpty()) {
        m_status = BoardItemStatus::unresolved_ref_link;
    }
    else {
        m_status = BoardItemStatus::removed;
    }   

    m_band_index = q.value(4).toInt();
    if (m_band_index < -1) {
        qWarning() << "invalid band index" << m_band_index << "will reset to 0";
        m_band_index = 0;
    }
    m_y_pos = q.value(5).toInt();
    m_y_delta = q.value(6).toInt();
    if (m_y_delta < -1) {
        qWarning() << "invalid ydelta" << m_y_delta << "will reset to 0";
        m_y_delta = 0;
    }
    _owner->addExtension(this);
};

ard::board_item_ext::~board_item_ext() 
{
    clearRefTopic();
};

/*ard::board_item* ard::board_item_ext::owner()
{
    auto rv = dynamic_cast<ard::board_item*>(cit_owner());
    return rv;
};

const ard::board_item* ard::board_item_ext::owner()const
{
    auto rv = dynamic_cast<const ard::board_item*>(cit_owner());
    return rv;
};*/

void ard::board_item_ext::assignSyncAtomicContent(const cit_primitive* _other)
{
    assert_return_void(_other, "expected board extension");
    auto* other = dynamic_cast<const ard::board_item_ext*>(_other);
    assert_return_void(other, QString("expected board item extension %1").arg(_other->dbgHint()));
    m_bshape                = other->m_bshape;
    m_band_index            = other->m_band_index;
    m_y_pos                 = other->m_y_pos;
    m_y_delta               = other->m_y_delta;
    m_ref_topic_syid        = other->m_ref_topic_syid;
    if (!m_ref_topic_syid.isEmpty()) {
        m_status = BoardItemStatus::unresolved_ref_link;
    }
    else {
        m_status = BoardItemStatus::removed;
    }
    ask4persistance(np_ATOMIC_CONTENT);
};

snc::cit_primitive* ard::board_item_ext::create()const
{
    return new ard::board_item_ext;
};

bool ard::board_item_ext::isAtomicIdenticalTo(const cit_primitive* _other, int&)const
{
    assert_return_false(_other, "expected item [1]")
    auto* other = dynamic_cast<const ard::board_item_ext*>(_other);
    assert_return_false(other, "expected item [2]")
    return true;
};

QString ard::board_item_ext::calcContentHashString()const
{
    QString rv;
    return rv;
};

uint64_t ard::board_item_ext::contentSize()const
{
    uint64_t rv = 0;
    return rv;
};

void ard::board_item_ext::setRefTopic(topic_ptr f) 
{
    clearRefTopic();
    if (f) {
        m_ref_topic = f;
        LOCK(m_ref_topic);
        m_ref_topic_syid = m_ref_topic->syid();
    }
};

void ard::board_item_ext::clearRefTopic() 
{
    if (m_ref_topic) {
        m_ref_topic->release();
        m_ref_topic = nullptr;
    }
    m_ref_topic_syid = "";
};

topic_ptr ard::board_item_ext::resolveRefTopic(const ArdDB* db)
{
    topic_ptr rv = nullptr;
    if (m_status == BoardItemStatus::removed ||
        m_status == BoardItemStatus::undefined) 
    {
        rv = nullptr;
    }
    else if (m_status == BoardItemStatus::unresolved_ref_link) {
        ASSERT(!m_ref_topic_syid.isEmpty(), "expected valid ref link");
        auto& m = db->syid2topic();
        auto i = m.find(m_ref_topic_syid);
        if (i != m.end()) {
            rv = i->second;
            setRefTopic(rv);
        }
        else {
            rv = nullptr;
            qWarning() << "unresolved btopic by syid" << m_ref_topic_syid << "will be deleted from board";
        }
    }
    return rv;
};

/**
board_band_info
*/
ard::board_band_info::board_band_info(QString str):m_band_label(str)
{

};

/*
ard::board_band_info::board_band_info(QString str)
    :m_band_label(str)
{};*/

bool ard::board_band_info::setBandWidth(int w) 
{
    bool rv = false;
    if (w < BBOARD_BAND_MIN_WIDTH) {
        w = BBOARD_BAND_MIN_WIDTH;
    }

    if (w < BBOARD_BAND_MAX_WIDTH) {
        if (m_band_width != w) {
            m_band_width = w;
            rv = true;
        }
    }

    return rv;
};

QString ard::board_band_info::bandLabel()const 
{ 
    if(!m_band_label.isEmpty())
        return m_band_label;
    return QString("%1").arg(m_band_index + 1);
}

bool ard::board_band_info::setBandLabel(QString s) 
{
    bool rv = false;
    if (s != m_band_label) {
        m_band_label = s;
        rv = true;
    }
    return rv;
};

void ard::board_band_info::toJson(QJsonObject& js)const 
{
    js["blabel"] = QString(m_band_label.toUtf8().toBase64());
    js["bwidth"] = QString("%1").arg(m_band_width);
};

void ard::board_band_info::fromJson(const QJsonObject& js) 
{
    m_band_label = QByteArray::fromBase64(js["blabel"].toString().toUtf8());
    //auto str = QByteArray::fromBase64(js["bwidth"].toString().toUtf8());
    auto str = js["bwidth"].toString();
    m_band_width = str.toInt();
    if (m_band_width < BBOARD_BAND_MIN_WIDTH || m_band_width > 1200) {
        m_band_width = BBOARD_BAND_DEFAULT_WIDTH;
    }
};

QString ard::board_band_info::dbgHint(QString)const
{
    QString rv = QString("band %1").arg(m_band_label);
    return rv;
};

/**
	mail_board
*/
ard::mail_board::mail_board() 
{
	m_title = "EMails";
};

void ard::mail_board::rebuildMappedTopicsBoard()
{
	std::function<void(ard::q_param*)> add_qp = [&](ard::q_param* q) 
	{
		auto bi = new board_band_info(q->title());
		bi->setBandWidth(q->boardBandWidth());
		m_bands.push_back(bi);
		m_topics.push_back(q);
		LOCK(q);
		if(q->isValid())q->prepare4Gui();
		m_t2b[q] = bi;
	};

	clearMappedTopicsBoard();
	auto d = ard::db();
	if (d && d->isOpen()) 
	{
		auto lst = d->rmodel()->boardRules();
		for (auto& i : lst)add_qp(i);
		ard::trail(QString("rebuild-mail-board [%1]").arg(lst.size()));
	}

	resetBandsIdx();
};

/*bool ard::mail_board::setBandWidth(int idx, int w)
{
	ard::band_board::setBandWidth(idx, w);
	auto r = m_topics[idx];
	r->setBoardBandWidth(w);
	return true;
};*/

/**
	folder_board
*/
ard::folders_board::folders_board()
{
	m_title = "Notes";
};

void ard::folders_board::rebuildMappedTopicsBoard()
{
	std::function<void(ard::locus_folder*)> add_folder = [&](ard::locus_folder* f)
	{
		qDebug() << "ykh-add-board-folder" << f->dbgHint();

		auto bi = new board_band_info(f->title());
		bi->setBandWidth(f->boardBandWidth());
		m_bands.push_back(bi);
		m_topics.push_back(f);
		LOCK(f);
		//if (f->isValid())q->prepare4Gui();
		m_t2b[f] = bi;
	};

	clearMappedTopicsBoard();
	auto d = ard::db();
	if (d && d->isOpen())
	{
		LOCUS_LIST folders_list;
		folders_list.push_back(ard::Sortbox());
		folders_list.push_back(ard::Reference());
		folders_list.push_back(ard::Maybe());		
		ard::selectCustomFolders(folders_list);		
		for (auto& i : folders_list)add_folder(i);
	}

	resetBandsIdx();
};


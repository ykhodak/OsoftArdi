#include <QToolBar>
#include <QApplication>
#include "BlackBoard.h"
#include "popup-widgets.h"
#include "custom-menus.h"
#include "ardmodel.h"
#include "utils.h"
#include "custom-boxes.h"
#include "OutlineMain.h"

#include "contact.h"
#include "custom-menus.h"
#include "BoardShapeSelector.h"
#include "BoardTemplateSelector.h"

using namespace ard::menu;
extern void register_no_popup_slide(ard::topic* f);
extern QString textFilesExtension();
extern QString imageFilesExtension();


ard::BlackBoard::BlackBoard(ard::selector_board* b)
{
    m_view = new BoardView(this);
	std::vector<EBoardCmd> cmd, rcmd;
	cmd.push_back(EBoardCmd::comment);
	cmd.push_back(EBoardCmd::rename);
	cmd.push_back(EBoardCmd::remove);
	cmd.push_back(EBoardCmd::colorBtn);
	cmd.push_back(EBoardCmd::slider);
	cmd.push_back(EBoardCmd::link);
	cmd.push_back(EBoardCmd::setShape);
	cmd.push_back(EBoardCmd::setTemplate);
	cmd.push_back(EBoardCmd::copy);
	cmd.push_back(EBoardCmd::paste);
	constructBoard(b, cmd, rcmd);

	connect(&m_board_watcher, &QTimer::timeout, [=]() {
		if (m_bb && m_bb->isThumbDirty())makeThumbnail();
	});
	m_board_watcher.start(500);
};


void ard::BlackBoard::setFocusOnContent()
{
    if (m_view) {
        m_view->setFocus();
    }
};

void ard::BlackBoard::reloadContent() 
{
    rebuildBoard();
};

ard::black_board_g_topic* ard::BlackBoard::register_bitem(ard::board_item* bi, board_band_info* band)
{
    auto rv = new ard::black_board_g_topic(this, bi);
    m_b2g[bi] = rv;
	registerBoardTopic(rv, band);
    return rv;
};

ard::GBLink* ard::BlackBoard::register_blink(ard::board_link_list* link_list, ard::board_link* lnk, black_board_g_topic* origin_g, black_board_g_topic* target_g)
{
    auto g = new ard::GBLink(this, link_list, lnk, origin_g, target_g);
    m_scene->addItem(g);
    m_l2g[lnk] = g;
    auto gitems = g->produceLinkItems();
    for (auto& g : gitems) {
        m_scene->addItem(g);
    }
    auto i = m_target_b2link_list.find(target_g->bitem());
    if (i != m_target_b2link_list.end()) {
        i->second.push_back(g);
    }
    else {
        std::list<ard::GBLink*> lst;
        lst.push_back(g);
        m_target_b2link_list[target_g->bitem()] = lst;
    }
    g->resetLink();
    return g;
};

void ard::BlackBoard::register_blink_list(ard::board_link_list* lnk_list, black_board_g_topic* origin_g, black_board_g_topic* target_g)
{
    if (lnk_list->size() > 0) {
        for (int i = 0; i < static_cast<int>(lnk_list->size()); i++) {
            auto ll = lnk_list->getAt(i);
            register_blink(lnk_list, ll, origin_g, target_g);
        }
    }
};

ard::board_item* ard::BlackBoard::firstSelectedBItem() 
{
	auto g = firstSelected();
	if (g) {
		auto bg = dynamic_cast<ard::black_board_g_topic*>(g);
		if (bg) {
			return bg->bitem();
		}
	}
	return nullptr;
};

void ard::BlackBoard::releaseLinkOriginList()
{
    for (auto i : m_link_origin_marks) {
        m_scene->removeItem(i.second);
    }
    m_link_origin_marks.clear();

    for (auto i : m_link_origin_list) {
        i->release();
    }
    m_link_origin_list.clear();
};

bool ard::BlackBoard::isInLinkOrigins(const ard::board_item* b)const 
{
    for (auto i : m_link_origin_list) {
        if (i == b)
            return true;
    }
    return false;
};

void ard::BlackBoard::updateLinkOriginMarks() 
{
    for (auto i : m_link_origin_list) 
    {
        auto j = m_link_origin_marks.find(i);
        if (j != m_link_origin_marks.end()) {
            auto k = m_b2g.find(i);
            if (k != m_b2g.end()) {
                auto mark = j->second;
                auto g = k->second;
                mark->updateMarkPos(g);
            }
        }
    }
};

void ard::BlackBoard::rebuildBoard() 
{
    clearBoard();
    if (m_bb) {
        rebuildBands();

        int bidx = 0;
        int xstart = 0;
        m_max_ybottom = 0;      
        const ard::C2BITEMS& bitems = m_bb->band2items();
        for (const auto& i : bitems) {
            auto band = m_bb->bandAt(bidx);
            assert_return_void(band, "expected band");
            auto band_width = band->bandWidth();
            auto ystart = layoutBand(band, i, xstart, false);
            xstart += band_width;
            if (ystart > m_max_ybottom)
                m_max_ybottom = ystart;
            bidx++;
        }

        m_max_ybottom += BBOARD_DELTA_EXPAND_HEIGHT;
		alignBandHeights();
        buildLinks();
    }

    makeGrid();
};

void ard::BlackBoard::clearBoard()
{
    m_b2g.clear();
    m_l2g.clear();
    m_target_b2link_list.clear();
    m_marked_links.clear();
    m_act2b.clear();
    if (m_currrent_control_link) {
        m_currrent_control_link->release();
        m_currrent_control_link = nullptr;
    }

    releaseLinkOriginList();

	ard::board_page<ard::selector_board>::clearBoard();
};

void ard::BlackBoard::makeGrid() 
{
    /*
    int y = gui::lineHeight();
    while (y < m_max_ybottom) {
        auto ln = m_scene->addLine(0, y, m_max_xright, y, QPen(QColor(Qt::gray)));
        ln->setZValue(BBOARD_ZVAL_BACK_WALL);
        y += gui::lineHeight();
    }
    */
};

void ard::BlackBoard::buildLinks() 
{
    if (m_bb) {
        auto& adj = m_bb->adjList();
        for (auto& i : adj) 
        {
            auto& origin = i.first;
            black_board_g_topic* origin_g = nullptr;
            black_board_g_topic* target_g = nullptr;

            auto k = m_b2g.find(origin);
            if (k != m_b2g.end()) {
                origin_g = k->second;
            }

            auto& targets = i.second;
            for (auto j : targets) {
                auto m = m_b2g.find(j.first);
                if (m != m_b2g.end()) {
                    target_g = m->second;
                }

                if (origin_g && target_g) {
                    auto lst = j.second;
                    register_blink_list(lst, origin_g, target_g);
                }
            }
        }
    }
};

/**
    here we have to 
      1.reset all origin's in band
      2.reset all targets in band
*/
void ard::BlackBoard::resetLinksInBands(const std::unordered_set<int>& bands)
{
    //...
    auto& adj = m_bb->adjList();
    for (auto& i : adj) 
    {
        auto bidx = i.first->bandIndex();
        auto k = bands.find(bidx);
        if (k != bands.end()) {
            /// target is inside affected band, we have to reset all it's targets
            auto& targets = i.second;
            for (auto i : targets) {
                auto j = m_target_b2link_list.find(i.first);
                if (j != m_target_b2link_list.end()) {
                    for (auto* lnk : j->second) {
                        lnk->resetLink();
                    }
                }
            }
        }
    }

    //...

    auto& bitems = m_bb->band2items();
    for (auto i : bands)
    {
        if (i < static_cast<int>(bitems.size())) {
            auto& lst = bitems[i];
            for (auto& bi : lst) {
                auto k = m_target_b2link_list.find(bi);
                if (k != m_target_b2link_list.end()) {
                    for (auto* lnk : k->second) {
                        lnk->resetLink();
                    }
                }
            }
        }
    }
};

int ard::BlackBoard::layoutBand(ard::board_band_info* band, const ard::BITEMS& bitems, int xstart, bool lookup4registered)
{
    int ydelta = 0;
	int ystart = 0;// gui::lineHeight();//for header
    for (auto& bi : bitems) {
        ydelta = 0;
        auto yd = bi->yDelta();
        if (yd > 0) {
            ydelta = yd;
            ystart += ydelta;
        }

        ard::B2G::iterator i = m_b2g.end();
        if (lookup4registered) {
            i = m_b2g.find(bi);
        }
        if (i != m_b2g.end()) {
            auto g = i->second;
            repositionInBand(band, g, xstart, ystart);
            ystart += g->boundingRect().height();
        }
        else {
            /// new item..
            auto g = register_bitem(bi, band);
            if (g) {
                repositionInBand(band, g, xstart, ystart);
                ystart += g->boundingRect().height();
            }
        }
    }
    return ystart;
};

void ard::BlackBoard::resetBand(const std::unordered_set<int>& bands2reset)
{
    const auto& bands = m_bb->bands();
    auto bands_count = bands.size();
    auto& bitems = m_bb->band2items();
    for (auto& band_idx : bands2reset){
        if (band_idx >= static_cast<int>(bands_count)) {
            ASSERT(0, "invalid bindex 1") << band_idx;
            return;
        }

        if (band_idx >= static_cast<int>(bitems.size())) {
            ASSERT(0, "invalid bindex 2") << band_idx;
            return;
        }
    }
    
	auto msel_lst = selectedGBItems();

    for (auto& band_idx : bands2reset) {
        auto band = m_bb->bandAt(band_idx);
        assert_return_void(band, "expected band");
        int xstart = band->xstart();
        auto& lst = bitems[band_idx];
        auto ystart = layoutBand(band, lst, xstart, true);

        if (ystart > m_max_ybottom) {
            m_max_ybottom = ystart + BBOARD_DELTA_EXPAND_HEIGHT;
			alignBandHeights();
        }

        auto i = m_band2g.find(band);
        if (i != m_band2g.end()) {
            i->second->update();
        }
    }

    std::unordered_set<int> bands_set2reset;
    for (auto i : bands2reset) {
        bands_set2reset.insert(i);
    }
    resetLinksInBands(bands_set2reset);

	for (auto i : msel_lst) {
		updateCurrentRelated(i);
    }

	updateSelectedMarks();
};

void ard::BlackBoard::setCurrentControlLink(ard::board_link* lnk)
{
	if (firstSelected()) {
		clearSelectedGBItems();
	}

    if (m_currrent_control_link && m_currrent_control_link == lnk) {
        return;
    }

	clearCurrentRelated();
    m_currrent_control_link = lnk;
    if (m_currrent_control_link) {
        LOCK(m_currrent_control_link);
        auto i = m_l2g.find(m_currrent_control_link);
        if (i != m_l2g.end()) {
            i->second->markAsControlSelected();
            //i->second->setFocus();
            i->second->update();
        }
    }
};


std::vector<ard::GBActionButton::EType> ard::BlackBoard::custom_mode_button_types4mode(CustomMode m)
{
    std::vector<GBActionButton::EType> rv;
    switch (m) 
    {
    case CustomMode::none:break;
    case CustomMode::link: rv.push_back(GBActionButton::add_link); break;
    case CustomMode::locate: 
        rv.push_back(GBActionButton::properties);
        rv.push_back(GBActionButton::show_item_links);
        //rv.push_back(GBActionButton::popup_item);
        rv.push_back(GBActionButton::find_item);
        break;
	case CustomMode::insert:break;// rv.push_back(GBActionButton::insert_item); break;
    case CustomMode::template_insert: rv.push_back(GBActionButton::insert_template); break;
    }

    return rv;
};

void ard::BlackBoard::repositionActionButtons(board_g_topic<ard::selector_board>* gi)
{
    if (m_custom_mode != CustomMode::none) {
        auto lst = custom_mode_button_types4mode(m_custom_mode);
        if (!lst.empty()) {
            //int dx = 0;
            auto rc = gi->boundingRect();
            auto pt = gi->pos();
            pt.setX(pt.x() + rc.width());
			auto new_y = pt.y() - gui::lineHeight();
			if (new_y < 0)new_y = 0;
            pt.setY(new_y);
            //auto rc_this = boundingRect();
            
            if (lst.size() > 1) {
                int total_btn_width = 0;
                /// shift/center action buttons
                for (auto i : lst) {
                    auto j = m_act2b.find(i);
                    if (j != m_act2b.end()) {
                        auto b = j->second;
                        auto rc_act_gb = b->boundingRect();
                        total_btn_width += rc_act_gb.width();
                    }
                }
                pt.setX(pt.x() + total_btn_width / 2);
            }

            for (auto i : lst) {
                auto j = m_act2b.find(i);
                if (j != m_act2b.end()) {
                    auto b = j->second;
                    b->setEnabled(true);
                    b->show();
                    auto rc_act_gb = b->boundingRect();
                    pt.setX(pt.x() - rc_act_gb.width());
                    b->setPos(pt);
                }
            }
        }

    }
};

void ard::BlackBoard::hideActionButtons()
{
    for (auto i : m_act2b)
        {
        auto b = i.second;
        b->hide();
        b->setEnabled(false);
    }
};

void ard::BlackBoard::updateActionButton()
{
    for (auto i : m_act2b) {
        auto b = i.second;
        if (b->isEnabled()) {
            b->update();
        }
    }
};


void ard::BlackBoard::updateCurrentRelated(board_g_topic<ard::selector_board>* curr_g)
{
	board_page<ard::selector_board>::updateCurrentRelated(curr_g);

	if (!m_marked_links.empty()) {
		for (auto& lnk : m_marked_links) {
			lnk->markAsNormal();
		}
		m_marked_links.clear();
	}

	auto gb = dynamic_cast<black_board_g_topic*>(curr_g);
	if (gb)
	{
		if (isInCustomEditMode()) {
			repositionActionButtons(gb);
		}

		//else {
		auto bi = gb->bitem();
		if (bi) {
			auto& adj = m_bb->adjList();
			auto i = adj.find(bi);
			if (i != adj.end()) {
				for (auto j : i->second) {
					auto i = m_target_b2link_list.find(j.first);
					if (i != m_target_b2link_list.end()) {
						for (auto& lnk : i->second) {
							if (lnk->g_origin()->bitem() == bi) {
								lnk->markAsSelected();
								m_marked_links.push_back(lnk);
							}
						}
					}
				}
			}
		}
	}
};

void ard::BlackBoard::clearCurrentRelated() 
{
	board_page<ard::selector_board>::clearCurrentRelated();
	if (m_currrent_control_link) {
		auto i = m_l2g.find(m_currrent_control_link);
		if (i != m_l2g.end()) {
			i->second->markAsNormal();
			i->second->update();
		}
		m_currrent_control_link->release();
		m_currrent_control_link = nullptr;
	}
};

void ard::BlackBoard::removeLinksFromOriginToAllTargets(ard::board_item* origin)
{
    auto& adj = m_bb->adjList();
    auto i = adj.find(origin);
    if (i != adj.end()) {
        auto& targets = i->second;
        for (auto j : targets) {
            auto k = m_target_b2link_list.find(j.first);
            if (k != m_target_b2link_list.end()) {
                auto& llst = k->second;
                for (auto it = llst.begin(); it != llst.end();) {
                    auto lnk = *it;
                    auto bi = lnk->g_origin()->bitem();
                    if (origin == bi) {
                        //qDebug() << "<<< we are origin and should remove alot of links to different targets" << j.first;
                        //qDebug() << "should clear m_target_b2link_list as we go";
                        auto ll = m_l2g.find(lnk->blink());
                        if (ll != m_l2g.end()) {
                            m_l2g.erase(ll);
                        }
                        lnk->removeLinkFromScene(m_scene);
                        it = llst.erase(it);
                    }
                    else {
                        it++;
                    }
                }
            }
        }
    }
};

void ard::BlackBoard::removeLinksFromAllOriginsToTarget(ard::board_item* target) 
{
    BITEMS origins_pointing_to_target;
    auto j = m_target_b2link_list.find(target);
    if (j != m_target_b2link_list.end()) {
        for (auto* lnk : j->second) {
            //auto origin = lnk->g_origin()->bitem();
            //origins_pointing_to_target.push_back(origin);
            lnk->removeLinkFromScene(m_scene);
            auto ll = m_l2g.find(lnk->blink());
            if (ll != m_l2g.end()) {
                m_l2g.erase(ll);
            }
            //...
        }
        m_target_b2link_list.erase(j);
    }
//  return origins_pointing_to_target;
};

void ard::BlackBoard::removeBItems(GB_LIST& lst) 
{
	std::unordered_set<ard::topic*> topics_set;
	std::vector<ard::topic*> ref_topics;
    for (auto i : lst) {
        auto bi = i->bitem();
        removeLinksFromAllOriginsToTarget(bi);
        removeLinksFromOriginToAllTargets(bi);
        int yDeltaAdj = bi->yDelta();
        auto height = i->boundingRect().height();
        yDeltaAdj += height;
        m_bb->removeBItem(bi, yDeltaAdj);
        m_b2g.erase(bi);
        auto f = bi->refTopic();
        if (f) {
			auto j = topics_set.find(f);
			if (j == topics_set.end()) 
			{
				ref_topics.push_back(f);
				topics_set.insert(f);
			}
        }
    }
    
	auto bands2reset = board_page<ard::selector_board>::removeTopicGBItems(ref_topics);
    resetLinksInBands(bands2reset);
    m_toolbar->sync2Board();
};

void ard::BlackBoard::removeSelected(bool silently)
{
	auto lst = selectedGBItems();
	if (lst.empty()) {
		if (m_currrent_control_link) {
			removeCurrentControlLink();
			return;
		}
	}

    if (lst.size() > 0) {
        if (!silently) {
            if (!ard::confirmBox(ard::mainWnd(), QString("Remove reference to %1 topic(s) from Blackboard?").arg(lst.size()))) {
                return;
            }
        }

        GB_LIST blst;
        for (auto i : lst) {
			auto gb = dynamic_cast<black_board_g_topic*>(i);
			if(gb && gb->bitem())blst.push_back(gb);
        }
        removeBItems(blst);
		clearCurrentRelated();
		clearSelectedGBItems();
    }
};

void ard::BlackBoard::removeCurrentControlLink() 
{
    if (m_currrent_control_link) {
        auto lnk_next2select = m_bb->getNext2SelectBLink(m_currrent_control_link);

        auto i = m_l2g.find(m_currrent_control_link);
        if (i != m_l2g.end()) 
        {
            auto g_link = i->second;
            auto target = g_link->g_target()->bitem();
            auto ref_topic = target->refTopic();
            assert_return_void(ref_topic, "expected reference topic");

            if (!ard::confirmBox(ard::mainWnd(), "Remove Link?")) {
                return;
            }

            /// delete reference to link gitem from target-map
            auto j = m_target_b2link_list.find(target);
            if (j != m_target_b2link_list.end()) {
                //for (auto* lnk : j->second) {
                for (auto m = j->second.begin(); m != j->second.end(); m++) {
                    if (*m == g_link) {
                        j->second.erase(m);
                        break;
                    }
                }
            }

            g_link->removeLinkFromScene(m_scene);
            m_l2g.erase(i);
            //<<<< --- remove from model at this point
            m_bb->removeBoardLink(m_currrent_control_link);
			clearCurrentRelated();
            //guiReleaseCurrentControlLink();
            if (lnk_next2select) {
                setCurrentControlLink(lnk_next2select);
            }
        }
    }
};

/*
void ard::BlackBoard::selectNext(Next2Item n) 
{
	auto g = firstSelected();
	if (g) {
		auto bg = dynamic_cast<black_board_g_topic*>(g);
		if (bg && bg->bitem()) {
			auto bi = m_bb->getNextBItem(bg->bitem(), n);
			if (bi) {
				auto j = m_b2g.find(bi);
				if(j != m_b2g.end())selectAsCurrent(j->second);
			}
		}
	}
};*/

ard::GBLink* ard::BlackBoard::current_link_g() 
{
    if (m_currrent_control_link) {
        auto i = m_l2g.find(m_currrent_control_link);
        if (i != m_l2g.end()) {
            return i->second;
        }
    }
    return nullptr;
};

void ard::BlackBoard::createBoardTopic(const QPointF& pt) 
{   
    auto dest = calcPointDropDestination(pt);
    dest.yDelta -= gui::lineHeight();//adjust for header
    topic_ptr f = nullptr;
    auto h = m_bb->ensureOutlineTopicsHolder();
    if (h) {
        f = new ard::topic();
        h->addItem(f);
        h->ensurePersistant(-1);
    }
    assert_return_void(f, "expected new topic");

    TOPICS_LIST lst;
    lst.push_back(f);

    auto blst = m_bb->insertTopicsBList(lst, dest.bandIndex, dest.yPos, dest.yDelta, ard::BoardItemShape::box);
    if (!blst.first.empty()) {
        std::unordered_set<int> bands2reset;
        bands2reset.insert(dest.bandIndex);
        resetBand(bands2reset);
        auto g1 = ensureVisible(f, false);
		if (g1) {
			auto bi = dynamic_cast<black_board_g_topic*>(g1);
			if (bi && bi->bitem()) {
				selectAsCurrent(bi);
				renameCurrent();
			}
			//bi = g1->
		}
    };

};

void ard::BlackBoard::insertTopic(topic_ptr f, int bidx, int ypos) 
{
	ard::board_item* sel_bitem = nullptr;

	auto g = firstSelected();
	if (g) {
		auto gb = dynamic_cast<black_board_g_topic*>(g);
		if (gb) sel_bitem = gb->bitem();
	}

    if (sel_bitem) {
        if (bidx == -1) {
            bidx = sel_bitem->bandIndex();
        }

        if (ypos == -1) {
            ypos = sel_bitem->yPos();
        }
    }

    if (bidx == -1)
        bidx = 0;
    if (ypos == -1)
        ypos = 0;

    TOPICS_LIST lst;
    lst.push_back(f);

    auto blst = m_bb->insertTopicsBList(lst, bidx, ypos);
    if (!blst.first.empty()) {
        std::unordered_set<int> bands2reset;
        bands2reset.insert(bidx);
        resetBand(bands2reset);
        ensureVisible(f);
    };
};

void ard::BlackBoard::showLinkProperties()
{
	auto g = firstSelected();
	if (g) {
		auto bg = dynamic_cast<ard::black_board_g_topic*>(g);
		if (bg) {
			BoardItemArrowsBox::showArrows(m_bb, bg->bitem());
		}
	}
};

void ard::BlackBoard::startCreateFromTemplateMode(BoardSample tmpl)
{   
    if (isInCustomEditMode()) {
        exitCustomEditMode();
    }
    m_toolbar->sync2Board();
    m_custom_mode = CustomMode::template_insert;
    m_createFromTemplate = tmpl;
};

bool ard::BlackBoard::isInCreateFromTemplateMode()const 
{
    return (m_custom_mode == CustomMode::template_insert);
    //return (m_createFromTemplate != BoardSample::None);
};

void ard::BlackBoard::suggestCreateFromTemplateAtPos(const QPointF& pt)
{
    assert_return_void(m_custom_mode == CustomMode::template_insert, "expected 'template-insert' mode");
    GBActionButton* b = nullptr;
    auto i = m_act2b.find(GBActionButton::insert_template);
    if (i == m_act2b.end()) {
        b = new GBActionButton(this);
        m_scene->addItem(b);
        b->setType(GBActionButton::insert_template);
        m_act2b[GBActionButton::insert_template] = b;
    }
    else {
        b = i->second;
    }

    if (b) {
        b->setEnabled(true);
        b->show();
        b->setPos(pt);
    }
};

void ard::BlackBoard::completeCreateFromTemplate(const QPointF& pt) 
{
    auto bandIndex = bandIndexAtPos(pt.x());
    if (bandIndex == -1) {
        ASSERT(0, "invalid bindex") << bandIndex;
        return;
    }

    int yDelta = pt.y();

    if (m_createFromTemplate != BoardSample::None) {
        ard::buildBBoardSample(m_createFromTemplate, m_bb, bandIndex, yDelta);
        rebuildBoard();
    };
    exitCustomEditMode();
};


void ard::BlackBoard::insertFromTemplate(const QPoint& pt) 
{
	BoardTemplateSelector::selectTemplte(board()->id(), pt);
};

ard::black_board_g_topic* ard::BlackBoard::findByBItem(ard::board_item* b)
{
    auto i = m_b2g.find(b);
    if (i != m_b2g.end()) {
        return i->second;
    }
    return nullptr;
};

std::pair<ard::gb_ptr, ard::gb_ptr> ard::BlackBoard::findDropSpot(const ard::BITEMS& band_items, const QPointF& ptInScene, ard::black_board_g_topic* gi1)
{
    std::pair<ard::gb_ptr, ard::gb_ptr> rv = {nullptr, nullptr};
    ard::gb_ptr prev_gb = nullptr;
    for (auto& i : band_items) {
        auto k = m_b2g.find(i);
        if (k != m_b2g.end()) {
            if (k->second != gi1) {
                const auto& rc = k->second->sceneBoundingRect();
                bool is_below = ptInScene.y() < rc.top();
                if (is_below) {
                    rv.first = prev_gb;
                    rv.second = k->second;
                    return rv;
                }
                prev_gb = k->second;
            }
        }
    }
    return rv;
};

ard::BlackBoard::BoardDropPos ard::BlackBoard::calcPointDropDestination(const QPointF& ptInScene) 
{
    ard::BlackBoard::BoardDropPos rv;
    rv.bandIndex = bandIndexAtPos(ptInScene.x());
    if (rv.bandIndex == -1) {
        ASSERT(0, "invalid bindex") << rv.bandIndex;
        return rv;
    }

    auto& b2bitems = m_bb->band2items();
    if (rv.bandIndex >= static_cast<int>(b2bitems.size())) {
        ASSERT(0, "invalid bindex") << rv.bandIndex;
        return rv;
    }

    auto& band_list = b2bitems[rv.bandIndex];
    for(auto i = band_list.rbegin();i != band_list.rend(); i++)
    {
        auto bi = *i;
        auto k = m_b2g.find(bi);
        if (k != m_b2g.end()) {
            const auto& rc = k->second->sceneBoundingRect();
            bool pt_below_topic = ptInScene.y() > rc.top();
            if (pt_below_topic) {
                rv.yPos = bi->yPos() + 1;
                rv.yDelta = (ptInScene.y() - rc.bottom());
                return rv;
            }
        }
    }

    rv.yPos = 0;
    rv.yDelta = ptInScene.y();

    return rv;
};

ard::BlackBoard::BoardDropPos ard::BlackBoard::calcTopicDropDestination(const QPointF& ptInScene, const ard::BITEMS& items2move, ard::black_board_g_topic* gi1)
{
    auto moved_item = gi1->bitem();

    BITEMS_SET bitems_set;
    bitems_set.insert(moved_item);
    for (auto i : items2move) {
        bitems_set.insert(i);
    }

    auto old_band_index = moved_item->bandIndex();

    ard::BlackBoard::BoardDropPos rv;
    rv.bandIndex = bandIndexAtPos(ptInScene.x());
    if (rv.bandIndex == -1) {
        ASSERT(0, "invalid bindex") << rv.bandIndex;
        return rv;
    }

    auto& b2bitems = m_bb->band2items();
    if (rv.bandIndex >= static_cast<int>(b2bitems.size())) {
        ASSERT(0, "invalid bindex") << rv.bandIndex;
        return rv;
    }

    auto& band_list = b2bitems[rv.bandIndex];
    for (auto i = band_list.rbegin(); i != band_list.rend(); i++)
    {
        auto bi = *i;
        if (bitems_set.find(bi) == bitems_set.end()) {
            auto k = m_b2g.find(bi);
            if (k != m_b2g.end()) {
                const auto& rc = k->second->sceneBoundingRect();
                bool pt_below_topic = ptInScene.y() > rc.top();
                if (pt_below_topic) {
                    if (old_band_index == bi->bandIndex() && moved_item->yPos() < bi->yPos()) {
                        ///adjust for continuos selection set
                        rv.yPos = bi->yPos() + 1 - bitems_set.size();
                    }
                    else {
                        rv.yPos = bi->yPos() + 1;
                    }
                    rv.yDelta = (ptInScene.y() - rc.bottom());
                    //qDebug() << "pt" << ptInScene.y() << "is below" << bi->refTopic()->title() << rc;
                    return rv;
                }
            }
        }
    }

    rv.yPos = 0;
    rv.yDelta = ptInScene.y();

    return rv;
};


void ard::BlackBoard::processMoved(board_g_topic<selector_board>* gi1)
{
	auto gi = dynamic_cast<black_board_g_topic*>(gi1);
	assert_return_void(gi, "expected board gitem");
    auto moved_item = gi->bitem();
    auto g_rc = gi->sceneBoundingRect();
    QPointF ptInScene((g_rc.left() + g_rc.right()) / 2, g_rc.top());
    auto bandIndex = bandIndexAtPos(ptInScene.x());

    if (bandIndex == -1) {
        ASSERT(0, "invalid bindex") << bandIndex;
        return;
    }
    auto band = m_bb->bandAt(bandIndex);
    assert_return_void(band, "expected band");
    qreal total_height = 0.0;

    auto old_band_idx = moved_item->bandIndex();
    ard::BITEMS items2move;
    if (!m_selected_marks.empty()) {
        auto first2move = *(m_selected_marks.begin());
        auto first_bidx = first2move->g()->bandIndex();
        for (auto i : m_selected_marks)
		{
            //auto g = i->g_bitem();
            //auto bi = g->bitem();
//			qDebug() << "ykh-idx" << first_bidx << i->g()->bandIndex();
            if (first_bidx == i->g()->bandIndex()) 
			{
				auto gb = dynamic_cast<black_board_g_topic*>(i->g());
				if (gb && gb->bitem()) 
				{
					auto bi = gb->bitem();
					items2move.push_back(bi);
					total_height += bi->yDelta() + i->g()->boundingRect().height();
				}
            }
            else {
                break;
            }
        }
    }
    else {
        items2move.push_back(moved_item);
        total_height = moved_item->yDelta() + gi->boundingRect().height();
    }

    if (items2move.empty())
        return;


    auto res = calcTopicDropDestination(ptInScene, items2move, gi);

    if (res.bandIndex != bandIndex) {
        ASSERT(0, "expected bandIndex") << bandIndex << "calculated" << res.bandIndex;
        return;
    }

    m_bb->moveToBand(items2move, res.bandIndex, res.yPos, res.yDelta, total_height);
//	auto b = gi->bitem();
    gi->resize2Content(band);
    std::unordered_set<int> bands2reset;
    bands2reset.insert(old_band_idx);
    bands2reset.insert(res.bandIndex);
    resetBand(bands2reset);

    if (isInLinkOrigins(moved_item)) {
        updateLinkOriginMarks();
    }


    std::unordered_set<int> bands_changed;
    bands_changed.insert(res.bandIndex);
    bands_changed.insert(old_band_idx);
    resetLinksInBands(bands_changed);
	updateSelectedMarks();
};


void ard::BlackBoard::onModifiedOutsideTopic(topic_ptr f) 
{
    auto bi = m_bb->findBItem(f);
    if (bi) {
		resizeGBItems(bi);
    }
};



bool ard::BlackBoard::isInCustomEditMode()const 
{
    bool rv = (m_custom_mode != CustomMode::none);
    return rv;
};

void ard::BlackBoard::exitCustomEditMode()
{
    if (isInLinkMode()) {
        exitLinkMode();
    }
    m_custom_mode = CustomMode::none;
    clearCustomEditModeGui();
    m_createFromTemplate = BoardSample::None;
    m_toolbar->sync2Board();
};

void ard::BlackBoard::setupActionButtonInCaseCustomEditMode()
{
    auto ok = isInCustomEditMode();
    if (ok) {
        auto lst = custom_mode_button_types4mode(m_custom_mode);
        if (!lst.empty()) {
            GBActionButton *b = nullptr;
            for (auto i : lst) {
                auto j = m_act2b.find(i);
                if (j != m_act2b.end()) {
                    b = j->second;
                    //maybe enable it
                }
                else {
                    b = new GBActionButton(this);
                    b->setType(i);
                    m_scene->addItem(b);
                    m_act2b[i] = b;
                }
            }
        }
    }
};

void ard::BlackBoard::initCustomEditMode(CustomMode m) 
{
    m_custom_mode = m;
    setupActionButtonInCaseCustomEditMode();
	auto f = firstSelectedRefTopic();
	if (f) {
		auto& lst = findGBItems(f);
		for (auto i : lst) {
			i->update();
			auto bg = dynamic_cast<ard::black_board_g_topic*>(i);
			if (bg) {
				updateCurrentRelated(bg);
			}			
		}
	}
};

void ard::BlackBoard::clearCustomEditModeGui()
{
    hideActionButtons();
    //if (m_curr_mark) {
    //    m_curr_mark->update();
   // }
};

bool ard::BlackBoard::isInLinkMode()const
{
    return (m_custom_mode == CustomMode::link);
};

void ard::BlackBoard::exitLinkMode()
{
    if (isInLinkMode()) {
        releaseLinkOriginList();
        m_custom_mode = CustomMode::none;
        clearCustomEditModeGui();
    }
};

void ard::BlackBoard::enterLinkMode()
{
	auto lst = selectedGBItems();
	if (lst.empty()) {
		ard::messageBox(this,"Select topic on board to start adding arrows (links).");
		return;
	}

    if (isInLinkMode()) {
        exitLinkMode();
        return;
    }

    if (isInCustomEditMode()) {
        exitCustomEditMode();
    }

    releaseLinkOriginList();
	for (auto& i : lst) 
	{
		auto bg = dynamic_cast<black_board_g_topic*>(i);
		if (bg) {
			auto b = bg->bitem();
			if (b) 
			{
				m_link_origin_list.push_back(b);
				LOCK(b);

				auto mark = new ard::GCurrLinkOriginMark(this);
				m_scene->addItem(mark);
				m_link_origin_marks[b] = mark;
			}
		}
	}

    initCustomEditMode(CustomMode::link);
    updateLinkOriginMarks();
};

void ard::BlackBoard::addLink()
{
    assert_return_void(isInLinkMode(), "expected link mode");
    assert_return_void(!m_link_origin_list.empty(), "expected link origin list");
	auto lst = selectedGBItems();
	for (auto& i : lst) {
		auto bg = dynamic_cast<black_board_g_topic*>(i);
		if(bg)doAddLinkFromSelectedOrigins(bg->bitem());
	}
};

void ard::BlackBoard::doAddLinkFromOrigin(ard::board_item* origin, ard::board_item* target, QString LinkLabel)
{
    auto lnk_list = m_bb->addBoardLink(origin, target);
    if (lnk_list) {
        auto lnk = lnk_list->getAt(lnk_list->size() - 1);
        if (!LinkLabel.isEmpty()) {
            lnk->setLinkLabel(LinkLabel, m_bb->syncDb());
        }

        black_board_g_topic* origin_g = nullptr;
        black_board_g_topic* target_g = nullptr;

        auto k = m_b2g.find(origin);
        if (k != m_b2g.end()) {
            origin_g = k->second;
        }
        auto m = m_b2g.find(target);
        if (m != m_b2g.end()) {
            target_g = m->second;
        }

        if (origin_g && target_g) {
            register_blink(lnk_list, lnk, origin_g, target_g);
            updateActionButton();
        }
    }
};

void ard::BlackBoard::doAddLinkFromSelectedOrigins(ard::board_item* target)
{
    assert_return_void(target, "expected target item");
    for (auto i : m_link_origin_list)
    {
        doAddLinkFromOrigin(i, target);
    }
};

void ard::BlackBoard::editLink(ard::GBLink* gl)
{
    auto lnk = gl->blink();
    if (BoardItemEditArrowBox::editArrow(m_bb, gl->g_origin()->bitem(), gl->g_target()->bitem(), lnk)) {
        gl->rebuildLabel();
    };
};

bool ard::BlackBoard::isInInsertMode()const 
{
    return (m_custom_mode == CustomMode::insert);
};

void ard::BlackBoard::enterInsertMode() 
{
    if (isInInsertMode()) {
        exitInsertMode();
        return;
    }
   
    if (isInCustomEditMode()) {
        exitCustomEditMode();
    }

    initCustomEditMode(CustomMode::insert);
};

void ard::BlackBoard::exitInsertMode() 
{
    if (isInInsertMode()) {
        m_custom_mode = CustomMode::none;
        clearCustomEditModeGui();
        m_add_progress_in_insert_mode.clear();
    }
};

void ard::BlackBoard::completeAddInInsertMode() 
{
    assert_return_void(isInInsertMode(), "expected 'insert' mode");
	auto g1 = firstSelected();
    //if (m_mselected.size() > 0 || m_currrent_item) 
	if(g1)
    {
		auto bg = dynamic_cast<ard::black_board_g_topic*>(g1);
		if (!bg)return;
		auto b = bg->bitem();
        if (b) {
            QString stitle;//, slabel;

            auto bidx = b->bandIndex();
            const auto& bands = m_bb->bands();
            if (bidx == static_cast<int>(bands.size()) - 1) {
                m_bb->addBands(1);
            }

            auto new_item_bidx = bidx + 1;

            topic_ptr f = nullptr;
            auto h = m_bb->ensureOutlineTopicsHolder();
            if (h) {
                int next_new_idx = 0;
                auto k = m_add_progress_in_insert_mode.find(b);
                if (k == m_add_progress_in_insert_mode.end()) {
                    m_add_progress_in_insert_mode[b] = next_new_idx;
                }
                else {
                    next_new_idx = k->second;
                    next_new_idx++;
                    m_add_progress_in_insert_mode[b] = next_new_idx;
                }

                static QString new_titles[] = {"A", "B", "C",  "D",  "E",  "F",  "G",  "H",  "I",  "J",  "K",  "L",  "M", "N"};
                stitle = QString("New Topic %1").arg(next_new_idx);
                if (next_new_idx >= 0 && next_new_idx < static_cast<int>(sizeof(new_titles))) {
                    stitle = new_titles[next_new_idx];
                    //slabel = stitle.toLower();
                }

                f = new ard::topic(stitle);
                h->addItem(f);
                h->ensurePersistant(-1);
            }
            assert_return_void(f, "expected new topic");

            TOPICS_LIST lst;
            lst.push_back(f);

            auto blst = m_bb->insertTopicsBList(lst, new_item_bidx, -1, -1, ard::BoardItemShape::box);
            if (!blst.first.empty()) {
                std::unordered_set<int> bands2reset;
                bands2reset.insert(new_item_bidx);
                resetBand(bands2reset);
                for (auto j : blst.first) {
                    doAddLinkFromOrigin(b, j);
                }
            };
        }
    }
    else
        {
            if(!m_act2b.empty()){
                    auto i = m_act2b.begin();
                    auto pt = i->second->scenePos();
                    //QPointF pt;
                    createBoardTopic(pt);
                }
        }
};

bool ard::BlackBoard::isInLocateMode()const 
{
    return (m_custom_mode == CustomMode::locate);
};

void ard::BlackBoard::enterLocateMode() 
{
	auto g1 = firstSelected();
	if (!g1) {
		ard::messageBox(this,"Select topic on board to find it in outline and enter 'locate on select' mode.");
		return;
	}

    if (isInLocateMode()) {
        exitLocateMode();
        return;
    }

    if (isInCustomEditMode()) {
        exitCustomEditMode();
    }

    initCustomEditMode(CustomMode::locate);
};

void ard::BlackBoard::exitLocateMode() 
{
    if (isInLocateMode()) {
        m_custom_mode = CustomMode::none;
        clearCustomEditModeGui();
    }
};

void ard::BlackBoard::editComment() 
{
	auto g = firstSelected();
    if (g) {
		ard::board_page<ard::selector_board>::editComment(g);
    }
    else {
        auto lnk_g = current_link_g();
        if (lnk_g) {
            editLink(lnk_g);
        }
    }
};

void ard::BlackBoard::renameBand(board_band<ard::selector_board>* g_band)
{
    if (g_band) {
        auto pt_scene = g_band->mapToScene(g_band->boundingRect().topLeft());
        auto pt_view = m_view->mapFromScene(pt_scene);
        pt_view.setX(pt_view.x() + 2 * gui::lineHeight());
        QRect rce(pt_view, QSize(g_band->band()->bandWidth() - 3 * gui::lineHeight(), gui::lineHeight()));
		ard::board_editor<ard::selector_board>::editBandLabel(this, rce, g_band->band()->bandIndex());
    }
};

void ard::BlackBoard::onBandControl(board_band_header<ard::selector_board>* g_band)
{
    auto bindex = g_band->band()->bandIndex();

    QMenu m(this);
    ard::setup_menu(&m);
    QAction* a = nullptr;
    ADD_MCMD("Insert Band", MCmd::bboard_add_band);
    ADD_MCMD("Rename Band", MCmd::bboard_rename_band);
    ADD_MCMD("Remove Band", MCmd::bboard_remove_band);

    connect(&m, &QMenu::triggered, [=](QAction* a)
    {
        auto d = ard::menu::unpackMcmd(a);
        switch (d.first) {
        case MCmd::bboard_add_band:
        {
            //m_bb->debugFunction();
            m_bb->insertBand(bindex);
            rebuildBoard();
        }break;
        case MCmd::bboard_remove_band:
        {
            if (ard::confirmBox(ard::mainWnd(), QString("Please confirm removing band from blackboard"))) {
                m_bb->removeBandAt(bindex);
                rebuildBoard();
            }
        }break;
        case MCmd::bboard_rename_band: 
        {
			auto g_band = bandAt(bindex);
			if (g_band) {
				renameBand(g_band);
			}
			//ASSERT(0, "need impl");
            //renameBand(g_band);
        }break;
        default:break;
        }
    });
    
    m.exec(QCursor::pos());
};

void ard::BlackBoard::applyNewShape(ard::BoardItemShape sh) 
{   
	std::unordered_set<int> bands2reset;
	auto msel = selectedGBItems();
	for (auto i : msel) 
	{
		auto bi = dynamic_cast<black_board_g_topic*>(i);
		if (bi && bi->bitem()) {
			bi->bitem()->setBShape(sh);
			bands2reset.insert(bi->bitem()->bandIndex());
		}
	}

	for (auto i : msel) {
		auto gb = dynamic_cast<black_board_g_topic*>(i);
		if (gb) {
			auto b = gb->bitem();
			auto bidx = b->bandIndex();
			auto band = m_bb->bandAt(bidx);
			if (band)gb->resize2Content(band);
			gb->update();
		}
	}

	resetBand(bands2reset);
};

void ard::BlackBoard::applyTextFilter() 
{
    auto key = m_filter_edit->text().trimmed();
    bool refilter = true;
    if (m_text_filter->isActive()) {        
        auto s2 = m_text_filter->fcontext().key_str;
        if (key == s2) {
            refilter = false;
        }
    }

    if (refilter) {
        TextFilterContext fc;
        fc.key_str = key;
        fc.include_expanded_notes = false;
        m_text_filter->setSearchContext(fc);
        bool active_filter = !key.isEmpty();

        //update board to filter
        for (auto i : m_b2g) 
        {
            auto gi = i.second;
            gi->setOpacity(DEFAULT_OPACITY);
            if (active_filter) {
                auto bi = gi->bitem();
                auto f = bi->refTopic();
                if (f) {
                    if (!f->hasText4SearchFilter(fc)) {
                        gi->setOpacity(DIMMED_OPACITY);
                    };
                }
            }

            gi->update();
        }
        for (auto i : m_l2g) {
            i.second->updateLink();
        }
    }
};

void ard::BlackBoard::debugFunction() 
{
	//qDebug() << "screenshot-file:" << m_bb->screenshotThumbFileName();

//	auto pm = m_view->grab();
//	auto mp1 = pm.scaled(MP1_WIDTH, MP1_HEIGHT, Qt::KeepAspectRatio, Qt::SmoothTransformation);
//	mp1.save(m_bb->screenshotThumbFileName());
};

void ard::BlackBoard::onBandDragEnter(board_band<ard::selector_board>*, QGraphicsSceneDragDropEvent *e)
{
	ard::dragEnterEvent(e);
};

void ard::BlackBoard::onBandDragDrop(board_band<ard::selector_board>* b, QGraphicsSceneDragDropEvent *e)
{
	const QMimeData* md = e->mimeData();
	if (!md)
		return;

	auto tmd = qobject_cast<const ard::TopicsListMime*>(md);
	if (tmd) {
		dropTopics(b, e, tmd);
	}
	else
	{
		if (md->hasUrls() ||
			md->hasText() ||
			md->hasHtml())
		{
			dropTextEx(b, e, md);
		}
	}
};


void ard::BlackBoard::dropTopics(board_band<ard::selector_board>* b, QGraphicsSceneDragDropEvent *e, const ard::TopicsListMime* mm)
{
	auto dest = calcPointDropDestination(e->scenePos());
	//qDebug() << "bb-drop" << dest.bandIndex << dest.yPos << dest.yDelta;

	int band_space = 1;
	ard::BoardItemShape default_shape = ard::BoardItemShape::box;

	topic_ptr f2ensure_visible_on_no_drop = nullptr;
	bool found_topics_in_bb = false;
	TOPICS_LIST topics2add;
	auto& lst = mm->topics();
	for (auto& i : lst) {
		auto b = board()->findBItem(i);
		if (!b) {
			auto cg = dynamic_cast<ard::contact_group*>(i);
			if (cg) {
				band_space = 2;
				qDebug() << "dropped c-group";
				default_shape = ard::BoardItemShape::box;
				std::set<ard::contact_group*> gfilter;
				gfilter.insert(cg);
				auto glst = ard::db()->cmodel()->groot()->getAsGroupListModel(&gfilter);
				for (auto g : glst) {
					//auto f = g->shortcutUnderlying();
					topics2add.push_back(g);
				}
			}
			else {
				auto t = i->prepareInjectIntoBBoard();
				if (t) {
					topics2add.push_back(t);
				}
			}
		}
		else {
			found_topics_in_bb = true;
			if (!f2ensure_visible_on_no_drop) {
				f2ensure_visible_on_no_drop = i;
			}
		}
	}

	if (topics2add.empty()) {
		if (f2ensure_visible_on_no_drop) {
			ard::messageBox(this,"Topic already in Board, press OK to locate");
			ensureVisible(f2ensure_visible_on_no_drop);
		}
		return;
	}

	auto expandAs = BoardTopicDropBox::showBoardTopicDropOptions(topics2add, default_shape, band_space);
	if (expandAs.insert_branch_type == ard::InsertBranchType::none)
		return;


	if (!topics2add.empty())
	{
		auto f = topics2add[0];
		auto res = board()->insertTopicsWithBranches(expandAs.insert_branch_type,
			topics2add,
			b->band()->bandIndex(),
			dest.yPos,
			dest.yDelta,
			expandAs.item_shape,
			expandAs.band_space);

		for (auto i : res.bitems) {
			auto f = i->refTopic();
			if (f) {
				auto cg = dynamic_cast<ard::contact_group*>(f);
				if (cg) {
					i->setBShape(BoardItemShape::circle);
				}
			}
		}

		rebuildBands();
		std::unordered_set<int> bset;
		for (auto& i : res.bands)bset.insert(i);
		resetBand(bset);
		alignBandHeights();
		//...
		auto& adj = board()->adjList();
		for (auto& o : res.origins)
		{
			auto i = adj.find(o);
			if (i != adj.end()) {
				black_board_g_topic* origin_g = nullptr;
				black_board_g_topic* target_g = nullptr;

				auto k = m_b2g.find(o);
				if (k != m_b2g.end()) {
					origin_g = k->second;
				}

				auto& targets = i->second;
				for (auto j : targets) {
					auto m = m_b2g.find(j.first);
					if (m != m_b2g.end()) {
						target_g = m->second;
					}

					if (origin_g && target_g) {
						auto lst = j.second;
						register_blink_list(lst, origin_g, target_g);
					}
				}
			}
		}

		ensureVisible(f);
	}
};

void ard::BlackBoard::dropTextEx(board_band<ard::selector_board>* b, QGraphicsSceneDragDropEvent *e, const QMimeData* m)
{
	auto dest = calcPointDropDestination(e->scenePos());

	TOPICS_LIST topics2add;
	topic_ptr f = nullptr;
	auto bb = board();
	auto h = bb->ensureOutlineTopicsHolder();
	if (h)
	{
		ard::InsertBranchResult res;

		if (m->hasHtml())
		{
			f = ard::dropHtml(h, h->items().size(), m->html());
			if (f)
			{
				topics2add.push_back(f);
				res = board()->insertTopicsWithBranches(InsertBranchType::single_topic,
					topics2add,
					b->band()->bandIndex(),
					dest.yPos,
					dest.yDelta,
					BoardItemShape::box);
			}
		}
		else if (m->hasUrls())
		{
			QString text_files = textFilesExtension();
			static QString image_files = imageFilesExtension();
			foreach(QUrl url, m->urls())
			{
				if (url.isLocalFile())
				{
					QFileInfo fi(url.toLocalFile());
					auto s = "*." + fi.suffix().toLower();
					if (text_files.indexOf(s) != -1)
					{
						f = ard::dropTextFile(h, h->items().size(), url);
						if (f) {
							topics2add.push_back(f);
						}
						//created = ard::dropTextFile(destination_parent, destination_pos, url);
					}
					else
					{
						if (image_files.indexOf(s) != -1) {
							f = ard::dropImageFile(h, h->items().size(), url);
							if (f) {
								topics2add.push_back(f);
							}
						}
					}
				}
				else
				{
					f = ard::dropUrl(h, h->items().size(), url);
					if (f) {
						topics2add.push_back(f);
					}
				}

				if (!topics2add.empty())
				{
					res = board()->insertTopicsWithBranches(InsertBranchType::single_topic,
						topics2add,
						b->band()->bandIndex(),
						dest.yPos,
						dest.yDelta,
						BoardItemShape::box);
				}
			}
		}
		else if (m->hasText())
		{
			f = ard::dropText(h, h->items().size(), m->text());
			if (f)
			{
				topics2add.push_back(f);
				res = board()->insertTopicsWithBranches(InsertBranchType::single_topic,
					topics2add,
					b->band()->bandIndex(),
					dest.yPos,
					dest.yDelta,
					BoardItemShape::box);
			}
		}

		if (f) {
			rebuildBands();
			std::unordered_set<int> bset;
			for (auto& i : res.bands)bset.insert(i);
			resetBand(bset);
			alignBandHeights();
			ensureVisible(f);
		}
	}
};

void ard::BlackBoard::selectShape(ard::BoardItemShape sh, const QPoint& pt) 
{
	BoardShapeSelector::editShape(board()->id(), sh, pt);
};

void ard::BlackBoard::pasteFromClipboard()
{
	auto bboard = board();
	assert_return_void(bboard, "expected board");

	const QMimeData *mm = nullptr;
	if (qApp->clipboard()) {
		mm = qApp->clipboard()->mimeData();
	}
	if (!mm) {
		return;
	}

	int dest_band_idx = 0;
	int dest_ypos = 0;

	auto g = firstSelected();
	if (g)
	{
		dest_band_idx = g->bandIndex();
		dest_ypos = g->ypos() + 1;
	}
	auto h = bboard->ensureOutlineTopicsHolder();
	if (h) {
		auto lst = ard::insertClipboardData(h, h->items().size(), mm, false);
		if (!lst.empty())
		{
			auto f = *lst.begin();
			//TOPICS_LIST lst;
			//lst.push_back(f);

			auto blst = bboard->insertTopicsBList(lst, dest_band_idx, dest_ypos);
			if (!blst.first.empty()) {
				std::unordered_set<int> bands2reset;
				bands2reset.insert(dest_band_idx);
				resetBand(bands2reset);
				ensureVisible(f);
			};
		}
	}
}

std::pair<bool, QString> ard::BlackBoard::renameCurrent()
{
	std::pair<bool, QString> rv{ true, "" };
	auto g = firstSelected();
	if (g)
	{
		auto gi = dynamic_cast<black_board_g_topic*>(g);
		auto bi = gi->bitem();
		if (!bi) {
			ASSERT(0, "expected bitem");
			return rv;
		}

		return renameGItem(gi);
		/*
		auto f = bi->refTopic();
		if (!f) {
			rv = { false, "Reference no resolved" };
			return rv;
		}

		if (!f->canRename()) {
			rv = { false, QString("Can't rename '%1'").arg(f->objName()) };
			return rv;
		}

		auto brct = g->boundingRect();// g->title_rect();
		auto top_l = brct.topLeft();
		top_l.setX(top_l.x() - gui::lineHeight());
		auto pt_scene = g->mapToScene(top_l);

		auto bandIndex = bandIndexAtPos(pt_scene.x());
		if (bandIndex == -1) {
			ASSERT(0, "invalid bindex") << bandIndex;
			return rv;
		}

		
		auto band = m_bb->bandAt(bandIndex);
		if (band) {
			auto j = m_band2g.find(band);
			if (j != m_band2g.end()) {
				auto bb = j->second;
				auto brc = bb->scenePos();
				if (pt_scene.x() < brc.x()) {
					pt_scene.setX(brc.x());
				}
				//pt_scene.setX(brc.x());
			}
		}
	

		//get here band index, then band left coord, use it for edit position calculation

		auto pt_view = m_view->mapFromScene(pt_scene);
		int w = BBOARD_BAND_DEFAULT_WIDTH - gui::lineHeight();
		if (brct.width() > w) {
			w = brct.width();
		}
		pt_view.setY(pt_view.y() - gui::lineHeight());
		if (pt_view.y() < 0)pt_view.setY(0);

		QRect rce(pt_view, QSize(w, 3 * gui::lineHeight()));
		if (static_cast<int>(brct.height()) > rce.height()) {
			rce.setHeight(brct.height());
		}

		ard::board_editor<ard::selector_board>::editTitle(this, rce);
		*/
	}
	else
	{
		auto lnk_g = current_link_g();
		if (lnk_g) {
			editLink(lnk_g);
		}
		else {
			rv = { false, "Select object to rename" };
		}
	}
	return rv;
};
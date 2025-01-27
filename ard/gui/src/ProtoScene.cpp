#include <math.h>
#include <QGraphicsView>
#include <QScrollBar>
#include <QApplication>
#include <QDesktopWidget>

#include "ProtoScene.h"
#include "anfolder.h"
#include "custom-g-items.h"
#include "contact.h"
#include "ardmodel.h"
#include "MainWindow.h"
#include "OutlineMain.h"
#include "OutlineSceneBase.h"
#include "email.h"
#include "custom-widgets.h"
#include "anGItem.h"
#include "ansearch.h"
#include "ethread.h"
#include "rule_runner.h"

extern int getLasteSelectedColorIndex();

extern QPalette default_palette;
bool g_ignore_vscroll = false;

/**
   ProtoView
*/
void ProtoView::ensureVisibleGItem(ProtoGItem* pg)
{
    if(!pg->g()->isSelected())
        pg->g()->setSelected(true);
    pg->g()->setFocus();
    QRectF rc2 = pg->g()->sceneBoundingRect();
    rc2.setWidth(10);
    v()->ensureVisible(rc2, 0, 0);
};

/**
   ProtoScene
*/
ProtoScene::ProtoScene():
    m_outline_policy(outline_policy_Pad)
{
};

ProtoScene::~ProtoScene()
{

};

void ProtoScene::clearOutline()
{
    m_edit_frame = nullptr;
    m_current_marks.clear();
    m_top_right_huds.clear();
    m_left_aligned_huds.clear();
    m_bottom_center_huds.clear();
}

void ProtoScene::setOutlinePolicy(EOutlinePolicy p)
{
    //  ASSERT(0, "ProtoScene::setOutlinePolicy");
    m_outline_policy = p;
};


void ProtoScene::selectGI(ProtoGItem* pg, bool propagate2workspace)
{
	if (!pg) {
		ASSERT(0, "expected proto item");
		return;
	}

	QGraphicsItem* g = pg->g();

	s()->clearSelection();
	if (!g->isSelected())
	{
		g->setSelected(true);
		if (g->flags() & QGraphicsItem::ItemIsFocusable && !g->hasFocus())
		{
			s()->setFocusItem(g);
		}
		g->update();
		installCurrentItemView(propagate2workspace);
	}
};

void ProtoScene::setupCurrentSpot(ProtoGItem* pg, bool force /*= false*/)
{
    if (v()->isInTitleEditMode()) {
        return;
    }

    if(pg->g()->isSelected())
        {
            qreal xpos = 0.0;
            for(GCURRENT_MARKS::reverse_iterator i = m_current_marks.rbegin(); i != m_current_marks.rend(); i++)
                {
                    CurrentMark* m = *i;
                    if(force || m->owner() != pg || !m->g()->isVisible())
                        {
                            xpos = m->setup(pg, xpos);
                        }
                }
        }
    else
        {
            ASSERT(0, "expected selected item");
        }
};


void ProtoScene::freeCurrMarks()
{
    for(GCURRENT_MARKS::iterator i = m_current_marks.begin(); i != m_current_marks.end(); i++)
        {
            CurrentMark* m = *i;
            s()->removeItem(m->g());
        }  
    m_current_marks.clear();
};

void ProtoScene::hideCurrMarks() 
{
    for (auto& i : m_current_marks) {
        i->g()->setVisible(false);
    }
};

ProtoGItem* ProtoScene::selectNext(bool go_up, ProtoGItem* gi)
{
	if (!gi)
		gi = currentGI();
	if (!gi)
		return nullptr;

	ProtoGItem* rv = nullptr;

	if (m_panel)
	{
		auto i = std::find(m_panel->m_outlined.begin(), m_panel->m_outlined.end(), gi);
		if (i == m_panel->m_outlined.end()) {
			ASSERT(0, "failed to locate GItem");
			return nullptr;
		}
		//ProtoGItem* rv = nullptr;
		if (go_up) {
			if (i == m_panel->m_outlined.begin())
				return nullptr;
			i--;
		}
		else {
			i++;
			if (i == m_panel->m_outlined.end())
				return nullptr;
		}
		rv = *i;
		if (rv) {
			selectGI(rv);
			QScrollBar* hsc = v()->v()->horizontalScrollBar();

			int hscroll = hsc->value();
			rv->g()->ensureVisible();
			if (hsc->value() != hscroll) {
				hsc->setValue(hscroll);
			}
			rv->g()->setFocus();
		}

		//* p = gi->p();
		//rv = p->selectNext(go_up, gi);
	}
	return rv;
}

std::vector<ProtoGItem*> ProtoScene::gitemsInRange(ProtoGItem* gi1, ProtoGItem* gi2)
{
    std::vector<ProtoGItem*> rv;
    if (m_panel) 
    {
        GITEMS_VECTOR& lst = m_panel->outlined();
        auto i1 = std::find(lst.begin(), lst.end(), gi1);
        if (i1 == lst.end())return rv;
        auto i2 = std::find(lst.begin(), lst.end(), gi2);
        if (i2 == lst.end())return rv;
        if (i2 > i1) {
            for (auto i = i1; i <= i2; i++) {
                rv.push_back(*i);
            }
        }
        else {
            for (auto i = i2; i <= i1; i++) {
                rv.push_back(*i);
            }
        }
    }
    return rv;
};


void ProtoScene::freePanels()
{
    clearOutline();
    if (m_panel) {
        delete m_panel;
        m_panel = nullptr;
    }
};

ProtoGItem* ProtoScene::footer() 
{
    ProtoGItem* rv = nullptr;
    if (hasPanel()) {
        rv = panel()->footer();
    }
    return rv;
};

ProtoPanel* ProtoScene::panel()
{
    return m_panel;
};

bool ProtoScene::hasPanel()const
{
    return (m_panel != nullptr);
};

void ProtoScene::addPanel(ProtoPanel* p)
{
    if (m_panel) {
        //delete m_panel;
        //m_panel = nullptr;
        //ASSERT(0, "Panel already defined in scene");
    }

    m_panel = p;
};

TOPICS_LIST ProtoScene::mselected()
{
    TOPICS_LIST lst;
    if (!hasPanel()) {
        ASSERT(0, "no panels in scene");
        return lst;
    }

    auto gcurr = currentGI(false);
    if (gcurr && !gcurr->isCheckSelected()) {
        auto t = gcurr->topic();
        lst.push_back(t->shortcutUnderlying());
    }

    ProtoPanel* p = panel();
    for (auto& i : p->outlined()) {     
        if (i->isCheckSelected()) {
            auto t = i->topic();
            lst.push_back(t->shortcutUnderlying());
        }
    }

    return lst;
};

void ProtoScene::clearMSelected()
{
    if (!hasPanel()) {
        ASSERT(0, "no panels in scene");
        return;
    }

    ProtoPanel* p = panel();
    for (auto& i : p->outlined()) {
        i->setCheckSelected(false);
    }
    m_last_mselected_by_mouse = nullptr;
};

void ProtoScene::storeMSelected() 
{
    auto lst = mselected();
    for (auto& f : lst) {
        m_stored_mselected.insert(f);
    }
};

void ProtoScene::restoreMSelected() 
{
    if (!m_stored_mselected.empty()) {
        ProtoPanel* p = panel();
        if (p) {
            for (auto& i : p->outlined()) {
                auto j = m_stored_mselected.find(i->topic());
                if (j != m_stored_mselected.end()) {
                    i->setCheckSelected(true);
                }
            }
        }
    }
    m_stored_mselected.clear();
};

void ProtoScene::registerMSelectedByMouse(topic_ptr f)
{
    m_last_mselected_by_mouse = f;
};

topic_ptr ProtoScene::lastMselectedByMouse()
{
    return m_last_mselected_by_mouse;
};

void ProtoScene::mselectAll(std::function<bool(topic_ptr)> pred )
{
    if (!hasPanel()) {
        ASSERT(0, "no panels in scene");
        return;
    }

    ProtoPanel* p = panel();
    for (auto& i : p->outlined()) {
        auto t = i->topic();
        bool special_topic = t->isSingleton() ||
            t->isGtdSortingFolder() ||
            (t->folder_type() == EFolderType::folderUserSorter);
        if(!special_topic && t->isStandardLocusable()){
            if (pred) {
                if (pred(t)) {
                    i->setCheckSelected(true);
                }
            }
            else {
                i->setCheckSelected(true);
            }
        }
    }
};


ProtoGItem* ProtoScene::findGItem(topic_ptr it)
{
    if (m_panel) {
        return m_panel->findGI(it);
    }

    return nullptr;
};

ProtoGItem* ProtoScene::findGItemByUnderlying(topic_ptr it) 
{
	return findGItem(it);
	/*
    if (hasPanel()) {
        ProtoPanel* p = panel();
        if (p) {
            for (auto& i : p->m_it2gi) {
                if (i.first->shortcutUnderlying() == it->shortcutUnderlying()) {
                    return i.second;
                }
            }
        }
    }
    return nullptr;*/
};

ProtoGItem* ProtoScene::findGItemByEid(QString eid) 
{
    if (hasPanel())
        {
            ProtoPanel* p = panel();
            for(auto& i : p->m_it2gi)
                {
                    auto it = i.first;
                    if (it->wrappedId().compare(eid, Qt::CaseInsensitive) == 0)
                        {
                            return i.second;
                        }
                }
        }

    return nullptr;
};


ProtoGItem* ProtoScene::findGItem(std::function<bool(topic_ptr)> findBy)
{
	if (hasPanel())
	{
		ProtoPanel* p = panel();
		for (auto& i : p->m_it2gi)
		{
			auto it = i.first;
			if (it && findBy(it))
			{
				return i.second;
			}
		}
	}

	return nullptr;
};


QRectF ProtoScene::enabledItemsBoundingRect() const
{
    QRectF rv(0,0,0,0);
    QList<QGraphicsItem *> all_items = s()->items();
    for(QList<QGraphicsItem *>::iterator i = all_items.begin(); i != all_items.end(); i++)
        {
            QGraphicsItem* g = *i;
            if(g->isEnabled())
                {
                    QPointF topLeft = g->mapToScene(g->boundingRect().topLeft());
                    QPointF bottomRight = g->mapToScene(g->boundingRect().bottomRight());
                    if(topLeft.x() < rv.left())
                        rv.setLeft(topLeft.x());
                    if(topLeft.y() < rv.top())
                        rv.setTop(topLeft.y());

                    if(bottomRight.x() > rv.right())
                        rv.setRight(bottomRight.x());
                    if(bottomRight.y() > rv.bottom())
                        rv.setBottom(bottomRight.y());
                }
        };
    return rv;  
};




void ProtoScene::showEditFrame(bool bshow, QRectF rc /*= QRectF()*/, EColumnType c /*= EColumnType::Title*/)
{
    if(!m_edit_frame)
        {
            m_edit_frame = new TitleEditFrame();
            s()->addItem(m_edit_frame);
        }

    if(bshow)
        {
            if(rc.isEmpty())
                {
                    bshow = false;
                }
        }

    if(bshow)
        {
            if (c == EColumnType::Annotation) {
                m_edit_frame->setNavyColor(false);
            }
            else {
                m_edit_frame->setNavyColor(true);
            }

            m_edit_frame->setRect(rc);
            m_edit_frame->setVisible(true);
        }
    else
        {
            m_edit_frame->setVisible(false);
        }
};

void ProtoScene::resetVisibleItemsOpacity(QGraphicsItem* g_except, qreal opacity){
    QRect rc(0,0,v()->v()->viewport()->width(), v()->v()->viewport()->height());
    QList<QGraphicsItem *> visible_items = v()->v()->items(rc);
    for(QList<QGraphicsItem *>::iterator i = visible_items.begin(); i != visible_items.end(); i++)
        {
            QGraphicsItem* g = *i;
            if(g == g_except)
                {
                    g->setOpacity(1.0);
                }
            else
                {
                    g->setOpacity(opacity);
                }
        }
};


void ProtoScene::gui_delete_mselected()
{
    //assert_return_void(gui::isDBAttached(), "expected attached DB");
    TOPICS_LIST lst = mselected();
    if (lst.size() == 0) {
		ard::messageBox(ard::mainWnd(), "Please select item to proceed");
        //removeSelItem();
    }
    else
    {
		TOPICS_LIST l2 = ard::reduce2ancestors(lst);
		auto just_threads = ard::select_ethreads(l2);
		bool confirmed = false;
		auto count = l2.size();
		if (count > 0)
		{
			QString msg = "";
			if (count == 1) {
				auto it = l2[0];
				msg = QString("<b><font color=\"red\">Delete %1</font></b> selected '%1'").arg(it->title());
			}
			else 
			{
				bool all_emails = (just_threads.size() == l2.size());
				QString obj_prefix = "items";
				if(all_emails)obj_prefix = "emails";
				msg = QString("<b><font color=\"red\">Delete %1</font></b> selected %2").arg(count).arg(obj_prefix);
			}
			confirmed = ard::confirmBox(ard::mainWnd(), msg);
		}

        if (confirmed) {
			{
 				if (!just_threads.empty()) {
					//if (trash_emails) {
					auto m = ard::gmail_model();
					if (m)m->trashThreadsSilently(just_threads);
					//}
				}
                for (auto& t : l2) {
                    t->killSilently(true);
                }
				auto p = gui::currPolicy();
				if (PolicyCategory::is_observable_policy(p)) {
					gui::rebuildOutline(p, true);
				}
				else {
					gui::rebuildOutline();
				}
				
				ard::rebuildFoldersBoard(nullptr);
            }
        }
    }
};


void ProtoScene::resetSceneBoundingRect()
{

    if(!hasPanel())
        return;

    resetPanelsGeometry();

	
    QRectF rect = enabledItemsBoundingRect();
    if (rect.isNull())
        {
            s()->setSceneRect(QRectF(0, 0, 1, 1));
        }
    else
        {
			g_ignore_vscroll = true;
            /// don't know, this messes up scroll
            s()->setSceneRect(rect);
        }
		
    updateAuxItemsPos();
 
   // v()->emit_inDerived_onResetSceneBoundingRect();
};

void ProtoScene::updateAuxItemsPos()
{
    updateSceneHudItemsPos();
    if (hasPanel()) {
        panel()->updateAuxItemsPos();
    }
    /*
    for(PROTO_PANELS::iterator i = m_panels.begin();i != m_panels.end();i++)
        {
            ProtoPanel* p = *i;
            p->updateAuxItemsPos();
        }
        */
};


void ProtoScene::resetPanelsGeometry()
{
    m_viewport_size = QSize(v()->v()->viewport()->width(),
                            v()->v()->viewport()->height());

//    int panels_count = panelsCount();
    if(hasPanel())
        {
            ProtoPanel* p = panel();
            p->setPanelWidth(m_viewport_size.width());
            p->resetGeometry();
        }
};

void ProtoScene::addTopRightHudButton(HudButton* h)
{
    h->setEnabled(false);
    h->setVisible(false);
    m_top_right_huds.push_back(h);
    s()->addItem(h);
};

void ProtoScene::addLeftAlignedHudButton(HudButton* h)
{
    m_left_aligned_huds.push_back(h);
    s()->addItem(h);
};

void ProtoScene::addBottomCenterHudButton(HudButton* h)
{
    //  h->setEnabled(false);
    //  h->setVisible(false);
    m_bottom_center_huds.push_back(h);
    s()->addItem(h);
};

void ProtoScene::updateSceneHudItemsPos()
{
    QGraphicsView* v2 = v()->v();

    //top-right aligned
    if(!m_top_right_huds.empty())
        {
            qreal ypos = 0;
            GHUD_BUTTONS::iterator i = m_top_right_huds.begin();
            while(i != m_top_right_huds.end())
                {
                    qreal line_h = 0;
                    qreal row_width = 0;
                    GHUD_BUTTONS::iterator k = i;
                    for(;k != m_top_right_huds.end(); k++)
                        {
                            HudButton* h = *k;
                            if(h->isBreak())
                                {
                                    break;
                                }
                            else
                                {
                                    qreal bh = h->boundingRect().height();
                                    if(bh > line_h)
                                        line_h = bh;

                                    row_width += h->boundingRect().width();
                                }
                        }
                    qreal xpos = row_width;
                    GHUD_BUTTONS::iterator j = i;
                    for(;j != k; j++)
                        {
                            HudButton* h = *j;
                            QPoint pt((int)(v2->viewport()->width() - xpos),
                                      (int)ypos);
                            QPointF pt2 = v2->mapToScene(pt);
                            h->setPos(pt2);
                            if(!h->isEnabled()){
                                h->setEnabled(true);
                                h->setVisible(true);
                            }      
                            xpos -= h->boundingRect().width();
                        }
                    if(j != m_top_right_huds.end())
                        {
                            HudButton* h = *j;
                            assert_return_void(h->isBreak(), "give me a break");
                            xpos = h->boundingRect().width();
                            ypos += line_h;
                            QPoint pt((int)(v2->viewport()->width() - xpos),
                                      (int)ypos);
                            //    QPoint pt(xpos, ypos);
                            QPointF pt2 = v2->mapToScene(pt);
                            h->setPos(pt2);
                            if(!h->isEnabled()){
                                h->setEnabled(true);
                                h->setVisible(true);
                            }
                            ypos += h->boundingRect().height();
                            j++;
                        }
                    i = j;
                }
        }

    //** bottom-center, usually hints
    if(!m_bottom_center_huds.empty())
        {
            QPoint pt((int)v2->viewport()->width(),
                      (int)v2->viewport()->height());
            qreal view_width = v2->viewport()->width();
            qreal view_height = v2->viewport()->height();
            qreal xpos = 0;
            qreal ypos = view_height;
            for(GHUD_BUTTONS::iterator i = m_bottom_center_huds.begin(); i != m_bottom_center_huds.end(); i++)
                {
                    HudButton* h = *i;
                    ypos -= h->boundingRect().height();
                    xpos = (view_width - h->boundingRect().width()) / 2;
                    h->setPos(xpos, ypos);
                }      
        }

    ProtoGItem* gcurr = currentGI();
    if(gcurr)
        {
            setupCurrentSpot(gcurr, true);
        }
};


bool ProtoScene::hasParam(QString name)const
{
    bool rv = false;
    PARAM_MAP::const_iterator i = m_param_map.find(name);
    if(i != m_param_map.end())
        {
            rv = true;
        }
    return rv;
};

/**
   ProtoGItem
*/
ProtoGItem::ProtoGItem():m_item(nullptr), m_p(nullptr)
{

};

void ProtoGItem::invalidate_proto()
{
    if(m_item != nullptr)
        m_item->release();
    m_item = nullptr;
};

ProtoGItem::ProtoGItem(topic_ptr item, ProtoPanel* p)
    :m_item(item), m_p(p)
{
    LOCK(item);
}

ProtoGItem::~ProtoGItem()
{
    if(m_item)
        m_item->release();
};

ard::topic* prevent_slide_req = nullptr;
void register_no_popup_slide(ard::topic* f) 
{
    prevent_slide_req = f;
}

void ProtoGItem::onBecomeCurrent(bool propagate2workspace)
{
    if (p()->hasProp(ProtoPanel::PP_CurrSpot))
    {
        if (g()->isSelected())
        {
            regenerateCurrentActions();
            if (prevent_slide_req == topic()) 
            {
                prevent_slide_req = nullptr;
                return;
            }
			if (!ard::isSelectorLinked()) {
				return;
			}
			if (propagate2workspace) {
				ard::open_page(topic());
			}
        }
        else
        {
            p()->s()->hideCurrMarks();
        }//isSelected
    }//PP_CurrSpot
};


void ProtoGItem::processHoverMoveEvent(QGraphicsSceneHoverEvent * e)
{
	SHit sh;
	EHitTest hit = hitTest(e->pos(), sh);

	Qt::CursorShape cr = Qt::ArrowCursor/*OpenHandCursor*/;

	bool forceDrawHint = false;

	if (forceDrawHint || dbp::configFileSupportCmdLevel() > 0)
	{
		QString hinf = "";
		switch (hit)
		{
		case hitUnknown:            hinf = "[hitUnknown]"; break;
		case hitTitle:              hinf = "[hitTitle]"; break;
		case hitMainIcon:           hinf = "[hitMainIcon]"; break;
		case hitSecondaryIcon:           hinf = "[hitMainIcon]"; break;
		case hitColorHash:          hinf = "[hitColorHash]"; break;
		case hitAnnotation:         hinf = "[hitAnnotation]"; break;
		case hitExpandedNote:       hinf = "[hitExpandedNote]"; break;
		case hitHotSpot:            hinf = QString("[hitHotSpot-%1]").arg(sh.item->title()); break;
		case hitToDo:               hinf = "[hitToDo]"; break;
		case hitCheckSelect:        hinf = "[hitCheckSelect]"; break;
		case hitActionBtn:          hinf = "[hitActionBtn]"; break;
		case hitAfterTitle:         hinf = "[hitAfterTitle]"; break;
		case hitResourceLabel:      hinf = "[hitResourceLabel]"; break;
		case hitTernaryIconArea:    hinf = "[hitTernaryIconArea]"; break;
		case hitTableColumn:        hinf = QString("[hitTableColumn] %1").arg(sh.column_number); break;
		case hitGanttBar:           hinf = "[hitGBar]"; break;
		case hitGanttBarRightBorder:hinf = "[hitGBar-RightBorder]"; cr = Qt::SizeHorCursor; break;
		case hitBox:                hinf = "[hitBox]"; break;
		case hitUrl:                hinf = "[hitUrl]"; break;
		case hitFatFingerSelect:    hinf = "[hitFatFinger]"; break;
		case hitFatFingerDetails:   hinf = "[hitFatFingerDetails]"; break;
		}

		QString id_info = m_item->dbgHint();
		main_wnd()->setWindowTitle(id_info + hinf + m_item->title());
	}

	bool clear_hint = true;
	switch (hit)
	{
	case hitToDo:
	{
		if (!topic()->isToDo())
		{
			clear_hint = false;
			p()->s()->setupHint(hit, this);
		}
	}break;
	default:break;
	}
};


bool ProtoGItem::preprocessMousePress(const QPointF&, bool )
{
    return false;
};

void ProtoGItem::on_clickedToDo()
{
}

void ProtoGItem::on_clickedFatFingerSelect()
{
    topic()->fatFingerSelect();
};

void ProtoGItem::on_clickedFatFingerDetails(const QPoint& p1)
{
	if (p()->hasProp(ProtoPanel::PP_FatFingerSelect) &&
		topic()->hasFatFinger()) 
	{
		topic()->fatFingerDetails(p1);
	}

//    topic()->fatFingerDetails(p);
};

void ProtoGItem::on_clickedTernary() 
{   
    auto f = topic();
    auto qt = dynamic_cast<ard::rule_runner*>(f);
    if (qt) {
        qt->make_gui_query();
    }
};


void ProtoGItem::on_clickedActionBtn()
{
    p()->s()->selectGI(this);
    auto f = topic();
    f->setToDo(0, ToDoPriority::notAToDo);
    
    auto h = ard::hoisted();
    if (h) {
        auto o = dynamic_cast<ard::task_ring_observer*>(h);
        if (o) {
            o->rebuild_observed();
        }
    }

    gui::rebuildOutline();
};


void ProtoGItem::on_clickedTitle(const QPointF& pt, bool wasSelected)
{
    if (wasSelected) {
        p()->s()->v()->renameSelected(EColumnType::Title, &pt);
    }
};

void ProtoGItem::on_clickedAnnotation(const QPointF& pt, bool wasSelected) 
{
    if (wasSelected) {
        p()->s()->v()->renameSelected(EColumnType::Annotation, &pt);
    }
};

void ProtoGItem::on_clickedExpandedNote(const QPointF&, bool wasSelected) 
{
    if (wasSelected) {
        topic()->fatFingerSelect();
        //p()->s()->v()->renameSelected(EColumnType::Annotation, &pt);
    }
};

void ProtoGItem::on_clickedUrl()
{
    //gui::openUrl(topic()->url());
};

void ProtoGItem::getOutlineEditRect(EColumnType column_type, QRectF& rc)
{
    rc = g()->mapRectToScene(rc);
    //-- recalc rc by using QFontMetrics and rect feature
    
    QString str = topic()->fieldMergedValue(column_type, "");

    QRect rc1;
    rectf2rect(rc, rc1);
    QFontMetrics fm(*ard::defaultFont());
    QRect rc2 = fm.boundingRect(rc1, Qt::AlignLeft|Qt::AlignTop|Qt::TextWordWrap, str);
    int h2 = rc2.height() + 2;
    if(h2 > rc.height())
        {
            rc.setHeight(h2);
        }
};

void ProtoGItem::recalcSize()
{
    g()->update();
};

void ProtoGItem::resetGeometry()
{
    updateGeometryWidth();
};


void ProtoGItem::updateGeometryWidth()
{
    g()->update();
};

void ProtoGItem::on_keyPressed(QKeyEvent * e)
{
    switch (e->key())
    {
    case Qt::Key_Return:
    {
        auto f = topic();
        assert_return_void(f, "expected topic");
        ard::open_page(f);
    }break;
    case Qt::Key_Delete:
    {
        p()->s()->gui_delete_mselected();
    }break;
    }
};

void ProtoGItem::optAddCurrentCommand(ECurrentCommand c)
{
    auto t = topic();
    assert_return_void(t, "expected topic");

    if (t->hasCurrentCommand(c)){
        CurrentMarkButton* m = new CurrentMarkButton(c);
        m->setRightXoffset(0);
        p()->s()->s()->addItem(m->g());
        p()->s()->current_marks().push_back(m);
    }
};

void ProtoGItem::regenerateCurrentActions() 
{
    ///@todo: this design is really bad, topic should return set of supported 
    ///commands, maybe with 'selector' optional parameter

#define ADD_MARK_BTN_IF_PROP(P, B)if(p()->hasProp(P))optAddCurrentCommand(B);

    auto t = topic();
    assert_return_void(t, "expected topic");

    p()->s()->freeCurrMarks();
    if (p()->isSelectorControl())
    {
        //we are some selector control
        ADD_MARK_BTN_IF_PROP(ProtoPanel::PP_CurrSelect, ECurrentCommand::cmdSelect);
        ADD_MARK_BTN_IF_PROP(ProtoPanel::PP_CurrMoveTarget, ECurrentCommand::cmdSelectMoveTarget);
        ADD_MARK_BTN_IF_PROP(ProtoPanel::PP_CurrDownload, ECurrentCommand::cmdDownload);
        ADD_MARK_BTN_IF_PROP(ProtoPanel::PP_CurrDelete, ECurrentCommand::cmdDelete);
        ADD_MARK_BTN_IF_PROP(ProtoPanel::PP_CurrOpen, ECurrentCommand::cmdOpen);
        ADD_MARK_BTN_IF_PROP(ProtoPanel::PP_CurrFindInShell, ECurrentCommand::cmdFindInShell);
        ADD_MARK_BTN_IF_PROP(ProtoPanel::PP_CurrEdit, ECurrentCommand::cmdEdit);
    }
    else
    {
        optAddCurrentCommand(ECurrentCommand::cmdEmptyRecycle);
       // optAddCurrentCommand(ECurrentCommand::cmdSendAll);
    }


    p()->s()->setupCurrentSpot(this);
};

void ProtoGItem::processDefaultAction()
{
    auto f = topic();
    assert_return_void(f, "expected topic");   
    f->setExpanded(!f->isExpanded());
    p()->onDefaultItemAction(this);
    auto sceneBuilder = p()->s()->v()->sceneBuilder();
    if (sceneBuilder) {
        model()->setAsyncCallRequest(AR_rebuildOutline, 0, 0, 0, sceneBuilder);
    }
    else {
        gui::rebuildOutline();
    }
};

void ProtoGItem::processDoubleClickAction()
{
    auto f = topic();
    assert_return_void(f, "expected topic");
    ard::open_page(f);
    auto sceneBuilder = p()->s()->v()->sceneBuilder();
    if (sceneBuilder) 
    {
        QMetaObject::invokeMethod(sceneBuilder, "topicDoubleClick",
            Qt::QueuedConnection,
            Q_ARG(void*, (void*)f));
    }
}

void gui::open(topic_ptr f)
{
    assert_return_void(f, "expected topic");
    
    EOutlinePolicy pol = gui::currPolicy();
    switch(pol)
        {
        case outline_policy_Pad:break;
        default:pol = outline_policy_Pad;break;
        }
    gui::outlineFolder(f, pol);
};

/**
   ProtoGSecondaryItem
*/
ProtoGSecondaryItem::ProtoGSecondaryItem(ProtoGItem* prim)
    :m_prim(prim)
{

};

ProtoGSecondaryItem::~ProtoGSecondaryItem()
{

};




/**
   ProtoPanel
*/
ProtoPanel::ProtoPanel(ProtoScene* s):
    m_panel_prop(0),
    m_s(s)
{

};

ProtoPanel::~ProtoPanel()
{

};

void ProtoPanel::setOContext(OutlineContext c)
{
    m_ocontext = c;
};


void ProtoPanel::registerGI(topic_ptr it1, ProtoGItem* gi, GITEMS_VECTOR* registrator)
{
	auto it = it1->shortcutUnderlying();
#ifdef _DEBUG
    if(!m_multi_G_per_item){
        ITEM_2_GITEM::iterator i = m_it2gi.find(it);
        if(i != m_it2gi.end())
            {
                it->setDebugMark1(true);
                qDebug() << "ERROR - duplicate items in outline." << it << it->dbgHint();
                //ASSERT(0, "ERROR - duplicate items in outline.") << it << it->dbgHint();
            }
    }
#endif

    m_it2gi.insert(std::make_pair(it, gi));
    if (registrator) {
        registrator->push_back(gi);
    }
    else {
        m_outlined.push_back(gi);
    }
};


ProtoGItem* ProtoPanel::findGI(topic_ptr it)
{
    ITEM_2_GITEM::iterator i = m_it2gi.find(it);
    if(i != m_it2gi.end())
        {
            return i->second;
        }
    return nullptr;
};

//ProtoGItem* ProtoPanel::selectNext(bool go_up, ProtoGItem* gi)
//{/
//    //....
//
//    return rv;
//}

void ProtoPanel::shiftDown(ProtoGItem* gi, qreal dy)
{
    auto i = std::find(m_outlined.begin(), m_outlined.end(), gi);
    if (i == m_outlined.end()) {
        ASSERT(0, "failed to locate GItem");
        return;
    }
    for (; i != m_outlined.end(); i++) {
        auto pg = *i;
        auto g = pg->g();
        g->moveBy(0, dy);
    }
};

QRectF ProtoPanel::boundingRectInSceneCoord()const
{
    QRectF rc(0,0,0,0);
    if(m_outlined.size() > 0)
        {
            GITEMS_VECTOR::const_iterator i = m_outlined.begin();
            const ProtoGItem* g = *i;
            rc.setTopLeft(g->g()->mapToScene(g->g()->boundingRect().topLeft()));

            GITEMS_VECTOR::const_reverse_iterator j = m_outlined.rbegin();
            g = *j;
            rc.setBottomRight(g->g()->mapToScene(g->g()->boundingRect().bottomRight()));
        }
    return rc;
};

void ProtoPanel::freeItems()
{
    for (GITEMS_VECTOR::iterator i = m_outlined.begin(); i != m_outlined.end(); i++)
    {
        ProtoGItem* g = *i;
        m_s->s()->removeItem(g->g());
        delete (g);
    }
    FREE_GITEM(m_vline1);
    FREE_GITEM(m_vline2);
    FREE_GITEM(m_hint_item);
    FREE_GITEM(m_check_select_vline);
    FREE_GITEM(m_header_line);
    clear();
};

void ProtoPanel::clear()
{
    m_vline1 = m_vline2 = m_check_select_vline = nullptr;
    m_header_line = nullptr;
    m_hint_item = nullptr;

    m_it2gi.clear();
    m_outlined.clear();
    m_aux2.clear();
};

bool ProtoPanel::isLastInOutline(ProtoGItem* gi)
{
    bool rv = false;
    auto i = m_outlined.rbegin();
    if (i != m_outlined.rend()) {
        rv = (*i == gi);
    }

    /*
    GITEM_2_IDX::iterator i = m_g2outline_idx.find(gi);
    if(i != m_g2outline_idx.end())
        {
            int idx = i->second;
            if(idx == (int)m_outlined.size() - 1)
                rv = true;
        }  
        */
    return rv;
};

void ProtoPanel::setPanelWidth(qreal val)
{
    m_panelWidth = val;
}

void ProtoPanel::clearAllProp() 
{ 
    m_panel_prop = 0; 
}

void ProtoPanel::setProp(std::set<EProp>* prop2set, std::set<EProp>* prop2remove /*= nullptr*/) 
{
    if (prop2remove) {
        for (auto& p : *prop2remove) {
            m_panel_prop &= ~(p);
        }
    }

    if (prop2set) {
        for (auto& p : *prop2set) {
            m_panel_prop |= p;
        }
    }

    m_selector_control = hasProp(ProtoPanel::PP_CurrSelect) ||
        hasProp(ProtoPanel::PP_CurrMoveTarget) ||
        hasProp(ProtoPanel::PP_CurrDownload) ||
        hasProp(ProtoPanel::PP_CurrDelete) ||
        hasProp(ProtoPanel::PP_CurrOpen) ||
        hasProp(ProtoPanel::PP_CurrFindInShell) ||
        hasProp(ProtoPanel::PP_CurrEdit);
};


#define RUN_ALL_PROP_TABLE     \
    ADD_P(PP_ActionButton);                     \
    ADD_P(PP_Annotation);                       \
    ADD_P(PP_InplaceEdit);                      \
    ADD_P(PP_RTF);                              \
    ADD_P(PP_DnD);                              \
    ADD_P(PP_CurrSelect);                       \
	ADD_P(PP_Thumbnail);                        \
    ADD_P(PP_Filter);                           \
    ADD_P(PP_CurrMoveTarget);                   \
    ADD_P(PP_ToDoColumn);                       \
    ADD_P(PP_CurrSpot);                         \
    ADD_P(PP_MultiLine);                        \
    ADD_P(PP_ExpandNotes);                      \



ProtoPanel::PANEL_PROPERTIES ProtoPanel::allProperties()
{
    PANEL_PROPERTIES pp;
#define ADD_P(P) pp.insert(P);
    RUN_ALL_PROP_TABLE;
#undef ADD_P
    return pp;
};

void ProtoPanel::showHint(QString text, const QPointF& pt)
{
    if(m_hint_item)
        {
            m_hint_item = new QGraphicsTextItem();
            m_hint_item->setFont(*utils::defaultBoldFont());
            m_hint_item->setDefaultTextColor(color::True1Red);
            s()->s()->addItem(m_hint_item);
        }
    m_hint_item->setPlainText(text);
    m_hint_item->setPos(pt);  

    m_hint_item->setEnabled(true);
    m_hint_item->setVisible(true);
    m_hint_item->update();
};

void ProtoPanel::hideHint()
{
    if(m_hint_item)
        {
            m_hint_item->setEnabled(false);
            m_hint_item->setVisible(false);  
        }
};

void ProtoPanel::createAuxItems()
{
    QPen pen4vline(COLOR_PANEL_SEP);
    bool hasHeader = false;
    if (hasProp(PP_CheckSelectBox))
    {
        m_check_select_vline = s()->s()->addLine(0, 0, 0, 0, pen4vline);
        m_check_select_vline->setEnabled(false);
        m_check_select_vline->setZValue(ZVALUE_FRONT);
        hasHeader = true;
    }

    if (hasHeader) {
        m_header_line = s()->s()->addLine(0, 0, 0, 0, pen4vline);
        m_header_line->setEnabled(false);
        m_header_line->setZValue(ZVALUE_FRONT);
    }
};

qreal ProtoPanel::textColumnWidth()const
{
    qreal rv = panelWidth() - 3 * (int)gui::lineHeight();
    return rv;
};

HierarchyBranchMark* ProtoPanel::createHierarchyBranchMark()
{
    return new HierarchyBranchMark(this);
};

void ProtoPanel::updateAuxItemsPos()
{
    qreal h = calcHeight();
    int top4line = 0;

    //bool hasHeader = false;
    if (!m_outlined.empty()) {      
        ProtoGItem* g = m_outlined[0];
        if(g->asHeaderTopic()){
            QPointF br = g->g()->mapToScene(g->g()->boundingRect().bottomRight());
            top4line = br.y();
        }
    }

    /*
    if(hasProp(PP_VLines))
        {       
            qreal x_pos = m_panelIdent + panelWidth() - gui::lineHeight();
            if (m_vline1) {
                m_vline1->setLine(x_pos, top4line, x_pos, h);
            }
            if (m_vline2) {
                x_pos -= gui::lineHeight();
                x_pos -= 1;
                m_vline2->setLine(x_pos, top4line, x_pos, h);
            }
        }*/

    if (hasProp(ProtoPanel::PP_CheckSelectBox))
        {
            //  hasHeader = true;
            assert_return_void((m_check_select_vline != nullptr), "expected V-lines");
            m_check_select_vline->setLine(gui::lineHeight(),
                                          top4line,
                                          gui::lineHeight(),
                                          h);
        }

    if (m_header_line) {
        if (top4line > 0) {
            m_header_line->setLine(0, top4line, m_panelIdent + panelWidth(), top4line);
            m_header_line->setVisible(true);
        }
        else {
            m_header_line->setVisible(false);
        }
    }

    //** aux2 - is some extra derived items
    for(AUX2_ITEMS::iterator i = m_aux2.begin();i != m_aux2.end();i++)
        {
            CurrentMark* cm = *i;
            cm->reset();
        }

    updateHudItemsPos();
};

void ProtoPanel::updateHudItemsPos()
{
    /*
      QGraphicsView* v2 = s()->v()->v();

      if(hasProp(PP_HudPanel))
      {
      QPoint pt(0, 0);
      QPointF pt2 = v2->mapToScene(pt);   
      qreal y_pos = pt2.y();

      
      GITEMS_VECTOR::iterator i = m_outlined.begin();
      if(m_topScrolled > 0)
      {
      while(y_pos < m_topScrolled)
      {
      ProtoGItem* g = *i;
      g->g()->setEnabled(false);
      g->g()->setVisible(false);
      y_pos += g->g()->boundingRect().height();
      i++;
      if(i == m_outlined.end())
      {
      ASSERT(0, "invalid index logic");
      break;
      }
      }
      }

      y_pos = pt2.y();//start from top
      for(;i != m_outlined.end();i++)
      {
      ProtoGItem* g = *i;

      if(!g->g()->isEnabled())
      {
      g->g()->setEnabled(true);
      g->g()->setVisible(true);
      }
      g->g()->setPos(panelIdent(), y_pos);
      y_pos += g->g()->boundingRect().height();
      }      
      }
    */
};

qreal ProtoPanel::calcHeight()
{
    QRectF rc = s()->enabledItemsBoundingRect();
    qreal h = rc.height();

    QGraphicsView* v = s()->v()->v();
    QPoint pt(v->viewport()->width(), 
              v->viewport()->height());
    QPointF pt_scene = v->mapToScene(pt);
    if(pt_scene.y() > h)
        {
            h = pt_scene.y();
        }

    return h;
};

void ProtoPanel::resetGeometry()
{
    for (GITEMS_VECTOR::iterator i = m_outlined.begin(); i != m_outlined.end(); i++)
    {
        ProtoGItem* g = *i;
        g->resetGeometry();
    }
    resetPosAfterGeometryReset();
};

void ProtoPanel::resetPosAfterGeometryResetForOutlines()
{
    if (!m_outlined.empty())
    {
        GITEMS_VECTOR::iterator i = m_outlined.begin();
        ProtoGItem* g = *i;
        qreal y_pos = g->g()->pos().y();
        y_pos += g->g()->boundingRect().height();
        i++;
        for (; i != m_outlined.end(); i++)
        {
            g = *i;
            qreal x_pos = g->g()->pos().x();
            g->g()->setPos(x_pos, y_pos);
            y_pos += g->g()->boundingRect().height();
            //g->resetGeometry();
        }
    }
};

void ProtoPanel::applyParamMap()
{
    std::set<ProtoPanel::EProp> p2set, p2rem;

    const PANEL_PROPERTIES& ppOUT = s()->getOUTproperties();
    for(PANEL_PROPERTIES::const_iterator i = ppOUT.begin(); i != ppOUT.end();i++)
        {
            ProtoPanel::EProp p2 = *i;
            p2rem.insert(p2);
            //SET_PPP(p2rem, p2);
            //clearProp(p);
        }  

    const PANEL_PROPERTIES& ppIN = s()->setINproperties();
    for(PANEL_PROPERTIES::const_iterator i = ppIN.begin(); i != ppIN.end();i++)
        {
            ProtoPanel::EProp p2 = *i;
            p2set.insert(p2);
            //SET_PPP(p2set, p2);
            //addProp(p);
        }
    setProp(&p2set, &p2rem);
};

QString ProtoPanel::shortTitle(QString s)
{
    QString rv = "";
    if (s.length() > 10)
    {
        rv = s.left(8) + "..";
    }
    else
    {
        rv = s;
    }
    return rv;
};


void ProtoPanel::resetClassicOutlinerGeometry()
{
//    qreal ypos = 0.0;
    if (m_outlined.size() > 0)
    {
        GITEMS_VECTOR::iterator i = m_outlined.begin();
        ProtoGItem* g = *i;
        g->resetGeometry();
        QPointF pt = g->g()->pos();
        qreal ypos = pt.y();//ykh!
        ypos += g->g()->boundingRect().height();
        pt.setY(ypos);
        i++;
        for (; i != m_outlined.end(); i++)
        {
            g = *i;
            g->g()->setPos(pt);
            g->resetGeometry();
            ypos += g->g()->boundingRect().height();
            pt.setY(ypos);
        }
    }
};

int ProtoPanel::actionBtnWidth() 
{
    if (m_action_btn_with == -1) {
        m_action_btn_with = utils::calcWidth(actionBtnText(), ard::defaultSmallFont());
        m_action_btn_with += 10;
    }
    return m_action_btn_with;
};

QString ProtoPanel::actionBtnText() 
{
    return "done";
};


ProtoGItem* ProtoPanel::produceOutlineItems(topic_ptr it, 
    const qreal& x_ident, 
    qreal& y_pos,
    GITEMS_VECTOR* registrator /*= nullptr*/)
{
    int outline_ident = (int)x_ident;
    anGItem* gi = new anGItem(it, this, outline_ident);
    gi->setPos(panelIdent(), y_pos);
    y_pos += gi->boundingRect().height();
    registerGI(it, gi, registrator);
    return gi;
}

#ifdef _DEBUG
std::pair<int, int> ProtoPanel::dbgPrint(QString prefix_label)
{
    static int call_counter = 1;
    //qDebug() << "<<====== proto-print =====";
    qDebug() << "pp-print" << this << prefix_label << call_counter++
            << "it2gi.size=" << m_it2gi.size() 
            << "outlined.size=" << m_outlined.size();
    std::pair<int, int> rv;
    rv.first = m_it2gi.size();
    rv.second = m_outlined.size();
    return rv;
};
#endif

/**
   HudButton
*/
HudButton::HudButton(QObject* owner, E_ID _id, QString _label, QString imageResource)
    :m_owner(owner),
     m_id(_id), 
     m_label(_label), 
     m_imageRes(imageResource),
     m_data(0),
     m_type(EHudType::Button),
     m_force_width(0)
{
    setupButton();
};

HudButton::HudButton(QObject* owner, QString _label, int _data, QString imageResource)
    :m_owner(owner),
     m_id(idUnknown),
     m_label(_label), 
     m_imageRes(imageResource),
     m_data(_data),
     m_type(EHudType::Button),
     m_force_width(0)
{
    setupButton();
};

HudButton::HudButton(QObject* owner, EHudType t, QString _label, E_ID _id, int _data, int _width)
    :m_owner(owner),
     m_id(_id), 
     m_label(_label), 
     m_imageRes(""),
     m_data(_data),
     m_type(t),
     m_force_width(_width)
{
    setupButton();
};


#define HUD_IMG_ONLY_SIZE 2 * gui::lineHeight()
#define HINT_ICO_WIDTH 32

void HudButton::setupButton()
{
    m_asImageOnly = false;
    qreal w = gui::lineHeight();
    qreal h = 1.5 * gui::lineHeight();

    static int MaxHintWidth = 400;
    static bool firstCall = true;
    if(firstCall)
        {
            QRect rcscreen = QApplication::desktop()->screenGeometry();
            int smin = rcscreen.width();
            if(smin > rcscreen.height())
                smin = rcscreen.height();

            smin -= 100;

            if(MaxHintWidth > smin)
                MaxHintWidth = smin;
            firstCall = false;
        }

    if (m_force_width > 0)
        {
            MaxHintWidth = m_force_width;
        }

    switch(hudType())
        {
        /*
        case EHudType::Hint:
            {
                if(!m_label.isEmpty())
                    {
                        h += HINT_ICO_WIDTH;
                        w = utils::calcWidth("one two three four five", utils::defaultBoldFont());
                        if(w > MaxHintWidth)
                            {
                                QRect rc2(0,0,MaxHintWidth, (int)h);
                                QFontMetrics fm(*utils::defaultBoldFont());
                                QRect rc3 = fm.boundingRect(rc2, Qt::TextWordWrap, m_label);
                                w = MaxHintWidth + ARD_MARGIN;
                                h = rc3.height() + HINT_ICO_WIDTH;
                                //w = MAX_HEIGHT;
                            }
                    }   
            }break;
            */
        case EHudType::Button:
            {
                if(!m_label.isEmpty())
                    {
                        w = utils::calcWidth(m_label, ard::defaultFont()) + 4*ARD_MARGIN;
                    }
                if(!m_imageRes.isEmpty())
                    {
                        if(m_label.isEmpty())
                            {
                                m_asImageOnly = true;
                                w = h = HUD_IMG_ONLY_SIZE;//2 * gui::lineHeight();
                            }
                        else
                            {
                                w += gui::lineHeight() + ARD_MARGIN;
                            }
                    }   
            }break;
        case EHudType::Break:h = 0;break;
        case EHudType::LineBreak:
        case EHudType::Label:
            {
                h = gui::lineHeight();
            }break;
        }

    setRect(0,0,w,h);
    setFlags(QGraphicsItem::ItemIgnoresTransformations);
    setZValue(ZVALUE_FRONT);
};

void HudButton::setHeight(int h)
{
    prepareGeometryChange();
    qreal w = boundingRect().width();
    setRect(0,0,w,h);
};

void HudButton::setWidth(int w)
{
    prepareGeometryChange();
    qreal h = boundingRect().height();
    setRect(0,0,w,h);
};


void HudButton::setMinSize(QSize sz)
{
    prepareGeometryChange();
    setRect(0,0,sz.width(),sz.height());
};

void HudButton::setImageStyleSize()
{
    m_asImageOnly = true;
    int w = HUD_IMG_ONLY_SIZE;//2 * gui::lineHeight();
    setMinSize(QSize(w, w));
};

void HudButton::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
{
    switch(hudType())
        {
        case EHudType::Button:
            {
                drawAsButton(painter);
            }break;
        case EHudType::LineBreak:
            {
                drawAsLine(painter);
            }break;
        case EHudType::Label:
            {
                drawAsLabel(painter);
            }break;
        case EHudType::Break:break;
        }
};

void HudButton::drawAsButton(QPainter *painter)
{
    QRectF rc = boundingRect();

    PGUARD(painter);
    painter->setRenderHint(QPainter::Antialiasing);

    if (!m_imageRes.isEmpty())
    {
        qreal img_w = gui::lineHeight();
        if (m_asImageOnly)
        {
            img_w = rc.height();
        }

        QPixmap pm;
        pm.load(m_imageRes);
        QPointF pt = rc.topLeft();
        if (m_label.isEmpty())
        {
            qreal dx = (rc.width() - img_w) / 2;
            pt.setX(pt.x() + dx);
        }

        QPen pen(Qt::gray);
        pen.setWidth(2);
        painter->setPen(pen);
        painter->drawEllipse(rc);
        painter->drawPixmap(pt.x(), pt.y(), img_w, img_w, pm);
        rc.setLeft(rc.left() + img_w + ARD_MARGIN);
    }

    if (!m_label.isEmpty())
    {
        QRectF rc_hotspot = rc;
        rc_hotspot.setLeft(rc_hotspot.left() + ARD_MARGIN);
        rc_hotspot.setRight(rc_hotspot.right() - ARD_MARGIN);
        QPainterPath r_path;
        r_path.addRoundedRect(rc_hotspot, 5, 5);
        painter->setPen(model()->penGray());
        QColor clr = default_palette.color(QPalette::Button);
        QBrush brush(clr);
        painter->setBrush(brush);
        painter->drawPath(r_path);
        QPen penText(color::invert(clr.rgb()));
        painter->setPen(penText);
        painter->setFont(*ard::defaultFont());
        painter->drawText(rc_hotspot, Qt::AlignVCenter | Qt::AlignHCenter, m_label);
    }
};

void HudButton::drawAsLine(QPainter *painter)
{
    QRectF rc = boundingRect();
    PGUARD(painter);
    painter->setRenderHint(QPainter::Antialiasing);
    QColor cl = painter->background().color();
    if(cl.black() == 255)    
        cl = QColor(80,80,80);    
    else    
        cl = cl.darker(200);

    QPen pen(cl, 3);
    painter->setPen(pen);  
    qreal m = (rc.top() + rc.bottom()) / 2;
    painter->drawLine((int)rc.left(), (int)m, (int)rc.right(), (int)m);
};


void HudButton::drawAsLabel(QPainter *painter)
{
    QRectF rc = boundingRect();
    PGUARD(painter);
    QColor clr = default_palette.color(QPalette::Button);
    QBrush brush(clr);
    painter->setBrush(brush);
    painter->setPen(clr);
    painter->drawRect(rc);

    painter->setRenderHint(QPainter::Antialiasing);
    QColor cl = painter->background().color();
    if(cl.black() == 255)    
        cl = QColor(80,80,80);    
    else    
        cl = Qt::black;

    QPen pen(cl, 3);
    painter->setPen(pen);
    painter->setFont(*utils::defaultBoldFont());
    painter->drawText(rc, Qt::AlignVCenter, m_label);
};

bool HudButton::preprocessMouseEvent()
{
    if (m_owner)
    {
        QMetaObject::invokeMethod(m_owner, "outlineHudButtonPressed",
            Qt::QueuedConnection,
            Q_ARG(int, m_id),
            Q_ARG(int, m_data));
    }
    return true;
};


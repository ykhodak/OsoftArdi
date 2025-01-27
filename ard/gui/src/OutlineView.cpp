#include <QKeyEvent>
#include <QMenu>
#include <OutlineSceneBase.h>
#include <QScrollBar>
#include <QLabel>
#include <QBitmap>
#include <QGridLayout>
#include <QScroller>
#include <QGestureEvent>
#include <QApplication>

#include "OutlineView.h"
#include "anfolder.h"
#include "ansyncdb.h"
#include "anGItem.h"
#include "ardmodel.h"
#include "contact.h"
#include "kring.h"
#include "TablePanel.h"
#include "OutlineTitleEdit.h"
#include "ProtoTool.h"
#include "OutlineScene.h"
#include "ansearch.h"
#include "popup-widgets.h"
#include "rule_runner.h"
#include "small_dialogs.h"

OutlineView::OutlineView(QWidget *parent) :
    ArdGraphicsView(parent),
    m_title_edit(nullptr),
    m_enableHScrollInClassicOutline(false),
    m_selectRequestOnRebuild(0)  
{
    m_invalidWithClosedDB = true;

    m_sceneBuilder = parent;

    connect(verticalScrollBar(), SIGNAL(valueChanged(int)),
            this, SLOT(verticalScrollValueChanged(int)));
    connect(horizontalScrollBar(), SIGNAL(valueChanged(int)),
            this, SLOT(horizontalScrollValueChanged(int)));
    setViewportUpdateMode(QGraphicsView::FullViewportUpdate);///this will probably slow down drawing but needed for scenes with custom(grid) backgrounds
    grabGesture(Qt::PinchGesture);
	setupKineticScroll(ArdGraphicsView::scrollKineticNone);
}

OutlineView::~OutlineView()
{
};

void OutlineView::setupSelectRequestOnRebuild(topic_ptr it)
{
    if (IS_VALID_DB_ID(it->id()))
    {
        m_selectRequestOnRebuild = it->id();
    }
}

bool OutlineView::wasSelectOnRebuildRequested() 
{
    return (m_selectRequestOnRebuild != 0);
};

void OutlineView::clearSelectRequestOnRebuild()
{
    m_selectRequestOnRebuild = 0;
}

void OutlineView::resetGItemsGeometry()
{
    OutlineSceneBase* sc = base_scene();
    if (sc->hasPanel()){
        ProtoPanel* p = sc->panel();
        p->resetGeometry();
    }
};

void OutlineView::rebuild()
{
    OutlineSceneBase* bs = base_scene();
    topic_ptr locateItem = nullptr;
    bool look4underlying_shortcut = false;

    bool setFocusOnSelected = false;
    if(m_selectRequestOnRebuild == 0)
        {
            ProtoGItem* sel_g = bs->currentGI(false);
            if(sel_g){
                locateItem = sel_g->topic();
                if (locateItem) {
                    auto f2 = locateItem->shortcutUnderlying();
                    if (f2 != locateItem) {
                        locateItem = f2;
                        look4underlying_shortcut = true;
                    }
                    //locateItem = locateItem->shortcutUnderlying();                    
                }
                setFocusOnSelected = sel_g->g()->hasFocus();
            }
        }
  
    if (locateItem) {
        LOCK(locateItem);
    }

    bs->rebuild();  
    if(m_selectRequestOnRebuild != 0)
        {
            if (locateItem) {
                locateItem->release();
                locateItem = nullptr;
            }

            ///@todo: here should be code to locate contact
            if(m_selectRequestOnRebuild == 0){
                    locateItem = model()->selectedHoistedTopic();
                }
            else{
                locateItem = dbp::defaultDB().lookupLoadedItem(m_selectRequestOnRebuild);
                }

            if (locateItem) {
                LOCK(locateItem);
            }
        }
    else {
        EOutlinePolicy p = base_scene()->outlinePolicy();
        switch (p) {
        case outline_policy_KRingTable:
        {
            DB_ID_TYPE cid_sel = dbp::configLastSelectedKRingKeyOID();
            if (cid_sel != 0 && cid_sel != 0) {
                if (locateItem) {
                    locateItem->release();
                    locateItem = nullptr;
                }
                locateItem = ard::lookupAs<ard::KRingKey>(cid_sel);
                if (locateItem) {
                    LOCK(locateItem);
                }
            }

            //ASSERT(0, "NA");
        }break;
        default:
            break;
        }
    }

    bool proceedResetSceneRect = true;
    if(locateItem){
            ProtoGItem* g = bs->findGItem(locateItem);
            if (!g) {
                if (look4underlying_shortcut) {
                    g = bs->findGItemByUnderlying(locateItem);
                }
            }

            if(!g)
                {
                    //*** parent could be collapsed right before the rebuild
                    auto p = locateItem->parent();
                    while(p && 
                          p->parent() &&
                          !p->parent()->isExpanded())
                        {
                            p = p->parent();
                        }
                    if(p)
                        {
                            g = bs->findGItem(p);

                            if(!g)
                                {
                                    //*** still it is possible that 2 levels up are collapsed
                                    p = p->parent();
                                    while(p)
                                        {
                                            g = bs->findGItem(p);
                                            if(g)break;         
                                            p = p->parent();
                                        }
                                }
                        }

                }

            if(m_selectRequestOnRebuild != 0)
                {
                    clearSelectRequestOnRebuild();
                }

            if(g)
                {
                    bs->resetSceneBoundingRect();
					bs->selectGI(g, false);//ykh123
                    g->g()->ensureVisible(QRectF(0,0,1,1));
                    if(setFocusOnSelected)
                        g->g()->setFocus();
                }

            if (locateItem) {
                locateItem->release();
                locateItem = nullptr;
            }
        }//locateItem

    if(proceedResetSceneRect){
        base_scene()->resetSceneBoundingRect();
    }

};

extern bool g_ignore_vscroll;
void OutlineView::verticalScrollValueChanged(int )
{
	OutlineSceneBase* bs = base_scene();
    bs->updateAuxItemsPos();

	if (!bs->isMainScene())
		return;

	if (g_ignore_vscroll) {
		//qDebug() << "vscroll-ignored";
		g_ignore_vscroll = false;
		return;
	}
    auto v = verticalScrollBar();
    if (v) {
        if (v->value() >= v->maximum()) 
        {
            auto pol = gui::currPolicy();
            if (pol == outline_policy_PadEmail)
            {
                if (!globalTextFilter().isActive())
                {
                    auto rr = ard::db()->gmail_runner();
                    if (rr)
                    {
						qDebug() << "vscroll-accepted" << v->value() << v->maximum();
						rr->runBackendTokenQ();						
                    }
                }
            }
        }
    }
};

void OutlineView::horizontalScrollValueChanged(int )
{
    base_scene()->updateAuxItemsPos();
};


void OutlineView::mousePressEvent(QMouseEvent *event)
{
    m_select_processed = false;
    ///ykh-testing
    if (!gui::isDBAttached() && m_invalidWithClosedDB){
		ard::files_dlg::showFiles();
        return;
    }

    OutlineSceneBase* bs = base_scene();
    QPointF pt_Event = mapToScene(event->pos());

    QGraphicsItem *gi = itemAt(event->pos());
    if (!gi) {
        ProtoGItem* f = bs->footer();
        if (f) {
            base_scene()->selectGI(f);          
            if (event->button() == Qt::LeftButton && f->preprocessMousePress(pt_Event, true))
            {
                return;
            }
        }
    }

    if (gi) {

#define RETURN_ON_PREPROCESS(T)    if(true){        \
        T* o = dynamic_cast<T*>(gi);                \
        if(o && o->preprocessMouseEvent())return;   \
    }                                               \

#define CHECK_TYPE_RETURN_ON_PREPROCESS(T)    if(true)              \
        {                                                           \
            T* g = dynamic_cast<T*>(gi);                            \
            if(g)                                                   \
                {                                                   \
                    ProtoGItem* proto_GI  = g;              \
                    bs->selectGI(proto_GI);                 \
                    if(g->preprocessMousePress(pt_Event, false))\
                        return;                             \
                }                                                   \
        }                                                           \


        if (event->button() == Qt::LeftButton) 
        {
            auto g1 = dynamic_cast<anGItem*>(gi);
            if (g1) {
                if (g1->preselectPreprocessMousePress(pt_Event)) {
                    m_select_processed = true;
                    return;
                }
            }

            RETURN_ON_PREPROCESS(HudButton);
            RETURN_ON_PREPROCESS(CurrentMarkButton);
            CHECK_TYPE_RETURN_ON_PREPROCESS(ProtoToolItem);
        }

#undef RETURN_ON_PREPROCESS
#undef CHECK_TYPE_RETURN_ON_PREPROCESS
    }

    m_ptMousePressPos = event->screenPos();

    QGraphicsView::mousePressEvent(event);
}

void OutlineView::mouseReleaseEvent(QMouseEvent * e) 
{
    if (!m_select_processed)
    {
        bool dnd = e->buttons() & Qt::LeftButton &&
            QLineF(e->screenPos(), m_ptMousePressPos).length() > QApplication::startDragDistance();

        if (!dnd) {
            OutlineSceneBase* bs = base_scene();
            bs->clearMSelected();
        }
    }
    QGraphicsView::mouseReleaseEvent(e);
};

OutlineSceneBase* OutlineView::base_scene()
{
    OutlineSceneBase* bs = dynamic_cast<OutlineSceneBase*>(scene());
    ASSERT(bs, "expected valid OutlineSceneBase object");
    return bs;
};

const OutlineSceneBase* OutlineView::base_scene()const
{
    const OutlineSceneBase* bs = dynamic_cast<const OutlineSceneBase*>(scene());
    ASSERT(bs, "expected valid OutlineSceneBase object");
    return bs;
};

/*
ArdGraphicsView::EScrollType OutlineView::kscroll()const
{
    ArdGraphicsView::EScrollType rv = scrollKineticVerticalOnly;  
    return rv;
};*/

void OutlineView::scrollContentsBy(int dx, int dy)
{
    if (!m_enableHScrollInClassicOutline)
        dx = 0;//no h-scroll

    QGraphicsView::scrollContentsBy(dx, dy);
    base_scene()->updateHudItemsPos();
};

void OutlineView::resizeEvent( QResizeEvent * e )
{
    base_scene()->resetSceneBoundingRect();
    QGraphicsView::resizeEvent(e);
};


void OutlineView::delayedResetSceneBoundingRect()
{
    OutlineSceneBase* bs = base_scene();
    bs->updateAuxItemsPos();
    ArdGraphicsView::delayedResetSceneBoundingRect();
};


void OutlineView::setEnableHScrollInClassicOutline(bool val)
{
    m_enableHScrollInClassicOutline = val;
};

void OutlineView::renameSelected(EColumnType field2edit ,
                                 const QPointF* ptClick, bool selectContent)
{
    OutlineSceneBase* bs = base_scene();
    if(!m_title_edit)
        {
            m_title_edit = new OutlineTitleEdit(this);
            connect(m_title_edit, SIGNAL(onEditDetached()),
                    this, SLOT(onTitleEditDetached()));
        }

    ProtoGItem* sel_g = bs->currentGI();
    if(sel_g){
            ProtoPanel* pnl = sel_g->p();
            if(!pnl->hasProp(ProtoPanel::PP_InplaceEdit))
                {
					dbg_print(QString("read-only panel %1 %2").arg(pnl->pname()).arg("can't change title"));
                    return;
                }

            auto it = sel_g->topic();
            assert_return_void(it, "expected topic");
            if (field2edit == EColumnType::Annotation) {
                if (!it->canHaveAnnotation()) {
                    qDebug() << "can't annotate item" << it->title();
                    return;
                }
            }
            else {
                if (!it->canRenameToAnything()){
                    qDebug() << "read-only item, can't change title" << it->title();
                    return;
                }
            }

            bool edit_empty_annotation = false;
            if (field2edit == EColumnType::Annotation) 
            {
                auto s = it->annotation().trimmed();
                if (s.isEmpty()) {
                    qDebug() << "<<<<==== we might want to force recalc for annotation";
                    //pnl->shiftDown(sel_g, gui::lineHeight());
                    //annotation_edit_requester = it;
                    edit_empty_annotation = true;
                    //rebuild();
                }
            }

            field2edit = it->treatFieldEditorRequest(field2edit);
            QString type_label = it->formValueLabel();

            QRectF rcF;
            sel_g->getOutlineEditRect(field2edit, rcF);
            if (edit_empty_annotation) {
                if (rcF.height() < 1) {
                    rcF.setHeight(gui::lineHeight());
                }
            }
                QPolygon plg = mapFromScene(rcF);
                QRect rc = plg.boundingRect();
                QPointF tlScene = rcF.topLeft();
                QPoint topLeftItem =  mapFromScene(tlScene);
                m_title_edit->attachEditorTopic(it, field2edit, type_label);
                bs->hideCurrMarks();
                m_title_edit->showEditWithSuggestedGeometry(rc, topLeftItem);

                if(ptClick){
                    QPointF ptF = mapFromScene(*ptClick);
                    QPoint pt((int)ptF.x(), (int)ptF.y());
                    pt = mapToGlobal(pt);
                    m_title_edit->setupEditTextCursor(pt);
                }
                
                if (selectContent) {
                    m_title_edit->selectAllContent();
                }

            updateTitleEditFramePos();

            if (edit_empty_annotation) {
                pnl->shiftDown(sel_g, gui::lineHeight());
            }
        }
}

bool OutlineView::isInTitleEditMode()const 
{
    bool rv = (m_title_edit && m_title_edit->isEnabled() && m_title_edit->isVisible());
    return rv;

}

void OutlineView::updateTitleEditFramePos()
{
    QRectF rc_frame;
    if(m_title_edit && m_title_edit->hasFieldEditor())
        {
            QSizeF sz(viewport()->width(), viewport()->height());
            QRect rce = m_title_edit->geometry();
            int dy = rce.bottom() - sz.height();

            if(dy > 0)
                {
                    QPoint pte_pos = m_title_edit->pos();
                    pte_pos.setY(pte_pos.y() - dy);
                    m_title_edit->move(pte_pos);
                }

            OutlineSceneBase* bs = base_scene();

            QRect rc = m_title_edit->geometry();
            QPolygonF plg = mapToScene(rc);
            rc_frame = plg.boundingRect();
            rc_frame.setLeft(rc_frame.left() - 3);
            rc_frame.setTop(rc_frame.top() - 3);            
            bs->showEditFrame(true, rc_frame, m_title_edit->columnType());
        }
    else {
        OutlineSceneBase* bs = base_scene();
        bs->showEditFrame(false);
    }
};


void OutlineView::onTitleEditDetached()
{
    OutlineSceneBase* bs = base_scene();
    bs->showEditFrame(false);
};

void OutlineView::emit_inDerived_onResetSceneBoundingRect()
{
    emit_onResetSceneBoundingRect();
};

void OutlineView::focusInEvent(QFocusEvent *e) 
{
    ArdGraphicsView::focusInEvent(e);
};

void OutlineView::focusOutEvent(QFocusEvent *e) 
{
    ArdGraphicsView::focusOutEvent(e);
};

/**
scene_view
*/
scene_view::~scene_view()
{
    if (scene) {
        scene->detachGui();
    }
};

scene_view::ptr scene_view::create_with_builder(std::function<void(OutlineScene* s)> builder,
	std::set<ProtoPanel::EProp> prop, 
	QWidget *parent,
	OutlineContext enforced_ocontext)
{
	scene_view::ptr rv(new scene_view);
	rv->view = new OutlineView(parent);
	auto s = new OutlineScene(rv->view);
	rv->scene = s;
	rv->scene_builder = builder;

	if (enforced_ocontext != OutlineContext::none) {
		rv->scene->set_enforced_ocontext(enforced_ocontext);
	}

	ProtoPanel::PANEL_PROPERTIES ppOUT = ProtoPanel::allProperties();
	rv->scene->assignGetOUTproperties(ppOUT);
	ProtoPanel::PANEL_PROPERTIES ppIN = prop;
	ppIN.insert(ProtoPanel::PP_CurrSpot);
	rv->scene->assignSetINproperties(ppIN);
	//rv->scene->setOutlinePolicy(p);
	if (gui::isDBAttached()) {
		rv->scene->attachHoisted(dbp::root());
	}
	rv->run_scene_builder();
	/*if (builder) {
		builder(s);
	}*/
	//rv->view->rebuild();
	return rv;
};

void scene_view::run_scene_builder() 
{
	if (scene_builder) {
		auto s = dynamic_cast<OutlineScene*>(scene);
		if (s) {
			scene_builder(s);
			s->resetSceneBoundingRect();
		}
	}
};
scene_view::ptr scene_view::create(QWidget *parent) 
{
    scene_view::ptr rv(new scene_view);
    rv->view = new OutlineView(parent);
    rv->scene = new OutlineSceneBase(rv->view);
    return rv;
};

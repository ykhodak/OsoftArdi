#include <QScrollBar>
#include "anfolder.h"
#include "custom-g-items.h"
#include "OutlineSceneBase.h"
#include "OutlineView.h"
#include "OutlinePanel.h"
#include "anGItem.h"
#include "ardmodel.h"
#include "TablePanel.h"
#include "OutlineMain.h"
#include "ProtoTool.h"

/**
   OutlineSceneBase
*/

ProtoView* OutlineSceneBase::v(){return m_view;};
const ProtoView* OutlineSceneBase::v()const{return m_view;};

OutlineSceneBase::OutlineSceneBase(OutlineView* _view):
    m_include_root_in_outline(false), 
    m_view(_view)
{
    m_view->setScene(this);
  //  connect(this, SIGNAL(selectionChanged()),
  //          this, SLOT(gitemSelected()));
};

OutlineSceneBase::~OutlineSceneBase()
{
    detachGui();
};

void OutlineSceneBase::detachGui()
{
    freePanels();
    detachHoisted();
};


void OutlineSceneBase::clearOutline()
{
    ProtoScene::clearOutline();
    if (hasPanel()) {
        panel()->clear();
    }
    clearArdScene();
};

void OutlineSceneBase::attachHoisted(topic_ptr h)
{
    detachHoisted();

    if(h){
        m_hoisted = h;
        LOCK(m_hoisted);
    }
};

void OutlineSceneBase::detachHoisted()
{
    if(m_hoisted){
        m_hoisted->release();
        m_hoisted = nullptr;
    }
};

void OutlineSceneBase::rebuild()
{
    doRebuild();
    //gitemSelected();
};

void OutlineSceneBase::attachOutlineBuilder(Builder::ptr&& b)
{
    m_outlineBuilder = std::move(b);
};

void OutlineSceneBase::setBuilderSearchString(QString str) 
{
    if (m_outlineBuilder) {
        m_outlineBuilder->setSearchString(str);
    }
};

void OutlineSceneBase::doRebuild()
{
    if(m_outlineBuilder){
        m_outlineBuilder->build();
    }
};

void OutlineSceneBase::updateItem(topic_ptr it)
{
    ProtoGItem* gi = findGItem(it);
    if(gi)
        {
            gi->g()->update();
        }
};


bool OutlineSceneBase::ensureVisible2(topic_ptr it, bool select /*= true*/)
{
	bool item_located = false;

	bool _rebuild = it->rollUpToRoot();

	if (_rebuild)
		m_view->rebuild();

	ProtoGItem* gi = findGItem(it);
	if (gi)
	{
		clearSelection();
		gi->g()->ensureVisible(QRectF(0, 0, 10, 10), 0, 50);
		if (select)selectGI(gi);
		auto ptg = gi->g()->boundingRect().topLeft();
		ptg = gi->g()->mapToScene(ptg);
		animateLocator(ptg);
		item_located = true;
	}
	else
	{
		dbg_print(QString("failed to locate item %1").arg(it->dbgHint()));
	}

	return item_located;
}

bool OutlineSceneBase::ensureVisibleByEid(QString eid)
{
	bool item_located = false;
	ProtoGItem* gi = findGItemByEid(eid);
	if (gi)
	{
		clearSelection();
		gi->g()->ensureVisible(QRectF(0, 0, 10, 10), 0, 50);
		selectGI(gi);
		auto ptg = gi->g()->boundingRect().topLeft();
		ptg = gi->g()->mapToScene(ptg);
		animateLocator(ptg);
		item_located = true;
	}
	else
	{
		dbg_print("failed to locate item by EID");
	}
	return item_located;
};

bool OutlineSceneBase::ensureVisible(std::function<bool(topic_ptr)> findBy)
{
	bool item_located = false;
	ProtoGItem* gi = findGItem(findBy);
	if (gi)
	{
		clearSelection();
		gi->g()->ensureVisible(QRectF(0, 0, 10, 10), 0, 50);
		selectGI(gi);
		item_located = true;
	}
	else
	{
		dbg_print("failed to locate item by function");
	}
	return item_located;
};

void OutlineSceneBase::updateHudItemsPos()
{
    if (hasPanel()) {
        panel()->updateHudItemsPos();
    }
};

void OutlineSceneBase::resetColors()
{
    s()->setBackgroundBrush(gui::colorTheme_BkColor());
};

void OutlineSceneBase::setOutlinePolicy(EOutlinePolicy op)
{
	if (m_outline_policy != op)
	{
		ProtoGItem* g = currentGI();

		if (outline()->scene() == this)
		{
			//@todo: some hack - ykh
			if (g) {
				gui::setupMainOutlineSelectRequestOnRebuild(g->topic());
			}
		}

		ProtoScene::setOutlinePolicy(op);
		freePanels();
		resetColors();
	}
}

ProtoGItem* OutlineSceneBase::currentGI(bool acceptToolButtons /*= true*/)
{
#define IN_CASE_OBJ_RETURN(T)if(true)           \
        {                                       \
            T* g = dynamic_cast<T*>(git);       \
            if(g){                              \
                    return g;                   \
                }                               \
        }                                       \

    QList<QGraphicsItem *> selected = s()->selectedItems();
    if(!selected.isEmpty())
        {
            QGraphicsItem* git = selected.first();
            IN_CASE_OBJ_RETURN(anGItem);
            IN_CASE_OBJ_RETURN(anGTableItem);
            if(acceptToolButtons)
                {
                    IN_CASE_OBJ_RETURN(ProtoToolItem);
                }
            IN_CASE_OBJ_RETURN(FormPanelNoteFooterGItem);
        }
    return nullptr;

#undef RETURN_IN_CASE
};

void OutlineSceneBase::installCurrentItemView(bool propagate2workspace)
{
    ProtoGItem* g = currentGI();
    if (!g) {
        for (GCURRENT_MARKS::iterator i = m_current_marks.begin(); i != m_current_marks.end(); i++)
        {
            CurrentMark* m = *i;
            if (m->owner())
            {
                m->detachOwner();
            }
        }
    }
	else
	{
		g->onBecomeCurrent(propagate2workspace);
		QObject* op = v()->sceneBuilder();
		if (op && op != outline()) {
			QMetaObject::invokeMethod(op, "currentGChanged", Qt::QueuedConnection, Q_ARG(ProtoGItem*, g));
		}
	}
};

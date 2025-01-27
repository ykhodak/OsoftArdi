#include <QGraphicsScene>
#include <QGraphicsSceneHoverEvent>
#include <QStyleOptionGraphicsItem>
#include <QPainter>
#include <QApplication>
#include <QDrag>
#include <QUrl>
#include <QDesktopServices>

#include "anGItem.h"
#include "utils.h"
#include "MainWindow.h"
#include "anfolder.h"
#include "ardmodel.h"
#include "OutlineMain.h"
#include "ProtoScene.h"
#include "TopicPanel.h"
#include "ansyncdb.h"
#include "OutlinePanel.h"
#include "TablePanel.h"

IMPLEMENT_OUTLINE_GITEM_EVENTS(anGItem);

ard::AllocPool anGItem::m_alloc_pool;

extern int getLasteSelectedColorIndex();

/*
  anGItem
*/
anGItem::anGItem()
{
}

anGItem::anGItem(topic_ptr item, ProtoPanel* p, int _ident)
    :ProtoGItem(item, p), OutlineGItem(_ident)
{
    setFlags(QGraphicsItem::ItemIsSelectable|QGraphicsItem::ItemIsFocusable);
    recalcGeometry();

    setAcceptHoverEvents(true);
    if(p->hasProp(ProtoPanel::PP_DnD))
        setAcceptDrops(true);

    setOpacity(DEFAULT_OPACITY);
    p->s()->s()->addItem(this);
}

anGItem::~anGItem()
{
}

void anGItem::setAsRootTopic()
{ 
    m_attr.asRootTopic = 1; 
};

void anGItem::setCheckSelected(bool v)
{ 
    m_attr.CheckSelected = v ? 1 : 0; 
};

void anGItem::updateGeometryWidth()
{
    recalcGeometry();
    prepareGeometryChange();
    update();
};

void anGItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    auto rc = identedRect();
    bool as_selected = (option->state & QStyle::State_Selected);
    if (as_selected)
    {
        PGUARD(painter);
        painter->setPen(Qt::NoPen);
        if (m_attr.Hover)
        {
            painter->setBrush(QBrush(color::HOVER_ITEM_BK));
        }
        else {
            QBrush brush(model()->brushSelectedItem());
            painter->setBrush(brush);
        }
        painter->drawRect(m_rect);

        if (!isRootTopic() &&
            p()->hasProp(ProtoPanel::PP_FatFingerSelect) &&
            topic()->hasFatFinger())
        {
            drawFatFingerSelect(rc, painter);
        }
    }
    else 
    {
        if (isCheckSelected()) {
            PGUARD(painter);
            painter->setPen(Qt::NoPen);
            QBrush brush(model()->brushMSelectedItem());
            painter->setBrush(brush);
            painter->drawRect(m_rect);
        }
        else 
        {
            if (m_attr.Hover)
            {
                PGUARD(painter);
                painter->setPen(Qt::NoPen);
                painter->setBrush(QBrush(color::HOVER_ITEM_BK));
                painter->drawRect(m_rect);
            }
        }
    }

#ifdef _DEBUG
    if (proto()->topic()->hasDebugMark1()) 
    {
        PGUARD(painter);
        painter->setPen(QPen(Qt::red, 2));
        QBrush brush(color::Red);
        painter->setBrush(brush);
        painter->drawRect(m_rect);
    }
#endif

    if (proto()->p()->hasProp(ProtoPanel::PP_Annotation)) {
        if (m_item->canHaveAnnotation()) {
            auto rc = annotationRect();
            if (!rc.isEmpty()) {
                drawAnnotation(rc, painter, as_selected, m_attr.Hover);
            }
        }
    }

    if (proto()->p()->hasProp(ProtoPanel::PP_ExpandNotes)) {
        if (m_item->hasNote()) {
            auto rc = expandedNoteRect();
            if (!rc.isEmpty()) {
                drawExpandedNote(rc, painter, as_selected);
            }
        }
    }


    if(isThumbnailDraw())
        {
            drawThumbnail(rc, painter, option);
        }
    else
        {
            if (!m_attr.asRootTopic) {
                if (p()->hasProp(ProtoPanel::PP_ActionButton))
                {
                    drawActionBtn(painter);
                }
            }
            drawIcons(rc, painter, option);
            drawTitle(rc, painter, option);
			///..
			if (m_attr.asRootTopic) 
			{
				auto f = topic();
				if (f->hasRButton()) 
				{
					static int radius = 5;
					PGUARD(painter);
					painter->setRenderHint(QPainter::Antialiasing, true);
					auto pm = f->getRButton(proto()->p()->ocontext());
					double dz = pm.width() / ICON_WIDTH;
					m_attr.rbutton_width = 72 / dz;
					QRect rcpm((int)rc.topRight().x() - m_attr.rbutton_width, (int)rc.topRight().y(),
						(int)m_attr.rbutton_width, (int)ICON_WIDTH);
					painter->setBrush(QBrush(Qt::white));
					painter->setPen(Qt::white);
					painter->drawRect(rcpm);
					//painter->drawRoundedRect(rcpm, radius, radius);
					painter->drawPixmap(rcpm, pm);
				}
			}
			//...
        }


    if (as_selected) 
    {
        if (!isRootTopic() &&
            p()->hasProp(ProtoPanel::PP_FatFingerSelect) &&
            topic()->hasFatFinger())
        {
            QRectF rc1 = identedRect();
            QRect rcico;
            rectf2rect(rc1, rcico);
            rcico.setLeft(rcico.right() - ICON_WIDTH);
            rcico.setBottom(rcico.top() + ICON_WIDTH);
            rcico.setWidth(ICON_WIDTH);
            rcico.setHeight(ICON_WIDTH);
            //painter->drawRect(rct);
            auto pm = getIcon_VDetails();
            painter->drawPixmap(rcico, pm);
        }
    }

    if(p()->hasProp(ProtoPanel::PP_DnD))
        {
            drawDnDMark(painter);
        }
    //drawDnD(painter, option);

#ifdef _DEBUG
    drawDebugInfo(painter, option);    
#endif

    QPen pen(color::SELECTED_ITEM_BK);
    painter->setPen(pen);
    painter->drawLine(m_rect.bottomLeft(), m_rect.bottomRight());
}

#ifdef _DEBUG
void anGItem::drawDebugInfo(QPainter *painter, const QStyleOptionGraphicsItem *option)
{
    Q_UNUSED(option);

    if(!dbgMark().isEmpty() || !topic()->dbgMark().isEmpty())
    {
        QString s = "";
        if (!topic()->dbgMark().isEmpty()) {
            s = QString("[%1]").arg(topic()->dbgMark());
        }
        if (!dbgMark().isEmpty()) {
            s += QString("{%1}").arg(dbgMark());
        }

        PGUARD(painter);
        QPen penText(Qt::blue);
        painter->setPen(penText);
        QFont f(*ard::defaultFont());
        f.setBold(true);
        painter->setFont(f);

        QRectF rc_tmp = identedRect();
        rc_tmp.setRight(rc_tmp.left() + utils::calcWidth(s, &f));
        painter->eraseRect(rc_tmp);
        painter->drawText(rc_tmp, 0, s);
    }
};
#endif

/*
void anGItem::drawCustomHotspots(const QRectF& rc1, QPainter *painter, topic_ptr f)
{
    auto hlist = f->customHotspots();
    if (!hlist.empty()) {
        QRectF rc = rc1;
        rc.setLeft(rc.left() + utils::calcWidth(f->title()) + 2 * ARD_MARGIN);
        rc.setLeft(rc.left() + gui::lineHeight());
        if (rc.width() < 100) {
            rc.setLeft(rc.right() - 100);
        }
        if (!m_attr.asRootTopic) {
            qreal rdelta = gui::lineHeight();
            rc.setRight(rc.right() - rdelta);
        }

        PGUARD(painter);
        painter->setRenderHint(QPainter::Antialiasing, true);
        auto fnt = utils::defaultTinyFont();
        painter->setFont(*fnt);

        for (auto& h : hlist) {
            //qDebug() << "custom-hspot" << h.title;
            if (!h.title.isEmpty())
            {
                //unsigned w = utils::calcWidth(h.title, fnt);
                auto sz = utils::calcSize(h.title, fnt);
                QRectF rc_hotspot = rc;
                rc_hotspot.setRight(rc_hotspot.left() + sz.width() + ARD_MARGIN);
                rc_hotspot.setTop(rc_hotspot.top() + 1);
                rc_hotspot.setBottom(rc_hotspot.top() + sz.height());

                if (rc_hotspot.right() > rc.right())
                    rc_hotspot.setRight(rc.right());

                QPainterPath r_path;
                r_path.addRoundedRect(rc_hotspot, 5, 5);
                painter->setPen(model()->penGray());
                QBrush brush(h.color);
                painter->setBrush(brush);
                painter->drawPath(r_path);
                painter->setBrush(Qt::NoBrush);
                //painter->setPen(Qt::NoPen);

                QPen penText(color::invert(h.color));
                painter->setPen(penText);
                painter->drawText(rc_hotspot, Qt::AlignLeft, h.title);
                //painter->setPen(Qt::NoPen);


                rc.setLeft(rc.left() + sz.width() + ARD_MARGIN);
            }
        }
    }
};*/

EHitTest anGItem::hitTest(const QPointF& pt, SHit& hit)
{
    if (proto()->p()->hasProp(ProtoPanel::PP_Annotation)) {
        if (m_item->canHaveAnnotation()) {
            auto rc = annotationRect();
            if (!rc.isEmpty()) {
                if (rc.contains(pt))
                {
                    return hitAnnotation;
                }
            }
        }
    }

    if (proto()->p()->hasProp(ProtoPanel::PP_ExpandNotes)) {
        if (m_item->hasNote()) {
            auto rc = expandedNoteRect();
            if (!rc.isEmpty()) {
                if (rc.contains(pt))
                {
                    return hitExpandedNote;
                }
            }
        }
    }


    const QRectF ident_rc = identedRect();

    if (!m_attr.asRootTopic) {
        if (p()->hasProp(ProtoPanel::PP_CheckSelectBox)) {
            QRectF rc3_check = m_rect;
            rc3_check.setRight(rc3_check.left() + gui::lineHeight());
            if (rc3_check.contains(pt))
            {
                return hitCheckSelect;
            }
        }

        if (p()->hasProp(ProtoPanel::PP_ActionButton)) {
            QRectF rc3_check = nonidentedRect();
            rc3_check.setRight(rc3_check.left() + proto()->p()->actionBtnWidth());
            rc3_check.setBottom(rc3_check.top() + gui::lineHeight());


            if (rc3_check.contains(pt))
            {
                return hitActionBtn;
            }
        }

        //...
        if (p()->hasProp(ProtoPanel::PP_FatFingerSelect) &&
            topic()->hasFatFinger())
        {
            if (g()->isSelected())
            {
                QRectF rc3_check = nonidentedRect();
                QRectF rct = QRectF(rc3_check.right() - ICON_WIDTH, nonidentedRect().top(), ICON_WIDTH, rc3_check.height());
                if (rct.contains(pt))
                {
                    return hitFatFingerDetails;
                }
            }
        }
        //..

        auto rc2_check = ident_rc;
        rc2_check.setLeft(rc2_check.right() - gui::lineHeight());
        if (m_item->isToDo() && 
            p()->hasProp(ProtoPanel::PP_ToDoColumn) && 
            rc2_check.contains(pt))
        {
            return hitToDo;
        }

        if (m_item->hasUrl())
        {
            rc2_check.translate(-gui::lineHeight(), 0);
            if (rc2_check.contains(pt))
            {
                return hitUrl;
            }
        }
    }

    hit.item = nullptr;
    return hitTestIdentedRect(ident_rc, pt, hit);
}

void anGItem::getOutlineEditRect(EColumnType column_type, QRectF& rc)
{
    if (EColumnType::Annotation == column_type) {
        rc = annotationRect();
        rc = g()->mapRectToScene(rc);
        return;
    }

    const QRectF ident_rc = identedRect();
    rc = ident_rc;
    
    if(!m_item->getIcon(p()->ocontext()).isNull())
        {
            rc.setLeft(rc.left() + ICON_WIDTH);
        }
    if(!m_item->getSecondaryIcon(p()->ocontext()).isNull())
        {
            rc.setLeft(rc.left() + ICON_WIDTH);
        }
    auto t3 = m_item->ternaryIconWidth();
    if(t3.first != TernaryIconType::none)
        {
            rc.setLeft(rc.left() + t3.second);
        }
    rc.setBottom(rc.bottom() + 1);
    rc.setTop(rc.top() - 2);
    
#ifndef ARD_BIG
    if (rc.height() < 2 * gui::lineHeight()) {
        rc.setHeight(2 * gui::lineHeight() + ARD_MARGIN);
    }
#endif //ARD_BIG  
    rc.setRight(rc.right() - ARD_MARGIN);
    if(rc.width() < 300)
        {
            rc = nonidentedRect();
            rc.setLeft(rc.left() + 2 * ARD_MARGIN);
            rc.setRight(rc.right() - ARD_MARGIN);
        }
  
    ProtoGItem::getOutlineEditRect(column_type, rc);
};

bool anGItem::preprocessMousePress(const QPointF& p, bool wasSelected)
{
    return implement_preprocessMousePress(p, wasSelected);
};

bool anGItem::preselectPreprocessMousePress(const QPointF& )
{
    auto proto_scene = p()->s();
    bool bShift = QGuiApplication::keyboardModifiers().testFlag(Qt::ShiftModifier);
    bool bCtrl = QGuiApplication::keyboardModifiers().testFlag(Qt::ControlModifier);

    //qDebug() << "preselectPreprocessMousePress shift=" << bShift << "ctrl=" << bCtrl << "isCheckSelected = " << isCheckSelected();
    //qDebug() << "isCheckSelected = " << isCheckSelected();
    if (bCtrl) 
    {
        auto g = proto_scene->currentGI();
        if (g) {
            if (!g->isCheckSelected()) {
                g->setCheckSelected(true);
            }
        }

        if (isCheckSelected()) {
            setCheckSelected(false);
        }
        else {
            setCheckSelected(true);
        }
        model()->setAsyncCallRequest(AR_UpdateGItem, topic(), nullptr);
        return true;
    }

    if (bShift) 
    {
        ard::topic* last_msel = nullptr;
        if (!last_msel) {
            auto g = proto_scene->currentGI();
            if (g) {
                last_msel = g->topic();
            }
        }

        if (last_msel) {
            if (bShift) 
            {
                proto_scene->clearMSelected();
                auto g_last = proto_scene->findGItem(last_msel);
                if (g_last) {
                    auto lst = proto_scene->gitemsInRange(g_last, this);
                    if (!lst.empty()) {
                        for (auto i : lst) {
                            i->setCheckSelected(true);
                            model()->setAsyncCallRequest(AR_UpdateGItem, i->topic(), nullptr);
                        }
                    }
                }
            }
        }

        return true;
    }


    return false;
};

void anGItem::mouseRightClick(const QPoint& pt)
{
    if (!m_attr.asRootTopic) 
    {
        auto f = topic();
        if (f && !f->isRootTopic()) 
        {
            //auto v1 = proto()->p()->s()->v()->v();
            //auto pt1 = v1->mapFromScene(pt);
            //auto pt2 = v1->mapToGlobal(pt1);
            proto()->on_clickedFatFingerDetails(pt);
            //qDebug() << "<< mouseRightClick" << pt;
        }
    }
};

void anGItem::resetGeometry()
{
    resetOutlineItemGeometry();
    ProtoGItem::resetGeometry();
};

void anGItem::dropEvent(QGraphicsSceneDragDropEvent *e) 
{
    dropGeneric(e);
};

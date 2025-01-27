#include <QLinearGradient>
#include <QStyleOptionGraphicsItem>
#include <QPainter>
#include <QGraphicsSceneMouseEvent>

#include "custom-g-items.h"
#include "utils.h"
#include "ardmodel.h"
#include "anfolder.h"
#include "MainWindow.h"
#include "TopicPanel.h"
#include "OutlineSceneBase.h"
#include "anGItem.h"
#include "ProtoScene.h"
#include "OutlineMain.h"
#include "EmailSearchBox.h"
#include "locus_folder.h"
#include "ansearch.h"

extern QPalette default_palette;

/**
   HierarchyBranchMark - shows subitems area
*/
HierarchyBranchMark::HierarchyBranchMark(ProtoPanel* p):m_panel(p)
{
    setOpacity(0.2);
};

void HierarchyBranchMark::setPathPoints(const QPointF& ptStart, const QPointF& ptTop, const QPointF& ptBottom)
{
    qreal w = (ptTop.x() - ptStart.x());
    qreal cx = w / 4.0;

    QPainterPath pp;
    pp.moveTo(ptStart);
    QPointF c1(ptStart.x() + cx, ptTop.y());
    QPointF c2(ptTop.x() - cx, ptStart.y());
    pp.cubicTo(c1, c2, ptTop);
    pp.lineTo(ptBottom);

    c1 = QPointF(ptBottom.x() - cx, ptBottom.y());
    c2 = QPointF(ptStart.x() + cx, ptStart.y());

    pp.cubicTo(c1, c2, ptStart);
    setPath(pp);
}


void HierarchyBranchMark::paint(QPainter *p, const QStyleOptionGraphicsItem* option, QWidget*)
{
    Q_UNUSED(option);
    p->setRenderHint(QPainter::Antialiasing, true);
    p->setPen(QPen(Qt::gray, 1));  
    QColor bk = p->background().color();
    if(bk.black() == 255)    
        bk = QColor(80,80,80);    
    else    
        bk = bk.darker(200);
    
    p->setBrush(bk);
    p->drawPath(path());
};


/**
   CurrentMark
*/
void CurrentMark::detachOwner()
{
    g()->hide();
    m_owner = nullptr;
};



/**
   CurrentMarkButton
*/
CurrentMarkButton::CurrentMarkButton(ECurrentCommand c):m_type(ECurrentMarkType::typeUknown), m_cmd(c)
{ 
    setZValue(ZVALUE_FRONT); 
    m_rightXoffset = 3 * gui::lineHeight();
};

void CurrentMarkButton::paint(QPainter *painter, const QStyleOptionGraphicsItem*, QWidget*)
{
    QRectF rc = boundingRect();
    PGUARD(painter);
    painter->setRenderHint(QPainter::Antialiasing, true);
    setZValue(ZVALUE_FRONT);

    switch (m_type)
    {
    case ECurrentMarkType::typeOutlineCurr:
    {
        QRectF rc_hotspot = rc;
        rc_hotspot.setLeft(rc_hotspot.left() + ARD_MARGIN);
        rc_hotspot.setRight(rc_hotspot.right() - ARD_MARGIN);
        QPainterPath r_path;
        r_path.addRoundedRect(rc_hotspot, 5, 5);
        painter->setPen(Qt::NoPen);
        painter->setBrush(model()->brushHotSelectedItem());
        painter->drawPath(r_path);
        QPen penText(color::invert(color::HOT_SELECTED_ITEM_BK));
        painter->setPen(penText);
        painter->setFont(*utils::defaultBoldFont());
        painter->drawText(rc_hotspot, Qt::AlignVCenter | Qt::AlignHCenter, label());

    }break;
    default:break;
    }
};

qreal CurrentMarkButton::setup(ProtoGItem* pg, qreal xpos)
{
    qreal rv = 0.0;

    m_type = pg->currentMarkType();
    
    if(m_type == ECurrentMarkType::typeUknown)
        {
            return 0.0;
        }

    QGraphicsItem* g = pg->g();

    switch(m_type)
        {
        case ECurrentMarkType::typeOutlineCurr:
            {
                qreal w = utils::calcWidth(label(), utils::defaultBoldFont()) + 2*ARD_MARGIN;


                qreal hdelta = gui::lineHeight() / 2;
                setRect(0,0,w, gui::lineHeight() + hdelta);
                QPointF pt = g->mapToScene(g->boundingRect().topRight());
                pt.setY(pt.y() - hdelta / 2);
                if(xpos > 0.0)
                    {
                        pt.setX(xpos);
                    }
                else
                    {
                        pt.setX(pt.x() - m_rightXoffset/*3 * gui::lineHeight()*/);
                        //qreal d = w + 3 * gui::lineHeight();
                    }
                pt.setX(pt.x() - w);
                setPos(pt);
                rv = pt.x();
            }break;
        default:ASSERT(0, "NA");break;
        }

    m_owner = pg;
    if(!isVisible()){
        show();
    }
    else{
        update();
    }

    return rv;
};

QString CurrentMarkButton::label()
{
    QString rv = "";
    if(m_type == ECurrentMarkType::typeOutlineCurr)
        {
            switch(m_cmd)
                {
                case ECurrentCommand::cmdSelect:        rv = "select";break;
                case ECurrentCommand::cmdSelectMoveTarget: rv = "add here";break;
                case ECurrentCommand::cmdEmptyRecycle:  rv = "empty Bin";break;
                case ECurrentCommand::cmdDownload:      rv = "download"; break;
                case ECurrentCommand::cmdDelete:        rv = "del"; break;
                case ECurrentCommand::cmdOpen:          rv = "open"; break;
                case ECurrentCommand::cmdFindInShell:   rv = "find"; break;
                case ECurrentCommand::cmdEdit:          rv = "edit"; break;
                }
        }
    return rv;
};


void CurrentMarkButton::mousePressEvent(QGraphicsSceneMouseEvent* e) 
{
    if (e->button() == Qt::LeftButton && preprocessMouseEvent()) {
        return ;
    }

    QGraphicsRectItem::mousePressEvent(e);
};

bool CurrentMarkButton::preprocessMouseEvent()
{
    if (m_owner)
    {
        QObject* op = m_owner->p()->s()->v()->sceneBuilder();

        //QPointF pt = pos();
        switch (m_type)
        {
        case ECurrentMarkType::typeOutlineCurr:
        {
            switch (m_cmd)
            {
            case ECurrentCommand::cmdEmptyRecycle:
            {
                auto r = m_owner->topic();
                if (r && r->isRecycleBin()) {
                    gui::emptyRecycle(r);
                }
                else {
                    ASSERT(0, "expected Recycle") << m_owner->topic()->objName();
                }
            }break;
            case ECurrentCommand::cmdSelectMoveTarget:
            case ECurrentCommand::cmdSelect:
            case ECurrentCommand::cmdDownload:
            case ECurrentCommand::cmdDelete:
            case ECurrentCommand::cmdOpen:
            case ECurrentCommand::cmdFindInShell:
            case ECurrentCommand::cmdEdit:
                ///somebody else should be handling this
                break;

            default:  ASSERT(0, "the command is not defined in Outline)"); break;
            }
        }break;
        default:ASSERT(0, "NA"); break;
        }

        if (op)
        {
            QMetaObject::invokeMethod(op, "currentMarkPressed", Qt::QueuedConnection, Q_ARG(int, (int)m_cmd), Q_ARG(ProtoGItem*, m_owner));
        }
    }

    return true;
};

/**
   CurrentMarkCloud

#define CPM_SELECT_WIDTH (gui::lineHeight() / 2.0)

CurrentMarkCloud::CurrentMarkCloud(EType t)
{
    setOpacity(DIMMED_OPACITY);
    setZValue(ZVALUE_BACK);
    m_type = t;
};

void CurrentMarkCloud::paint(QPainter *painter, const QStyleOptionGraphicsItem*, QWidget*)
{
    PGUARD(painter);
    painter->setRenderHint(QPainter::Antialiasing, true);
    QRectF rcF = boundingRect();

    switch (m_type)
    {
    case typeCurrent:
    case typeRoot:
    {
        QPointF c = rcF.center();
        qreal r = rcF.width();
        if (rcF.height() < r)
            r = rcF.height();
        r = r / 2.0;

        painter->setPen(Qt::NoPen);
        QRadialGradient gr(c, r);
        gr.setColorAt(0.3, painter->background().color());
        switch (m_type)
        {
        case typeCurrent:gr.setColorAt(1.0, color::SELECTED_ITEM_BK); break;
        case typeRoot:gr.setColorAt(1.0, color::Olive); break;
        default:ASSERT(0, "NA"); break;
        }

        painter->setBrush(QBrush(gr));
        painter->drawEllipse(c, rcF.width(), rcF.height());
    }break;

    case typeCPM_Highlight:
    {
        qreal m = ARD_MARGIN;
        QRectF rcF2 = rcF;
        rcF2.setLeft(rcF2.left() + m);
        rcF2.setTop(rcF2.top() + m);
        rcF2.setWidth(rcF2.width() - 2 * m);
        rcF2.setHeight(rcF2.height() - 2 * m);

        QPen pen(color::Yellow);
        pen.setWidth(3);
        pen.setStyle(Qt::DotLine);
        painter->setPen(pen);
        QBrush br(model()->brushSelectedItem());
        painter->setBrush(br);
        int radius = 5;
        painter->drawRoundedRect(rcF2, radius, radius);
    }break;

    case typeHoisted_Highlight:
    {
        qreal m = 3 * ARD_MARGIN;
        QRectF rcF2 = rcF;
        rcF2.setLeft(rcF2.left() + m);
        rcF2.setTop(rcF2.top() + m);
        rcF2.setWidth(rcF2.width() - 2 * m);
        rcF2.setHeight(rcF2.height() - 2 * m);

        QPen pen(color::Gray);
        pen.setWidth(3);
        pen.setStyle(Qt::DotLine);
        painter->setPen(pen);
        painter->setBrush(Qt::NoBrush);
        int radius = 5;
        painter->drawRoundedRect(rcF2, radius, radius);
    }break;
    }
};

void CurrentMarkCloud::reset()
{
    if (m_owner)
    {
        setup(m_owner, 0.0);
    }
    else
    {
        ASSERT(0, "expected owner item");
    }
};

qreal CurrentMarkCloud::setup(ProtoGItem* pg, qreal)
{
    m_owner = pg;
    switch(m_type)
        {
        case typeCurrent:
        case typeRoot:
            {
                qreal inflateFactor = 1.1;
                QRectF rcF = pg->g()->boundingRect();
                QRectF rcF2 = rcF;
                rcF2.setRight(rcF.width() * inflateFactor);
                rcF2.setBottom(rcF.height() * inflateFactor);
                prepareGeometryChange();
                setRect(rcF2);
  
                qreal dx = (rcF2.width() - rcF.width()) / 2;
                qreal dy = (rcF2.height() - rcF.height()) / 2;

                qreal x = pg->g()->pos().x() - dx;
                qreal y = pg->g()->pos().y() - dy;
                setPos(x, y);   
            }break;
        case typeCPM_Highlight:
            {
                QRectF rcF = pg->g()->boundingRect();
                QRectF rcF2 = rcF;
                rcF2.setRight(rcF.width() + 2 * CPM_SELECT_WIDTH);
                rcF2.setBottom(rcF.height() + 2 * CPM_SELECT_WIDTH);
                prepareGeometryChange();
                setRect(rcF2);  

                qreal x = pg->g()->pos().x() - CPM_SELECT_WIDTH;
                qreal y = pg->g()->pos().y() - CPM_SELECT_WIDTH;
                setPos(x, y);
            }break;
        case typeHoisted_Highlight:
            {
                QRectF rcF = pg->g()->boundingRect();
                QRectF rcF2 = rcF;
                //rcF2.setRight(rcF.width() + 2 * CPM_SELECT_WIDTH);
                rcF2.setBottom(rcF.height() + 2 * CPM_SELECT_WIDTH);
                prepareGeometryChange();
                setRect(rcF2);  

                qreal x = pg->g()->pos().x();// - CPM_SELECT_WIDTH;
                qreal y = pg->g()->pos().y() - CPM_SELECT_WIDTH;
                setPos(x, y);
            }break;
        }


    if(!isVisible()){
        show();
    }
    else{
        update();
    }

    return 0.0;
};*/

TitleEditFrame::TitleEditFrame()
{

};

void TitleEditFrame::paint(QPainter *p, const QStyleOptionGraphicsItem* , QWidget*)
{
    PGUARD(p);
    p->setRenderHint(QPainter::Antialiasing,true);
    QRectF rcF = boundingRect();
    QColor clr(m_navy_selected ? color::NavySelected : color::YellowSelected);
    clr = clr.darker(200);
  
    QPen pen(clr);
    pen.setWidth(4);
    p->setPen(pen);
    p->setBrush(Qt::NoBrush);
    p->drawRect(rcF);
};




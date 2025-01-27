#include <QGraphicsView>
#include <QStyleOptionGraphicsItem>
#include <QGraphicsPixmapItem>
#include "TablePanel.h"
#include "ardmodel.h"
#include "OutlineSceneBase.h"
#include "OutlineView.h"
#include "custom-widgets.h"
#include "kring.h"

/*
  column
*/
QString TablePanelColumn::type2label(EColumnType t, bool shortLabel)
{
    return columntype2label(t, shortLabel);
};

QString TablePanelColumn::label()const
{
    QString rv = type2label(type());
    return rv;
};

bool TablePanelColumn::isTitleColumn()const
{
    bool rv = ( type() == EColumnType::Title ||
                type() == EColumnType::ContactTitle ||
                type() == EColumnType::FormFieldValue ||
                type() == EColumnType::KRingTitle);
    return rv;
};

/*
  TablePanel
*/
TablePanel::TablePanel(ProtoScene* s)
    :TopicPanel(s), m_text_column_width(10.0)
{
    std::set<EProp> p, p2remove;
    //SET_PPP(p, PP_Thumbnail);
    SET_PPP(p, PP_Filter);
    SET_PPP(p, PP_CurrSpot);
    SET_PPP(p, PP_MultiLine);
    SET_PPP(p, PP_RTF);
    SET_PPP(p, PP_DnD);
    SET_PPP(p, PP_InplaceEdit);
    setProp(&p, &p2remove);
};

TablePanel::~TablePanel()
{
    for(COLUMNS::iterator i = columns().begin();i != columns().end();i++)
        {
            TablePanelColumn* c = *i;
            delete (c);
        }
    columns().clear();
};

void TablePanel::rebuildPanel()
{
    auto t = topic();

    if (!t) {
        return;
    }

    bool includeHoistedTopic = !t->isRootTopic();

    qreal x_ident = 0.0;
    m_outline_progress_y_pos = 0;
    outlineHoisted(x_ident, includeHoistedTopic);

    createAuxItems();
}

void TablePanel::clear()
{
    TopicPanel::clear();
    m_column_lines.clear();
};

void TablePanel::freeItems()
{
    for(LINES_LIST::iterator i = m_column_lines.begin();i != m_column_lines.end(); i++){
            QGraphicsLineItem* li = *i;
            FREE_GITEM(li);
        }
    TopicPanel::freeItems();
};

void TablePanel::createAuxItems()
{
    TopicPanel::createAuxItems();

    QPen pen4vline(COLOR_PANEL_SEP);
    for(COLUMNS::iterator i = columns().begin();i != columns().end() - 1;i++)
    {
        QGraphicsLineItem* li = s()->s()->addLine(0,0,0,0, pen4vline);
        m_column_lines.push_back(li);
        li->setEnabled(false);
    }
};

void TablePanel::updateAuxItemsPos()
{
    TopicPanel::updateAuxItemsPos();

    qreal h = calcHeight();
    int col_no = 0;
    for(LINES_LIST::iterator i = m_column_lines.begin();i != m_column_lines.end();i++)
        {
            if(col_no >= static_cast<int>(columns().size())){
                ASSERT(0, "invalid col number") << m_column_lines.size() << columns().size();
                break;
            }
            
            QGraphicsLineItem* li = *i;
            TablePanelColumn* c = columns()[col_no];
            if(c->visible()){                
                qreal x_pos = c->xpos() + c->width();
                li->setVisible(true);
                li->setLine(x_pos, 0, x_pos, h);
            }
            else{
                li->setVisible(false);
            }
            col_no++;
        }
};


ProtoGItem* TablePanel::produceOutlineItems(topic_ptr it,
                                            const qreal& x_ident, 
                                            qreal& y_pos,
                                            GITEMS_VECTOR* registrator/* = nullptr*/)
{
    int outline_ident = (int)x_ident;
    anGTableItem* gi = new anGTableItem(it, this, outline_ident);
    gi->setPos(panelIdent(), y_pos);
    y_pos += gi->boundingRect().height();
    registerGI(it, gi, registrator);
    return gi;
};

TablePanel& TablePanel::addColumn(EColumnType t)
{
    qreal w = 0;
    QFont* fnt = ard::defaultFont();
    TablePanelColumn* c = new TablePanelColumn(t);
    switch(t)
        {
        case EColumnType::Selection:
            w = gui::lineHeight();
            break;
    /*    case EColumnType::Node:
            w = utils::calcWidth("1.2.3", ard::defaultOutlineLabelFont());
            break;
            */
        case EColumnType::Title:
            w  = 3 * gui::lineHeight();
            break;
        //case EColumnType::ShortName:
        //    w  = utils::calcWidth("AB", ard::defaultOutlineLabelFont());
        //    break;
        case EColumnType::ContactEmail:
            w  = utils::calcWidth("MyVeryLongName@yahoo.com", fnt);
            break;
        case EColumnType::ContactPhone:
            w = utils::calcWidth("10 111 111 1111", fnt);
            break;
        case EColumnType::ContactAddress:
            w = utils::calcWidth("111 Delancey st", fnt);
            break;
        case EColumnType::ContactOrganization:
            w = utils::calcWidth("Brave New World", fnt);
            break;
        case EColumnType::FormFieldName:
            w = utils::calcWidth("Address Name", fnt);
            break;

        case EColumnType::KRingTitle: 
            w = 3 * gui::lineHeight();
            break;
        case EColumnType::KRingLogin: 
            w = utils::calcWidth("me-name@gmail.com", fnt);
            //w = 2 * gui::lineHeight();
            break;
        case EColumnType::KRingPwd: 
            w = utils::calcWidth("me-long-pwd11", fnt);
            //w = 3 * gui::lineHeight();
            break;
        case EColumnType::KRingLink: 
            w = 3 * gui::lineHeight();
            break;
        case EColumnType::KRingNotes:
            w = 3 * gui::lineHeight();
            break;
        case EColumnType::Uknown:
            break;
        default:break;
        }

    QString sl = c->label();
    qreal w2 =  utils::calcWidth(sl, ard::defaultOutlineLabelFont()) + 3 * ARD_MARGIN;
    if(w2 > w)
        w = w2;

    c->setWidth(w);
    m_columns.push_back(c);

    return *this;
};

TablePanelColumn* TablePanel::findColumn(EColumnType t)
{
    for(TablePanel::COLUMNS::iterator i = columns().begin(); i != columns().end();i++)
        {
            TablePanelColumn* c = *i;
            if(c->type() == t)
                return c;
        }

    return nullptr;
};

qreal TablePanel::textColumnWidth()const
{
    return m_text_column_width;
};

TablePanelColumn* TablePanel::findFirstTitleColumn()
{
    for(auto& c : columns()){
        if (c->isTitleColumn()){
                return c;
            }
    }
    return nullptr;
};

qreal TablePanel::calcNonTitleColumnsWidth()
{
    qreal rv = 0.0;
    for(auto& c : columns()){
        if (!c->isTitleColumn()){
            rv += c->width();
        }        
    }
    return rv;
};

void TablePanel::resetGeometry()
{
#define MIN_TEXT_COL_WIDTH 200
    
    if(columns().size() == 0){
        ASSERT(0, "expected columns in the table");
        return;
    }

    // we have recalc column pos from width and vieport width
    // there is one auto expanding column typeLabel, the
    // rest all have fixed sizes
    QWidget* vp = s()->v()->v()->viewport();
    QPoint pt(vp->width(),
              vp->height());

    QPointF pt2 = s()->v()->v()->mapToScene(pt);
    qreal panel_view_width = pt2.x();
    
    qreal x_pos = panelIdent();
    TablePanelColumn* labelCol = findFirstTitleColumn();
    assert_return_void(labelCol, "expected label column");
    qreal calc_nt_width = calcNonTitleColumnsWidth();
    m_text_column_width = panel_view_width - calc_nt_width;
    if(m_text_column_width < MIN_TEXT_COL_WIDTH){
        m_text_column_width = MIN_TEXT_COL_WIDTH;
    }    
    labelCol->setWidth(m_text_column_width);

    for(auto& c : columns()){
        if(x_pos > panel_view_width)
            {
                c->setVisible(false);
            }
        else{
            c->setVisible(true);
            c->setXPos(x_pos);
            x_pos += c->width();
        }
    }
    
    resetClassicOutlinerGeometry(); 
};


/*
  anGTableItem
*/
IMPLEMENT_OUTLINE_GITEM_EVENTS(anGTableItem);

anGTableItem::anGTableItem(topic_ptr item, TablePanel* p, int _ident)
    :ProtoGItem(item, p), OutlineGItem(_ident)
{
    setFlags(QGraphicsItem::ItemIsSelectable|QGraphicsItem::ItemIsFocusable);
    recalcGeometry();
 
    setAcceptHoverEvents(true);
    if(p->hasProp(ProtoPanel::PP_DnD))
        setAcceptDrops(true);

    setOpacity(DEFAULT_OPACITY);
    p->s()->s()->addItem(this);
};

void anGTableItem::resetGeometry()
{
    resetOutlineItemGeometry();
    ProtoGItem::resetGeometry();
};


EHitTest anGTableItem::hitTest(const QPointF& pt, SHit& hit)
{
    EHitTest h = hitUnknown;

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

    if (!isRootTopic() && 
        p()->hasProp(ProtoPanel::PP_FatFingerSelect) &&
        proto()->topic()->hasFatFinger()) 
    {
        if (g()->isSelected()) {
            int hot_height = m_rect.height();
            if (hot_height > FAT_FINGER_MAX_H) {
                hot_height = FAT_FINGER_MAX_H;
            }
            auto rct = nonidentedRect();
            rct.setLeft(rct.right() - FAT_FINGER_WIDTH);
            rct.setWidth(FAT_FINGER_WIDTH);
            rct.setHeight(hot_height);
            //QRectF rct(m_rect.right() - FAT_FINGER_WIDTH, 0, FAT_FINGER_WIDTH, hot_height);
            if (rct.contains(pt))
            {
                return hitFatFingerSelect;
            }
        }
    }

    QRectF rc = m_rect;
    int idx = 0;
    TablePanel* t = dynamic_cast<TablePanel*>(p());
    for(TablePanel::COLUMNS::iterator i = t->columns().begin(); i != t->columns().end();i++)
        {
            TablePanelColumn* c = *i;

            qreal x = c->xpos();
            qreal w = c->width();

            // rectf2rect(m_rect, rc);
            rc.setLeft(x);
            rc.setWidth(w);

            if(rc.contains(pt))
                {
                    h = hitTableColumn;
                    hit.column_number = idx;
                    switch(c->type())
                        {
                        case EColumnType::Title:
                            {
                                rc.setLeft(rc.left() + outlineIdent());
                                EHitTest h = hitTestIdentedRect(rc, pt, hit);
                                if(h != hitUnknown)
                                    {
                                        return h;
                                    }
                            }break;
                        default:break;
                        }

                    return h;
                }

            idx++;
        }

    return h;
};

void anGTableItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *)
{
#define CASE_COL_TYPE(T, D)case T:{D(rc, painter, option);}break;
//#define CASE_KRING_COL_TYPE(T, D)case T:{auto k = dynamic_cast<ard::KRingKey*>(it);if(k){D(rc, painter, option);}}break;


    auto rc = identedRect();
    bool as_selected = (option->state & QStyle::State_Selected);
    if (as_selected)
        {
        PGUARD(painter);
            painter->setPen(Qt::NoPen);
            QBrush brush(model()->brushSelectedItem());
            painter->setBrush(brush);
            painter->drawRect(m_rect);
            if (!isRootTopic() && 
                p()->hasProp(ProtoPanel::PP_FatFingerSelect) &&
                topic()->hasFatFinger()) 
            {
                drawFatFingerSelect(rc, painter);
            }
        }

    PGUARD(painter);

    if (proto()->p()->hasProp(ProtoPanel::PP_Annotation)) {
        if (m_item->canHaveAnnotation()) {
            auto rc = annotationRect();
            if (!rc.isEmpty()) {
                drawAnnotation(rc, painter, as_selected, m_attr.Hover);
            }
        }
    }

    painter->setRenderHint(QPainter::Antialiasing,true);

    TablePanel* t = dynamic_cast<TablePanel*>(p());
    assert_return_void(t, "expected table panel");
    auto it = proto()->topic();
    assert_return_void(it, "expected topic");
    for(TablePanel::COLUMNS::iterator i = t->columns().begin(); i != t->columns().end();i++)
        {
            TablePanelColumn* c = *i;
            if(!c->visible())
                continue;
            
            qreal x = c->xpos();
            qreal w = c->width();

            QRectF rc = nonidentedRect();
            rc.setLeft(x);
            rc.setWidth(w);

            auto column_type = c->type();
            switch(column_type)
                {
                case EColumnType::Selection:
                    {
                        if(t->topic() != topic())
                            {
                                utils::drawCheckBox(painter, rc, topic()->isTmpSelected(), 0, true);
                            }
                    }break;
                case EColumnType::Title:
                case EColumnType::ContactTitle:
                case EColumnType::KRingTitle:
                    {
                        rc.setLeft(rc.left() + outlineIdent());
                        drawIcons(rc, painter, option);
                        drawTitle(rc, painter, option);

                        if(topic()->hasToDoPriority())
                            {
                                QRect rc_prio;
                                rectf2rect(rc, rc_prio);
                                rc_prio.setLeft(rc_prio.right() - gui::lineHeight());
                                utils::drawPriorityIcon(topic(), rc_prio, painter);
                            }
                    }break;
                 CASE_COL_TYPE(EColumnType::ContactEmail, drawEmail);
                 CASE_COL_TYPE(EColumnType::ContactPhone, drawContactPhone);
                 CASE_COL_TYPE(EColumnType::ContactAddress, drawContactAddress);
                 CASE_COL_TYPE(EColumnType::ContactOrganization, drawContactOrganization);
                 CASE_COL_TYPE(EColumnType::ContactNotes, drawContactNotes);
                 CASE_COL_TYPE(EColumnType::FormFieldName, drawFormFieldName);
                 CASE_COL_TYPE(EColumnType::FormFieldValue, drawFormFieldValue);

                 CASE_COL_TYPE(EColumnType::KRingLogin, drawKRingLogin);
                 CASE_COL_TYPE(EColumnType::KRingPwd, drawKRingPwd);
                 CASE_COL_TYPE(EColumnType::KRingLink, drawKRingLink);
                 CASE_COL_TYPE(EColumnType::KRingNotes, drawKRingNotes);
                default:ASSERT(0, "NA");
                }
        }

    if(p()->hasProp(ProtoPanel::PP_DnD))
        {
            drawDnDMark(painter);
        }

    QPen pen(color::SELECTED_ITEM_BK);
    painter->setPen(pen);
    painter->drawLine(m_rect.bottomLeft(), m_rect.bottomRight());

#undef CASE_COL_TYPE
};

void anGTableItem::updateGeometryWidth()
{
    recalcGeometry();
    prepareGeometryChange();
    update();
};


bool anGTableItem::preprocessMousePress(const QPointF& p, bool wasSelected)
{
    return implement_preprocessMousePress(p, wasSelected);
};

void anGTableItem::getOutlineEditRect(EColumnType column_type, QRectF& rc)
{
    if (EColumnType::Annotation == column_type) {
        rc = annotationRect();
        rc = g()->mapRectToScene(rc);
        return;
    }

    rc =  m_rect;
    TablePanel* t = dynamic_cast<TablePanel*>(p());
    TablePanelColumn* c = t->findColumn(column_type);
    if(c)
        {
            rc.setLeft(c->xpos() + outlineIdent());
//#ifdef ARD_BIG
            rc.setWidth(c->width());
//#endif
        }
    else
        {
            ASSERT(0, QString("Column not found %1").arg(static_cast<int>(column_type)));
            return;
        }

#define MIN_EDIT_WIDTH 250

#ifdef ARD_BIG
  //  rc.setWidth(c->width());
    if (rc.width() < MIN_EDIT_WIDTH)
    {
        //rc = m_rect;
        //rc.setRight(rc.right() - ARD_MARGIN);
        int rpos = rc.left() + MIN_EDIT_WIDTH;
        if (rpos > m_rect.right()) {
            rpos = m_rect.right();
        }
        rc.setLeft(rpos - MIN_EDIT_WIDTH);
        rc.setRight(rpos);
}
#else
    if (rc.width() < MIN_EDIT_WIDTH)
    {
        rc = m_rect;
        rc.setRight(rc.right() - ARD_MARGIN);
    }
#endif
  
#undef MIN_EDIT_WIDTH

    ProtoGItem::getOutlineEditRect(column_type, rc);
};


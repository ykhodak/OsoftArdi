#include "ContactGroupPanel.h"
#include "ardmodel.h"
#include "contact.h"
#include "anGItem.h"

/**
    anContactCardGItem
*/
class anContactCardGItem : public anGItem
{
public:
    anContactCardGItem(ard::contact* c, ProtoPanel* p, int _ident) :
        anGItem(c, p, _ident)
    {
    
    };

    void   recalcGeometry()override 
    {
        int oneLineH = (int)gui::lineHeight();
        int w = proto()->p()->panelWidth();
        int h = oneLineH + 2 * utils::outlineSmallHeight() + ARD_MARGIN;

        m_rect = QRectF(0, 0,
            w,
            h);
    };

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)override;
};

void anContactCardGItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *)
{
    if (option->state & QStyle::State_Selected)
    {
        PGUARD(painter);
        painter->setPen(Qt::NoPen);
        QBrush brush(model()->brushSelectedItem());
        painter->setBrush(brush);
        painter->drawRect(m_rect);
    }

    PGUARD(painter);   
    painter->setRenderHint(QPainter::Antialiasing, true);

    auto c = dynamic_cast<ard::contact*>(topic());
    QRectF rc = identedRect();

    QRectF rcFrame = rc;
    rcFrame.setTop(rcFrame.top() + 1);
    rcFrame.setBottom(rcFrame.bottom() - 3);

    painter->setFont(*utils::defaultBoldFont());
    QString stitle = c->impliedTitle();
    globalTextFilter().drawTextLine(painter, rc, stitle).width();
    auto pm = m_item->getIcon(p()->ocontext());
    if (!pm.isNull()) {
        QRect rcIcon;
        rectf2rect(rc, rcIcon);
        rcIcon.setBottom(rcIcon.top() + ICON_WIDTH);
        rcIcon.setLeft(rcIcon.left() - (ICON_WIDTH + ARD_MARGIN));
        rcIcon.setRight(rcIcon.left() + ICON_WIDTH);
        painter->drawPixmap(rcIcon,pm);
    }

    rc.setTop(rc.top() + utils::outlineDefaultHeight());

    rc.setLeft(rc.left() + gui::lineHeight() / 2);
    painter->setFont(*ard::defaultSmallFont());

    QString s = c->contactEmail();
    if (!s.isEmpty()) {        
        globalTextFilter().drawTextLine(painter, rc, s).width();
        rc.setTop(rc.top() + utils::outlineSmallHeight());
    }

    s = c->contactPhone();
    if (!s.isEmpty()) {
        globalTextFilter().drawTextLine(painter, rc, s).width();
    }
}


/**
    ContactGroupPanel
*/
ContactGroupPanel::ContactGroupPanel(ProtoScene* s) :
    OutlinePanel(s)
{
    clearAllProp();
    std::set<EProp> p;
    SET_PPP(p, PP_Filter);
    //SET_PPP(p, PP_InplaceEdit);
    setProp(&p);

    m_multi_G_per_item = true;
    rebuildGroupDataModel();
}

ContactGroupPanel::~ContactGroupPanel() 
{
    releaseGroupDataModel();
};

void ContactGroupPanel::releaseGroupDataModel() 
{
    //snc::release_container(m_topic_list.begin(), m_topic_list.end());
    //m_topic_list.clear();
	snc::clear_locked_vector(m_topic_list);
};

void ContactGroupPanel::onDefaultItemAction(ProtoGItem* g) 
{
    auto f1 = g->topic();
    if (f1) {
        auto cg = dynamic_cast<ard::contact_group*>(f1->shortcutUnderlying());
        if (cg) {
            for (auto f : m_topic_list) {
                if (f != f1) {
                    if (f->isExpanded()) {
                        f->setExpanded(false);
                    }
                }
            }
        }
    }
};

void ContactGroupPanel::rebuildPanel()
{
    qreal x_ident = 0.0;
    m_outline_progress_y_pos = 0;

    for(auto& f : m_topic_list){
        /*
        qDebug() << "g-size=" 
            << f->items().size() 
            << "exp=" << f->isExpanded() 
            << f->title();
            */
        produceOutlineItems(f,
            x_ident,
            m_outline_progress_y_pos);
        bool explore_branch = globalTextFilter().isActive();
        if (!explore_branch) {
            explore_branch = !f->items().empty() && f->isExpanded();
        }
        if (explore_branch) {
            for (auto i : f->items()) {
                auto f = dynamic_cast<ard::topic*>(i);
                if (f) {
                    auto c = dynamic_cast<ard::contact*>(f->shortcutUnderlying());
                    if (c && globalTextFilter().filterIn(c)) {
                        produceContactCard(c,
                            x_ident + gui::lineHeight(),
                            m_outline_progress_y_pos);
                    }
                }
            }
        }
    }
};


void ContactGroupPanel::rebuildGroupDataModel()
{
    releaseGroupDataModel();
    if (ard::isDbConnected()) {
        m_topic_list = ard::db()->cmodel()->groot()->getAsGroupListModel();
    }
};

ProtoGItem* ContactGroupPanel::produceContactCard(ard::contact* c, const qreal& x_ident, qreal& y_pos)
{
    int outline_ident = (int)x_ident;
    anContactCardGItem* gi = new anContactCardGItem(c, this, outline_ident);
    gi->setPos(panelIdent(), y_pos);
    y_pos += gi->boundingRect().height();
    registerGI(c, gi);
    return gi;
};

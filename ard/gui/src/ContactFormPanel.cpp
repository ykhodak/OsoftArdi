#include "contact.h"
#include "ContactFormPanel.h"
#include "ardmodel.h"

class FormGItem : public anGTableItem
{
public:
    FormGItem(formfield_ptr f, TablePanel* p, int _ident) :anGTableItem(f, p, _ident) {};

    void paint(QPainter *p, const QStyleOptionGraphicsItem *option, QWidget *wdg)
    {
        anGTableItem::paint(p, option, wdg);

        auto ff = dynamic_cast<FormFieldTopic*>(m_item);

        QString type_label = ff->formValueLabel();
        if (!type_label.isEmpty())
        {
            PGUARD(p);
            QFont f(*ard::defaultSmallFont());
            p->setFont(f);
            p->setPen(model()->penGray());
            QRectF rc = m_rect;
            if (rc.width() < 200) {
                type_label = type_label.at(0);
            }
            int w = utils::calcWidth(type_label, ard::defaultSmallFont());
            rc.setLeft(rc.right() - w);
            p->drawText(rc, Qt::AlignRight | Qt::AlignVCenter, type_label);
        }
    }
};

/**
    FormPanel
*/
FormPanel::FormPanel(ProtoScene* s) :
    TablePanel(s)
{
    addColumn(EColumnType::FormFieldValue);
};

void FormPanel::build_form4topic(topic_ptr it)
{
    ASSERT_VALID(it);
    attachTopic(it);
    std::set<EColumnType> exclude_columns;
    exclude_columns.insert(EColumnType::ContactNotes);
    exclude_columns.insert(EColumnType::KRingNotes);
    TOPICS_LIST lst = it->produceFormTopics(nullptr, &exclude_columns);
    qreal y_pos = rebuildAsListPanel(lst, false);

    FormFieldTopic* f = new FormFieldTopic(it, it->formNotesColumn()/*EColumnType::ContactNotes*/, "");
    m_notes_footer = new FormPanelNoteFooterGItem(f, this);
    m_notes_footer->setPos(0, y_pos);
    registerGI(f, m_notes_footer);

    createAuxItems();
};

ProtoGItem* FormPanel::produceOutlineItems(topic_ptr it,
    const qreal& x_ident,
    qreal& y_pos,
    GITEMS_VECTOR* registrator /*= nullptr*/)
{
    auto ff = dynamic_cast<FormFieldTopic*>(it);
    if (!ff) {
        ASSERT(0, "expected form topic");
        return nullptr;
    }

    int outline_ident = (int)x_ident;
    auto gi = new FormGItem(ff, this, outline_ident);
    gi->setPos(panelIdent(), y_pos);
    y_pos += gi->boundingRect().height();
    registerGI(it, gi, registrator);
    return gi;
};


void FormPanel::createAuxItems()
{
    TablePanel::createAuxItems();
    QPen pen4vline(COLOR_PANEL_SEP);
    m_notes_footer_line = s()->s()->addLine(0, 0, 0, 0, pen4vline);
    m_notes_footer_line->setEnabled(false);

    auto c = dynamic_cast<ard::contact*>(topic());
    if (c) {
        QPixmap pm = c->getIcon(ocontext());
        if (!pm.isNull()) {
            pm = pm.scaled(gui::lineHeight(), gui::lineHeight(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
            QGraphicsPixmapItem* pi = new QGraphicsPixmapItem(pm);
            m_pxmaps.push_back(pi);
            s()->s()->addItem(pi);
            pi->setEnabled(false);
        }
    }
};

void FormPanel::updateAuxItemsPos()
{
    QSize szVPort = s()->viewportSize();

    qreal h = calcHeight();
    if (m_notes_footer) {
        h = m_notes_footer->pos().y();
    }
    int col_no = 0;
    for(auto& li : m_column_lines)
    {
        TablePanelColumn* c = columns()[col_no];
        qreal x_pos = c->xpos() + c->width();
        li->setLine(x_pos, 0, x_pos, h);
        col_no++;
    }

    if (m_notes_footer_line) {
        m_notes_footer_line->setLine(0, h, panelWidth(), h);
    }

    for (auto p : m_pxmaps) {
        p->setPos(szVPort.width() - p->boundingRect().width(), 0);
    }
};

void FormPanel::freeItems()
{
    for(auto& li : m_pxmaps){
        FREE_GITEM(li);
    }

    if (m_notes_footer) {
        FREE_GITEM(m_notes_footer);
    }

    if (m_notes_footer_line) {
        FREE_GITEM(m_notes_footer_line);
    }

    TablePanel::freeItems();
};

void FormPanel::clear()
{
    TablePanel::clear();
    m_pxmaps.clear();
    m_notes_footer = nullptr;
    m_notes_footer_line = nullptr;
};


/**
    FormPanelNoteFooterGItem
*/
FormPanelNoteFooterGItem::FormPanelNoteFooterGItem(FormFieldTopic::ptr tt, FormPanel* p)
    :anGTableItem(tt, p, 0)
{
};

void FormPanelNoteFooterGItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *)
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
    QRectF rcText = m_rect;
    QString note_text = topic()->formValue();
    painter->setFont(*ard::defaultFont());
    if (option->state & QStyle::State_Selected){
        QPen penText(Qt::black);
        painter->setPen(penText);
    }
    else{
        QPen pen(QColor(color::invert(painter->background().color().rgb())));
        painter->setPen(pen);
    }

    globalTextFilter().drawTextMLine(painter, painter->font(), rcText, note_text, false);
};

EHitTest FormPanelNoteFooterGItem::hitTest(const QPointF&, SHit& hit)
{
    EHitTest h = hitTableColumn;
    hit.column_number = 0;
    return h;
};

void FormPanelNoteFooterGItem::getOutlineEditRect(EColumnType, QRectF& rc)
{
    rc = m_rect;
    rc = g()->mapRectToScene(rc);
}

void FormPanelNoteFooterGItem::recalcGeometry()
{
    int oneLineH = (int)gui::lineHeight();
    int w = proto()->p()->panelWidth();
    int h = oneLineH;

    QString note_text = topic()->formValue();

    QFontMetrics fm(*ard::defaultFont());
    QRect rc2(0, 0, w, MP1_HEIGHT);
    QRect rc3 = fm.boundingRect(rc2, Qt::TextWordWrap, note_text);
    h = rc3.height();
    if (h < oneLineH)
        h = oneLineH;

    m_rect = QRectF(0, 0,
        w,
        h);
};

#include "BoardTemplateSelector.h"

void ard::BoardTemplateSelector::selectTemplte(DB_ID_TYPE board_id, const QPoint& pt)
{
    auto d = new BoardTemplateSelector(board_id);
    d->move(pt);
    d->show();
};

ard::BoardTemplateSelector::BoardTemplateSelector(DB_ID_TYPE board_id) :m_board_id(board_id)
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowSystemMenuHint);
    setAttribute(Qt::WA_DeleteOnClose);

    bmargin = ARD_MARGIN * 2;
    bwidth = 80;
    int selector_w = 0;
    int selector_h = 0;

    if (true) {
        ROW r;
        r.push_back(TemplateCell{ BoardSample::Greek, QRect(), "Greeks"});
		r.push_back(TemplateCell{ BoardSample::Stocks, QRect(), "Stocks" });
		r.push_back(TemplateCell{ BoardSample::Gtd, QRect(), "Gtd" });
        r.push_back(TemplateCell{ BoardSample::ProductionControl, QRect(), "Production Control"});
        selector_w = r.size() * (bwidth + bmargin) + bmargin;
        m_cells.emplace_back(std::move(r));
    }

    selector_h = m_cells.size() * (bwidth + bmargin) + bmargin;
    resize(QSize(selector_w, selector_h));
};

void ard::BoardTemplateSelector::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::ActivationChange) {
        if (!isActiveWindow()) {
            close();
        }
    }
};


void ard::BoardTemplateSelector::mousePressEvent(QMouseEvent *e)
{
    auto pt = e->pos();
    for (auto& i : m_cells) {
        for (auto& c : i) {
            if (c.rect.contains(pt)) {
                close();
                asyncExec(AR_BoardInsertTemplate, m_board_id, static_cast<int>(c.sample));
                return;
            }
        }
    }
    close();
}

void ard::BoardTemplateSelector::paintEvent(QPaintEvent *) 
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);
    QPen pen(Qt::gray, 2);
    p.setPen(pen);

    int ridx = 0;
    for (auto& i : m_cells) {
        QRect rc(bmargin, bmargin + ridx * (bwidth + bmargin), bwidth, bwidth);
        for (auto& c : i) {
            PGUARD(&p);
            QPen pen(QColor(color::Gray), 1);
            p.setPen(pen);
            p.drawRect(rc);

            c.rect = rc;

            p.setFont(*ard::defaultSmall2Font());
            p.drawText(rc, Qt::AlignCenter | Qt::TextWordWrap, c.name);
            rc.translate(bwidth + bmargin, 0);
        }
        ridx++;
    }
};
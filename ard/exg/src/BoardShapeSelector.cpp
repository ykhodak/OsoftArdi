#include "BoardShapeSelector.h"

void ard::BoardShapeSelector::editShape(DB_ID_TYPE board_id, ard::BoardItemShape sh, const QPoint& pt)
{
    BoardShapeSelector* d = new BoardShapeSelector(board_id, sh);
    d->move(pt);
    d->show();
    return;
};

ard::BoardShapeSelector::BoardShapeSelector(DB_ID_TYPE board_id, ard::BoardItemShape sh):m_board_id(board_id), m_sel_shape(sh)
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowSystemMenuHint);
    setAttribute(Qt::WA_DeleteOnClose);

    bmargin = ARD_MARGIN * 2;
    bwidth = 80;
    int selector_w = 0;
    int selector_h = 0;

    if (true) {
        ROW r;
        r.push_back(ShapeCell{ BoardItemShape::text_normal, QRect() });
        r.push_back(ShapeCell{ BoardItemShape::text_italic, QRect() });
        r.push_back(ShapeCell{ BoardItemShape::box, QRect() });
        selector_w = r.size() * (bwidth + bmargin) + bmargin;
        m_cells.emplace_back(std::move(r));
    }


    if (true) {
        ROW r;
        //r.push_back(ShapeCell{ BoardItemShape::box });
        r.push_back(ShapeCell{ BoardItemShape::circle, QRect() });
        r.push_back(ShapeCell{ BoardItemShape::triangle, QRect() });
        r.push_back(ShapeCell{ BoardItemShape::rombus, QRect() });
        selector_w = r.size() * (bwidth + bmargin) + bmargin;

        m_cells.emplace_back(std::move(r));     
    }

    if (true) {
        ROW r;      
        r.push_back(ShapeCell{ BoardItemShape::pentagon, QRect() });
        r.push_back(ShapeCell{ BoardItemShape::hexagon, QRect() });
        m_cells.emplace_back(std::move(r));
    }

    /*
    if (true) {
        ROW r;
        r.push_back(ShapeCell{ BoardItemShape::text_normal });
        r.push_back(ShapeCell{ BoardItemShape::text_italic });
        r.push_back(ShapeCell{ BoardItemShape::text_bold });
        m_cells.emplace_back(std::move(r));
    }
    */

    selector_h = m_cells.size() * (bwidth + bmargin) + bmargin;

    resize(QSize(selector_w, selector_h));
};

void ard::BoardShapeSelector::paintEvent(QPaintEvent *)
{
    QPainter p(this);

    p.setRenderHint(QPainter::Antialiasing, true);
    QPen pen(Qt::gray, 2);
    p.setPen(pen);
    
    int ridx = 0;
    for (auto& i : m_cells) {
        QRect rc(bmargin, bmargin + ridx * (bwidth + bmargin), bwidth, bwidth);
        for (auto& c : i) {
            c.rect = rc;

            bool as_selected = (c.shape == m_sel_shape);
            QBrush brush(as_selected ? color::MSELECTED_ITEM_BK : color::White);
            p.setBrush(brush);
            //p.setBrush(QBrush(color::White));
            if (as_selected) {
                PGUARD(&p);
                QPen pen(QColor(color::MSELECTED_ITEM_BK), 2);
                p.setPen(pen);
                p.drawRect(rc);
            }


            switch (c.shape) 
            {
            case ard::BoardItemShape::box: 
            {
                static int radius = 10;
                int h = gui::lineHeight();
                int dy = (rc.height() - h) / 2;
                QRect rc2 = rc;
                rc2.setTop(rc2.top() + dy);
                rc2.setHeight(h);
                p.drawRoundedRect(rc2, radius, radius);
            }break;
            case ard::BoardItemShape::circle: 
            {
                p.drawEllipse(rc);
            }break;
            case ard::BoardItemShape::text_normal:            
            case ard::BoardItemShape::text_bold: 
            {
                p.setFont(*ard::getTextlikeFont(c.shape));
                p.drawText(rc, Qt::AlignCenter, tr("Text"));
            }break;
            case ard::BoardItemShape::text_italic: 
            {
                p.setFont(*ard::getTextlikeFont(c.shape));
                p.drawText(rc, Qt::AlignCenter, tr("AltText"));
            }break;
            /*
            case ard::BoardItemShape::text_normal: 
            {
                p.setFont(*ard::defaultFont());
                p.drawText(rc, Qt::AlignCenter, tr("Text"));
            }break;
            case ard::BoardItemShape::text_italic:
            {
                auto f = *ard::defaultFont();
                f.setItalic(true);
                p.setFont(f);
                p.drawText(rc, Qt::AlignCenter, tr("Text"));
            }break;
            case ard::BoardItemShape::text_bold:
            {
                p.setFont(*ard::defaultBoldFont());
                p.drawText(rc, Qt::AlignCenter, tr("Text"));
            }break;
            */
            default: 
            {
                auto plg = ard::boards_model::build_shape(c.shape, rc);
                p.drawPolygon(plg);
            }
            }
            /*
            auto rce = ard::boards_model::calc_edit_rect(c.shape, rc);
            if (!rce.isEmpty()) {
                QBrush brush(as_selected ? color::MSELECTED_ITEM_BK : color::White);
                p.setBrush(brush);
                p.drawRect(rce);
            }*/

            rc.translate(bwidth + bmargin, 0);
        }
        ridx++;
    }
};

void ard::BoardShapeSelector::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::ActivationChange) {
        if (!isActiveWindow()) {
            close();
        }
    }
};


void ard::BoardShapeSelector::mousePressEvent(QMouseEvent *e)
{
    auto pt = e->pos();
    for (auto& i : m_cells) {
        for (auto& c : i) {
            if (c.rect.contains(pt)) {
                close();
                asyncExec(AR_BoardApplyShape, m_board_id, static_cast<int>(c.shape));
                return;
            }
        }
    }
    close();
}

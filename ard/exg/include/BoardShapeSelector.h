#pragma once

#include "a-db-utils.h"
#include "custom-widgets.h"
#include "board.h"

namespace ard {
    class BoardShapeSelector : public QWidget
    {
    public:
        static void editShape(DB_ID_TYPE board_id, ard::BoardItemShape sh, const QPoint& pt);

    protected:
        class ShapeCell
        {
        public:
            BoardItemShape  shape;
            QRect           rect;
        };

        using ROW = std::vector<ShapeCell>;
        using CELLS = std::vector<ROW>;

        BoardShapeSelector(DB_ID_TYPE board_id, ard::BoardItemShape sh);

        void changeEvent(QEvent *event)override;
        void paintEvent(QPaintEvent *e)override;
        void mousePressEvent(QMouseEvent *e)override;

        int bmargin;
        int bwidth;
        CELLS m_cells;
        DB_ID_TYPE m_board_id;
        ard::BoardItemShape m_sel_shape;
    };
}
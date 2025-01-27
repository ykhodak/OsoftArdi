#include "a-db-utils.h"
#include "custom-widgets.h"
#include "board.h"

namespace ard {
    class BoardTemplateSelector : public QWidget
    {
    public:
        static void selectTemplte(DB_ID_TYPE board_id, const QPoint& pt);

    protected:
        class TemplateCell
        {
        public:
            BoardSample     sample;
            QRect           rect;
            QString         name;
        };

        using ROW = std::vector<TemplateCell>;
        using CELLS = std::vector<ROW>;

        BoardTemplateSelector(DB_ID_TYPE board_id);

        void changeEvent(QEvent *event)override;
        void paintEvent(QPaintEvent *e)override;
        void mousePressEvent(QMouseEvent *e)override;


        int bmargin;
        int bwidth;
        CELLS m_cells;
        DB_ID_TYPE m_board_id;
    };
}
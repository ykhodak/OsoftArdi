#include <QWidget>
#include "snc-tree.h"
#include "anfolder.h"

class SearchMap : public QWidget
 {
   Q_OBJECT
 public:
   SearchMap();
   void mapRoot(topic_ptr r);
   void filter(QString s);
 protected:
   void paintEvent(QPaintEvent *e);
   void resizeEvent(QResizeEvent *e);
   void remap();
   void drawCell(QPainter& p, snc::cit* it);

 protected:
   int m_grid_dx;
   int m_cellsPerRow;
   snc::MemFindAllIndexedPipe m_piper;
};

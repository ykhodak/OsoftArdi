#include <QPaintEvent>
#include <QPainter>
#include <math.h> 

#include "SearchMap.h"
#include "a-db-utils.h"
#include "anfolder.h"

SearchMap::SearchMap()
{
  if(is_big_screen())
    {
      setMinimumWidth(200);
      setMinimumHeight(400);
    }
  setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
};

void SearchMap::drawCell(QPainter& p, snc::cit* it)
{
  DB_ID_TYPE topic_idx = it->userData();
  int row_idx = (int)(topic_idx / m_cellsPerRow);
  int col_idx = topic_idx - row_idx * m_cellsPerRow;
  int y = row_idx * m_grid_dx;
  int x = col_idx * m_grid_dx;
  QRect rcC(x, y,m_grid_dx,m_grid_dx);
  p.drawRect(rcC);
};

void SearchMap::paintEvent(QPaintEvent *e)
{
  QWidget::paintEvent(e);
  QRect rc = e->rect(); 
  QPainter p(this);
  
  QPen pen(color::Black);
  QBrush brush(color::Black);
  p.setPen(pen);
  p.setBrush(brush);
  p.drawRect(rc);

  QBrush b2(color::ReutersChart);
  p.setBrush(b2);

  if(m_piper.isFilterOn())
    {
      if(m_piper.keywordFiltered().size() > 0)
    {
       for(auto& it : m_piper.keywordFiltered()) {
          drawCell(p, it);
        }
    }
    }
  else
    {
    for(auto& it : m_piper.items()){
      drawCell(p, it);
    }      
    }
};

void SearchMap::resizeEvent(QResizeEvent *)
{
  remap();
};

void SearchMap::mapRoot(topic_ptr r)
{
  assert_return_void(r, "expected topic");
  m_piper.prepare(r);
};

void SearchMap::filter(QString s)
{
  m_piper.filterByKeyword(s);
  remap();
};

void SearchMap::remap()
{
#define MAX_CELL_SIZE 4

  int map_size = m_piper.items().size();
  if(map_size == 0)
    map_size = 10;
  QRect rc = rect();
  qreal dxr = sqrt((double)rc.width() * rc.height() / map_size); 
  int dx2 = (int)dxr;
  while(dx2 > MAX_CELL_SIZE)
    {
      int colls = rc.width() / dx2;
      int rows = rc.height() / dx2;
      int int_map_size = colls * rows;
      if(int_map_size >= map_size)break;
      dx2--;
    }

  m_grid_dx = dx2;

  if(m_grid_dx < 4)
    {
      m_grid_dx = 4;
    }

  m_cellsPerRow = rc.width() / m_grid_dx;

  update(rc);
#undef MAX_CELL_SIZE
};

#pragma once

#include <list>
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include <QGraphicsSceneMouseEvent>
#include "a-db-utils.h"

class ProtoPanel;
class OutlinePanelGV;
class anItem;
class ProtoGItem;
class HudButton;
class ProtoToolItem;

#define PANEL_SPLITTER_WIDTH  5
#define PANEL_SPLITTER_HEIGHT 20



class CurrentMark
{
public:
  CurrentMark():m_owner(nullptr){};
  virtual ~CurrentMark(){};

  virtual qreal          setup(ProtoGItem* pg, qreal xpos) = 0;
  virtual QGraphicsItem* g() = 0;
  virtual void           reset(){};

  void detachOwner();
  ProtoGItem* owner(){return m_owner;}
  const ProtoGItem* owner()const{return m_owner;}
protected:
  ProtoGItem* m_owner;
};

typedef std::vector<CurrentMark*> AUX2_ITEMS;

class CurrentMarkButton : public CurrentMark,
    public QGraphicsRectItem
{
public:
    CurrentMarkButton(ECurrentCommand c /*= ECurrentCommand::cmdDefault*/);
    void paint(QPainter *painter, const QStyleOptionGraphicsItem* option, QWidget*)override;
    qreal setup(ProtoGItem* pg, qreal xpos)override;
    QGraphicsItem* g()override { return this; }
    bool preprocessMouseEvent();
    QString label();
    void setRightXoffset(int dx) { m_rightXoffset = dx; }
protected:
    void mousePressEvent(QGraphicsSceneMouseEvent* e)override;

    ECurrentMarkType    m_type;
    ECurrentCommand m_cmd;
    int      m_rightXoffset;
};


class TitleEditFrame: public QGraphicsRectItem
{
public:
  TitleEditFrame();
  void paint(QPainter *painter, const QStyleOptionGraphicsItem* option, QWidget*);
  void setNavyColor(bool bset) { m_navy_selected = bset; }
protected:
  void mousePressEvent(QGraphicsSceneMouseEvent * e )
  {
    QGraphicsRectItem::mousePressEvent(e);
    e->ignore();
  };
  void mouseReleaseEvent(QGraphicsSceneMouseEvent * e )
  {
    QGraphicsRectItem::mouseReleaseEvent(e);
    e->ignore();
  };
protected:
    bool m_navy_selected{true};
};


class HierarchyBranchMark: public QGraphicsPathItem
{
public:
  HierarchyBranchMark(ProtoPanel* p);
  void paint(QPainter *painter, const QStyleOptionGraphicsItem* option, QWidget*);
  void setPathPoints(const QPointF& ptStart, const QPointF& ptTop, const QPointF& ptBottom);

protected:
  ProtoPanel* m_panel;
};

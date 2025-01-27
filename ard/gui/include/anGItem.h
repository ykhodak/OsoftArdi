#ifndef ANGITEM_H
#define ANGITEM_H

#include "OutlineGItem.h"
#include "mpool.h"

/**
   anGItem - scene item for outline panels
*/
class anGItem : public QGraphicsItem,
    public ProtoGItem,
    public OutlineGItem
{
    DECLARE_OUTLINE_GITEM;
    DECLARE_IN_ALLOC_POOL(anGItem);
public:

    anGItem();
    anGItem(topic_ptr item, ProtoPanel* p, int _ident);
    virtual ~anGItem();
    
    bool        isCheckSelected()const override{ return m_attr.CheckSelected; };
    void        setCheckSelected(bool v)override;
    bool        asHeaderTopic()const override { return isRootTopic(); };
    void        setAsRootTopic()override;
    
    EHitTest    hitTest(const QPointF& pt, SHit& hit)override;
    void        getOutlineEditRect(EColumnType column_type, QRectF& rc)override;

    bool        preprocessMousePress(const QPointF&, bool wasSelected)override;
    bool        preselectPreprocessMousePress(const QPointF&)override;
    void        mouseRightClick(const QPoint&)override;
    void        resetGeometry()override;
protected:
    void        dropEvent(QGraphicsSceneDragDropEvent *e)override;
    //void        drawCustomHotspots(const QRectF& rc, QPainter *painter, topic_ptr);

#ifdef _DEBUG
    virtual void    drawDebugInfo(QPainter *painter, const QStyleOptionGraphicsItem *option);
#endif    
  void            updateGeometryWidth()override;
protected:
  DECLARE_OUTLINE_GITEM_EVENTS;
};

#endif // ANGITEM_H

#pragma once

#include <QGraphicsItem>
#include "utils.h"
#include "ProtoScene.h"
#include "TopicPanel.h"

class anItem;
class TablePanelColumn;

namespace ard 
{
	class topic;
    class KRingKey;
};

#define FAT_FINGER_WIDTH 3 * gui::lineHeight()
#define FAT_FINGER_MAX_H 1.5 * gui::lineHeight()


#define DECLARE_OUTLINE_GITEM public:                          \
QRectF    boundingRect() const override { return m_rect; }; \
QGraphicsItem* g()override { return this; };            \
const QGraphicsItem* g()const override { return this; };    \
void            request_prepareGeometryChange() override { prepareGeometryChange(); }\
void      paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)override;\
protected:\
    ProtoGItem*           proto()override { return this; };\
    const ProtoGItem*     proto()const override { return this; };\



/**
   OutlineGItem - utility base class contains code
   for handling outline items in scene, works with QGraphicsItem
*/
class OutlineGItem: public DnDHelper
{
public:
    OutlineGItem();
    OutlineGItem(int outline_ident);
    virtual ~OutlineGItem();

    virtual ProtoGItem* proto() = 0;
    virtual const ProtoGItem* proto()const = 0;
    
    /// item rect without annotation area, idented in outline
    const QRectF&   identedRect()const;
    /// item rect without annotation area
    const QRectF&   nonidentedRect()const;
    /// annotation only area
    const QRectF&   annotationRect()const;
    /// note only area
    const QRectF&   expandedNoteRect()const;

    int             outlineIdent()const { return m_attr.outline_ident; };

protected:
    void             updateGuiAfterDnd(){ proto()->g()->update();};
    QRectF           dndRect()const{return identedRect();};
    virtual bool isRootTopic()const{return m_attr.asRootTopic;};
    virtual void drawToDoColumn(const QRectF& rc, QPainter *painter);
    virtual void drawDateLabel(QPainter *painter, const QStyleOptionGraphicsItem *option, const QRectF& rc);
    virtual void drawLabels(QPainter *painter, const QStyleOptionGraphicsItem *option);
    virtual void drawTitle(const QRectF& rc, QPainter *painter, const QStyleOptionGraphicsItem *option);
    virtual void drawIcons(QRectF& rc, QPainter *painter, const QStyleOptionGraphicsItem *option);
    virtual void drawActionBtn(QPainter *painter);
    virtual void drawExpandedNote(const QRectF& rc, QPainter *p, bool as_selected);
    virtual void drawAnnotation(const QRectF& rc, QPainter *p, bool as_selected, bool hover);

    void    drawCompletedLabel(QRectF& rc, QPainter *painter, const QStyleOptionGraphicsItem *option);
    void    drawEmail(QRectF& rc, QPainter *painter, const QStyleOptionGraphicsItem *option);
    void    drawFormFieldName(QRectF& rc, QPainter *painter, const QStyleOptionGraphicsItem *option);
    void    drawFormFieldValue(QRectF& rc, QPainter *painter, const QStyleOptionGraphicsItem *option);

    void    drawContactPhone(QRectF& rc, QPainter *painter, const QStyleOptionGraphicsItem *option);
    void    drawContactAddress(QRectF& rc, QPainter *painter, const QStyleOptionGraphicsItem *option);
    void    drawContactOrganization(QRectF& rc, QPainter *painter, const QStyleOptionGraphicsItem *option);
    void    drawContactNotes(QRectF& rc, QPainter *painter, const QStyleOptionGraphicsItem *option);
    void    drawFatFingerSelect(const QRectF& rc, QPainter *painter);

    void    drawKRingLogin(QRectF& rc, QPainter *painter, const QStyleOptionGraphicsItem *option);
    void    drawKRingPwd(QRectF& rc, QPainter *painter, const QStyleOptionGraphicsItem *option);
    void    drawKRingLink(QRectF& rc, QPainter *painter, const QStyleOptionGraphicsItem *option);
    void    drawKRingNotes(QRectF& rc, QPainter *painter, const QStyleOptionGraphicsItem *option);
	void	drawTitleWithThumbnail(const QRectF& rc, QPainter *painter, const QStyleOptionGraphicsItem *option);
	void	drawThumbnail(QRectF& rc, QPainter *painter, const QStyleOptionGraphicsItem *option);

    virtual EHitTest hitTestIdentedRect(const QRectF& rc, const QPointF& pt, SHit& hit);
    virtual bool onHitColumn(TablePanelColumn* c, const SHit& sh, const QPointF& p, bool wasSelected);

    virtual EDragOver      calcDragOverType(QGraphicsSceneDragDropEvent *event);
    virtual EDragOver  calcDragItemOver(topic_cptr dndItem, QGraphicsSceneDragDropEvent *event);
    virtual EDragOver  calcDragGenericOver(QGraphicsSceneDragDropEvent *event);    

    virtual void       dropGeneric(QGraphicsSceneDragDropEvent *e);
  
protected:
    virtual void recalcGeometry();
    bool isThumbnailDraw()const;
    void resetOutlineItemGeometry();
    void implement_hoverEnterEvent(QGraphicsSceneHoverEvent * e);
    void implement_hoverLeaveEvent(QGraphicsSceneHoverEvent * e);
    void implement_hoverMoveEvent(QGraphicsSceneHoverEvent * e);
    bool implement_preprocessMousePress(const QPointF& p, bool wasSelected);
    void implement_keyPressEvent(QKeyEvent * event );
    bool implement_mousePressEvent(QGraphicsSceneMouseEvent *);
    void implement_mouseReleaseEvent(QGraphicsSceneMouseEvent *);
    void implement_mouseMoveEvent(QGraphicsSceneMouseEvent *);

    virtual void    drawText(QString s, QRectF& rc, QPainter *painter, const QStyleOptionGraphicsItem *option);
protected:
    QRectF          m_rect;
    union UATTR
    {
        uint64_t flag;
        struct
        {
            unsigned outline_ident          : 10;
            unsigned ident_adjustment       : 7;
            unsigned annotation_height      : 12;
            unsigned expanded_note_height   : 12;
            unsigned asRootTopic            : 1;
            unsigned CheckSelected          : 1;
            unsigned Hover                  : 1;
			unsigned rbutton_width			: 7;
        };
    } m_attr;
};

#define DECLARE_OUTLINE_GITEM_EVENTS                                    \
    void hoverEnterEvent            (QGraphicsSceneHoverEvent* )override; \
    void hoverLeaveEvent            (QGraphicsSceneHoverEvent* )override; \
    void hoverMoveEvent             (QGraphicsSceneHoverEvent* )override; \
    void keyPressEvent              (QKeyEvent* e )override;    \
    void dragEnterEvent             (QGraphicsSceneDragDropEvent *)override; \
    void dragLeaveEvent             (QGraphicsSceneDragDropEvent *)override; \
    void dragMoveEvent              (QGraphicsSceneDragDropEvent *)override; \
    void mousePressEvent            (QGraphicsSceneMouseEvent* )override; \
    void mouseReleaseEvent          (QGraphicsSceneMouseEvent* )override; \
    void mouseMoveEvent             (QGraphicsSceneMouseEvent* )override; \
    void mouseDoubleClickEvent      (QGraphicsSceneMouseEvent* )override; \




#define IMPLEMENT_OUTLINE_GITEM_EVENTS(T)                               \
    void T::hoverEnterEvent(QGraphicsSceneHoverEvent * e )              \
    {                                                                   \
        implement_hoverEnterEvent(e);                                   \
    };                                                                  \
    void T::hoverLeaveEvent(QGraphicsSceneHoverEvent * e )              \
    {                                                                   \
        implement_hoverLeaveEvent(e);                                   \
    };                                                                  \
    void T::hoverMoveEvent(QGraphicsSceneHoverEvent * e)                \
    {                                                                   \
        implement_hoverMoveEvent(e);                                    \
        QGraphicsItem::hoverMoveEvent(e);                               \
    }                                                                   \
    void T::keyPressEvent(QKeyEvent * e )                               \
    {                                                                   \
        QGraphicsItem::keyPressEvent(e);                                \
        implement_keyPressEvent(e);                                     \
    }                                                                   \
    void T::dragEnterEvent(QGraphicsSceneDragDropEvent *e)              \
    {                                                                   \
        process_dragEnterEvent(e);                                      \
    };                                                                  \
    void T::dragLeaveEvent(QGraphicsSceneDragDropEvent *e)              \
    {                                                                   \
        process_dragLeaveEvent(e);                                      \
    };                                                                  \
    void T::dragMoveEvent(QGraphicsSceneDragDropEvent * event )         \
    {                                                                   \
        if(proto()->p()->hasProp(ProtoPanel::PP_DnD))                   \
            {                                                           \
                if(event->isAccepted())                                 \
                    {                                                   \
                        EDragOver dragOver = calcDragOverType(event);   \
                        if(dragOver != m_dragOver)                      \
                            {                                           \
                                m_dragOver = dragOver;                  \
                                update();                               \
                            }                                           \
                    }                                                   \
            }                                                           \
    };                                                                  \
    void T::mousePressEvent         (QGraphicsSceneMouseEvent* e)       \
    {                                                                   \
        if(!implement_mousePressEvent(e)){                              \
            QGraphicsItem::mousePressEvent(e);                          \
        }                                                               \
    };                                                                  \
    void T::mouseReleaseEvent(QGraphicsSceneMouseEvent* e)              \
    {                                                                   \
        QGraphicsItem::mouseReleaseEvent(e);                            \
        implement_mouseReleaseEvent(e);                                 \
    };                                                                  \
    void T::mouseMoveEvent(QGraphicsSceneMouseEvent* e)                 \
    {                                                                   \
        QGraphicsItem::mouseMoveEvent(e);                               \
        implement_mouseMoveEvent(e);                                    \
    };                                                                  \
    void T::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *  )         \
    {                                                                   \
        proto()->processDoubleClickAction();                            \
    };                                                                  \






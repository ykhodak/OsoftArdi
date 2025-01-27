#pragma once

#include <QWidget>
#include "tooltopic.h"
#include "TopicPanel.h"
#include "anGItem.h"
#include "custom-widgets.h"

class TablePanelColumn
{
public:
    TablePanelColumn(EColumnType t):m_type(t){m_width = 20;}

    EColumnType type()const{return m_type;}
    const qreal& width()const{return m_width;}
    void setWidth(const qreal& w){m_width = w;};

    const qreal& xpos()const{return m_XPos;}
    void setXPos(const qreal&  v){m_XPos = v;};
    QString label()const;

    bool visible()const{return m_visible;}
    void setVisible(bool val){m_visible = val;}
    bool isTitleColumn()const;
    
    static QString type2label(EColumnType t, bool shortLabel = true);

protected:
    EColumnType m_type;
    qreal m_width;
    qreal m_XPos;//derived value from width & viewport width
    bool m_visible{true};
};

/**
   TablePanel - table with header in outline
*/
class TablePanel: public TopicPanel
{
public:
  
    typedef std::vector<TablePanelColumn*> COLUMNS;

public:
    TablePanel(ProtoScene* s);
    virtual ~TablePanel();

    TablePanel&       addColumn(EColumnType t);

    const COLUMNS&    columns()const{return m_columns;}
    COLUMNS&          columns(){return m_columns;}
    void              rebuildPanel()override;
    void              freeItems   ()override;
    void              clear       ()override;
    qreal             textColumnWidth()const override;
    void              createAuxItems()override;
    void              updateAuxItemsPos()override;
    QString           pname()const override{return "table-panel";};
    TablePanelColumn* findColumn(EColumnType t);
	ProtoGItem*     produceOutlineItems(topic_ptr it,
		const qreal& x_ident,
		qreal& y_pos,
		GITEMS_VECTOR* registrator = nullptr)override;

protected:
    void              resetGeometry()override;
    TablePanelColumn* findFirstTitleColumn();
    qreal             calcNonTitleColumnsWidth();
protected:
    COLUMNS m_columns;
    qreal m_text_column_width;
    LINES_LIST m_column_lines;
};

/**
   anGTableItem - scene item for table panels
*/
class anGTableItem : public QGraphicsItem,
             public ProtoGItem,
             public OutlineGItem
{
    DECLARE_OUTLINE_GITEM;
public:
 
  anGTableItem(topic_ptr item, TablePanel* p, int _ident);
  bool            isCheckSelected()const override { return false; };
  EHitTest        hitTest(const QPointF& pt, SHit& hit)override;
  void            resetGeometry()override;
  void            updateGeometryWidth()override;
  bool            preprocessMousePress(const QPointF&, bool wasSelected)override;
  void            getOutlineEditRect(EColumnType column_type, QRectF& rc)override;
protected:
  DECLARE_OUTLINE_GITEM_EVENTS;
};

class FormPanel;

/**
a big multiline note at the bottom of panel
*/
class FormPanelNoteFooterGItem : public anGTableItem
{
public:
    FormPanelNoteFooterGItem(FormFieldTopic::ptr tt, FormPanel* p);

    void            paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *)override;
    EHitTest        hitTest(const QPointF& pt, SHit& hit)override;
    void            getOutlineEditRect(EColumnType column_type, QRectF& rc)override;
    void            recalcGeometry()override;
};




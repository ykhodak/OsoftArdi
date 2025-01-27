#pragma once


#include "a-db-utils.h"
#include "custom-g-items.h"
#include "ProtoScene.h"

/*
  ProtoToolItem - some button control on scene, with or without image,
  could be set of buttons alocated in one row
*/
class ProtoToolItem : public ProtoGItem,
              public QGraphicsItem
{
public:
  enum EType
    {
      typeToolButton,
      typeButtonsSet,
      typeTrueImgSize
    };

  class SetButton
  {
  public:
    SetButton(EAsyncCallRequest c, QString l, QPixmap* i, int param = -1, QVariant param2 = QVariant());
    SetButton(EAsyncCallRequest c, QString l, QString iconRes, int param = -1, QVariant param2 = QVariant());
    
    EAsyncCallRequest m_command;
    QString           m_label;
    QPixmap           m_icon;
    QRectF            m_rect;
    int               m_param;
    QVariant          m_param2;
  };
  typedef std::vector<SetButton> BUTTONS_SET;

  ProtoToolItem(ProtoPanel*);
  ProtoToolItem(ProtoPanel*, topic_ptr);
  virtual ~ProtoToolItem();
  
  QGraphicsItem* g(){return this;};
  const QGraphicsItem* g()const{return this;};
  virtual EHitTest  hitTest(const QPointF&, SHit&){return hitTitle;};
  virtual QRectF    boundingRect() const{return m_rect;};
  virtual void      applyCommand(){};
  virtual QString   label()const;
  virtual QString   value()const{return "";};
  virtual bool      asCurrent()const{return /*false*/isSelected();};
  void              setHeight(int h);
  void              setMinSize(QSize sz);
  
  QString   iconRes()const{return m_iconRes;};
  void      setIconRes(QString s){m_iconRes = s;}
  //  QRectF          calcTitleRect()const;

  EAsyncCallRequest command()const{return m_command;}
  void              setCommand(EAsyncCallRequest cmd){m_command = cmd;}
  int               commandParam()const{return m_command_param;}
  void              setCommandParam(int val){m_command_param = val;}

  void              setLabel(QString s){m_label = s;}

  void              setButtonsSet(const BUTTONS_SET& bset, int h = -1);
  void              setPixmap(QPixmap pm, QString str_label);

  EType             buttonType()const{return m_type;}
  void              setButtonType(EType t){m_type = t;}
  void              request_prepareGeometryChange(){prepareGeometryChange();}

  bool              preprocessMousePress(const QPointF&, bool wasSelected);
  void              updateGeometryWidth();


protected:
  void              init();

  void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

  void drawToolButton(QPainter *painter);
  void drawButtonSet(QPainter *painter);
  void drawTrueImgButton(QPainter *painter);

protected:
  QRectF            m_rect;
  QString           m_iconRes;
  EType             m_type;
  EAsyncCallRequest m_command;
  QString           m_label;
  QPixmap           m_icon;
  int               m_command_param;
  BUTTONS_SET       m_buttons_set;
};

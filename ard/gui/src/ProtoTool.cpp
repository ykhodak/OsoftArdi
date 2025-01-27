#include "ProtoTool.h"
#include "utils.h"
#include "ardmodel.h"
#include "custom-widgets.h"
#include "tooltopic.h"

/**
   ProtoToolItem 
*/
ProtoToolItem::SetButton::SetButton(EAsyncCallRequest c, QString l, QPixmap* i, int param, QVariant param2)
  :m_command(c), m_label(l),  m_param(param), m_param2(param2)
{
  if(i){    
      m_icon = *i;
    }
};

ProtoToolItem::SetButton::SetButton(EAsyncCallRequest c, QString l, QString iconRes, int param, QVariant param2)
  :m_command(c), m_label(l),  m_param(param), m_param2(param2)
{
  m_icon = QPixmap(iconRes);
};


ProtoToolItem::ProtoToolItem(ProtoPanel* p)
  :ProtoGItem(new ard::tool_topic, p)
{
  init();
};

ProtoToolItem::ProtoToolItem(ProtoPanel* p, topic_ptr o)
  :ProtoGItem(o, p)
{
  LOCK(o);
  init();
};

void ProtoToolItem::init()
{
  setFlags(QGraphicsItem::ItemIsSelectable|QGraphicsItem::ItemIsFocusable);
  m_rect = QRectF(0,0,m_p->panelWidth(),gui::lineHeight());
  setOpacity(DEFAULT_OPACITY);
  p()->s()->s()->addItem(this);
  m_type = typeToolButton;
  m_command = AR_none;
  m_command_param = 0;
};

void ProtoToolItem::setHeight(int h)
{
  prepareGeometryChange();
  m_rect.setHeight(h);
};

void ProtoToolItem::setMinSize(QSize sz)
{
  prepareGeometryChange();
  m_rect.setRect(0,0,sz.width(),sz.height());
};

void ProtoToolItem::setButtonsSet(const BUTTONS_SET& bset, int h)
{
  int b_height = h;
  if(b_height == -1)
    {
      b_height = gui::lineHeight();
    }
  
  m_buttons_set = bset;
  setButtonType(typeButtonsSet);
  int w = 0;
  for(BUTTONS_SET::iterator i = m_buttons_set.begin();i != m_buttons_set.end();i++)
    {
      SetButton& sb = *i;
      w += b_height + ARD_MARGIN + utils::calcWidth(sb.m_label, utils::defaultBoldFont()) + 10*ARD_MARGIN;
    }
  setMinSize(QSize(w, b_height));
};

void ProtoToolItem::setPixmap(QPixmap pm, QString str_label)
{
    //m_icon = //QPixmap(pic_resource);
    m_icon = pm;
    auto sz = m_icon.size();    
    m_label = str_label;
    int w = sz.height() + ARD_MARGIN + utils::calcWidth(str_label, utils::defaultBoldFont()) + 10 * ARD_MARGIN;
    setMinSize(QSize(w, sz.height()));
    m_type = typeTrueImgSize;
};

ProtoToolItem::~ProtoToolItem()
{
  if(m_item)
    m_item->release();
};

bool ProtoToolItem::preprocessMousePress(const QPointF& pt1, bool)
{
  QPointF pt = mapFromScene(pt1);

  if(typeButtonsSet == buttonType())
    {
      for(BUTTONS_SET::iterator i = m_buttons_set.begin();i != m_buttons_set.end();i++)
    {
      SetButton& sb = *i;
      if(sb.m_rect.contains(pt))
          {
          switch(sb.m_command)
        {
        case AR_none:break;
        default:
          {         
            QObject* op = p()->s()->v()->sceneBuilder();
            if(op)
            {
            QMetaObject::invokeMethod(op, "toolButtonPressed",
                          Qt::QueuedConnection,
                          Q_ARG(int, sb.m_command),
                          Q_ARG(int, sb.m_param),
                          Q_ARG(QVariant, sb.m_param2));
              }
            if(AR_BuilderDelegate != sb.m_command)
              {
                ard::asyncExec(sb.m_command, sb.m_param);
              }
          }
        }
        }
    }      
    }
  else
  {
      if (m_command != AR_none)
      {
          ard::asyncExec(m_command, m_command_param);
      }
      else
      {
          applyCommand();
      }
  }
  return true;
};

QString ProtoToolItem::label()const
{
  if(!m_label.isEmpty())
    {
      return m_label;
    }
  ASSERT(m_item, "expected item");
  return m_item->title();
};


void ProtoToolItem::drawToolButton(QPainter *painter)
{
    QRectF rc = m_rect;

    bool asCurr = asCurrent();
    if (asCurr)
    {
        PGUARD(painter);
        painter->setPen(Qt::NoPen);
        QBrush brush(model()->brushSelectedItem());
        painter->setBrush(brush);
        painter->drawRect(m_rect);
    }

    PGUARD(painter);
    if (asCurr)
    {
        QPen penText(Qt::black);
        painter->setPen(penText);
    }
    painter->setFont(*ard::defaultFont());
    int flags = Qt::AlignLeft;
    painter->drawText(rc, flags, label());
    qreal lw = utils::calcWidth(label());
    flags = Qt::AlignRight;
    QString s = QString("%1").arg(value());
    painter->drawText(rc, flags, s);
    if (!iconRes().isEmpty())
    {
        qreal w = utils::calcWidth(s);
        rc.setLeft(rc.left() + w);
        QPointF pt = rc.topLeft();
        pt.setX(pt.x() + lw + 1);
        int img_w = (int)rc.height();
        QPixmap pm(iconRes());
        painter->drawPixmap((int)pt.x(), (int)pt.y(), img_w, img_w, pm);
    }
};

void ProtoToolItem::drawTrueImgButton(QPainter *p) 
{
    QRectF rc = m_rect;

    bool asCurr = asCurrent();
    if (asCurr)
    {
        PGUARD(p);
        p->setPen(Qt::NoPen);
        QBrush brush(model()->brushSelectedItem());
        p->setBrush(brush);
        p->drawRect(m_rect);
    }

    PGUARD(p);
    p->setRenderHint(QPainter::Antialiasing, true);
    if (asCurr)
    {
        QPen penText(Qt::black);
        p->setPen(penText);
    }
    p->setFont(*utils::defaultBoldFont());

    int flags = Qt::AlignLeft | Qt::AlignVCenter;
    QPointF pt = rc.topLeft();  
    int img_y = m_icon.height();
    p->drawPixmap(pt, m_icon);
    rc.setLeft(rc.left() + img_y);
    p->drawText(rc, flags, m_label);
};

void ProtoToolItem::drawButtonSet(QPainter *painter)
{
    QRectF rc = m_rect;

    bool asCurr = asCurrent();
    if (asCurr)
    {
        PGUARD(painter);
        painter->setPen(Qt::NoPen);
        QBrush brush(model()->brushSelectedItem());
        painter->setBrush(brush);
        painter->drawRect(m_rect);
    }

    PGUARD(painter);
    painter->setRenderHint(QPainter::Antialiasing, true);
    if (asCurr)
    {
        QPen penText(Qt::black);
        painter->setPen(penText);
    }
    painter->setFont(*utils::defaultBoldFont());
    int flags = Qt::AlignLeft | Qt::AlignVCenter;
    if (rc.height() > 1.5*gui::lineHeight())
    {
        flags |= Qt::TextWordWrap;
    }
    int img_w = gui::lineHeight();
    QPointF pt = rc.topLeft();
    for (BUTTONS_SET::iterator i = m_buttons_set.begin(); i != m_buttons_set.end(); i++)
    {
        SetButton& sb = *i;
        qreal x_begin = pt.x();
        if (!sb.m_icon.isNull())
        {
            int img_y = (int)((rc.height() - img_w) / 2);
            painter->drawPixmap((int)pt.x(), img_y, img_w, img_w, sb.m_icon);
            pt.setX(pt.x() + img_w + ARD_MARGIN);
        }
        if (!sb.m_label.isEmpty())
        {
            rc.setLeft(pt.x());
            painter->drawText(rc, flags, sb.m_label);
            pt.setX(pt.x() + utils::calcWidth(sb.m_label, utils::defaultBoldFont()) + 10 * ARD_MARGIN);
        }
        sb.m_rect = m_rect;
        sb.m_rect.setLeft(x_begin);
        sb.m_rect.setRight(pt.x());
    }
};

void ProtoToolItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
{
  switch(m_type)
    {
    case typeToolButton:drawToolButton(painter);break;
    case typeButtonsSet:drawButtonSet(painter);break;
    case typeTrueImgSize:drawTrueImgButton(painter); break;
    }
};


void ProtoToolItem::updateGeometryWidth()
{
  int h = m_rect.height();
  if(h <= 0)
    h = gui::lineHeight();
  m_rect = QRectF(0,0,m_p->panelWidth(), h);

  prepareGeometryChange();
  update();
};

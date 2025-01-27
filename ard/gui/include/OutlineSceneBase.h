#pragma once

#include <map>
#include <QGraphicsScene>

#include "ProtoScene.h"
#include "custom-widgets.h"

class OutlineSceneBase: public ArdGraphicsScene,
                        public ProtoScene
{
  friend class OutlineView;
  Q_OBJECT
public:

    
  class Builder
  {
  public:
        using ptr = std::unique_ptr<Builder>;

        Builder(OutlineSceneBase* bscene):m_baseScene(bscene){};
        virtual ~Builder(){};
        virtual void build() = 0;
        virtual void setBuilderParameter(QString) {};
        void setSearchString(QString str) { m_str_search = str; }
        QString searchString()const { return m_str_search; }
  protected:
      OutlineSceneBase* m_baseScene{nullptr};
      QString           m_str_search;
  };
 

  OutlineSceneBase(OutlineView* _view);
  virtual ~OutlineSceneBase();

  ArdGraphicsScene* s()override{return this;};
  const ArdGraphicsScene* s()const override{return this;};
  ProtoView* v()override;
  const ProtoView* v()const override;

  ProtoGItem*     currentGI(bool acceptToolButtons = true)override;
  bool            ensureVisible2(topic_ptr it, bool select = true);
  bool            ensureVisibleByEid(QString eid);
  bool            ensureVisible(std::function<bool (topic_ptr)> findBy);
    
  OutlineView*    outline_view(){return m_view;}
  void            clearOutline()override;
  void            detachGui();

  topic_ptr       hoisted(){return m_hoisted;}
  void            attachHoisted(topic_ptr h);
  void            detachHoisted();

  void            updateItem(topic_ptr it)override;

  virtual QString name()const {return "outline-scene-base";};
  
  void            setOutlinePolicy(EOutlinePolicy p)override;
  void            rebuild();
  void            updateHudItemsPos();

  void            setIncludeRoot(bool val){m_include_root_in_outline = val;};

  void            attachOutlineBuilder(Builder::ptr&& b);
  void            setBuilderSearchString(QString str);
  void            resetColors();
protected:
  virtual void		doRebuild();
  void				installCurrentItemView(bool propagate2workspace)override;


protected:
  bool                  m_include_root_in_outline;
  OutlineView*          m_view{nullptr};
  topic_ptr             m_hoisted{nullptr};
  Builder::ptr          m_outlineBuilder;
};

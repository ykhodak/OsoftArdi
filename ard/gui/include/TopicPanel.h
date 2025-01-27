#pragma once

#include <map>
#include <vector>
#include <QGraphicsScene>
#include "utils.h"
#include "ProtoScene.h"

namespace ard 
{
	class topic;
};

/**
   TopicPanel - panel of a tree with topic() as root node
   a base class for other more specific types of outline
*/
class TopicPanel :public ProtoPanel
{
public:
  TopicPanel(ProtoScene* s);
  virtual ~TopicPanel();


  QString              name()const{return "generic-topic-panel";};

  void              attachTopic (topic_ptr);
  void              detachTopic ();
  void              attachTopicsList(TOPICS_LIST& lst);
  topic_ptr         topic()const;
  bool              includeRoot()const{return m_includeRoot;}
  void              setIncludeRoot(bool v){m_includeRoot = v;}

  void      outlineHoisted(qreal ident, bool includeHoisted = true);
  void      outlineHoistedAsList(qreal ident, bool includeHoisted = true);

protected:
  ///display normal outline
  void outlineBase(ard::topic *f, qreal xident, GITEMS_VECTOR* goutlined = nullptr);
  void outlineBaseAsList(ard::topic *f, qreal xident, GITEMS_VECTOR* goutlined = nullptr);
  virtual bool shouldOutlineParentBase(ard::topic*)const { return  true; }

protected:
    TOPICS_LIST         m_hoisted_topics;
    qreal               m_outline_progress_y_pos{0};
    int                 m_outline_y_offset{0};
    bool                m_includeRoot;
};

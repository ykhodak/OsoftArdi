#pragma once

#include "TopicPanel.h"


class anGItem;
class anGThumbnail;
class QGraphicsLineItem;
class HierarchyBranchMark;

/**
   OutlinePanel - shows tree as outline with collapsable nodes
*/

class OutlinePanel: public TopicPanel
{
  enum EType
    {
      panel_Uknown = -1,
      panel_Outline,
      panel_List,
      panel_MList
    };
public:
  OutlinePanel(ProtoScene* s);
 
  void           outlineTopic(ard::topic *f, int yoffset);
  void           outlineTopicsList(TOPICS_LIST& lst, int yoffset);
  virtual void   outlineTopicAsList(ard::topic *f);
  void           listMTopics(TOPICS_LIST& topic_list);
  void           clear()override;
  void           rebuildPanel()override;
  int            outlinedCount()const{return static_cast<int>(m_outlined.size());}
  void           createAuxItems()override;
  QString        pname()const override{return "outline-panel";};
  void           setCurrentSelected(topic_ptr t){m_currSelected = t;};
  void           resetPosAfterGeometryReset()override { resetPosAfterGeometryResetForOutlines(); }
  const TOPICS_LIST&  outlinedTopTopics()const { return m_topic_list; }
protected:
  void           resetGeometry()override;
protected:
  EType         m_type;
  TOPICS_LIST   m_topic_list;//todo - deprecate
  topic_ptr     m_currSelected{nullptr};
};


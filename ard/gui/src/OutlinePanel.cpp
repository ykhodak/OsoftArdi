#include "OutlinePanel.h"
#include "anGItem.h"
#include "anfolder.h"
#include "OutlineView.h"
#include "custom-g-items.h"

OutlinePanel::OutlinePanel(ProtoScene* s):
    TopicPanel(s),
    m_type(panel_Uknown)
{
    std::set<EProp> p;
    SET_PPP(p, PP_RTF);
    SET_PPP(p, PP_DnD);
    SET_PPP(p, PP_Filter);
    SET_PPP(p, PP_ToDoColumn);
    SET_PPP(p, PP_CurrSpot);
    SET_PPP(p, PP_MultiLine);
    SET_PPP(p, PP_InplaceEdit);
    setProp(&p);
};
  
void OutlinePanel::clear()
{
    TopicPanel::clear();
    m_outline_progress_y_pos = 0.0;  
};


/*
  resetGeometry - we have to recalc/reset items y-pos, because width&height could
  have changed during resize, for example - we keep multiline records
*/
void OutlinePanel::resetGeometry()
{
    resetClassicOutlinerGeometry();
};

void OutlinePanel::rebuildPanel()
{
    qreal x_ident = 0.0;
    m_outline_progress_y_pos = m_outline_y_offset;

    auto t = topic();

    switch (m_type)
    {
    case panel_Outline:
    {
        if (!t)
            return;

        if (m_panelWidth < 150 || m_panelWidth > 1200)
            m_panelWidth = 700;

        outlineHoisted(x_ident, m_includeRoot);
    }break;
    case panel_List:
    {
        if (!t)
            return;
        if (m_panelWidth < 150 || m_panelWidth > 1200)
            m_panelWidth = 700;
        outlineHoistedAsList(x_ident, m_includeRoot);
    }break;
    case panel_MList:
    {
        for (auto f : m_topic_list)
        {
            produceOutlineItems(f,
                x_ident,
                m_outline_progress_y_pos);
        }
    }break;
    default:return;
    }

    createAuxItems();
};

void OutlinePanel::outlineTopic(ard::topic *f, int yoffset)
{
    m_type = panel_Outline;
    m_outline_y_offset = yoffset;
    attachTopic(f);
    rebuildPanel();
};

void OutlinePanel::outlineTopicAsList(ard::topic* f)
{
    m_type = panel_List;
    m_outline_y_offset = 0;
    attachTopic(f);
    rebuildPanel();
    //TopicPanel::outlineTopic(f, 0);
};

void OutlinePanel::outlineTopicsList(TOPICS_LIST& lst, int yoffset) 
{
    m_type = panel_Outline;
    m_outline_y_offset = yoffset;
    attachTopicsList(lst);
    rebuildPanel();
};

void OutlinePanel::createAuxItems()
{
	/*
    if(hasProp(ProtoPanel::PP_CurrSelect))
        {
            topic_ptr asCurr = m_currSelected;
            if(!asCurr)
                {
                    if(gui::isDBAttached())
                        {
                            asCurr = ard::hoisted();
                        }
                }
            if(!asCurr)
                {
                    ProtoGItem* gi = findGI(asCurr);
                    if(gi)
                        {
                            CurrentMarkCloud* curr_hoisted = new CurrentMarkCloud(CurrentMarkCloud::typeHoisted_Highlight);
                            curr_hoisted->setup(gi, 0.0);
                            m_aux2.push_back(curr_hoisted);
                            s()->s()->addItem(curr_hoisted);
                        }
                }      
        }
		*/
    TopicPanel::createAuxItems();
};

void OutlinePanel::listMTopics(TOPICS_LIST& topic_list)
{
    m_type = panel_MList;
    m_topic_list = topic_list;
    rebuildPanel();
};

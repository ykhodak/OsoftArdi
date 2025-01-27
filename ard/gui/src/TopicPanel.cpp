#include "TopicPanel.h"
#include "anfolder.h"
#include "OutlineSceneBase.h"
#include "OutlineView.h"
#include "custom-g-items.h"


/**
   TopicPanel
*/

TopicPanel::TopicPanel(ProtoScene* s):
    ProtoPanel(s),
    //m_topic(nullptr),
    m_outline_progress_y_pos(0.0),
    m_includeRoot(true)
{
};

TopicPanel::~TopicPanel()
{
    detachTopic();
};

topic_ptr TopicPanel::topic()const 
{
    topic_ptr rv = nullptr;
    if (!m_hoisted_topics.empty())
    {
        auto i = m_hoisted_topics.begin();
        rv = *i;
    }
    return rv;
}

void TopicPanel::attachTopic(topic_ptr t)
{
    if (!m_hoisted_topics.empty()) 
    {
        auto i = m_hoisted_topics.begin();
        if (*i != t) 
        {
            detachTopic();
            if (t)
            {
                LOCK(t);
                m_hoisted_topics.push_back(t);
            }
        }
    }
    else 
    {
        if (t)
        {
            LOCK(t);
            m_hoisted_topics.push_back(t);
        }
    }
};

void TopicPanel::detachTopic()
{
    for (auto& i : m_hoisted_topics) {
        i->release();
    }
    m_hoisted_topics.clear();

    clear();
};

void TopicPanel::attachTopicsList(TOPICS_LIST& lst) 
{
    detachTopic();
    m_hoisted_topics = lst;
    for (auto& i : m_hoisted_topics) {
        LOCK(i);
    }
};

void TopicPanel::outlineHoisted(qreal x_ident, bool includeHoisted /*= true*/)
{
    //auto t = topic();
    //assert_return_void(t, "expected hoisted");

    for (auto& t : m_hoisted_topics)
    {
        if (includeHoisted)
        {
            auto g = produceOutlineItems(t,
                x_ident,
                m_outline_progress_y_pos);
            if (g) {
                g->setAsRootTopic();
            }
        }

        outlineBase(t, x_ident, nullptr);
    }
}

//..
void TopicPanel::outlineHoistedAsList(qreal x_ident, bool includeHoisted /*= true*/)
{
    auto t = topic();
    assert_return_void(t, "expected hoisted");

    bool expandHoisted = true;
    if (includeHoisted)
    {
        auto g = produceOutlineItems(t,
            x_ident,
            m_outline_progress_y_pos);
        if (g) {
            g->setAsRootTopic();
        }
        expandHoisted = t->isExpanded();
    }

    if (expandHoisted) {
        if (hasProp(PP_ActionButton)) {
            x_ident += actionBtnWidth();
        };
        outlineBaseAsList(t, x_ident, nullptr);
    }
}


void TopicPanel::outlineBase(ard::topic *f, qreal xident, GITEMS_VECTOR* goutlined /*= nullptr*/)
{
    if (!shouldOutlineParentBase(f)) {
        return;
    }

    bool filterSupported = hasProp(ProtoPanel::PP_Filter);
    bool filterIn = true;      
	f->applyOnVisible([&](ard::topic* it)
	{
            filterIn = true;

            if(filterSupported && 
                globalTextFilter().isActive())
                {
                    if(it->isExpanded())
                        {
                            filterIn = true;
                        }
                    else
                        {
                            filterIn = globalTextFilter().filterIn(it);
                        }
                }

            if (!filterIn) {
                if (it == ard::hoistedInFilter()) {
                    filterIn = true;
                }
            }

            if(filterIn){
                    produceOutlineItems(it, 
                                        xident, 
                                        m_outline_progress_y_pos,
                                        goutlined);
                }
            if(it->isExpanded()){
                    outlineBase(it, xident + ICON_WIDTH, goutlined);
                }
	});
};

void TopicPanel::outlineBaseAsList(ard::topic *f, qreal xident, GITEMS_VECTOR* goutlined /*= nullptr*/)
{
    bool filterSupported = hasProp(ProtoPanel::PP_Filter);
    bool filterIn = true;

	f->applyOnVisible([&](ard::topic* it)
    {
        filterIn = true;

        if (filterSupported &&
            globalTextFilter().isActive())
        {
            if (it->isExpanded())
            {
                filterIn = true;
            }
            else
            {
                filterIn = globalTextFilter().filterIn(it);
            }
        }

        if (!filterIn) {
            if (it == ard::hoistedInFilter()) {
                filterIn = true;
            }
        }

        if (filterIn) {
            produceOutlineItems(it,
                xident,
                m_outline_progress_y_pos,
                goutlined);
        }
        if (it->isExpandedInList()) {
            outlineBase(it, xident + ICON_WIDTH, goutlined);
        }
	});
};




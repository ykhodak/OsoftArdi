#include "ard-mime.h"
#include "anfolder.h"

#ifndef QT_NO_CLIPBOARD

/*
ard::TopicsListMime::TopicsListMime(const TOPICS_LIST& lst) 
{
    m_topics = lst;
    for (auto& i : m_topics) {
        auto f2 = i->shortcutUnderlying();
        LOCK(f2);
    }
};*/

ard::TopicsListMime::TopicsListMime(topic_ptr f) 
{
    auto f2 = f->shortcutUnderlying();
    LOCK(f2);
    m_topics.push_back(f2);
};

ard::TopicsListMime::~TopicsListMime() 
{
    for (auto& i : m_topics) {
        i->release();
    }
};


#endif //#ifndef QT_NO_CLIPBOARD

#pragma once

#include <QMimeData>
#include "a-db-utils.h"

class anItem;
class QGraphicsItem;

#ifndef QT_NO_CLIPBOARD

namespace ard {
	class topic;
    class TopicsListMime : public QMimeData
    {
		Q_OBJECT;
    public:
		template<class IT> TopicsListMime(IT b, IT e);
        TopicsListMime(topic_ptr f);
        virtual ~TopicsListMime();

        const TOPICS_LIST&  topics()const { return m_topics; }
        TOPICS_LIST&        topics(){ return m_topics; }
    protected:
        TOPICS_LIST     m_topics;
    };
};


template<class IT> 
ard::TopicsListMime::TopicsListMime(IT b, IT e) 
{
	IT i = b;
	while (i != e) 
	{
		auto f = *i;
		LOCK(f);
		m_topics.push_back(f);
		i++;
	}
};

#endif //#ifndef QT_NO_CLIPBOARD

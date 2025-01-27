#include <QTimer>
#include "gmail/GmailRoutes.h"
#include "GoogleClient.h"
#include "Endpoint.h"

 #include "a-db-utils.h"
#include "ansearch.h"
#include "email.h"
#include "ethread.h"
#include "rule.h"
#include "rule_runner.h"
#include "email_draft.h"

using namespace googleQt;


/**
    anobservable_topic
*/
ard::observable_topic::~observable_topic() 
{
    snc::clear_locked_vector(m_observed_topics);
};

void ard::observable_topic::applyOnVisible(std::function<void(ard::topic*)> fnc) 
{
	for (auto& f : m_observed_topics)
	{
		fnc(f);
	}
};

void ard::observable_topic::standard_sort()
{
    std::sort(m_observed_topics.begin(), m_observed_topics.end(), [](ard::topic* t1, ard::topic* t2)
    {
        //auto it1 = dynamic_cast<ard::topic*>(t1);
        //auto it2 = dynamic_cast<ard::topic*>(t2);
		bool rv = (t1->mod_time() > t2->mod_time());
        return rv;
    });
};

void ard::observable_topic::clear_observed()
{
    snc::clear_locked_vector(m_observed_topics);
};



/**
    local_search_observer
*/
QString ard::local_search_observer::title()const
{
    QString rv = "search:" + m_local_search;
    return rv;
};

void ard::local_search_observer::selectByText(QString local_search)
{
    m_local_search = local_search;
    rebuild_observed();
};

void ard::local_search_observer::rebuild_observed()
{
    assert_return_void(ard::db(), "expected open DB");
    setExpanded(true);
    snc::clear_locked_vector(m_observed_topics);
    m_observed_topics = dbp::findByText(m_local_search, &dbp::defaultDB());
};

/**
    task_ring_observer
*/
QString ard::task_ring_observer::title()const 
{
    return "Task Ring";
};

void ard::task_ring_observer::rebuild_observed()
{
    assert_return_void(ard::db(), "expected open DB");
    setExpanded(true);
    snc::clear_locked_vector(m_observed_topics);
	m_observed_topics = dbp::findToDos(&dbp::defaultDB()); 
    standard_sort();
};

/**
notes_observer
*/
QString ard::notes_observer::title()const
{
    return "Notes";
};

void ard::notes_observer::rebuild_observed()
{
    assert_return_void(ard::db(), "expected open DB");

    setExpanded(true);
    snc::clear_locked_vector(m_observed_topics);
	m_observed_topics = dbp::findNotes(&dbp::defaultDB());
    standard_sort();
};

/**
bookmarks_observer
*/
QString ard::bookmarks_observer::title()const
{
	return "Bookmarks";
};

void ard::bookmarks_observer::rebuild_observed()
{
	assert_return_void(ard::db(), "expected open DB");

	setExpanded(true);
	snc::clear_locked_vector(m_observed_topics);
	m_observed_topics = dbp::findBookmarks(&dbp::defaultDB());
	standard_sort();
};



QString ard::pictures_observer::title()const
{
	return "Pictures";
};

void ard::pictures_observer::rebuild_observed()
{
	assert_return_void(ard::db(), "expected open DB");

	setExpanded(true);
	snc::clear_locked_vector(m_observed_topics);
	m_observed_topics = dbp::findPictures(&dbp::defaultDB());
	standard_sort();
};

/**
    comments_observer
*/
QString ard::comments_observer::title()const 
{
    return "Comments";
};

void ard::comments_observer::rebuild_observed() 
{
    assert_return_void(ard::db(), "expected open DB");

    setExpanded(true);
    snc::clear_locked_vector(m_observed_topics);
	m_observed_topics = dbp::findAnnotated(&dbp::defaultDB());
    standard_sort();
};

/**
color_observer
*/
QString ard::color_observer::title()const
{
    return "Marked with Color";
};

void ard::color_observer::rebuild_observed()
{
    assert_return_void(ard::db(), "expected open DB");

    setExpanded(true);
    snc::clear_locked_vector(m_observed_topics);
    auto td_list = dbp::findColors(&dbp::defaultDB());
    for (auto t : td_list) {
        //topic_ptr t = dynamic_cast<topic_ptr>(i);
        //if (!t->IsUtilityFolder()) 
        //{
            auto cidx = t->colorIndex();
            if (m_inlude_filter.find(cidx) != m_inlude_filter.end()) {
                m_observed_topics.push_back(t);
            }
        //}
    }
    standard_sort();
};

void ard::color_observer::setIncludeFilter(const std::set<ard::EColor>& f)
{
    m_inlude_filter = f;
};

void rerun_q_param() 
{
    if (ard::hasGoogleToken())
    {
        auto c = ard::google();
        if (c) {
            auto q = ard::db()->gmail_runner();
            if (q)
            {
                auto db = ard::db();
                if (db && db->isOpen()) 
				{
                    auto d = dbp::configFileELabelHoisted();
                    auto lb = db->rmodel()->findRule(d);
                    if (!lb) {
						qWarning("ERROR, Failed to locate rule, using default");
                        lb = db->rmodel()->findRule(ard::static_rule::default_qtype());
                    }
                    if (lb) {
                        q->run_q_param(lb);
                    }
                    else {
                        qWarning("ERROR, Failed to locate default G-label");
                    }
                }
            }
        }
    }
}


#pragma once

#include "anfolder.h"

namespace ard {
    /**
    observable_topic - we don't own items as parent but
    maintain list of collected in some way topics that
    can be outlined
    */
    class observable_topic : public ard::topic
    {
    public:
        virtual ~observable_topic();

        virtual void    rebuild_observed() = 0;
        virtual void    clear_observed();

		void			applyOnVisible(std::function<void(ard::topic*)> fnc)override;
        bool isStandardLocusable()const override { return false; };
    protected:
        void        standard_sort();
    protected:
		TOPICS_LIST		m_observed_topics;
    };

    /**
    text search local DB
    */
    class local_search_observer : public observable_topic
    {
    public:
        QString     title()const override;
        void        selectByText(QString local_search);
        void        rebuild_observed()override;
        bool        isExpandedInList()const override { return true; };
    protected:
        QString     m_local_search;
    };


    /**
        grouped tasks from local DB with thread roots
    */
    class task_ring_observer : public observable_topic
    {
    public:
        QString     title()const override;
        void        rebuild_observed()override;
        bool        isExpandedInList()const override { return true; };
    };

    /**
    notes
    */
    class notes_observer : public observable_topic
    {
    public:
        QString     title()const override;
        void        rebuild_observed()override;
        bool        isExpandedInList()const override { return true; };
    };

	/**
	bookmarks
	*/
	class bookmarks_observer : public observable_topic
	{
	public:
		QString     title()const override;
		void        rebuild_observed()override;
		bool        isExpandedInList()const override { return true; };
	};

	/**
	pictures
	*/
	class pictures_observer : public observable_topic
	{
	public:
		QString     title()const override;
		void        rebuild_observed()override;
		bool        isExpandedInList()const override { return true; };
	};

    /**
    commented topics
    */
    class comments_observer : public observable_topic
    {
    public:
        QString     title()const override;
        void        rebuild_observed()override;
        bool        isExpandedInList()const override { return true; };
    };

    /**
    color marked
    */
    class color_observer : public observable_topic
    {
    public:
        QString     title()const override;
        void        rebuild_observed()override;
        bool        isExpandedInList()const override { return true; };
        void        setIncludeFilter(const std::set<ard::EColor>& f);
    protected:
        std::set<ard::EColor> m_inlude_filter;
    };
}

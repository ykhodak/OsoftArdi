#pragma once

#include "PopupCard.h"

class MainWindow;

extern void update_net_traffic_progress();

namespace ard {
	class mail_board_page;
	class folders_board_page;
	class topic_tab_page;
	class workspace_tab_bar;
	using WSPACE_PAGES = std::vector<topic_tab_page*>;

	class workspace : public ard::TopicWidget
	{
	public:
		workspace();
		~workspace();

		void		detachGui();

		topic_ptr   topic()override;
		void    resetAnnotationCardPos()override;
		void    saveModified()override;
		void    reloadContent()override;
		void    locateFilterText()override;
		void    closeTopicWidget(TopicWidget*)override;
		void    selectNote(topic_ptr);
		void    setFocusOnContent()override;
		void    setupAnnotationCard(bool edit_it)override;
		void    zoomView(bool zoom_in)override;
		void    detachCardOwner()override;
		TopicWidget* findTopicWidget(ard::topic*)override;
		int     indexOfPage(const topic_tab_page*)const;
		int     indexOfPage(topic_ptr f)const;
		bool    hasDetailsToolbarButton()const;
		void	selectPage(topic_ptr f);

		void    new_page();
		void    addPage(topic_tab_page*);
		void    replacePage(topic_tab_page*);
		void    closePage(int index);
		void    closePage(topic_ptr f);
		topic_tab_page*			selectPage(int index);
		topic_tab_page*			pageAt(int index);
		void					updatePageTab(int index);
		int						count()const;
		topic_tab_page*						currentPage();
		const topic_tab_page*				currentPage()const;
		template<class T>T*					currentPageAs();
		template<class T>T*					findPage();
		std::pair<ard::BlackBoard*, int>	firstBlackboard();
		std::pair<ard::topic_tab_page*, int> firstUnlocked();
		ard::mail_board_page*				mailBoard();
		ard::folders_board_page*			foldersBoard();
		std::vector<ard::BlackBoard*>		allBlackboard();
		void								updateBlackboards(topic_ptr f);

		void        updateCardHeader()override;
		bool        toJson(QJsonObject& js)const override;
		void		makeActiveInPopupLayout()override {};
		topic_tab_page* createPage(topic_ptr f);
		void    hideAndSave();
	protected:
		void    rebuildTabStyleMap();
		void    do_close_page(int index);
		void    do_addPage(topic_tab_page* p);
		void    put_page2cache(topic_tab_page*);
		void	storeTabs();
		void	restoreTabs();

		template<class O, class P>
		topic_tab_page* pickup_or_createPage(topic_ptr f);

		QBoxLayout*         m_content_box{ nullptr };
		workspace_tab_bar*  m_wbar{nullptr};
		PopupTabBarButtons* m_bar_buttons{ nullptr };
		WSPACE_PAGES        m_pages;
		std::map<EOBJ, topic_tab_page*>   m_pages_cache;

		friend class ::MainWindow;
		friend void ::update_net_traffic_progress();
		friend class workspace_tab_bar;
	};

	class topic_tab_page : public ard::TopicWidget
	{
	public:
		topic_tab_page(QIcon icon);
		virtual ~topic_tab_page();

		void saveModified()override {};
		void locateFilterText()override {};
		void setFocusOnContent()override {};
		void updateCardHeader()override;
		void makeActiveInPopupLayout()override;

		QIcon               pageIcon()const { return m_icon; }
		void                closeWidget()override;

		virtual bool		isRegularSlideLocked()const { return true; }
		bool                isSlideLocked()const { return m_slide_locked; }
		void                setSlideLocked(bool val);

	protected:
		QBoxLayout*         m_page_context_box{ nullptr };
		QIcon               m_icon;
		workspace*			m_tabcard;
		bool                m_slide_locked{ false };

		friend class workspace;
	};
}


template<class T>
T*	ard::workspace::currentPageAs() 
{
	auto p = currentPage();
	if (p) {
		auto bb = dynamic_cast<T*>(p);
		if (bb)return bb;
	}
	return nullptr;
};

template<class T>
T* ard::workspace::findPage() 
{
	for (auto& p : m_pages)
	{
		auto p1 = dynamic_cast<T*>(p);
		if (p1)return p1;
	}
	return nullptr;
};
#pragma once
#include "anfolder.h"

namespace ard 
{
	class rules_model;

	/**
		we prepare and control query run
		- check history
		- run backend token (on v-scroll)
		- make batch query (recursivelly)

		rule() property is selected query to execute
	*/
	class rule_runner : public ard::topic
	{
	public:
		rule_runner(rules_model* m);
		virtual ~rule_runner();

		ard::q_param*		rule()				{return m_qparam;}
		const ard::q_param*	rule()const			{ return m_qparam; }
		void				run_q_param(ard::q_param*);

		QString				title()const override;
		QString				objName()const override { return "runner"; };
		QPixmap				getIcon(OutlineContext)const override;
		QPixmap				getSecondaryIcon(OutlineContext)const override { return QPixmap(); };
		bool				hasRButton()const override;
		QPixmap				getRButton(OutlineContext)const override;
		std::pair<TernaryIconType, int>   ternaryIconWidth()const override;
		void				setTernaryIconWidth(int val)const	{ m_ternary_icon_width = val; }
		int					downloadProgressPercentage()const	{ return m_down_progress_percentage; }
		bool				killSilently(bool) override			{ return false; };

		bool				canAcceptChild(cit_cptr it)const override;
		bool				canMove()const override				{ return false; };
		bool				canRename()const override			{ return false; };
		bool				canAttachNote()const override		{ return false; };
		bool				canCheck4NewEmails()const;

		void				checkHistory(bool deep_check);
		void				qrelist(bool notifyGui, QString context);/// we don't query cloud, olny local cache
		topic_ptr			outlineCompanion()override;


		bool				addItem(topic_ptr it)override;
		void				detach_q();
		bool				isDefinedQ()const;
		void				applyOnVisible(std::function<void(ard::topic*)> fnc)override;

		QString				outlineLabel()const;

		void				make_gui_query();
		void				runBackendTokenQ();
	protected:
		void				makeHistoryQ(ulong hist_id, const std::vector<googleQt::HistId>& id_list);
		void				doMakeQ(int batch_size = -1);
		/// first we select N records then some more on next level
		void				makeBatchQ(int results2query, QString pageToken, bool scrollRun);

	protected:
		ard::q_param*	m_qparam{ nullptr };
		rules_model*	m_rmodel{ nullptr };

		bool			m_makeq_inprogress{ false };
		bool			m_check_hist_inprogress{ false };
		uint64_t		m_historyid_update_req{ 0 };
		mutable int		m_ternary_icon_width{ 0 };
		mutable int		m_down_progress_percentage{ 0 };
		mutable int		m_total_batch_size{0};
		//friend class locus_folder;
	};
};

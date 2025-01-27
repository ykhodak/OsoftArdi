#pragma once

#ifdef ARD_GD

#include <QTimer>
#include "anfolder.h"
#include "tooltopic.h"
#include "ansyncdb.h"
#include "gmail/GmailCache.h"


namespace ard {
	class rule;
	class static_rule;
	class rules_root;
	class rule_ext;
	class rules_model;
	class rule_runner;

	class q_param : public ard::topic 
	{
	public:
		q_param(QString title);
		virtual QString					qstr()const = 0;
		virtual bool					is_identical(const q_param*)const = 0;
		virtual bool					isFilter()const = 0;
		bool							isFilterTarget()const {return m_is_filter_target;}
		ENoteView						noteViewType()const override { return ENoteView::None; };
		bool							isValid()const override;
		googleQt::mail_cache::query_ptr	get_q();
		googleQt::mail_cache::query_ptr	ensure_q();
		googleQt::mail_cache::query_ptr	rebuild_q();/// this is expensive call

		void							remapThreads(bool notifyOnRemap, QString strContext);
		void							prepare4Gui();
		int								unregisterObserved(const GTHREADS& tset);
		void							applyOnVisible(std::function<void(ard::topic*)> fnc)override;
		void							applyOnVisible(std::function<void(ard::ethread*)> fnc);
		void							clear()override;
		QString							tabMarkLabel()const override;
		int								boardBandWidth()const;
		void							setBoardBandWidth(int val);
		int								indexOf(ard::ethread* et);
		QPixmap							getIcon(OutlineContext)const override;
		int								schedulePriority()const { return m_schedule_priority; };
		void							setSchedulePriority(int val) { m_schedule_priority = val; }
	protected:
		googleQt::mail_cache::query_ptr m_q;
		rules_model*					m_rmodel{nullptr};
		THREAD_VEC						m_observed_threads;/// we don't own wrapped threads
		int								m_board_band_width{ BBOARD_BAND_DEFAULT_WIDTH };
		bool							m_is_filter_target{false};
		bool							m_accept_draft{ false };
		bool							m_accept_trash{ false };
		bool							m_accept_read{ true };
		int								m_schedule_priority{0};
		friend class					rules_model;
		friend class					rule_runner;
	};

	using QPARAM_SET = std::unordered_set<q_param*>;


	class rules_model
	{
	public:
		rules_model(ArdDB* db);
		~rules_model();

		ard::static_rule_root*			sroot();
		ard::rules_root*				rroot();
		const ard::static_rule_root*	sroot()const;
		const ard::rules_root*			rroot()const;

		void							loadBoardRules();
		void							runBoardRules();

		q_param*						findRule(DB_ID_TYPE d);
		/// find first rule that has thread
		q_param*						findFirstRule(ard::ethread* et);
		TOPICS_LIST						allTabRules();
		std::vector<q_param*>			boardRules();
		int								unregisterObserved(const GTHREADS& tset);
		bool							prefilter(googleQt::mail_cache::ThreadData*);

		void							scheduleRuleRun(ard::q_param* r, int priority = 0);
		void							unscheduleRuleRun(ard::q_param* r);
		void							scheduleBoardRules();
		void							clearRuleSchedule();
		std::vector<q_param*>			lookupRules(const googleQt::mail_cache::query_set& sq);
	protected:		
		void							runRuleSchedule();

		ard::static_rule_root*			m_sroot{nullptr};
		ard::rules_root*				m_rroot{ nullptr };
		QPARAM_SET						m_scheduled_q;
		//googleQt::mail_cache::query_set	m_scheduled_q;
		QTimer							m_schedule_timer;
	};

    class static_rule : public q_param
    {       
    public:
		/// category on gmail messages, used in gmail query
        enum class etype 
        {
            all = 1,
            primary,
            draft,
            sent,
            with_attachment,
			unread,
			MAX_static_rule_id
        };
        QString     qstr()const override;
		bool		is_identical(const q_param*)const override;
		etype		qtype()const;
		bool		isFilter()const override{ return false; }

        bool        canBeMemberOf(const topic_ptr)const override;
        bool        isSynchronizable()const override { return false; }
        bool        isPersistant()const override { return false; }
        EOBJ        otype()const override { return objEmailLabel; };
        topic_ptr   outlineCompanion()override;
		QString		tabMarkLabel()const override;

        static DB_ID_TYPE  default_qtype();
        
    protected:
		static_rule(QString title, etype ltype, rules_model* m);

        friend class static_rule_root;
    };

	using SRULE_MAP = std::map<static_rule::etype, static_rule*>;

    class static_rule_root : public RootTopic
    {
    public:
		static_rule_root		(::ArdDB* db, rules_model* m);
		EOBJ					otype()const override { return objLabelRoot; }
        QString                 title()const override { return "Labels"; };
		bool					canAcceptChild(const snc::cit* it)const override;
		bool					isRootTopic()const override { return true; }
		ESingletonType			getSingletonType()const override { return ESingletonType::labelsHolder; }
		q_param*				findStaticRule(ard::static_rule::etype t);
	protected:
		SRULE_MAP				m_srules_map;
    };

	class rule : public q_param
	{
	public:
		rule(QString name = 0);

		bool							isFilter()const override;
		QString							qstr()const override;
		bool							is_identical(const q_param*)const override;
		bool							canBeMemberOf(const topic_ptr)const override;
		EOBJ							otype()const override { return objQFilter; };
		QString							objName()const override { return "mrule"; };
		cit_prim_ptr					create()const override;
		bool							hasCurrentCommand(ECurrentCommand c)const override;
		bool							killSilently(bool) override;

		rule_ext*						rext();
		const rule_ext*					rext()const;
		rule_ext*						ensureRExt();

	protected:
		void							mapExtensionVar(cit_extension_ptr e)override;

		rule_ext*						m_rext{nullptr};
	};

	class rule_ext : public ardiExtension<rule_ext, ard::rule>
	{
		DECLARE_DB_EXTENSION_PERSISTANT(snc::EOBJ_EXT::extQ, "qfilter-ext", "ard_ext_rule");
	public:
		///default constructor
		rule_ext();
		///for recovering from DB
		rule_ext(topic_ptr _owner, QSqlQuery& q);

		void					assignSyncAtomicContent(const cit_primitive* other)override;
		snc::cit_primitive*		create()const override;
		bool					isAtomicIdenticalTo(const cit_primitive* other, int& iflags)const override;
		QString					calcContentHashString()const override;
		uint64_t				contentSize()const override;

		void					setupRule(const ORD_STRING_SET& subject, QString phrase, const ORD_STRING_SET& from, bool as_exclusion);

		
		QString					userid()const { return m_userid; }
		const ORD_STRING_SET	subject()const { return m_words_in_subject; }
		QString					exact_phrase()const { return m_exact_phrase; }
		const ORD_STRING_SET	from()const { return m_from_list; }
		QString					fromStr()const;
		QString					subjectStr()const;
		const ORD_STRING_SET&	fastFrom()const { return m_fast_from; }

		bool					isExclusionFilter()const {return m_exclusion_filter;}
	protected:
		void					rebuildPrefilter()const;

		QString					m_userid;
		ORD_STRING_SET			m_words_in_subject;
		QString					m_exact_phrase;
		ORD_STRING_SET			m_from_list;
		bool					m_exclusion_filter{true};
		mutable ORD_STRING_SET	m_fast_from;
		mutable QString			m_str;
		mutable bool			m_str_is_dirty{true};

		friend class rule;
		friend class rules_root;
		friend class rules_model;
	};

	class rules_root : public RootTopic
	{
	public:
		DECLARE_ROOT(rules_root, objQFilterRoot, ESingletonType::qfiltersHolder);
		~rules_root();
		QString                 title()const override { return "Filters"; };
		QString                 objName()const override { return "QRoot"; };

		void					clearRules();
		void					resetRules();
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Woverloaded-virtual"
#endif
		void					applyOnVisible(std::function<void(ard::rule*)> fnc);
#ifdef __clang__        
#pragma clang diagnostic pop
#endif
		rule*					addRule(QString name);
		q_param*				findRule(DB_ID_TYPE d);
	protected:
		std::vector<ard::rule*>	m_filter_rules;
		friend class			rules_model;
	};
};



#endif //ARD_GD

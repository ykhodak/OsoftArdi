#include <ctime>
#include "a-db-utils.h"
#include "rule.h"
#include "email.h"
#include "ethread.h"
#include "locus_folder.h"

#define RULES_TIMER								1000
#define RULES_SCHEDULE_TIME_INTERVAL			10000
#define RULES_PRIORITY_SCHEDULE_TIME_INTERVAL	2000

/**
q_param
*/
ard::q_param::q_param(QString title):ard::topic(title)
{
};

googleQt::mail_cache::query_ptr	ard::q_param::get_q() 
{
	return m_q;
};

bool ard::q_param::isValid()const 
{
	return (m_q != nullptr);
};

googleQt::mail_cache::query_ptr	ard::q_param::ensure_q() 
{
	auto st = ard::gstorage();
	if (st) {
		m_q = st->qstorage()->ensure_q(qstr(), "", isFilter());
		if(m_q)m_q->setName(title());
	}
	return m_q;
};

googleQt::mail_cache::query_ptr	ard::q_param::rebuild_q() 
{
	auto st = ard::gstorage();
	if (st) {
		for (auto& i : m_observed_threads)i->release();
		m_observed_threads.clear();

		ard::trail(QString("rebuild-q [%1]").arg(title()));

		if (m_q) {
			 st->qstorage()->remove_q(m_q);
		}

		m_q = st->qstorage()->ensure_q(qstr(), "", isFilter());
		if(m_q)m_q->setName(title());
	}
	return m_q;
};

void ard::q_param::prepare4Gui() 
{
	if (m_q && m_observed_threads.empty()) {
		remapThreads(false, "prepare-gui");
	}
};

QPixmap ard::q_param::getIcon(OutlineContext )const 
{
	return isFilter() ? getIcon_RuleFilter() : QPixmap();
};

void ard::q_param::remapThreads(bool notifyOnRemap, QString strContext)
{
	ard::trail(QString("remap-threads [%1][%2][%3]").arg(title()).arg(strContext).arg(notifyOnRemap?"Y":"N"));

	bool as_filter_target = m_is_filter_target && dbp::configFileFilterInbox();
	bool as_prefilter_target = m_is_filter_target && dbp::configFilePreFilterInbox();

	assert_return_void(m_q, "expected Q-filter");
	for (auto& i : m_observed_threads)i->release();
	m_observed_threads.clear();
	m_observed_threads.reserve(m_q->threads_arr().size());
	static uint64_t lbl_trash = googleQt::mail_cache::trash_label_mask();
	static uint64_t lbl_draft = googleQt::mail_cache::draft_label_mask();
	static uint64_t lbl_unread = googleQt::mail_cache::unread_label_mask();

	googleQt::CACHE_LIST<googleQt::mail_cache::ThreadData> prefilter_list;

	auto& lst = m_q->threads_arr();
	for (auto& i : lst)
	{
		if (!i) {
			qWarning() << "listFilterQ/ERROR. Invalid query result object in list.";
			break;
		}

		if (as_filter_target){
			if (i->filterMask() != 0)continue;			
		}

		if (as_prefilter_target){
			if (i->prefilterMask() != 0)continue;
			if (!i->isPrefiltered()){
				if (m_rmodel->prefilter(i.get())) {
					prefilter_list.push_back(i);
					continue; 
				}
			}
		}		

		if (!m_accept_trash && i->hasLabel(lbl_trash)) {
			continue;
		}
		if (!m_accept_draft && i->hasLabel(lbl_draft)) {
			continue;
		}

		if (!m_accept_read && !i->hasLabel(lbl_unread)) {
			continue;
		}

		ethread_ptr t = ard::gmail_model()->lookupFirstWrapperOrWrapGthread(i);
		if (!t) {
			ard::error(QString("remap-failed-to-wrap [%1]").arg(i->id()));
			//ASSERT(0, "listFilterQ/no-shadow for tread") << i->id();
			continue;
		}

		auto h_msg = t->headMsg();
		if (!h_msg) {
			ard::error(QString("rule-remap [%1] no-head thread [%2]").arg(title()).arg(i->id()));
			continue;
		}

		m_observed_threads.push_back(t);
		LOCK(t);
		//qDebug() << "ykh-c1" << t->m_attr.Color;
	}

	if (notifyOnRemap) {
		ard::asyncExec(AR_RuleDataReady, this);
	}

	if (!prefilter_list.empty()) 
	{
		auto st = ard::gstorage();
		if (st && st->isValid()) {
			if (st->updatePrefiltered(prefilter_list)) 
			{
				ard::trail(QString("prefilter-db-upd [%1]").arg(prefilter_list.size()));
			};
		}
	}
};

int ard::q_param::unregisterObserved(const GTHREADS& tset)
{
	int rv = 0;
	for(THREAD_VEC::iterator i = m_observed_threads.begin(); i != m_observed_threads.end();)
	{
		auto t = dynamic_cast<ard::ethread*>(*i);
		//assert_return_0(t, "expected ethread");
		assert_return_0(t->optr(), "expected valid ethread");
		if (tset.find(t->optr().get()) != tset.end()) {
			qDebug() << "unreg-observed" << t->optr()->id() << t;
			t->release();
			i = m_observed_threads.erase(i);
			rv++;
		}
		else {
			i++;
		}
	}

	return rv;
};

void ard::q_param::applyOnVisible(std::function<void(ard::topic*)> fnc)
{
	for (auto& i : m_observed_threads)
	{
		fnc(i);
	}
};

void ard::q_param::applyOnVisible(std::function<void(ard::ethread*)> fnc) 
{
	for (auto& i : m_observed_threads)
	{
		fnc(i);
	}
};

void ard::q_param::clear() 
{
	ard::topic::clear();
	for (auto& i : m_observed_threads)i->release();
	m_observed_threads.clear();
};

QString ard::q_param::tabMarkLabel()const
{
	QString rv = "";
	if (m_q) {
		if (m_q->unreadCount() > 0) {
			rv = QString("%1").arg(m_q->unreadCount());
		}
	}
	return rv;
}

int	ard::q_param::boardBandWidth()const 
{
	return m_board_band_width;
};

void ard::q_param::setBoardBandWidth(int val) 
{
	if (val >= BBOARD_BAND_MIN_WIDTH && val <= BBOARD_BAND_MAX_WIDTH) 
	{
		m_board_band_width = val;
		dbp::configStoreBoardRulesSettings();
	}
};

int	ard::q_param::indexOf(ard::ethread* et) 
{
	int Max = static_cast<int>(m_observed_threads.size());
	for (int i = 0; i < Max; i++) 
	{
		if (m_observed_threads[i] == et)return i;
	}
	return -1;
};

/**
rules_model
*/
ard::rules_model::rules_model(ArdDB* db) 
{
	m_sroot = new ard::static_rule_root(db, this);
	m_rroot = new ard::rules_root(db);

	m_schedule_timer.setInterval(RULES_TIMER);
	m_schedule_timer.stop();
	QObject::connect(&m_schedule_timer, &QTimer::timeout, [=]()
	{
		if (!ard::gmail()) {
			m_schedule_timer.stop();
			ard::trail(QString("stop r-scheduler on invalid gmail"));
			return;
		}
		runRuleSchedule();
	});
};

ard::rules_model::~rules_model() 
{
	clearRuleSchedule();
	if (m_sroot) {
		m_sroot->release();
		m_sroot = nullptr;
	}
	if (m_rroot) {
		m_rroot->release();
		m_sroot = nullptr;
	}
};

ard::static_rule_root*			ard::rules_model::sroot() { return m_sroot; };
ard::rules_root*				ard::rules_model::rroot() { return m_rroot; };
const ard::static_rule_root*	ard::rules_model::sroot()const { return m_sroot; };
const ard::rules_root*			ard::rules_model::rroot()const { return m_rroot; };

ard::q_param* ard::rules_model::findRule(DB_ID_TYPE d)
{
	ard::q_param* rv = nullptr;
	if (d < static_cast<int>(static_rule::etype::MAX_static_rule_id)) ///<<< some small magic number we assume it's not a real DBID, treat it as static enum
	{
		auto t = static_cast<static_rule::etype>(d);
		rv = m_sroot->findStaticRule(t);
	}
	else
	{
		rv = m_rroot->findRule(d);
	}
	return rv;
};

std::vector<ard::q_param*> ard::rules_model::lookupRules(const googleQt::mail_cache::query_set& sq) 
{
	std::vector<ard::q_param*> rv;
	auto rlst = boardRules();
	for (auto& r : rlst) {
		if (sq.find(r->m_q) != sq.end())
		{
			rv.push_back(r);
		}
	}
	return rv;
};

TOPICS_LIST	ard::rules_model::allTabRules()
{
	TOPICS_LIST rv = m_sroot->itemsAs<ard::topic>();
	for (auto& r : m_rroot->m_filter_rules) {
		rv.push_back(r);
	}
	return rv;
};

std::vector<ard::q_param*> ard::rules_model::boardRules()
{
	std::vector<ard::q_param*> rv;
	auto p = findRule(static_cast<int>(ard::static_rule::etype::primary));
	if (p)rv.push_back(p);
	for (auto&r : m_rroot->m_filter_rules) {
		rv.push_back(r);
	}
	return rv;
};

ard::q_param* ard::rules_model::findFirstRule(ard::ethread* et) 
{
	for (auto& r : m_rroot->m_filter_rules)
	{
		if (r->isValid()) {
			auto idx = r->indexOf(et);
			if (idx != -1)
				return r;
		}
	}
	
	auto r = m_sroot->findStaticRule(static_rule::etype::primary);
	auto idx = r->indexOf(et);
	if (idx != -1)
		return r;

	r = m_sroot->findStaticRule(static_rule::etype::all);
	idx = r->indexOf(et);
	if (idx != -1)
		return r;

	return nullptr;
};

int	ard::rules_model::unregisterObserved(const GTHREADS& tset)
{
	int rv = 0;
	for (auto&r : m_rroot->m_filter_rules) {
		if (r)rv += r->unregisterObserved(tset);
	}

	auto& l3 = m_sroot->items();
	for (auto&i : l3) {
		auto r = dynamic_cast<ard::q_param*>(i);
		if (r)rv += r->unregisterObserved(tset);		
	}
	return rv;
};

bool ard::rules_model::prefilter(googleQt::mail_cache::ThreadData* o) 
{
	assert_return_false(ard::isDbConnected(), "expected connected DB");
	assert_return_false(m_rroot, "expected valid rules root");
	ASSERT_VALID(m_rroot);
	if (m_rroot->m_filter_rules.empty()) {
		o->markPrefiltered();
		return false;
	}

#define SCHEDULE_PREFILTERED(R) o->setPrefilterMask(R->m_q.get());\
	auto i = m_scheduled_q.find(R);\
	if (i == m_scheduled_q.end())\
	{\
		scheduleRuleRun(R);\
	}\


	STRING_SET from_set;
	auto h = o->head();
	if (h) {
		QString subject = h->subject();
		auto s_from = ard::recoverEmailAddress(h->from()).toLower();
		for (auto& r : m_rroot->m_filter_rules) 
		{
			if (!r->isValid()) {
				//ASSERT(0, "expected valid rule");
				continue;
			}
			auto e = r->rext();
			if (!e) {
				ASSERT(0, "expected rule extention");
				continue;
			}

			/*
#ifdef _DEBUG


			dbg << "<<<< check prefilter on " << s_from << "rule=" << r->title();
			for (auto x : e->m_fast_from) {
				dbg << ":" << x;
			}
#endif
*/
			auto j = e->m_fast_from.find(s_from);
			if (j != e->m_fast_from.end())
			{
				SCHEDULE_PREFILTERED(r);
				//scheduleRuleRun(r);
				//o->setPrefilterMask(r->m_q.get());
#ifdef _DEBUG
				auto dbg = qDebug();
				dbg.noquote();
				dbg << QString("+F/from[%1][%2][%3]").arg(s_from).arg(r->m_q->filterFlag()).arg(r->title());
				for (auto x : e->m_fast_from) {
					dbg << QString("[%1]").arg(x);
				}
#endif
				return true;
			}

			if (!e->m_words_in_subject.empty()) 
			{
				for (auto& s : e->m_words_in_subject) 
				{
					auto idx = subject.indexOf(s);
					if (idx != -1) 
					{
						SCHEDULE_PREFILTERED(r);
						//scheduleRuleRun(r);
						//o->setPrefilterMask(r->m_q.get());
#ifdef _DEBUG
						auto dbg = qDebug();
						dbg.noquote();
						dbg << QString("+F/subject[%1][%2][%3][%4]").arg(subject).arg(s).arg(r->m_q->filterFlag()).arg(r->title());
#endif
						return true;
					}
				}
			}
		}
	}
	return false;
};

void ard::rules_model::scheduleRuleRun(ard::q_param* r, int priority)
{
	//auto q = r->get_q();
	if (r->isValid()) {
		auto i = m_scheduled_q.find(r);
		if (i == m_scheduled_q.end()) 
		{
			r->setSchedulePriority(priority);			
			m_scheduled_q.insert(r);
			ard::trail(QString("scheduled-rule [%1][%2][%3]").arg(r->title()).arg(priority).arg(m_scheduled_q.size()));
			LOCK(r);
			if (!m_schedule_timer.isActive()) {
				m_schedule_timer.start();
			}

			//if (startNowWithoutSchedule) {
			//	runRuleSchedule();
			//}
			//else 
			//{
			//}
		}
		else {
			ard::trail(QString("already-scheduled-rule %1 [%2]").arg(r->title()).arg(m_scheduled_q.size()));
		}
	}
};

void ard::rules_model::unscheduleRuleRun(ard::q_param* r) 
{
	if (r->isValid()) {
		auto i = m_scheduled_q.find(r);
		if (i != m_scheduled_q.end())
		{
			ard::trail(QString("unscheduled-rule %1").arg(r->title()));
			r->setSchedulePriority(0);
			m_scheduled_q.erase(i);
			r->release();
		}
	}
};

void ard::rules_model::scheduleBoardRules() 
{
	ard::trail("scheduleBoardRules");
	auto p = findRule(static_cast<int>(ard::static_rule::etype::primary));
	auto rlst = boardRules();
	for (auto& r : rlst) {
		if (r != p) {
			scheduleRuleRun(r, 1);
		}
	}
	scheduleRuleRun(p, 0);
};

void ard::rules_model::clearRuleSchedule() 
{
	if (!m_scheduled_q.empty()) {
		ard::trail(QString("cleared-rule-schedule %1").arg(m_scheduled_q.size()));
		for (auto& q : m_scheduled_q)
		{
			q->setSchedulePriority(0);
			q->release();
		}
		m_scheduled_q.clear();
	}
};

void ard::rules_model::runRuleSchedule() 
{
	if (m_scheduled_q.empty()) {
		if (m_schedule_timer.isActive()) {
			m_schedule_timer.stop();
			ard::trail(QString("r-scheduler empty"));
			return;
		}
	}

	assert_return_void(ard::gmail(), "expected gmail");
	auto cr = ard::gmail()->cacheRoutes();
	assert_return_void(cr, "expected gmail cache");

	if (!m_scheduled_q.empty())
	{
		std::vector<ard::q_param*> lst;
		for (auto& q : m_scheduled_q) {
			lst.push_back(q);
		}
		std::sort(lst.begin(), lst.end(), [](ard::q_param* q1, ard::q_param* q2)
		{
			return (q1->schedulePriority() > q2->schedulePriority());
		});
		//for (auto& q : lst)
		//{
		//	qDebug() << "scheduled" << q->schedulePriority() << q->title();
		//}

		auto firstQ = *lst.begin();
		int time_run_interval = 0;
		auto lastRun = cr->lastQRunTime();
		bool runOne = (lastRun == 0);
		if (!runOne) 
		{
			auto now = time(nullptr);
			time_run_interval = (now - lastRun);
			if (firstQ->schedulePriority() > 0) {
				runOne = (time_run_interval * 1000 > RULES_PRIORITY_SCHEDULE_TIME_INTERVAL);
			}
			else {
				runOne = (time_run_interval * 1000 > RULES_SCHEDULE_TIME_INTERVAL);
			}
		}

		if (runOne)
		{
			QString rtitle = firstQ->title();
			ard::trail(QString("schedule/run-one [%1][%2][%3]").arg(rtitle).arg(time_run_interval).arg(lst.size()));
			qDebug() << "schedule/runOne" << time_run_interval << rtitle;
			LOCK(firstQ);
			unscheduleRuleRun(firstQ);			
			auto cqr = ard::gmail()->cacheRoutes()->getQCache_Async(firstQ->get_q(), 50);
			cqr->then([=](googleQt::mail_cache::tdata_result r)
			{
				auto received = r->from_cloud.size();
				auto total_t = firstQ->get_q()->threads_arr().size();			
				//arg(m_qparam->get_q()->threads_arr().size()).arg(
				//qrelist(true, QString("batch-q/completed [%1][%2][%3]").arg(m_qparam->get_q()->threads_arr().size()).arg(results2query).arg(nextToken));
				if (firstQ)
				{
					firstQ->remapThreads(true, QString("schedule-q/completed [%1][%2]").arg(received).arg(total_t));
					firstQ->release();
				}
				else 
				{
					qWarning() << "schedule-q/completed expected Q" << rtitle;
				}
			},
			[&](std::unique_ptr<googleQt::GoogleException> ex)
			{
				qWarning() << "schedule-q/exception" << ex->what();
				if (firstQ) {
					firstQ->release();
				}
				else {
					qWarning() << "schedule-q/exception expected Q";
				}
			});
		}
	}
};

void ard::rules_model::loadBoardRules()
{	
	auto r_lst = boardRules();
	for (auto&i : r_lst) {
		if (!i->isValid()) {
			i->ensure_q();
		}
	}
	ard::trail(QString("rules-loaded %1").arg(r_lst.size()));

	auto cfg_set = dbp::configLoadBoardRulesSettings();

	for (auto&i : r_lst) {
		if (i->isValid()) {
			auto j = cfg_set.find(i->id());
			if (j != cfg_set.end())i->m_board_band_width = j->second;
		}
	}
};

void ard::rules_model::runBoardRules()
{
	googleQt::mail_cache::query_set qlst;
	auto r_lst = boardRules();
	for (auto&i : r_lst){		
		if (!i->isValid()) {
			i->ensure_q();
		}
		auto q = i->get_q();
		if (q) {
			qlst.insert(q);
		}
	}
		
	if (!qlst.empty())
	{
		googleQt::GmailRoutes* gm = ard::gmail();
		if (gm && gm->cacheRoutes()) {
			auto t = gm->cacheRoutes()->runQRulesCache_Async(qlst);
			t->then([]()
			{
				qDebug() << "ykh/completed runQRulesCache_Async";
				googleQt::GmailRoutes* gm = ard::gmail();
				if (gm && gm->cacheRoutes()) {
					auto st = gm->cacheRoutes()->storage();
					auto lst = st->qstorage()->filterRules();
					for (auto& q : lst) {
						qDebug() << "ykh-new" << q->unreadCount();
					}
					ard::asyncExec(AR_RebuildLocusTab);
				}
			});
		}
	}
};

/**
	static_rule_root
*/
ard::static_rule_root::static_rule_root(::ArdDB* db, rules_model* m)
{
	setupAsRoot(db, QString("Labels"));

	static_rule* lb = nullptr;
#define ADD_ST_RULE(L, T) lb = new static_rule(L, T, m);addItem(lb);m_srules_map[T]=lb;
	
	ADD_ST_RULE("Inbox", ard::static_rule::etype::primary);
	lb->m_is_filter_target = true;

	ADD_ST_RULE("Unread", ard::static_rule::etype::unread);
	lb->m_accept_read = false;
	ADD_ST_RULE("Sent", ard::static_rule::etype::sent);
	ADD_ST_RULE("All", ard::static_rule::etype::all);
	ADD_ST_RULE("Drafts", ard::static_rule::etype::draft);
	lb->m_accept_draft = true;
#undef ADD_ST_RULE
};

bool ard::static_rule_root::canAcceptChild(const snc::cit* it)const
{
	auto c = dynamic_cast<const ard::static_rule*>(it);
	if (!c) {
		return false;
	}
	return true;
};

ard::q_param* ard::static_rule_root::findStaticRule(ard::static_rule::etype t)
{
	auto i = m_srules_map.find(t);
	if (i != m_srules_map.end()) {
		return i->second;
	}
	/*
	auto lst = itemsAs<ard::q_param>();
	for (auto& lb : lst) {
		if (lb->id() == d) {
			return lb;
		}
	}*/
	return nullptr;
};

ard::static_rule::static_rule(QString title, etype ltype, rules_model* m):q_param(title)
{
    m_id = static_cast<DB_ID_TYPE>(ltype);
	m_rmodel = m;
};

ard::static_rule::etype ard::static_rule::qtype()const
{
	etype rv = etype::primary;
    rv = static_cast<etype>(m_id);
    return rv;
};

DB_ID_TYPE ard::static_rule::default_qtype()
{
    return static_cast<int>(etype::primary);
};

QString ard::static_rule::qstr()const
{
    QString rv;
    auto et = static_cast<etype>(m_id);
    switch (et) 
    {
    case etype::all:				rv = ""; break;
    case etype::primary:			rv = "category:{personal updates forums} OR label:SENT"; break;
    case etype::draft:				rv = "label:DRAFT"; break;
	case etype::sent:				rv = "{category:{personal updates forums} OR label:SENT} label:SENT"; break;
	case etype::with_attachment:	rv = "{category:{personal updates forums} OR label:SENT} has:attachment"; break;
	case etype::unread:				rv = "{category:{personal updates forums} OR label:SENT} label:UNREAD"; break;
    default:break;
    }
    return rv;
};

bool ard::static_rule::is_identical(const q_param* q1)const 
{
	auto q = dynamic_cast<const static_rule*>(q1);
	if (q) {
		return (q->qtype() == qtype());
	}
	return false;
};

bool ard::static_rule::canBeMemberOf(const topic_ptr f)const
{
    ASSERT_VALID(this);
    bool rv = false;
    if (dynamic_cast<const ard::static_rule_root*>(f) != nullptr) {
        rv = true;
    }
    return rv;
};

topic_ptr ard::static_rule::outlineCompanion()
{
    topic_ptr rv = nullptr;
    auto et = static_cast<etype>(m_id);
    switch (et)
    {
	case etype::draft: rv = ard::Backlog(); break;
    default:break;
    }
    return rv;
};

QString	ard::static_rule::tabMarkLabel()const 
{
	if (qtype() == etype::unread) {
		return q_param::tabMarkLabel();
	}
	return "";
};

/**
rules_root
*/
IMPLEMENT_ROOT(ard, rules_root, "Filters", ard::rule);
ard::rules_root::~rules_root() 
{
	clearRules();
};

ard::rule* ard::rules_root::addRule(QString name)
{
	auto user = dbp::configEmailUserId();
	assert_return_null(!user.isEmpty(), "expected valid userid");
	auto r = new ard::rule(name);
	auto e = r->ensureRExt();
	assert_return_null(e, "expected rule extension");
	e->m_userid = user;
	addItem(r);
	ensurePersistant(1);
	resetRules();
	return r;
};


void ard::rules_root::applyOnVisible(std::function<void(ard::rule*)> fnc)
{
	for (auto& i : m_filter_rules)
	{
		fnc(i);
	}
};

void ard::rules_root::clearRules() 
{
	ard::trail("clear-rules");
	snc::clear_locked_vector(m_filter_rules);
};

void ard::rules_root::resetRules()
{
	ard::trail("reset-rules");
	snc::clear_locked_vector(m_filter_rules);
	auto user = dbp::configEmailUserId();
	for (auto& i : m_items) 
	{
		auto r = dynamic_cast<rule*>(i);
		if (r && r->rext() && r->rext()->userid() == user)
		{
			LOCK(r);
			m_filter_rules.push_back(r);
		}
	}

	std::sort(m_filter_rules.begin(), m_filter_rules.end(), [](snc::cit* t1, snc::cit* t2)
	{
		return (t1->title() < t2->title());
	});
};

ard::q_param* ard::rules_root::findRule(DB_ID_TYPE d)
{
	for (auto& i : m_filter_rules) {
		if (i->id() == d) {
			return i;
		}
	}
	return nullptr;
};


/**
rule
*/
ard::rule::rule(QString name):q_param(name)
{

};

QString ard::rule::qstr()const 
{
	auto e = rext();
	if (e) 
	{
		if (e->m_str_is_dirty)
		{
			e->m_str = "";
			std::vector<QString> from_list;
			for (auto& s : e->m_from_list)
			{
				auto from = ard::recoverEmailAddress(s);
				if (!from.isEmpty()) {
					from_list.push_back(from);
					//e->m_str += QString("{from: ").arg();
				}
				//qDebug() << "from=" << s;
			}
			if (!from_list.empty()) {
				e->m_str += "{";
				for (auto& s : from_list) {
					e->m_str += QString("from:%1 ").arg(s);
				}
				e->m_str += "}";
			}

			if (!e->m_words_in_subject.empty()) 
			{
				if (!e->m_str.isEmpty()) {
					e->m_str += " OR ";
				}

				e->m_str += "{";
				for (auto& s : e->m_words_in_subject) {
					e->m_str += QString("subject:(%1) OR ").arg(s);
				}
				int pos = e->m_str.lastIndexOf("OR", -1);
				if (pos != -1) {
					e->m_str.remove(pos-1, e->m_str.size());
				}
				e->m_str += "}";
				//QString str_subject = QString("{subject:(CEC) OR subject:(risk)}");
				//qDebug() << "<<subject-as:" << str_subject;
				//e->m_str += QString("subject:(%1)").arg(e->m_words_in_subject);
				//e->m_str += str_subject;
			}

			if (!e->m_exact_phrase.isEmpty()) 
			{
				e->m_str += QString("\"%1\"").arg(e->m_exact_phrase);
			}

			//qDebug() << "query:" << e->m_str;

			e->m_str_is_dirty = false;		
		}
		return e->m_str;
	}

	return "";
};

bool ard::rule::isFilter()const 
{ 
	auto e = rext();
	if (e)
	{
		return e->isExclusionFilter();
	}
	return false; 
}

bool ard::rule::is_identical(const q_param* q1)const 
{
	auto q = dynamic_cast<const rule*>(q1);
	if (q) {
		return (q->qstr() == qstr());
	}
	return false;
};

bool ard::rule::canBeMemberOf(const topic_ptr f)const 
{
	ASSERT_VALID(this);
	bool rv = false;
	if (dynamic_cast<const ard::rules_root*>(f) != nullptr) {
		rv = true;
	}
	return rv;
};

bool ard::rule::hasCurrentCommand(ECurrentCommand c)const 
{
	return (c == ECurrentCommand::cmdEdit || 
			c == ECurrentCommand::cmdSelect || 
			c == ECurrentCommand::cmdDelete);
};


bool ard::rule::killSilently(bool)
{
	if (ard::isGoogleConnected()) {
		auto storage = ard::gstorage();
		if (storage) {
			auto q = storage->qstorage()->lookup_q(qstr(), "", isFilter());
			if (q) {
				storage->qstorage()->remove_q(q);
			}
		}
	}
	return ard::topic::killSilently(false);
};

cit_prim_ptr ard::rule::create()const 
{
	return new ard::rule;
};

ard::rule_ext* ard::rule::rext() 
{
	return ensureRExt();
};

const ard::rule_ext* ard::rule::rext()const 
{
	ard::rule* ThisNonCost = (ard::rule*)this;
	return ThisNonCost->ensureRExt();
};

void ard::rule::mapExtensionVar(cit_extension_ptr e) 
{
	topic::mapExtensionVar(e);
	if (e->ext_type() == snc::EOBJ_EXT::extQ) {
		ASSERT(!m_rext, "duplicate Q ext");
		m_rext = dynamic_cast<ard::rule_ext*>(e);
		ASSERT(m_rext, "expected Q ext");
	}
};

ard::rule_ext* ard::rule::ensureRExt() 
{
	ASSERT_VALID(this);
	if (m_rext)
		return m_rext;

	ensureExtension(m_rext);
	return m_rext;
};

/**
rule_ext
*/
ard::rule_ext::rule_ext() 
{

};

ard::rule_ext::rule_ext(topic_ptr _owner, QSqlQuery& q) 
{
	attachOwner(_owner);
	m_mod_counter		= q.value(1).toInt();
	m_userid			= q.value(2).toString();
	auto str			= q.value(3).toString();
	if (!str.isEmpty()) {
		auto lst = str.split(";");
		for (auto& s : lst) {
			if (!s.isEmpty())m_words_in_subject.insert(s);
		}
	}
	m_exact_phrase		= q.value(4).toString();
	str			= q.value(5).toString();
	if (!str.isEmpty()) {
		auto lst = str.split(";");
		for (auto& s : lst) {
			if (!s.isEmpty())m_from_list.insert(s);
		}
	}

	auto tmp = q.value(6).toInt();
	if (tmp > 0) {
		m_exclusion_filter = true;
	}
	else {
		m_exclusion_filter = false;
	}

//	lst					= .split(";", Qt::SkipEmptyParts);
//	for (auto& s : lst)m_from_list.insert(s);
	rebuildPrefilter();
	_owner->addExtension(this);
};


void ard::rule_ext::setupRule(const ORD_STRING_SET& subject, QString phrase, const ORD_STRING_SET& from, bool as_exclusion)
{
	if (m_words_in_subject != subject || m_exact_phrase != phrase || m_from_list != from || m_exclusion_filter != as_exclusion)
	{
		//auto old_q = owner()->get_q();
		m_words_in_subject = subject;
		m_exact_phrase = phrase;
		m_from_list = from;
		m_exclusion_filter = as_exclusion;
		rebuildPrefilter();
		ask4persistance(np_ATOMIC_CONTENT);
		if (owner() && owner()->dataDb()) {
			ensureExtPersistant(owner()->dataDb());
		}
		m_str_is_dirty = true;
		owner()->rebuild_q();
		auto db = ard::db();
		if (db && db->isOpen()) {
			db->rmodel()->scheduleRuleRun(owner());
		}
	}
};

void ard::rule_ext::rebuildPrefilter()const
{
	m_fast_from.clear();
	for (auto& i : m_from_list) {
		auto s = ard::recoverEmailAddress(i).toLower();
		m_fast_from.insert(s);
	}
};

void ard::rule_ext::assignSyncAtomicContent(const cit_primitive* _other)
{
	assert_return_void(_other, "expected board extension");
	auto* other = dynamic_cast<const ard::rule_ext*>(_other);
	assert_return_void(other, QString("expected qfilter extension %1").arg(_other->dbgHint()));
	m_userid = other->m_userid;
	m_words_in_subject = other->m_words_in_subject;
	m_exact_phrase = other->m_exact_phrase;
	m_from_list = other->m_from_list;
	m_exclusion_filter = other->m_exclusion_filter;
	rebuildPrefilter();
	ask4persistance(np_ATOMIC_CONTENT);
};

bool ard::rule_ext::isAtomicIdenticalTo(const cit_primitive* _other, int&)const
{
	assert_return_false(_other, "expected item [1]");
	auto* other = dynamic_cast<const ard::rule_ext*>(_other);
	assert_return_false(other, "expected item [2]");
	if (m_userid != other->m_userid ||
		m_words_in_subject != other->m_words_in_subject ||
		m_from_list != other->m_from_list ||
		m_exact_phrase != other->m_exact_phrase ||
		m_exclusion_filter != other->m_exclusion_filter)
	{
		QString s = QString("ext-ident-err:%1").arg(extName());
		sync_log(s);
		on_identity_error(s);
		return false;
	}
	return true;
};

snc::cit_primitive* ard::rule_ext::create()const
{
	return new ard::rule_ext;
};

QString ard::rule_ext::calcContentHashString()const
{
	QString rv;
	return rv;
};

uint64_t ard::rule_ext::contentSize()const
{
	uint64_t rv = 0;
	return rv;
};

QString	ard::rule_ext::fromStr()const 
{
	return googleQt::slist2str(m_from_list.begin(), m_from_list.end(), ";");
};

QString	ard::rule_ext::subjectStr()const 
{
	return googleQt::slist2str(m_words_in_subject.begin(), m_words_in_subject.end(), ";");
};

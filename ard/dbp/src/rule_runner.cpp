#include <QTimer>
#include "gmail/GmailRoutes.h"
#include "GoogleClient.h"
#include "Endpoint.h"

#include "a-db-utils.h"
#include "rule_runner.h"
#include "rule.h"
#include "email.h"
#include "ethread.h"
#include "email_draft.h"

using namespace googleQt;
QDateTime   gmail_last_check_time;
extern bool gui_can_check4new_email();
extern void onArdiProgress(qint64 bytesProcessed, qint64 total);

#define SINGLE_Q_SIZE 50

/**
rule_runner
*/
ard::rule_runner::rule_runner(rules_model* m):m_rmodel(m)
{
};

ard::rule_runner::~rule_runner()
{
	if (m_qparam) {
		m_qparam->release();
		m_qparam = nullptr;
	}
};

QString ard::rule_runner::title()const
{
	QString rv;
	if (m_qparam) {
		rv = m_qparam->title() + " ";
	}
	rv += QString(" (%1)").arg(dbp::configEmailUserId());
	return rv;
};

bool ard::rule_runner::hasRButton()const 
{
	return false;
};

QPixmap	ard::rule_runner::getRButton(OutlineContext)const
{
	return QPixmap();
}

QPixmap	ard::rule_runner::getIcon(OutlineContext)const 
{
	return QPixmap();
};

topic_ptr ard::rule_runner::outlineCompanion()
{
	if (m_qparam) {
		return m_qparam->outlineCompanion();
	}
	return nullptr;
};

void ard::rule_runner::run_q_param(ard::q_param* lb)
{
	if (!ard::isGoogleConnected()) {
		if (!ard::hasGoogleToken()) {
			return;
		}
	}

	if (m_makeq_inprogress) {
		if (lb == m_qparam) {
			ard::trail(QString("run_q_param conflated-already-running [%1]").arg(m_qparam->title()));
			return;
		}

		auto cl = ard::google();
		if (cl) {
			ard::trail(QString("cancel-running-rule [%1] in favor of [%2]").arg(m_qparam->title()).arg(lb->title()));
			cl->cancelAllRequests();
		}
		
		//qDebug() << "select-by-label/ignored.makeq-in-progress";
		//return;
	}

	if (m_qparam) {
		m_qparam->release();
	}
	m_qparam = lb;

	if (m_qparam) {
		LOCK(m_qparam);
		setExpanded(true);
		if (m_qparam->ensure_q())
		{
			qrelist(false, "prerun");
			doMakeQ();
		}
		else
		{
			ASSERT(0, "failed to ensure Q");
		}
	}
};


void ard::rule_runner::qrelist(bool notifyGui, QString context)
{
	assert_return_void(ard::gmail(), "expected gmail model");
	assert_return_void(ard::gmail_model(), "expected gmail model");
	assert_return_void(ard::isDbConnected(), "expected connected DB");
	ASSERT(m_items.size() == 0, "expected empty topic");
	if (m_qparam && m_qparam->isValid()) {
		m_qparam->remapThreads(notifyGui, context);
	}
};


void ard::rule_runner::runBackendTokenQ()
{		
	assert_return_void(m_qparam, "expected qparam");
	assert_return_void(m_qparam->get_q(), "expected Q-filter");
	assert_return_void(ard::gmail(), "expected gmail model");
	assert_return_void(ard::gmail_model(), "expected gmail model");

	if (!m_qparam->get_q()->isCacheLoaded()) {
		auto st = ard::gstorage();
		if (st) {
			auto old_num = m_qparam->get_q()->threads_arr().size();
			st->qstorage()->loadAllCacheData(m_qparam->get_q());
			auto str = QString("loaded-all-q-cache [%1][%2]->[%3]").arg(m_qparam->title()).arg(old_num).arg(m_qparam->get_q()->threads_arr().size());
			gmail_last_check_time = QDateTime::currentDateTime();
			ard::trail(str);
			qrelist(true, str);
			gui::rebuildOutline();
		}
		return;
	}

	/*auto token = m_qparam->get_q()->backendToken();
	if (token.isEmpty()) {
		ard::trail(QString("run-backend-token - empty token [%1]").arg(m_qparam->title()));
		return;
	}*/

	assert_return_void(!m_makeq_inprogress, "Q already in progress");

	if (!canCheck4NewEmails()) {
		ard::trail(QString("conflated-backend-token-q [%1]").arg(m_qparam->title()));
		return;
	}	

	auto prev_tnum = m_qparam->get_q()->threads_arr().size();
	//auto tnum = //m_qparam->m_observed_threads.size();
	auto tnum = prev_tnum + SINGLE_Q_SIZE;
	ard::trail(QString("running-backend-token-q [%1][%2]->[%3][%4]").arg(m_qparam->title()).arg(prev_tnum).arg(tnum).arg(m_qparam->m_observed_threads.size()));
	m_total_batch_size = tnum;
	makeBatchQ(tnum, "", true);	
};


void ard::rule_runner::doMakeQ(int batch_size)
{
	assert_return_void(m_qparam, "expected Q");
	assert_return_void(m_qparam->get_q(), "expected Q-filter");
	assert_return_void(ard::gmail(), "expected gmail model");
	assert_return_void(ard::gmail_model(), "expected gmail model");
	assert_return_void(!m_makeq_inprogress, "Q already in progress");

	m_rmodel->unscheduleRuleRun(m_qparam);

	if (batch_size < 0) {
		batch_size = 50;
	}
	m_total_batch_size = batch_size;
	makeBatchQ(batch_size, "", false);
};

void ard::rule_runner::makeBatchQ(int results2query, QString pageToken, bool scrollRun)
{
	assert_return_void(ard::gmail(), "expected gmail");
	assert_return_void(ard::gmail()->cacheRoutes(), "expected gmail cache");
	assert_return_void(m_qparam->get_q(), "expected valid qparam");
	//ard::trail(QString("batch-q [%1][%2][%3][%4]").arg(m_qparam->title()).arg(results2query).arg(pageToken).arg(scrollRun?"Y":"N"));
	gmail_last_check_time = QDateTime::currentDateTime();

	int single_q_size = SINGLE_Q_SIZE;
	if (scrollRun) {
		single_q_size = 200;
	}

	ard::google()->endpoint()->diagnosticSetRequestContext(QString("makeQ/%1/%2")
		.arg(results2query)
		.arg(pageToken));
	m_down_progress_percentage = 1;
	auto cqr = ard::gmail()->cacheRoutes()->getQCache_Async(m_qparam->get_q(), single_q_size, pageToken, scrollRun);
	auto p = cqr->createProgressNotifier();
	if (p) {
		p->setMaximum(0, QString("query gmail q='%1'").arg(m_qparam->title()));
		QObject::connect(p, &googleQt::TaskProgress::valueChanged, [p, this](int val)
		{
			if (p->maximum() != 0) {
				m_down_progress_percentage = 100 * val / p->maximum();
			}
			ard::asyncExec(AR_UpdateGItem, this);
			//qDebug() << "[q-prog][" << p->statusText() << "][" << QString("%1/%2").arg(val).arg(p->maximum()) << "]" << downloadProgressPercentage() << title();
		});

	}
	m_makeq_inprogress = true;
	LOCK(this);
	cqr->then([=](mail_cache::tdata_result r)
	{
		int threads_arr_size = -1;
		auto gq = m_qparam->get_q();
		if (gq) {
			threads_arr_size = gq->threads_arr().size();
		}
		ard::trail(QString("q-result [%1]+[%2][%3]").arg(threads_arr_size).arg(r->from_cloud.size()).arg(cqr->nextPageToken()));
		m_down_progress_percentage = 0;

		auto nextToken = cqr->nextPageToken();
		bool recursive_call = false;
		int new_query_batch = results2query - single_q_size;
		onArdiProgress(m_total_batch_size - new_query_batch, m_total_batch_size);
		if (new_query_batch > 0) {			
			if (!nextToken.isEmpty()) {
				bool nextRun = scrollRun;
				if (r->from_cloud.size() > 0 && scrollRun) {
					ard::trail(QString("drop-scroll-run [%1][%2][%3]").arg(m_qparam->title()).arg(r->from_cloud.size()).arg(nextToken));
					nextRun = false;
				}
				recursive_call = true;
				makeBatchQ(new_query_batch, nextToken, nextRun);
			}
		}

		if (!recursive_call) {
			m_makeq_inprogress = false;
			if (ard::isDbConnected()) 
			{
				gq = m_qparam->get_q();
				if (gq) {
					threads_arr_size = gq->threads_arr().size();
				}
				qrelist(true, QString("batch-q/completed [%1][%2][%3]").arg(threads_arr_size).arg(results2query).arg(nextToken));
				gui::rebuildOutline();
			}
		}
		release();
	},
		[&](std::unique_ptr<GoogleException> ex)
	{
		m_down_progress_percentage = 0;
		m_makeq_inprogress = false;
#ifdef API_QT_AUTOTEST
		if (!ApiAutotest::INSTANCE().isCancelRequested()) {
			ASSERT(0, "Failed to makeQ") << ex->what();
		}
#else
		qWarning() << "run-g-query exception" << ex->what();
#endif
		release();
	});
};

void ard::rule_runner::makeHistoryQ(ulong hist_id, const std::vector<googleQt::HistId>& id_list)
{
	assert_return_void(m_qparam, "expected Q");
	assert_return_void(m_qparam->get_q(), "expected Q-filter");
	assert_return_void(ard::gmail(), "expected gmail model");
	assert_return_void(ard::gmail_model(), "expected gmail model");
	assert_return_void(!m_makeq_inprogress, "Q already in progress");
	m_historyid_update_req = hist_id;

	auto rfetcher = ard::gmail()->cacheRoutes()->newThreadResultFetcher(m_qparam->get_q());

	auto cqr = ard::gmail()->cacheRoutes()->getCacheThreadList_Async(id_list, rfetcher);
	auto p = cqr->progressNotifier();
	if (p) {
		QObject::connect(p, &googleQt::TaskProgress::valueChanged, [p, this](int val)
		{
			if (p->maximum() != 0) {
				m_down_progress_percentage = 100 * val / p->maximum();
			}
			ard::asyncExec(AR_UpdateGItem, this);
		});
	}

	m_makeq_inprogress = true;
	///! have to lock owner before running something in async context
	/// since owner might get deleted during async processing
	LOCK(this);
	cqr->then([=](mail_cache::tdata_result r)
	{
		auto st = ard::gstorage();
		ard::trail(QString("Q-history [%1] [%2]").arg(r->from_cloud.size()).arg(cqr->nextPageToken()));
		if (m_historyid_update_req != 0)
		{
			if (st) {
				st->setHistoryId(m_historyid_update_req);
				ard::trail(QString("Q-history-updated [%1]").arg(m_historyid_update_req));
			}
			m_historyid_update_req = 0;
		}

		m_down_progress_percentage = 0;
		m_makeq_inprogress = false;
		qrelist(true, QString("history-q %1").arg(r->from_cloud.size()));
		gui::rebuildOutline();

		//...
		release();
		EOutlinePolicy main_pol = gui::currPolicy();
		if (main_pol == outline_policy_PadEmail)
		{
			//qDebug() << "scheduled requst for makeq";
			doMakeQ(50);
		}
		//else {
		//	qDebug() << "skipped q requst for makeq";
		//}
	},
		[&](std::unique_ptr<GoogleException> ex)
	{
		m_down_progress_percentage = 0;
		m_makeq_inprogress = false;
#ifdef API_QT_AUTOTEST
		if (!ApiAutotest::INSTANCE().isCancelRequested()) {
			ASSERT(0, "Failed to makeHistoryQ") << ex->what();
		}
#else
		ASSERT(0, "Failed to makeHistoryQ") << ex->what();
#endif
		release();
	});
};


void ard::rule_runner::make_gui_query()
{
	ard::gui_check_and_notify_on_gmail_access_error();
	ard::asyncExec(AR_check4NewEmail, this);
}

bool ard::rule_runner::canAcceptChild(cit_cptr)const
{
	return false;
};

bool ard::rule_runner::addItem(topic_ptr)
{
	ASSERT(0, "NA");
	return false;
}

void ard::rule_runner::detach_q()
{
	if (m_makeq_inprogress) {
		qWarning() << "ERROR detach-q while makeq is in progress";
	}
	if (m_qparam) {
		m_qparam->release();
		m_qparam = nullptr;
	}

	m_makeq_inprogress = false;
};

bool ard::rule_runner::isDefinedQ()const
{
	bool rv = false;
	if (m_qparam) {
		rv = (m_qparam->get_q() != nullptr);
	}
	return rv;
};

bool ard::rule_runner::canCheck4NewEmails()const
{
	if (m_makeq_inprogress)
		return false;

	if (!ard::isDbConnected())
		return false;

	if (!isDefinedQ())
		return false;

	if (gmail_last_check_time.isValid()) {
		auto d2 = QDateTime::currentDateTime();
		auto msd = gmail_last_check_time.msecsTo(d2);
		if (msd < 2000) {
			return false;
		}
	}

	return true;
};


void ard::rule_runner::applyOnVisible(std::function<void(ard::topic*)> fnc)
{
	if (m_qparam) {
		return m_qparam->applyOnVisible(fnc);
	}
};

QString	ard::rule_runner::outlineLabel()const 
{
	QString rv = "--";
	if (m_qparam && m_qparam->isValid()) {
//#ifdef _DEBUG
		rv = QString("%1/%2").arg(m_qparam->get_q()->unreadCount()).arg(m_qparam->m_observed_threads.size());
//#else
//		rv = QString("%1").arg(m_qparam->get_q()->unreadCount());
//#endif
	}
	return rv;
};

std::pair<TernaryIconType, int> ard::rule_runner::ternaryIconWidth()const
{
	if (m_ternary_icon_width <= 0) {
		m_ternary_icon_width = 2*gui::lineHeight();
	}
	std::pair<TernaryIconType, int> rv{ TernaryIconType::gmailFolderStatus,  m_ternary_icon_width };
	return rv;
}

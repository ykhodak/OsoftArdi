/////////////////////////////
#define ARD_VERSION 675
/////////////////////////////

#include <QApplication>
#include <iostream>
#include <QComboBox>
#include <QFileIconProvider>
#include <QDomDocument>
#include <QDomNodeList>
#include <QMenu>
#include <QFileDialog>
#include <QDesktopWidget>
#include <csetjmp>
#include "email.h"
#include "ethread.h"
#include "contact.h"
#include "MainWindow.h"
#include "ansyncdb.h"
#include "ardmodel.h"
#include "TabControl.h"
#include "OutlineMain.h"
#include "OutlineScene.h"
#include "small_dialogs.h"
#include "OutlineMain.h"
#include "syncpoint.h"
#include "address_book.h"
#include "NoteFrameWidget.h"
#include "NoteEdit.h"
#include "ADbgTester.h"
#include "UFolderBox.h"
#include "db-merge.h"
#include "ansearch.h"
#include "board.h"
#include "ard-db.h"
#include "anurl.h"
#include "utils.h"
#include "PopupCard.h"
#include "BlackBoard.h"
#include "rule.h"
#include "board.h"
#include "rule_runner.h"
#include "mail_board_page.h"

#include "google/endpoint/ApiAppInfo.h"
#include "google/endpoint/ApiAuthInfo.h"
#include "google/endpoint/GoogleWebAuth.h"
using namespace googleQt;

///file to be "touched"
extern void check_cleanup();

int get_app_version_as_int(){return ARD_VERSION;};

#ifdef ARD_OPENSSL
#error openssl enryption support is disabled
#endif

QString gmail_access_last_exception;
QString gmail_send_last_exception;
extern QDateTime gmail_last_check_time;
static int g_gmail_zero_status_reconnect_counter = 0;


#ifdef _DEBUG 

void test_runner() 
{
    ADbgTester::test_print_fonts();
}

void MainWindow::debugFunction()
{
	
};


void MainWindow::debugFunction2()
{
	auto w = ard::wspace();
	if (w)
	{
		auto mb = w->mailBoard();
		if (mb)
		{
			auto bidx = mb->currentBand();
			if (bidx != -1)
			{
				//qDebug() << "current-band-index" << bidx;
				auto b = dynamic_cast<ard::mail_board*>(mb->board());
				if (b) {
					auto r = b->topics()[bidx];
					//qDebug() << "current-rule" << r->title();
					mb->rebuildBoardBand(r);
				}
			}
		}
	}
};

#endif

void ard::rule_runner::checkHistory(bool deep_check)
{
	//qDebug() << QString("check-history [%1]").arg(deep_check ? "Y" : "N");
	//ard::trail(QString("check-history [%1]").arg(deep_check?"Y":"N"));

	if (!ard::isGoogleConnected()) {
		if (!ard::hasGoogleToken()) {
			qWarning() << "gmail-checkHistory, skipped - no token";
			return;
		}
	}

    auto r = ard::gmail();
    assert_return_void(r, "expected gmail");
    auto st = ard::gstorage();
    assert_return_void(st, "expected gmail storage");
    QString userId = dbp::configEmailUserId();
    assert_return_void(!userId.isEmpty(), "Gmail userId is not defined");

    if (m_check_hist_inprogress) {
        qWarning() << "check-hist/ignored.check-hist-in-progress";
        return;
    }
	
	if (m_makeq_inprogress) {
		qWarning() << "makeQ-inprogress, skipped";
		return;
	}


    //m_down_progress_percentage = 1;
    //ard::asyncExec(AR_UpdateGItem, this);

    QString s;
    googleQt::gmail::HistoryListArg histArg(userId, st->lastHistoryId());
    //histArg.setMaxResults(10);
    auto hres = r->getHistory()->list_Async(histArg);
    m_check_hist_inprogress = true;
	gmail_last_check_time = QDateTime::currentDateTime();
	LOCK(this);
    hres->then([=](std::unique_ptr<googleQt::history::HistoryRecordList> r)
    {
        auto hist_id = r->historyid().toULong();
        m_check_hist_inprogress = false;
        if(r->nextpagetoken().isEmpty() && (st->lastHistoryId() == hist_id))
        {   
			auto pol = gui::currPolicy();
			if (pol == outline_policy_PadEmail) 
			{
				QTimer::singleShot(100, [=]() {
					m_down_progress_percentage = 0;
					auto pol = gui::currPolicy();
					if (pol == outline_policy_PadEmail) 
					{
						if (deep_check) {
							qWarning() << "deep-check on 0-hist diff";
							doMakeQ();
						}
						//else {
						//	ard::asyncExec(AR_UpdateGItem, this);
						//}
						//gui::rebuildOutline();
					}
				});
			}
        }
        else
        {
			m_down_progress_percentage = 1;
			ard::asyncExec(AR_UpdateGItem, this);

            std::set<QString> msg_in_hist;
            std::vector<HistId> id_list;
            auto& hr_lst = r->history();
            for (auto& hr : hr_lst) 
            {
				ard::trail(QString("history-record [%1] messages[%2] added[%3] deleted[%4] scheduled[%5]")
					.arg(hr.id())
					.arg(hr.messages().size())
					.arg(hr.messagesadded().size())
					.arg(hr.messagesdeleted().size())
					.arg(msg_in_hist.size()));

                auto& msg_lst = hr.messages();
                for (auto& m : msg_lst) 
                {
                    auto i = msg_in_hist.find(m.threadid());
                    if (i == msg_in_hist.end())
                    {
                        msg_in_hist.insert(m.threadid());
						ard::trail(QString("history-msg [%1] [%2] [%3]").arg(m.id()).arg(m.threadid()).arg(m.historyid()));
                        HistId h;
                        h.id = m.threadid();
                        h.hid = hist_id;
                        id_list.push_back(h);
                    }
                }
            }

            if (!id_list.empty()) 
            {
                makeHistoryQ(hist_id, id_list);
            }
            else 
            {
                m_down_progress_percentage = 0;
                ard::asyncExec(AR_UpdateGItem, this);
            }
        }
		release();
		g_gmail_zero_status_reconnect_counter = 0;
    },
        [&](std::unique_ptr<GoogleException> ex)
    {
        m_check_hist_inprogress = false;
        m_down_progress_percentage = 0;
        ard::asyncExec(AR_UpdateGItem, this);
#ifdef API_QT_AUTOTEST
        if (!ApiAutotest::INSTANCE().isCancelRequested()) {
            ASSERT(0, "Failed to check on history") << ex->what();
        }
#else
		qWarning() << "history check exception" << ex->what();

        switch (ex->statusCode()) 
        {
        case 404: 
        {
            //qWarning() << "making Q 404/history";
			ard::trail(QString("history-404 [%1]").arg(ex->what()));
            doMakeQ();
        }break;
        case 0: 
        {            
			if (g_gmail_zero_status_reconnect_counter == 0) {
				g_gmail_zero_status_reconnect_counter = 1;
			}
			else {
				if (g_gmail_zero_status_reconnect_counter < 128) {
					g_gmail_zero_status_reconnect_counter *= 2;
				}
			}
            qWarning() << "reconnecting gmail on exception" << g_gmail_zero_status_reconnect_counter << ex->what();
			int reconnect_delay = 1000 * g_gmail_zero_status_reconnect_counter;
			ard::trail(QString("schedule-reconnect in [%1] ms").arg(reconnect_delay));
			QTimer::singleShot(reconnect_delay, [=]() {
				ard::asyncExec(AR_ReconnectGmailUser);
			});
        }break;
        default: 
        {
            gmail_access_last_exception = ex->what();
        }
        }
#endif
		release();
    });
};

extern void requestRecoveryMode(int signal_value);
jmp_buf env;

void on_sigabrt(int s)
{
    qWarning() << "caught signal" << s;
    requestRecoveryMode(s);
    longjmp(env, s);
}

bool tryCatchAbort(TRY_ABORT_FUNCTION func, void* vparam, QString contextInfo)
{
    Q_UNUSED(contextInfo);

#ifdef _DEBUG
    (*func)(vparam);
    return true;
#else

#ifdef Q_OS_WIN32
    try
    {
        (*func)(vparam);
    }
    catch (std::exception & ex)
    {
        qWarning() << "generic-exception" << ex.what();
        ard::ArdStackWalker sw;
        sw.ShowCallstack(GetCurrentThread(), sw.GetCurrentExceptionContext());
    }
    catch (...)
    {
        ard::ArdStackWalker sw(StackWalker::AfterCatch);
        sw.ShowCallstack();
    }
    return true;
#else
    bool rv = true;
    int r = setjmp(env);
    if (r == 0) {
        signal(SIGTERM, &on_sigabrt);
        signal(SIGSEGV, &on_sigabrt);
        signal(SIGINT, &on_sigabrt);
        signal(SIGILL, &on_sigabrt);
        signal(SIGABRT, &on_sigabrt);
        signal(SIGFPE, &on_sigabrt);
        (*func)(vparam);
    }
    else {
        qWarning() << "aborted: " << r << contextInfo;
        rv = false;
    }
    return rv;
#endif
#endif
}

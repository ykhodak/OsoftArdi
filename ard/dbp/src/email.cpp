#include <QFile>
#include <QDesktopServices>
#include <QTextDocument>
#include <QTextDocumentFragment>
#include <QDir>
#include <QFileIconProvider>
#include <QTimer>

#include "a-db-utils.h"
#include "email.h"
#include "ethread.h"
#include "GoogleClient.h"
#include "gdrive-syncpoint.h"
#include "ansearch.h"
#include "google/endpoint/ApiAppInfo.h"
#include "google/endpoint/ApiAuthInfo.h"
#include "google/endpoint/GoogleWebAuth.h"
#include "GoogleClient.h"
#include "Endpoint.h"
#include "gmail/GmailRoutes.h"
#include "gmail/GmailCache.h"
#include "extnote.h"
#include "email_draft.h"
#include "rule.h"
#include "rule_runner.h"


#ifdef API_QT_AUTOTEST
#include "google/AUTOTEST/GoogleAutotest.h"
static googleQt::GoogleAutotest autotest(nullptr);
#endif

#define APP_KEY     "588332951231-dc305h2m6ssf3qi7ojeo0n9iii47hlo4.apps.googleusercontent.com"
#define APP_SECRET  "h22c18uzu3uxCOiUDvSMjX1f"

extern QString gmailDBFilePath();
extern QString gmailCachePath();

using namespace googleQt;

IMPLEMENT_ALLOC_POOL(email);

bool g__request_gdrive_scope_access = false;
double net_traffic_progress = 0.0;
void update_net_traffic_progress();

#ifdef API_QT_AUTOTEST
int g_wrapper_msg_alloc_counter = 0;
#endif


#ifdef _DEBUG
void printMessageShort(messages::MessageResource* r)
{
    qDebug() << "id="<< r->id()
              << "tid=" << r->threadid()
              << "snippet=" << r->snippet();
    const std::vector <QString>& labels = r->labelids();
    if(labels.size() > 0){
        QString slb = "";
        for (auto lb : labels) {
            slb += lb;
            slb += " ";
        }        
        qDebug() << "labels=" << slb;
    }    
}
#else
void printMessageShort(messages::MessageResource* ){};
#endif

ard::email::email(googleQt::mail_cache::msg_ptr d)
    :m_o(d)
{
    m_emailFlags.flags = 0;

#ifdef API_QT_AUTOTEST
    g_wrapper_msg_alloc_counter++;
#endif
};

ard::email::~email()
{
    //detachObj();
#ifdef API_QT_AUTOTEST
    g_wrapper_msg_alloc_counter--;
#endif
}

bool ard::email::isValid()const 
{
	return (m_o != nullptr);
};

topic_ptr ard::email::produceLabelSubject()
{
    auto f = parent();
    if (f) {
        auto tp = dynamic_cast<ard::ethread*>(f);
        assert_return_null(tp, "expected thread parent");
        return tp->produceLabelSubject();
    }
    return nullptr;
};

cit_base* ard::email::create()const
{
	assert_return_null(0, "email - can't create empty wrapper");
	//return nullptr;
    //return new email();
};

QString ard::email::objName()const
{
    return "email";
};


QString ard::email::title()const
{
    calc_headers();
    return m_calc_title;
};

QString ard::email::plainSubject()const
{
    calc_headers();
    return m_calc_plain_subject;
};

int ard::email::accountId()const
{
    int rv = -1;
    if (m_o)
    {
        rv = m_o->accountId();
    }
    return rv;
};

QString ard::email::wrappedId()const
{
    QString rv;
    if (m_o)
        {
            rv = m_o->id();
        }
    return rv;
};

QString ard::email::plainFrom()const
{
    QString rv;
    if (m_o)
        {
            rv = QTextDocumentFragment::fromHtml(m_o->from()).toPlainText();
        }
    return rv;
};

QString ard::email::plainTo()const
{
    QString rv;
    if (m_o)
        {
            rv = QTextDocumentFragment::fromHtml(m_o->to()).toPlainText();
        }
    return rv;
};

QString ard::email::plainCC()const
{
    QString rv;
    if (m_o)
        {
            rv = QTextDocumentFragment::fromHtml(m_o->cc()).toPlainText();
        }
    return rv;
};

QString ard::email::plainBCC()const
{
    QString rv;
    if (m_o)
        {
            rv = QTextDocumentFragment::fromHtml(m_o->bcc()).toPlainText();
        }
    return rv;
};

QString ard::email::from()const
{
    QString rv;
    if (m_o)
        {
            rv = m_o->from();
        }
    return rv;
};

QString ard::email::to()const
{
    QString rv;
    if (m_o)
        {
            rv = m_o->to();
        }
    return rv;
};

QString ard::email::CC()const
{
    QString rv;
    if (m_o)
        {
            rv = m_o->cc();
        }
    return rv;

};

QString ard::email::BCC()const
{
    QString rv;
    if (m_o)
        {
            rv = m_o->bcc();
        }
    return rv;
};

QString ard::email::references()const
{
    QString rv;
    if (m_o)
        {
            rv = m_o->references();
        }
    return rv;
};

QString ard::email::snippet()const
{
    QString rv;
    if (m_o)
    {
        rv = m_o->snippet();
    }
    return rv;
};

qlonglong ard::email::internalDate()const
{
    qlonglong rv = 0;
    if (m_o)
    {
        rv = m_o->internalDate();
    }
    return rv;
};

QString ard::email::dateColumnLabel()const
{
    if (m_o)
        {
            calc_headers();
        }
    return m_calc_date_label;
}

std::vector<mail_cache::label_ptr> ard::email::getLabels()const
{
    auto gm = ard::gmail();
    if (m_o && gm && gm->cacheRoutes())
        {
            return gm->cacheRoutes()->getMessageLabels(m_o.get());
        }    

    std::vector<mail_cache::label_ptr> on_error;
    return on_error;
};

bool ard::email::hasLabelId(QString label_id)const
{
    assert_return_false(ard::gmail(), "expected gmail");
    assert_return_false(ard::gmail()->cacheRoutes(), "expected gmail cache");

    bool rv = false;
    if (m_o){
            auto gm = ard::gmail();
            if (gm){
                    rv = gm->cacheRoutes()->messageHasLabel(m_o.get(), label_id);
                }
        }
    return rv;
};

bool ard::email::hasSysLabel(googleQt::mail_cache::SysLabel sl)const
{
    bool rv = false;
    if (m_o) {
        rv = m_o->hasReservedSysLabel(sl);
    }
    return rv;
};

bool ard::email::isStarred()const
{   
    return hasLabelId("STARRED");
};

bool ard::email::isImportant()const
{
    return hasLabelId("IMPORTANT");
}

bool ard::email::isUnread()const
{
    return hasLabelId("UNREAD");
};

bool ard::email::hasPromotionLabel()const
{
    return hasLabelId("CATEGORY_PROMOTIONS");
};

bool ard::email::hasSocialLabel()const
{
    return hasLabelId("CATEGORY_SOCIAL");
};

bool ard::email::hasUpdateLabel()const
{
    return hasLabelId("CATEGORY_UPDATES");
};

bool ard::email::hasForumsLabel()const
{
    return hasLabelId("CATEGORY_FORUMS");
};

bool ard::email::hasAttachment()const
{
    if (!m_o)
        return false;

    if (!m_o->isLoaded(googleQt::EDataState::body))
        return false;


    if (m_emailFlags.attachments_queried) {
        return m_emailFlags.has_attachments ? true : false;
    }

    bool rv = false;
    auto storage = ard::gstorage();
    if (storage) {
        rv = (m_o->getAttachments(storage).size() > 0);
    }

    m_emailFlags.attachments_queried = 1;
    m_emailFlags.has_attachments = rv ? 1 : 0;
    return rv;
};

googleQt::mail_cache::ATTACHMENTS_LIST ard::email::getAttachments()
{
	googleQt::mail_cache::ATTACHMENTS_LIST rv;
	if (m_o)
	{
		auto storage = ard::gstorage();
		if (storage) {
			rv = m_o->getAttachments(storage);
		}
	}
	return rv;
};


void ard::email::setUnread(bool set_it)
{
    assert_return_void(ard::gmail(), "expected gmail");
    assert_return_void(ard::gmail()->cacheRoutes(), "expected gmail cache");

    if (m_o)
    {
        if (set_it) {
            if (isUnread())
                return;
        }
        else {
            if (!isUnread())
                return;
        }

        googleQt::GmailRoutes* gm = ard::gmail();
        if (gm)
        {
            gm->cacheRoutes()->setUnread_Async(m_o.get(), set_it)->then(
				[]()
            {
            },
                [](std::unique_ptr<GoogleException> ex)
            {
                ASSERT(0, "Failed to toggle 'Unread'") << ex->what();
            });
        }
    }
};

void ard::email::setStarred(bool set_it)
{
	assert_return_void(ard::gmail(), "expected gmail");
	assert_return_void(ard::gmail()->cacheRoutes(), "expected gmail cache");

	if (m_o)
	{
		googleQt::GmailRoutes* gm = ard::gmail();
		if (gm)
		{
			gm->cacheRoutes()->setStarred_Async(m_o.get(), set_it)->then
			([]()
			{
				//printMessageShort(r.get());
			},
				[](std::unique_ptr<GoogleException> ex)
			{
				ASSERT(0, "Failed to toggle 'Starred'") << ex->what();
			});
		}
	}
};

void ard::email::setImportant(bool set_it)
{
	assert_return_void(ard::gmail(), "expected gmail");
	assert_return_void(ard::gmail()->cacheRoutes(), "expected gmail cache");

	if (m_o)
	{
		googleQt::GmailRoutes* gm = ard::gmail();
		if (gm)
		{
			gm->cacheRoutes()->setImportant_Async(m_o.get(), set_it)->then(
				[]()
			{
				//printMessageShort(r.get());
			},
				[](std::unique_ptr<GoogleException> ex)
			{
				ASSERT(0, "Failed to toggle 'Important'") << ex->what();
			});
		}
	}
};


void ard::email::downloadAttachment(googleQt::mail_cache::att_ptr a)
{
    assert_return_void(ard::gmail(), "expected gmail");
    assert_return_void(ard::gmail()->cacheRoutes(), "expected gmail cache");

    switch (a->status())
    {
    case googleQt::mail_cache::AttachmentData::statusDownloaded:
    {
        ASSERT(0, "Attachment already downloaded") << a->attachmentId() << a->localFilename();
        return;
    }break;
    case googleQt::mail_cache::AttachmentData::statusDownloadInProcess:
    {
        ASSERT(0, "Attachment download already started") << a->attachmentId() << a->localFilename();
        return;
    }break;
    case googleQt::mail_cache::AttachmentData::statusNotDownloaded:break;
    }

    if (m_o)
    {
        googleQt::GmailRoutes* gm = ard::gmail();
        if (gm)
        {
            auto cr = gm->cacheRoutes();
            assert_return_void(cr, "expected cache root");
            auto dest_dir = ard::getAttachmentDownloadDir();
            auto t = cr->downloadAttachment_Async(m_o,
                a,
                dest_dir);
            t->then([=]() 
			{
#ifdef _DEBUG
				qDebug() << "attachement downloaded" << a->localFilename();
#endif
				/*
                QString destinationThumbFolder;
                QDir dest_thumb_dir;
                if (!dest_thumb_dir.exists(destinationThumbFolder)) {
                    if (!dest_thumb_dir.mkpath(destinationThumbFolder)) {
                        qWarning() << "Failed to create directory" << destinationThumbFolder;
                        return;
                    };
                }*/
            });
        }
    }
};

void ard::email::download_all_attachments()
{
	qDebug() << "ykh/download_all_attachments";

	assert_return_void(ard::gmail(), "expected gmail");
	assert_return_void(ard::gmail()->cacheRoutes(), "expected gmail cache");

	if (m_o)
	{
		googleQt::GmailRoutes* gm = ard::gmail();
		if (gm)
		{
			auto cr = gm->cacheRoutes();
			assert_return_void(cr, "expected cache route");
			auto dest_dir = ard::getAttachmentDownloadDir();
			auto t = cr->downloadAllAttachments_Async(m_o, dest_dir);
			t->then([=]()
			{
				auto m = ard::gmail_model();
				if (m) {
					m->fire_all_attachments_downloaded(this);
				}

#ifdef _DEBUG
				qDebug() << "all attachement downloaded" << dest_dir;
#endif
			});
		}
	}
};

void ard::email::calc_headers()const
{
    if (!m_emailFlags.calculated)
    {
        if (m_o)
        {
            m_calc_title = "";
#ifdef _DEBUG
            //                  m_calc_title = QString("[%1] %2   ").arg(m_d->id()).arg(m_d->internalDate());
            //                  m_calc_title = QString("[%1] [%2]   ").arg(m_d->id()).arg(m_d->userId());
#endif
            QString s_from = m_o->from();
            s_from.remove(QRegExp("<[^>]*>"));
            m_calc_title += s_from;
            m_emailFlags.from_length = s_from.length();

            m_calc_plain_subject = QTextDocumentFragment::fromHtml(m_o->subject()).toPlainText();
            m_calc_title += " " + m_calc_plain_subject;

            QString s_snippet = QTextDocumentFragment::fromHtml(m_o->snippet()).toPlainText();
            m_calc_title += " " + s_snippet;

            m_emailFlags.subject_length = m_calc_plain_subject.length();
            m_emailFlags.snippet_length = s_snippet.length();

            ///date label
            QDateTime currDate = QDateTime::currentDateTime();

            qlonglong d1 = m_o->internalDate();
            QDateTime dt = QDateTime::fromMSecsSinceEpoch(d1);
            if (dt.isValid())
            {
                QDate dt_mail = dt.date();
                QDate dt_curr = currDate.date();

                if (dt_mail == dt_curr)
                {
                    ///time part
                    m_calc_date_label = dt.toString("hh:mm A");
                }
                else if (dt_mail.year() == dt_curr.year())
                {
                    //same year month + date
                    m_calc_date_label = dt.toString("MMM d");
                }
                else
                {
                    //year or more old
                    m_calc_date_label = dt.toString("M/d/yy");
                }
            }

            ///date label
            m_emailFlags.calculated = 1;
        }
    }
};

QString ard::email::dbgHint(QString s)const
{
    QString rv;
    if (m_o) {
        rv += m_o->id();
    }
    rv += " " + ard::topic::dbgHint(s);
    return rv;
};

ENoteView ard::email::noteViewType()const
{
    ENoteView rv = ENoteView::None;
    if (m_o)
        {
            rv = ENoteView::View;
        }
    return rv;
};

QPixmap ard::email::getIcon(OutlineContext c)const
{    
    if(!isUnread())
        {
            return ard::topic::getIcon(c);
        }
    return getIcon_NotLoadedEmail();    
};

QPixmap ard::email::getSecondaryIcon(OutlineContext)const
{
    return QPixmap();
};

void ard::email::prepareCustomTitleFormatting(const QFont& fntNormal,
    QList<QTextLayout::FormatRange>& fr)
{
    if (m_o)
        {
            calc_headers();

            QFont fnt_bold(fntNormal);
            QFont fnt_small(fntNormal);

            fnt_bold.setBold(true);            
            fnt_small.setPointSize(fnt_small.pointSize() - 2);

           // QFont fnt(pf);
           // fnt.setBold(true);
            QTextCharFormat f_normal;
            f_normal.setFont(fntNormal);

            QTextCharFormat f_normal_gray = f_normal;
            f_normal_gray.setForeground(Qt::darkGray);

            QTextCharFormat f_small_gray;
            f_small_gray.setFont(fnt_small);
            f_small_gray.setForeground(Qt::darkGray);

            QTextCharFormat f_from_read;
            f_from_read.setFont(fnt_bold);
            f_from_read.setForeground(Qt::darkBlue);

            QTextCharFormat f_from_unread;
            f_from_unread.setFont(fntNormal);
            f_from_unread.setForeground(Qt::darkBlue);


            QTextCharFormat f_bold;
            f_bold.setFont(fnt_bold);
            
            QTextCharFormat f_bold_gray = f_bold;
            f_bold_gray.setForeground(Qt::darkGray);

            QTextLayout::FormatRange fr_tracker;
            fr_tracker.start = 0;
            fr_tracker.length = m_emailFlags.from_length;
            //fr_tracker.format = f_from;
            //fr_tracker.format = f_small_gray;//f_bold_gray;
            fr_tracker.format = isUnread() ? f_from_read : f_from_unread;
            fr.push_back(fr_tracker);

            fr_tracker.start = 1 + m_emailFlags.from_length;
            fr_tracker.length = m_emailFlags.subject_length;
            fr_tracker.format = isUnread() ? f_bold : f_normal_gray;
            fr.push_back(fr_tracker);

            fr_tracker.start = 2 + m_emailFlags.from_length + m_emailFlags.subject_length;
            fr_tracker.length = m_emailFlags.snippet_length;
            fr_tracker.format = f_normal;
            
            fr.push_back(fr_tracker);
        }
};

topic_ptr ard::email::prepareInjectIntoBBoard() 
{
    auto p = parent();
    if (p) {
        return p->prepareInjectIntoBBoard();
    }
    return nullptr;
};

bool ard::email::isContentLoaded()const
{
    bool rv = false;
    if (m_o)
        {
            rv = m_o->isLoaded(EDataState::body);
        }
    return rv;
};

email_ptr ard::email::create_out_of_shadow_obj(googleQt::mail_cache::msg_ptr& d)
{
    email_ptr em = new email(d);
   // em->attachObj(d);
    return em;
};


int recoverContent(ard::note_ext* c, googleQt::mail_cache::msg_ptr m)
{
    QString html = m->html();
    if (html.isEmpty()) 
    {
        extern QString asHtmlDocument(QString sPlainText);
        QString s_snippet = m->snippet();
        html = asHtmlDocument(s_snippet);
        c->setNoteHtml(html, s_snippet);
    }
    else {
        c->setNoteHtml(html, m->plain());
    }
	return html.size();
}

ard::note_ext* ard::email::mainNote()
{
    if (m_note_ext)
    {
        return m_note_ext;
    }
    if (m_o)
    {
        if (!m_o->isLoaded(EDataState::body))
        {
            assert_return_null(ard::gmail(), "expected gmail");
            assert_return_null(ard::gmail()->cacheRoutes(), "expected gmail cache");

            auto gc = ard::google();
            auto gm = ard::gmail();
            if (gc && gm)
            {
                gc->endpoint()->diagnosticSetRequestContext("loadMsgBody4Note");

				LOCK(this);
                std::vector<QString> id_list;
                id_list.push_back(m_o->id());
				ard::trail(QString("get-body [%1][%2]").arg(m_o->id()).arg(title().left(20)));
                mail_cache::GMailCacheQueryTask* q = gm->cacheRoutes()->getCacheMessages_Async(EDataState::body, id_list);
                q->then([=](std::unique_ptr<googleQt::CacheDataResult<googleQt::mail_cache::MessageData>>)
                {
                    if (isUnread()) {
						auto m = ard::gmail_model();
						if (m) {
							ESET l2;
							l2.insert(this);
							m->markUnread(l2, false);
						}
                    }
                    auto e = ensureNote();
                    if (e) {
                        auto size = recoverContent(e, m_o);
						ard::trail(QString("body-ok [%1][%2][%3]").arg(m_o->id()).arg(size).arg(title().left(20)));
                        ard::asyncExec(AR_NoteLoaded, this);
                    }
					release();					
                },
                    [=](std::unique_ptr<GoogleException> ex)
                {
                    ASSERT(0, "Failed to download body of message") << m_o->id() << ex->what();
					release();
                });
            }
        }

        auto e = ensureNote();
        if (e) {
            recoverContent(e, m_o);
        }
    }
    else
    {
        ensureNote();
    }
    return m_note_ext;
};

const ard::note_ext* ard::email::mainNote()const
{
    ard::email* ThisNonConst = (ard::email*)this;
    return ThisNonConst->mainNote();
};

ard::note_ext* ard::email::ensureNote()
{
    ASSERT_VALID(this);
    if (m_note_ext)
        return m_note_ext;

    ensureExtension(m_note_ext);
    m_note_ext->disablePersistance();
    return m_note_ext;
};

/*
bool ard::email::kill(bool silently)
{
    ASSERT_VALID(this);
    auto p = parent();
    if (!p) {
        return false;
    }

    return p->kill(silently);
};*/


bool ard::email::killSilently(bool)
{
	ASSERT(0, "NA");
	return false;
}

DB_ID_TYPE ard::email::underlyingDbid()const
{
    return 0;
};

ethread_ptr ard::email::parent()
{
	return dynamic_cast<ard::ethread*>(cit_parent());
};

ethread_cptr ard::email::parent()const
{
	ethread_cptr f = dynamic_cast<const ard::ethread*>(cit_parent());
    return const_cast<ard::ethread*>(f);
};

void ard::email::setToDo(int done_percent, ToDoPriority prio)
{
    auto p = parent();
    if (p) {
        p->setToDo(done_percent, prio);
    }
};

bool ard::email::isToDo()const
{
    auto p = parent();
    if (p) {
        return p->isToDo();
    }
    return false;
};

bool ard::email::isToDoDone()const
{
    auto p = parent();
    if (p) {
        return p->isToDoDone();
    }
    return false;
};

int ard::email::getToDoDonePercent()const
{
    auto p = parent();
    if (p) {
        return p->getToDoDonePercent();
    }
    return 0;
};

int ard::email::getToDoDonePercent4GUI()const
{
    auto p = parent();
    if (p) {
        return p->getToDoDonePercent4GUI();
    }
    return 0;
};

unsigned char ard::email::getToDoPriorityAsInt()const
{
    auto p = parent();
    if (p) {
        return p->getToDoPriorityAsInt();
    }
    return 0;
};

ToDoPriority ard::email::getToDoPriority()const
{
    auto p = parent();
    if (p) {
        return p->getToDoPriority();
    }
    return ToDoPriority::notAToDo;
};

bool ard::email::hasToDoPriority()const
{
    auto p = parent();
    if (p) {
        return p->hasToDoPriority();
    }
    return false;
};

topic_ptr ard::email::getToDoContext()
{
    return parent();
};

void ard::email::setColorIndex(EColor c)
{
    auto p = parent();
    if (p) {
        p->setColorIndex(c);
    }
};

ard::EColor ard::email::colorIndex()const
{
    auto p = parent();
    if (p) {
        return p->colorIndex();
    }
    return ard::EColor::none;
};

bool ard::email::canBeMemberOf(const topic_ptr f)const 
{
    ASSERT_VALID(this);
    bool rv = false;
    if (dynamic_cast<const ard::ethread*>(f) != nullptr) {
        rv = true;
    }
    return rv;
};

bool ard::email::canAcceptChild(cit_cptr )const 
{
    return false;
};

topic_ptr ard::email::produceMoveSource(ard::email_model* m)
{
	auto p = parent();
	if (p) {
		return p->produceMoveSource(m);
	}
	return nullptr;
};

QString ard::email::annotation()const
{
    auto p = parent();
    if (p) {
        return p->annotation();
    }
    return "";
};

QString ard::email::annotation4outline()const
{
    auto p = parent();
    if (p) {
        if (p->headMsg() == this) {
            return p->annotation();
        }
    }

    return "";
};

void ard::email::setAnnotation(QString s, bool gui_update)
{
    auto p = parent();
    if (p) {
        p->setAnnotation(s, gui_update);
    }
};

bool ard::email::canHaveAnnotation()const
{
    auto p = parent();
    if (p) {
        return p->canHaveAnnotation();
    }
    return false;
};

ard::email_model::email_model()
{
	install_clients_release_timer();
	install_drafts_scheduler_timer();
}

ard::email_model::~email_model()
{
    disableGoogle();
};

void ard::email_model::install_clients_release_timer() 
{
	m_clients_release_timer.setInterval(1000);
	connect(&m_clients_release_timer, &QTimer::timeout, [=]()
	{
		for (auto g : m_clients2release) {
			if (g && !g->isQueryInProgress()) {
				googleQt::releaseClient(g);
			}
		}

		m_clients2release.erase(std::remove_if(m_clients2release.begin(),
			m_clients2release.end(),
			[&](googleQt::gclient_ptr g)
		{
			return !g;
		}),
			m_clients2release.end());

		if (m_clients2release.empty()) {
			m_clients_release_timer.stop();
		}
	});
};

void ard::email_model::install_drafts_scheduler_timer() 
{
	m_drafts_scheduler_timer.setInterval(5000);
	connect(&m_drafts_scheduler_timer, &QTimer::timeout, [=]()
	{
		run_drafts_scheduler();
	});
};

void ard::email_model::run_drafts_scheduler()
{
	using DSET = std::unordered_set<ard::email_draft*>;
	DSET resolved;

	for (auto d : m_scheduled_drafts)
	{
		auto de = d.first->draftExt();
		if (de) {
			auto gc = d.second.gclient;
			auto em = d.second.attachements_forward_origin;
			if (gc && em && em->optr())
			{
				auto gm = gc->gmail();
				if (gm) {
					auto cr = gm->cacheRoutes();
					assert_return_void(cr, "expected cache route");
					int status_mask = (googleQt::mail_cache::AttachmentData::EStatus::statusDownloadInProcess |
						googleQt::mail_cache::AttachmentData::EStatus::statusNotDownloaded);
					auto lst = cr->getAttachmentsWithStatus(em->optr(), status_mask);
					if (lst.empty()) 
					{
						if (de->rebuild_unresolved_attachements(gc, em)) {
							resolved.insert(d.first);
						};
					}
				}
			}
		}
	}

	for (auto&i : resolved) {
		auto j = m_scheduled_drafts.find(i);
		if (j != m_scheduled_drafts.end()) {
			m_scheduled_drafts.erase(j);
			i->release();
		}
	}

	if (m_scheduled_drafts.empty()) {
		m_drafts_scheduler_timer.stop();
	}
};

bool ard::email_model::isGoogleConnected()
{
    bool rv = false;
    if (m_gclient) {
        rv = true;
    }
    return rv;
};

bool ard::email_model::reconnectGoogle(QString userId)
{
    disableGoogle();
    dbp::configSetEmailUserId(userId);
    return true;
};


void ard::email_model::disableGoogle()
{
    if (ard::isDbConnected()) {
        auto q = ard::db()->gmail_runner();
        if (q) {
            q->detach_q();
        }
    }

    detachModelGui();

    if (m_gclient) {
        m_gclient->cancelAllRequests();
        if (m_gclient->gmail()->cacheRoutes()) {
            m_gclient->gmail()->cacheRoutes()->clearCache();
        }

        auto mr = m_gclient->gmail();
        if (mr) {
            auto cr = mr->cacheRoutes();
            if (cr) {
                auto st = cr->storage();
                if (st) {
                    st->setHistoryId(0);
                }
            }
        }

            
        releaseGClient();
#ifdef API_QT_AUTOTEST
        extern QString getMPoolStatisticsAsString(QString sprefix);
        qDebug() << getMPoolStatisticsAsString("G-layer-disabled");
#endif
    }

	ard::trail("gmail OFF");
    ard::asyncExec(AR_OnDisconnectedGmail);
    ard::asyncExec(AR_rebuildOutline);
	auto d = ard::db();
	if (d && d->isOpen()) {
		d->rmodel()->rroot()->clearRules();
	}
	m_gmail_cache_attached = false;
};

void ard::disconnectGoogle() 
{
    if (!gmail_model())
        return;

    gmail_model()->disableGoogle();
};

bool ard::reconnectGoogle(QString userId)
{
    if (!gmail_model())
        return false;

    return gmail_model()->reconnectGoogle(userId);
};

bool ard::authAndConnectNewGoogleUser()
{
    if (!gmail_model())
        return false;

    return gmail_model()->authAndConnectNewGoogleUser();
};


bool ard::email_model::initGClientUsingExistingToken()
{
	auto user = dbp::configEmailUserId();	
    QString token_file = dbp::configEmailUserTokenFilePath(user);
	if (!QFile::exists(token_file)) 
	{
		if (dbp::configSwitchToFallbackEmailUserId()) 
		{
			user = dbp::configEmailUserId();
			token_file = dbp::configEmailUserTokenFilePath(user);
		}
	}


    if (QFile::exists(token_file))
    {
		ard::trail(QString("init-gmail-using-token [%1][%2]").arg(user).arg(token_file));
        std::unique_ptr<ApiAppInfo> appInfo(new ApiAppInfo);
        appInfo->setKeySecret(APP_KEY, APP_SECRET);
        std::unique_ptr<ApiAuthInfo> authInfo(new ApiAuthInfo(token_file));
        authInfo->setEmail(dbp::configEmailUserId());

        if (!authInfo->reload()) {
            qWarning() << "Error reading token file: " << token_file;
            return false;
        }
        else
        {
            m_gclient = googleQt::createClient(appInfo.release(), authInfo.release());
			ard::trail(QString("gmail-allocate [0x%1][%2]").arg((quintptr)m_gclient.get()).arg(m_gclient.use_count()));
#ifdef API_QT_AUTOTEST
            ApiAutotest::INSTANCE().enableProgressEmulation(true);
#endif
            return setupGmailCache();
        }
    }

    return false;
}

int scopeLabels2ScopeCode(const std::vector<QString>& scopes)
{
    int rv = 0;
    static std::unordered_map<QString, int> s2c;
    if (s2c.empty()) {
        s2c[GoogleWebAuth::authScope_gdrive_appdata()] = 1;
        s2c[GoogleWebAuth::authScope_gmail_compose()] = 2;
        s2c[GoogleWebAuth::authScope_gmail_modify()] = 4;
        s2c[GoogleWebAuth::authScope_gdrive()] = 8;
    }

    for (auto i : scopes) {
        auto j = s2c.find(i);
        if (j != s2c.end()) {
            rv += j->second;
        }
        else {
            ASSERT(0, "unsupported scope") << i;
        }
    }
    return rv;
}

QString ScopeCode2ScopeLabels(int scode) 
{
    QString rv;
    if (scode >= 2) 
    {
        rv = "drive.appdata gmail.compose";
        if (scode >= 4)
        {
            rv = "drive.appdata gmail.modify";
        }
        if (scode >= 8)
        {
            rv += " drive";
        }
    }
    return rv;
}

bool ard::email_model::guiInitGClientByAcquiringNewToken()
{
    static bool google_auth_started = false;

    if (google_auth_started) {
		ard::messageBox(gui::mainWnd(), "Gmail authorization in process.");
        return false;
    }

    std::unique_ptr<googleQt::GoogleClient> rv;

    std::unique_ptr<ApiAppInfo> appInfo(new ApiAppInfo);
    appInfo->setKeySecret(APP_KEY, APP_SECRET);

    std::vector<QString> scopes;
    scopes.push_back(GoogleWebAuth::authScope_gmail_modify());
    scopes.push_back(GoogleWebAuth::authScope_gdrive_appdata());
	if (g__request_gdrive_scope_access) {
		scopes.push_back(GoogleWebAuth::authScope_gdrive());
		g__request_gdrive_scope_access = false;
	}
    QUrl codeAuthUrl = GoogleWebAuth::getCodeAuthorizeUrl(appInfo.get(), scopes);

#ifdef API_QT_AUTOTEST
    ApiAutotest::INSTANCE().enableProgressEmulation(true);
	ard::messageBox(gui::mainWnd(), QString("Link to open: %1").arg(codeAuthUrl.toString()));
#else
    QDesktopServices::openUrl(codeAuthUrl);
#endif
    
    QString code = gui::oauth2Code("");
    if (code.isEmpty())
    {        
        return false;
    };

    if (!code.isEmpty())
    {
        QDir d1;
        if(!d1.exists(gmailCachePath())){
            if(!d1.mkpath(gmailCachePath())){
				ard::errorBox(ard::mainWnd(), "Failed to create gmail cache folder. Please restart your device and try again.");
                return false;
            }
        }            
            
        QString tmp_token = dbp::configTmpUserTokenFilePath();
        if (QFile::exists(tmp_token)) {
            if (!QFile::remove(tmp_token)) {
                qWarning() << "failed to delete tmp token file " << tmp_token;
				ard::messageBox(gui::mainWnd(), "Failed to delete temporary token file. Please restart your device and try again.");
                return false;
            }
        }

        auto scope_code = scopeLabels2ScopeCode(scopes);

        bool got_token = false;
        std::unique_ptr<ApiAuthInfo> authInfo(new ApiAuthInfo(tmp_token, scope_code));

        try {
            google_auth_started = true;
            got_token = GoogleWebAuth::getTokenFromCode(appInfo.get(), code, authInfo.get());
            if (!got_token) {
                qWarning() << "Error, Access token not granted: " << tmp_token;
				ard::messageBox(gui::mainWnd(), "Error, Access token not granted.");
                return false;
            }
            qWarning() << "auth completed, token stored in file " << tmp_token << "token-scope:" << scope_code;
        }
        catch (GoogleException& e)
        {
            qWarning() << "ERROR. guiInitGClientByAcquiringNewToken. Exception=" << e.what();
			ard::messageBox(gui::mainWnd(), QString("Error, provided access token was not accepted. Please try to authorize again. Exception details: %1").arg(e.what()));
            //ard::resolveGoogleException(e);
            return false;
        }
        google_auth_started = false;

        if(got_token)
            {
                m_gclient = googleQt::createClient(appInfo.release(), authInfo.release());
				

                googleQt::gdrive::AboutArg arg;
                arg.setFields("user(displayName,emailAddress,permissionId), storageQuota(limit,usage)");
                auto t = m_gclient->gdrive()->getAbout()->get_Async(arg);
                t->then([=](std::unique_ptr<googleQt::about::AboutResource> r)
                {
#ifdef API_QT_AUTOTEST
                    //QString account_email = dbp::configEmailUserId();
                    QString account_email = "";
                    auto res = gui::edit(dbp::configEmailUserId(), "New AUTOTEST account", true);
                    if (res.first) {
                        account_email = res.second;
                    }
#else
                    const about::UserInfo& u = r->user();
                    QString account_email = u.emailaddress();
#endif
                    if (account_email.isEmpty()) {
                        qWarning() << "failed to obtain account email address.";
						ard::messageBox(gui::mainWnd(), "Failed to obtain account email address. Please restart your device and try again.");
                        return;
                    }
                    else{
                        qWarning() << "Created gmail client for account" << account_email;
                    }

                    dbp::configSetEmailUserId(account_email);
                    QString token_file = dbp::configEmailUserTokenFilePath(dbp::configEmailUserId());
                    if (!renameOverFile(tmp_token, token_file)) {
                        qWarning() << "failed to rename tmp token file " << tmp_token << token_file;
						ard::messageBox(gui::mainWnd(), "Failed to rename temporary token file. Please restart your device and try again.");
                        return;
                    }

                    m_gclient->setUserId(account_email);
                    bool ok = setupGmailCache();
                    ASSERT(ok, "failed to setup gmail cache");
                    if (ok) {
                        qWarning() << "Setup GmailCache for" << account_email;
                        QTimer::singleShot(300, [=]() 
                        {
                            ard::asyncExec(AR_rebuildOutline);
                        });
                    }
                },
                    [=](std::unique_ptr<googleQt::GoogleException> ex)
                {
                    ASSERT(0, "ERROR guiInitGClientByAcquiringNewToken/setupGmailCache") << ex->statusCode() << ex->what();
                });
                return true;
            }            
    }

    return false;
}

bool ard::email_model::authAndConnectNewGoogleUser()
{
    disableGoogle();
    return guiInitGClientByAcquiringNewToken();
};

bool ard::email_model::setupGmailCache() 
{
#ifdef API_QT_AUTOTEST
    if (m_gclient) {
        autotest.setClient(m_gclient.get());
    }
#endif
	m_gmail_cache_attached = false;
    QString userId = dbp::configEmailUserId();
    if (userId.isEmpty()) {
        releaseGClient();
        ASSERT(0, "Expected valid userId");
        return false;
    }

    if (m_gclient) {
        googleQt::GmailRoutes* mr = m_gclient->gmail();
        try {
            m_gclient->endpoint()->diagnosticSetRequestContext("connectGoogle");
            auto cache_file = gmailCachePath();
            if (!mr->setupCache(gmailDBFilePath(), ard::getAttachmentDownloadDir(), cache_file))
            {
                QString s = QString("Failed to initialize SQLite cache database: %1 It is recommended to reset cache file, you will have to resync contacts data after cache reset. Please confirm resetting cache file.").arg(gmailDBFilePath());
                if (ard::confirmBox(ard::mainWnd(), s)) {
                    QString cache_bak = gmailDBFilePath() + ".bak";
                    if (!renameOverFile(gmailDBFilePath(), cache_bak)) {
						ard::errorBox(ard::mainWnd(), "Failed to reset gmail cache file, please reboot your computer.");
                    }
                };
                releaseGClient();
            }            
			else
			{
				m_gmail_cache_attached = true;
				ard::trail(QString("setup-gmail-cache-OK [%1]").arg(userId));
				auto d = ard::db();
				if (d && d->isOpen()) {
					d->rmodel()->rroot()->resetRules();
					d->rmodel()->loadBoardRules();
					if (dbp::configFileMaiBoardSchedule())
						d->rmodel()->scheduleBoardRules();
				}
				ard::asyncExec(AR_RebuildLocusTab);
				ard::asyncExec(AR_OnConnectedGmail);
			}
        }
        catch (GoogleException& e)
        {
            qWarning() << "email_model::connectGoogle user=" << m_gclient->userId() << "Exception=" << e.what();
            ard::resolveGoogleException(e);
        }
    }

	if (m_gclient)
	{
		net_traffic_progress = 0.0;
		connect(m_gclient.get(), &googleQt::ApiClient::downloadProgress, this, &ard::email_model::download_progress);
		connect(m_gclient.get(), &googleQt::ApiClient::uploadProgress, this, &ard::email_model::upload_progress);
	}

    return (m_gclient != nullptr);
};

bool ard::email_model::connectGoogle()
{
    if (m_gclient){
        return true;
    }

    if (!ard::isDbConnected()) {
        return false;
    }

#ifndef API_QT_AUTOTEST
    if (!QSslSocket::supportsSsl())
    {
        static bool first_time = true;
        if (first_time) {
            first_time = false;
            qWarning() << "SSL support not enabled: " << QSslSocket::sslLibraryVersionString();
			ard::messageBox(gui::mainWnd(), "SSL support required. GMail module will be disabled. Please reinstall OpenSSL and restart program.");
        }
        return false;
    }
#endif //API_QT_AUTOTEST

#ifdef API_QT_AUTOTEST
    static QString autotest_log = defaultRepositoryPath() + "/log/autotest.log";
    if (!autotest.init(autotest_log)) {
        ASSERT(0, "failed to setup AUTOTEST log");
    }
#endif //API_QT_AUTOTEST

    bool rv = initGClientUsingExistingToken();
    return rv;
};

googleQt::mail_cache::GMailSQLiteStorage* ard::email_model::gstorage()
{
	if (m_gclient) {
		auto st = m_gclient->gmail_storage();
		if (!st->isValid()) {
			qWarning() << "invalid gmail storage requested";
			return nullptr;
		}
		return st;
	}
	return nullptr;
	/*
    googleQt::mail_cache::GMailSQLiteStorage* rv = nullptr;

    googleQt::GmailRoutes* r = ard::gmail();
    if (r && r->cacheRoutes()) {
        rv = r->cacheRoutes()->storage();
    }
    return rv;*/
};

/*
googleQt::GoogleClient* ard::email_model::gclient()
{ 
    if (!m_gclient)
        return nullptr; 
    return m_gclient.get();     
}*/

googleQt::gclient_ptr ard::google()
{
	auto m = gmail_model();

    if (!m)
        return nullptr;

    if (m->m_gclient) {
        return m->m_gclient;
    }

    if (!m->connectGoogle()) {
        return nullptr;
    }

    return m->m_gclient;
};

googleQt::GmailRoutes* ard::gmail()
{
    googleQt::GmailRoutes* rv = nullptr;
    auto gc = ard::google();
    if (gc != nullptr) {
        rv = gc->gmail();
    }
    return rv;
};

googleQt::mail_cache::GMailSQLiteStorage* ard::gstorage()
{
    if (!gmail_model())
        return nullptr;

    return gmail_model()->gstorage();
};

bool ard::isGoogleConnected()
{
    if (!gmail_model())
        return false;

    return gmail_model()->isGoogleConnected();
};

bool ard::isGmailConnected() 
{
	if (!gmail_model())
		return false;

	return gmail_model()->isGmailConnected();
};

bool ard::deleteGoogleCache()
{
    if (!gmail_model())
        return false;

    gmail_model()->disableGoogle();

    bool rv = true;
    QString cache_file = gmailDBFilePath();
    if (QFile::exists(cache_file))
    {
        if (!QFile::remove(cache_file)) {
            qWarning() << "Failed to delete cache file: " << cache_file;
            rv = false;
        }
        else {
            qDebug() << "Cache file deleted" << cache_file;
        }
    }   

    return rv;
};

bool ard::hasGoogleToken() 
{
    bool rv = false;
    auto userid = dbp::configEmailUserId();
    if (!userid.isEmpty()) {
        QString token_file = dbp::configEmailUserTokenFilePath(userid);
        rv = QFile::exists(token_file);
    }
    return rv;
};

std::pair<bool, bool> ard::guiConditionalyCheckAuthorizeGoogle()
{
    std::pair<bool, bool> rv = { true, false };

    if (!ard::isGoogleConnected()) {
        if (!ard::hasGoogleToken()) {
            rv.first = false;
            if (ard::confirmBox(ard::mainWnd(), "Google Access Token is not found. Whould you like to authorize access to Gmail?")) {
                ard::asyncExec(AR_GoogleConditianalyGetNewTokenAndConnect);
                rv.first = true;
            }
            else {
                return rv;
            }
        }
    }

    return rv;
};

bool ard::revokeGoogleToken()
{
    bool rv = true;
    QString token_file = dbp::configEmailUserTokenFilePath(dbp::configEmailUserId());
    if (QFile::exists(token_file))
    {
        if (!QFile::remove(token_file)) {
            qWarning() << "Failed to delete token file: " << token_file;
            rv = false;
        }
    }
    if (gmail_model()) {
        gmail_model()->disableGoogle();
    }
    return rv;
};

bool ard::revokeGoogleTokenWithConfirm()
{
	bool rv = false;

    if (ard::isGoogleConnected()) {
        if (ard::confirmBox(ard::mainWnd(), "Google API is enabled and access token appears to be valid. Press 'Yes' to delete token and disconnect Ardi from Gmail & other Google services."))
            {
                if (!ard::revokeGoogleToken()) {
					ard::errorBox(ard::mainWnd(), "Failed to remove token file. You might need to restart your device.");
                }
                else {
					ard::messageBox(gui::mainWnd(), "Token file deleted, Gmail & Sync will be disabled for Ardi.");
					rv = true;
                    //dbp::configFileSetSyncEnabled(false);
                }
            }
    }
    else{
        ard::revokeGoogleToken();
		rv = true;
    }
	return rv;
};

void ard::email_model::detachModelGui()
{    
	for (size_t i = 0; i < m_wrapped.size(); i++) {
		auto t = m_wrapped[i];
		if (t) { 
			auto o = t->optr();
			if (o)o->setUserData(0);
			t->release(); 
		}
	}
	m_wrapped.clear();
};

void ard::email_model::releaseWrapper(googleQt::mail_cache::thread_ptr o)
{
	auto d = o->userData();
	if (d != 0) {
		o->setUserData(0);
		if (d < m_wrapped.size()) {
			auto t = m_wrapped[d];
			if (t) {
				t->release();
				m_wrapped[d] = nullptr;
			}
		}
	}
};

void ard::email_model::releaseGClient() 
{
    if (m_gclient) 
	{
		//qWarning() << "request g-client release" << m_gclient.get() << m_gclient.use_count();
        QObject::connect(m_gclient.get(), &QObject::destroyed, [](QObject* o) {
            qWarning() << "destroyed g-client" << o;
        });
		if (m_gclient->isQueryInProgress()) 
		{
			ard::trail(QString("schedule-gmail-dispose [0x%1][%2]").arg((quintptr)m_gclient.get()).arg(m_gclient.use_count()));
			//qWarning() << "schedule client (in-progress) to destroy" << m_gclient.get() << m_gclient.use_count();
			m_gclient->cancelAllRequests();
			m_clients2release.push_back(m_gclient);
			if (!m_clients_release_timer.isActive()) {
				m_clients_release_timer.start();
			}
		}
		else 
		{
			ard::trail(QString("gmail-dispose [0x%1][%2]").arg((quintptr)m_gclient.get()).arg(m_gclient.use_count()));
			googleQt::releaseClient(m_gclient);
		}
        m_gclient = nullptr;
		m_gmail_cache_attached = false;
    }
};


ethread_ptr ard::email_model::lookupFirstWrapperOrWrapGthread(googleQt::mail_cache::thread_ptr& o)
{
	auto d = o->userData();
	if(d > 0 && d < m_wrapped.size())
    {
		auto t = m_wrapped[d];
		if (t) {
			ASSERT_VALID(t);
			if (!t->optr()) {
				ASSERT(0, "expected valid wrapper object");
				return nullptr;
			}
			if (t->optr() != o) {
				ASSERT(0, "inconsistant wrapper object link") << o->id() << t->optr()->id() << o.get() << t->optr().get();
				return nullptr;
			}
			if (!t->headMsg()) {
				ard::trail(QString("no-head-thread/wrap-reset [%1][%2][%3][%4][%5]").arg(o->id()).arg(o->messages().size()).arg(o->head() == nullptr ? "N" : "Y").arg(d).arg(m_wrapped.size()));
				t->resetThread();
				if (!t->headMsg()) 
				{
					ard::error(QString("no-head-thread/wrap-reset [%1][%2][%3]").arg(o->id()).arg(o->messages().size()).arg(o->head() == nullptr ? "N" : "Y"));
					return nullptr;
				}
			}
			if (o->head() != t->headMsg()->optr()) {
				ard::trail(QString("reset-thread-on-head/wrep [%1][%2][%3][%4]").arg(o->id()).arg(t->title().left(20)).arg(d).arg(m_wrapped.size()));
				t->resetThread();
			}
			return t;
		}
    }

	auto t = new ard::ethread(o);
	doRegister(t);
    return t;
};

void ard::email_model::doRegister(ethread_ptr t) 
{
	auto o = t->optr();
	assert_return_void(t, "expected thread wrapper");
	assert_return_void(o, "expected thread underlying");

	auto sz = m_wrapped.size();
	if (sz == 0) {
		m_wrapped.push_back(nullptr);		
		sz++;
	}
	m_wrapped.push_back(t);		
	o->setUserData(sz);
};

std::vector<email_ptr> ard::email_model::lookupOrWrapGthreadMessages(std::unordered_map<QString, std::vector<QString>> m1)
{
    std::vector<email_ptr> rv;

    auto gc = ard::google();
    if (!gc)
        return rv;

    if (!ard::gmail())
        return rv;

    if (!ard::gmail()->cacheRoutes())
        return rv;

    auto storage = ard::gstorage();
    if (!storage) {
        return rv;
    }
    
    std::vector<QString> thread_ids2block_select;
    for (auto& i : m1) {
        thread_ids2block_select.push_back(i.first);
    }
    auto arr = storage->loadThreadsByIdsFromDb(thread_ids2block_select);
    if (!arr.empty()) {
        for (auto i : arr) 
        {           
            auto j = m1.find(i->id());
            if (j == m1.end()) {
                ASSERT(0, "failed to locate thread_id in provided map");
                continue;
            }

            auto ath = lookupFirstWrapperOrWrapGthread(i);
            if (ath) {
                for (auto k : j->second)
                {
                    auto gmsg = i->findMessage(k);
                    if (!gmsg) {
                        ASSERT(0, "failed to locate msgid in gthread");
                        continue;
                    }
                    auto e = ath->lookupMessage(gmsg);
                    ASSERT(e, "failed to locate msg in thread wrapper") << gmsg->id();
                    rv.push_back(e);
                }
            }
        }
    }

    return rv;
};

void ard::email_model::attachAdoptedThread(ethread_ptr t, googleQt::mail_cache::thread_ptr d)
{
    assert_return_void(t, "expected thread wrapper");
    assert_return_void(t->parent(), "expected parent of thread wrapper");
    assert_return_void(d, "expected thread");
    assert_return_void(d->head(), "expected thread head msg");
    assert_return_void(!t->optr(), "expected unattached thread ptr");

    t->m_o = d;
    t->resetThread();
    registerAdopted(t);
};

topic_ptr ard::email_model::adoptThread(ethread_ptr t)
{
	if (t->parent()) {
		ASSERT(0, "Thread has parent already");
		return nullptr;
	}

	if (!t->optr()) {
		ASSERT(0, "Expected linked thread wrapper") << t->dbgHint();
		return nullptr;
	}

	if (!t->headMsg()) {
		ASSERT(0, "Thread has no head message");
		return nullptr;
	}

	auto r = ard::db()->threads_root();
	if (r) {
		if (r->addItem(t)) {
			if (!t->getThreadExt()) {
				t->ensureThreadExt();
			}
		}
		registerAdopted(t);
		LOCK(t);//ykh!1 ?
		return r;
	}
	return nullptr;
};

void ard::email_model::registerAdopted(ethread_ptr t)
{
    auto o = t->optr();
    assert_return_void(t, "expected thread wrapper");
    assert_return_void(o, "expected thread underlying");
    assert_return_void(t->parent(), "expected parent of thread wrapper");


	auto d = o->userData();
	if (d == 0 || d >= m_wrapped.size() || m_wrapped[d] == nullptr) {
		doRegister(t);
		LOCK(t);
	}
};


THREAD_VEC ard::email_model::linkThreadWrappers(THREAD_VEC& th_vec)
{
    THREAD_VEC thr_unresolvable;

    if (th_vec.empty())
        return thr_unresolvable;

    auto gc = ard::google();
    if (!gc)
        return thr_unresolvable;

    if (!ard::gmail())
        return thr_unresolvable;

    if (!ard::gmail()->cacheRoutes())
        return thr_unresolvable;

    auto storage = ard::gstorage();
    if (!storage) {
        return thr_unresolvable;
    }

    ///...
	S2TREADS			threads2block_resolve;
    STRING_SET          resolved_in_block;
    std::vector<QString> thread_ids2block_select;
    for (auto& t : th_vec) {
        auto e = t->getThreadExt();
        if (e) {
            auto thread_id = e->threadId();

            auto i = threads2block_resolve.find(thread_id);
            if (i != threads2block_resolve.end()) {
                THREAD_VEC& arr = i->second;
                arr.push_back(t);
                //continue;
            }
            else {
                THREAD_VEC arr;
                arr.push_back(t);
                threads2block_resolve[thread_id] = arr;
                thread_ids2block_select.push_back(thread_id);
                ASSERT(t->parent(), "expected thread with valid parent") << thread_id << t->dbgHint();
            }
        }
    }

    std::function<void()> loadThreadsFromCacheDb = [&]()
    {
        auto arr = storage->loadThreadsByIdsFromDb(thread_ids2block_select);
        if (!arr.empty()) {
            for (auto i : arr) {
                auto thread_id = i->id();
                auto jt = threads2block_resolve.find(thread_id);
                if (jt != threads2block_resolve.end()) {
                    resolved_in_block.insert(thread_id);
                    auto& arr = jt->second;
                    for (auto th : arr) {
                        attachAdoptedThread(th, i);
                    }
                }
                else {
                    qWarning() << "resolveAdoptedThreads/lookup-err" << thread_id;
                }
            }
        }
    };

    CRUN(loadThreadsFromCacheDb);
	ard::trail(QString("resolved-threads %1 of %2").arg(resolved_in_block.size()).arg(thread_ids2block_select.size()));
    //qWarning() << "db-resolved" << resolved_in_block.size() << "threads of" << thread_ids2block_select.size();

    gc->endpoint()->diagnosticSetRequestContext("linkThreadWrappers");

    std::vector<googleQt::HistId> id_list2load_from_cache;      
    for (auto& t : th_vec) {
        auto ex = t->getThreadExt();
        if (ex) {
            auto thread_id = ex->threadId();
            auto k = resolved_in_block.find(thread_id);
            if (k != resolved_in_block.end()) {
                continue;
            }
            //qWarning() << "non-resolved-thread syid=" << t->syid() << "oid=" << t->id() << "tid=" << ex->threadId();
            qWarning() << QString("scheduled2resolve TID=%1 DBID=%2 SYID=%3").arg(ex->threadId()).arg(t->id()).arg(t->syid());

            auto i = m_threads2resolve.find(thread_id);
            if (i != m_threads2resolve.end()) {
                THREAD_VEC& arr = i->second;
                arr.push_back(t);
            }
            else {
                THREAD_VEC arr;
                arr.push_back(t);
                m_threads2resolve[thread_id] = arr;
                ASSERT(t->parent(), "expected thread with valid parent") << thread_id << t->dbgHint();

                googleQt::HistId h;
                h.id = thread_id;
                h.hid = 0;
                id_list2load_from_cache.push_back(h);
            }
        }
        else {
            thr_unresolvable.push_back(t);
            qWarning() << "thread without extension" << t->dbgHint() << "scheduled to delete";
        }
    }

    
    if (!id_list2load_from_cache.empty()) {
        asyncQueryCloudAndLinkThreadWrappers(id_list2load_from_cache);
    }
 

    return thr_unresolvable;
};

void ard::email_model::asyncQueryCloudAndLinkThreadWrappers(const std::vector<googleQt::HistId>& id_list2load) 
{ 
    ard::gmail()->cacheRoutes()->getCacheThreadList_Async(id_list2load)->then
    (
        [=](mail_cache::tdata_result lst)
    {
        auto db_connected = ard::isDbConnected();
        if (!db_connected) {
            qWarning() << "linkThreadWrappers exit as DB is not connected";
            return;
        }

        for (auto& i : lst->result_list) {
            auto thread_id = i->id();
            auto jt = m_threads2resolve.find(thread_id);
            if (jt != m_threads2resolve.end()) {
                auto& arr = jt->second;
                for (auto th : arr) {
                    attachAdoptedThread(th, i);
                }
                m_threads2resolve.erase(jt);
            }
            else {
                qWarning() << "resolveAdoptedThreads/lookup-err" << thread_id;
            }
        }

        if (!lst->result_list.empty()) 
        {
            ard::asyncExec(AR_RebuildGuiOnResolvedCloudCache);
        }
    },
        [](std::unique_ptr<GoogleException> ex)
    {
        ASSERT(0, "Failed to resolve emails") << ex->what();
    });
};
void ard::email_model::fire_all_attachments_downloaded(ard::email* e) 
{
	all_attachments_downloaded(e);
};

void ard::email_model::schedule_draft(ard::email_draft* d)
{
	qDebug() << "ykh/schedule_draft" << d->title();

	auto gc = ard::google();
	assert_return_void(gc, "expected gclient");
	assert_return_void(d, "expected draft");
	auto i = m_scheduled_drafts.find(d);
	assert_return_void((i == m_scheduled_drafts.end()), QString("draft already scheduled").arg(d->title()));
	auto de = d->draftExt();
	assert_return_void(de, "expected draft extension");
	if (de->are_all_attachements_locally_resolved()) {
		d->send_draft(gc);
		return;
	}

	if (d->attachements_forward_origin) {
		d->attachements_forward_origin->download_all_attachments();
	}
	else {
		qWarning() << "forward origin is not defined";
		return;
	}

	draft_schedule_point p;
	p.gclient = gc;
	p.attachements_forward_origin = d->attachements_forward_origin;
	m_scheduled_drafts[d] = p;
	LOCK(d);
	if (!m_drafts_scheduler_timer.isActive()) {
		m_drafts_scheduler_timer.start(2000);
	}
};

void ard::email_model::markUnread(const ESET& lst, bool set_it) 
{
	assert_return_void(ard::gmail(), "expected gmail");
	auto cache_route = ard::gmail()->cacheRoutes();
	assert_return_void(cache_route, "expected gmail cache");
	std::vector<googleQt::mail_cache::MessageData*> mlst;
	for (auto& i : lst){
		auto m = i->optr();
		if (m) {
			mlst.push_back(m.get());
		}
	}

	auto t = cache_route->setLabel_Async(googleQt::mail_cache::sysLabelId(googleQt::mail_cache::SysLabel::UNREAD), 
		mlst, 
		set_it, 
		true);

	t->then([]()
	{
		ard::asyncExec(AR_RebuildLocusTab);
	}, 
	[](std::unique_ptr<GoogleException> ex) 
	{
		ASSERT(0, "Failed to toggle 'Unread'") << ex->what();
	});
};


void ard::email_model::markTrashed(const ESET& lst) 
{
	assert_return_void(ard::gmail(), "expected gmail");
	auto cache_route = ard::gmail()->cacheRoutes();
	assert_return_void(cache_route, "expected gmail cache");
	std::vector<googleQt::mail_cache::MessageData*> mlst;
	std::vector<QString> log_ids;
	for (auto& i : lst) {
		auto m = i->optr();
		if (m) {
			mlst.push_back(m.get());
			log_ids.push_back(m->id());
		}
	}

	auto t = cache_route->setLabel_Async(googleQt::mail_cache::sysLabelId(googleQt::mail_cache::SysLabel::TRASH),
		mlst,
		true,
		true);

	t->then([log_ids]()
	{
		ard::asyncExec(AR_RebuildLocusTab);
		for (auto& i : log_ids){
			ard::trail(QString("email-set-TRASH %1").arg(i));
		}
	},
		[log_ids](std::unique_ptr<GoogleException> ex)
	{
		ASSERT(0, "Failed to set 'Trash'") << ex->what();
		for (auto& i : log_ids) {
			ard::trail(QString("ERROR-email-set-TRASH %1").arg(i));
		}
	});
};


void ard::email_model::trashThreadsSilently(const THREAD_SET& lst)
{
	auto rm = ard::db()->rmodel();
	assert_return_void(rm, "expected R-model");
	auto gm = ard::gmail_model();
	if (!gm) {
		ASSERT(0, "expected gmail model");
		return;
	}

	GTHREADS gthreads;
	ESET all_emails;
	for (auto& i : lst) 
	{
		auto h = i->headMsg();
		if (!h)continue;
		auto o = i->optr();
		if (!o)continue;
		ard::trail(QString("del-ethread %1/%2[%3]").arg(o->id()).arg(h->optr()->id()).arg(h->title().left(32)));
		gthreads.insert(o.get());
		all_emails.insert(h);
		auto& lst = i->items();
		for (auto& j : lst) {
			auto e = dynamic_cast<ard::email*>(j);
			if (e) {
				all_emails.insert(e);
			}
		}
	}
	markTrashed(all_emails);

	//for (auto& i : all_emails)ard::close_popup(i);	
	rm->unregisterObserved(gthreads);
	for (auto& i : lst) {
		auto o = i->optr();
		if (o) {
			gm->releaseWrapper(o);
		}

		/*auto p = i->parent();
		if (p) {
			p->remove_cit(i, false);
			i->release();
		}
		else {
			auto o = i->optr();
			if (o) {
				gm->releaseWrapper(o);
			}
		}*/
	}
};

void onArdiProgress(qint64 bytesProcessed, qint64 total)
{
	auto new_progress = net_traffic_progress;
	if (total > 0)
	{
		if (bytesProcessed == total) {
			new_progress = 0;
		}
		else {
			new_progress = (double)bytesProcessed / total;
		}
	}
	else
	{
		new_progress = 0.0;
	}
	if (new_progress != net_traffic_progress) {
		net_traffic_progress = new_progress;
		update_net_traffic_progress();
	}
};


void ard::email_model::download_progress(qint64 bytesProcessed, qint64 total)
{
	onArdiProgress(bytesProcessed, total);
	//CHECK_PROGRESS;
//	qDebug() << "gmail-download-progress" << bytesProcessed << total << net_traffic_progress;
};

void ard::email_model::upload_progress(qint64 bytesProcessed, qint64 total)
{
	onArdiProgress(bytesProcessed, total);

	//CHECK_PROGRESS;
//	qDebug() << "gmail-upload-progress" << bytesProcessed << total << net_traffic_progress;
};

#ifdef _DEBUG
void ard::email_model::debugFunction() 
{
	auto f = new ard::topic;
	qDebug() << f->title();
	ethread_ptr t = new ard::ethread(nullptr);
	qDebug() << t->title();
};
#endif

ard::email_attachment::email_attachment(googleQt::mail_cache::att_ptr att) :m_att(att)
{
    m_title = QString("(%1) %2")
        .arg(googleQt::size_human(static_cast<float>(m_att->size())))
        .arg(m_att->filename());
}

QPixmap ard::email_attachment::getIcon(OutlineContext)const
{
    if (m_att) {
        auto status = m_att->status();
        switch (status)
            {
            case googleQt::mail_cache::AttachmentData::statusDownloaded:
            {
                if (m_local_file_icon.isNull()) {
                    auto storage = ard::gstorage();
                    if (storage) {
                        QString filePath = storage->findAttachmentFile(m_att);
                        if (!filePath.isEmpty()) {
                            QFileInfo fi(filePath);
                            QFileIconProvider p;
                            QIcon ic = p.icon(fi);
                            if (ic.isNull()) {
                                ASSERT(0, "failed to load file icon") << filePath;
                            }
                            else {
                                QSize sz(48, 48);
                                m_local_file_icon = ic.pixmap(sz);
                            }
                        }
                    }
                }

                if(!m_local_file_icon.isNull()) {
                    return m_local_file_icon;
                }

                return getIcon_AttachmentFile();
            }break;
            case googleQt::mail_cache::AttachmentData::statusDownloadInProcess:
                {
                    return getIcon_InProgress();
                }break;
            case googleQt::mail_cache::AttachmentData::statusNotDownloaded:
                break;
            }

    }
    return QPixmap();
};

QPixmap ard::email_attachment::getSecondaryIcon(OutlineContext)const
{
    return QPixmap();
};

bool ard::email_attachment::hasCurrentCommand(ECurrentCommand c)const
{
    bool rv = false;
    if (m_att) {
        switch (c)
        {
        case ECurrentCommand::cmdDownload:
            rv = (m_att->status() == googleQt::mail_cache::AttachmentData::statusNotDownloaded);
            break;
        case ECurrentCommand::cmdOpen:
        case ECurrentCommand::cmdFindInShell:
            rv = (m_att->status() == googleQt::mail_cache::AttachmentData::statusDownloaded);
            break;
        default:
            break;
        }
    }
    return rv;
};

/**
    email_account_info
*/
ard::email_account_info::email_account_info(googleQt::mail_cache::acc_ptr acc)
{ 
    m_acc = std::move(acc);
    m_title = m_acc->userId();
    updateAttr();
}

bool ard::email_account_info::hasCurrentCommand(ECurrentCommand c)const
{
    if(m_isCurrent)
        return false;
    return (c == ECurrentCommand::cmdEdit || c == ECurrentCommand::cmdSelect);
};

void ard::email_account_info::updateAttr()
{
    m_isCurrent = !m_title.isEmpty() && (m_title.compare(dbp::configEmailUserId(), Qt::CaseInsensitive) == 0);
   // m_font_prop.setBold(m_isCurrent);
};

int ard::email_account_info::accountId()const
{
    int rv = -1;
    if (m_acc) {
        rv = m_acc->accountId();
    }
    return rv;
};

QPixmap ard::email_account_info::getIcon(OutlineContext)const
{
    if (m_isCurrent) {
        return getIcon_GreenCheck();
    }

    return QPixmap();
}

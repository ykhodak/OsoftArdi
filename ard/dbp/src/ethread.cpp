#include "a-db-utils.h"
#include "ethread.h"
#include "email.h"
#include "GoogleClient.h"
#include "gdrive-syncpoint.h"
#include "ansearch.h"
#include "rule.h"
#include "rule_runner.h"

using namespace googleQt;

#ifdef API_QT_AUTOTEST
int g_wrapper__thread_alloc_counter = 0;
#endif

IMPLEMENT_ALLOC_POOL(ethread);


ard::ethread::ethread( googleQt::mail_cache::thread_ptr d):m_o(d)
{
    if (d) {
        resetThread();
    }   
#ifdef API_QT_AUTOTEST
    g_wrapper__thread_alloc_counter++;
#endif
};


ard::ethread::ethread()
{
#ifdef API_QT_AUTOTEST
    g_wrapper__thread_alloc_counter++;
#endif
};

ard::ethread::~ethread() {
    clear_thread();
#ifdef API_QT_AUTOTEST
    g_wrapper__thread_alloc_counter--;
#endif
}

bool ard::ethread::isValid()const
{
	return (m_o != nullptr && m_headMsg1 != nullptr);
};

void ard::ethread::clear_thread()
{
    if (m_headMsg1) {
        m_headMsg1->detachOwner();
        m_headMsg1->release();
        m_headMsg1 = nullptr;
    }
};

email_ptr ard::ethread::headMsg()
{
    return m_headMsg1;
};

email_cptr ard::ethread::headMsg()const
{
    return m_headMsg1;
};


googleQt::mail_cache::thread_ptr ard::ethread::optr()
{
    return m_o;
};


QString ard::ethread::threadId()const
{
    if (m_o)
    {
        return m_o->id();
    }
    return "";
};

void ard::ethread::clear()
{
    ard::topic::clear();
    clear_thread();
}

cit_prim_ptr ard::ethread::create()const
{
    return new ard::ethread();
};

topic_ptr ard::ethread::produceLabelSubject()
{
    auto h = headMsg();
    if (!h) {
        return nullptr;
    }
    return this;
};

void ard::ethread::resetThread()
{
    clear_thread();
    forceOutlineHeightRecalc();
    m_headMsg1 = nullptr;   
    if (m_o && m_o->head()) {
        auto h = m_o->head();
        m_headMsg1 = new email(h);
        if (m_headMsg1) {
            m_headMsg1->attachOwner(this);
            auto s = m_headMsg1->plainFrom();
            if (!s.isEmpty()) {
				//qDebug() << "at-t" << m_o->id() << s;

                auto r = color::calcColorHashIndex(s);
                m_attr.colorHashIndex = r.first;
                m_attr.colorHashChar = r.second;
            }
            for (auto& i : m_o->messages()) {
                if (i != m_headMsg1->optr()) {
                    auto m2 = new email(i);
                    if (m2) {
                        addItem(m2);
                    }
                }
            }//for
        }
    }
};

email_ptr ard::ethread::lookupMessage(googleQt::mail_cache::msg_ptr d) 
{
    if (m_headMsg1->optr() == d) {
        return m_headMsg1;
    }

    for (auto i : m_items) {
        ard::email* e = dynamic_cast<ard::email*>(i);
        if (e) {
            if (e->optr() == d) {
                return e;
            }
        }
    }

    return nullptr;
};

QString ard::ethread::title()const
{
    QString rv;
    auto h = headMsg();
    if (h) {
        ASSERT_VALID(h);
        rv = h->title();
    }
    return rv;
};

QString ard::ethread::objName()const
{
    return "thread";
};

QString ard::ethread::wrappedId()const
{
    QString rv;
    if (m_o)
    {
        rv = m_o->id();
    }
    return rv;
};

bool ard::ethread::injectInOutlineSpace()
{
    if (!optr()) {
        ASSERT(0, "Expected linked thread wrapper") << dbgHint();
        return false;
    }

    auto p = parent();
    if (!p) {
        bool rv = false;
        auto gm = ard::gmail_model();
        if (gm) {
            auto p = gm->adoptThread(this);
            if (p) {
                rv = p->ensurePersistant(-1);
            }
        }
        return rv;
    }
    else {
        ensurePersistant(-1);
    }

    return true;
};

topic_ptr ard::ethread::prepareInjectIntoBBoard()
{
    if (!injectInOutlineSpace()) {
        ASSERT(0, QString("failed to insert in outline space to setup bboard item '%1'").arg(title()));
        return nullptr;
    }

    return ard::topic::prepareInjectIntoBBoard();
};

void ard::ethread::setToDo(int done_percent, ToDoPriority prio)
{
    if (!injectInOutlineSpace()) {
        ASSERT(0, QString("failed to insert in outline space to setup todo '%1'").arg(title()));
        return;
    }

    ard::topic::setToDo(done_percent, prio);
};

void ard::ethread::setColorIndex(EColor c)
{
    if (!injectInOutlineSpace()) {
        ASSERT(0, QString("failed to insert in outline space to setup todo '%1'").arg(title()));
        return;
    }

    ard::topic::setColorIndex(c);
};

void ard::ethread::setAnnotation(QString s, bool gui_update)
{
    if (!injectInOutlineSpace()) {
        ASSERT(0, QString("failed to insert in outline space to setup todo '%1'").arg(title()));
        return;
    }

    ard::topic::setAnnotation(s, gui_update);
};

#define IMPL_BFUN_FROM_HEADER_MSG(N) bool ard::ethread::N()const{\
    bool rv = false;\
    auto h = headMsg();\
    if (h) {\
        rv = h->N();\
    }\
    return rv;\
};\


IMPL_BFUN_FROM_HEADER_MSG(hasCustomTitleFormatting);
//IMPL_BFUN_FROM_HEADER_MSG(isStarred);
//IMPL_BFUN_FROM_HEADER_MSG(isImportant);
//IMPL_BFUN_FROM_HEADER_MSG(hasPromotionLabel);
//IMPL_BFUN_FROM_HEADER_MSG(hasSocialLabel);
//IMPL_BFUN_FROM_HEADER_MSG(hasUpdateLabel);
//IMPL_BFUN_FROM_HEADER_MSG(hasForumsLabel);
IMPL_BFUN_FROM_HEADER_MSG(hasAttachment);
IMPL_BFUN_FROM_HEADER_MSG(canHaveCard);

QString ard::ethread::dateColumnLabel()const
{
    auto h = headMsg();
    QString rv = "";
    if (h) {
        rv = h->dateColumnLabel();
    }
    return rv;
};

qlonglong ard::ethread::internalDate()const
{
    qlonglong rv = 0;
    if (m_o)
    {
        rv = m_o->internalDate();
    }
    return rv;
};

ENoteView ard::ethread::noteViewType()const
{
    ENoteView rv = ENoteView::None;
    auto h = headMsg();
    if (h) {
        rv = h->noteViewType();
    }
    return rv;
};

QString ard::ethread::impliedTitle()const 
{
    if (m_o)
    {
        return ard::topic::impliedTitle();
    }

    QString rv = "";
    if (m_EthreadExt) {
        rv = QString("email - waiting for data, unresolved ref: %1").arg(m_EthreadExt->threadId());
    }
    else {
        rv = QString("email - invalid DB ref: %1").arg(id());
    }
    return rv;
};

topic_ptr ard::ethread::popupTopic()
{
    auto h = headMsg();
    return h;
};

ard::note_ext* ard::ethread::mainNote()
{
    auto h = headMsg();
    ard::note_ext* r = nullptr;
    if (h) {
        r = h->mainNote();
    }
    return r;
};

const ard::note_ext* ard::ethread::mainNote()const 
{
    ard::ethread* ThisNonConst = (ard::ethread*)this;
    return ThisNonConst->mainNote();
};

topic_ptr ard::ethread::tspaceTopic()
{
    auto h = headMsg();
    if (h) {
        return h;
    }
    return nullptr;
};

void ard::ethread::prepareCustomTitleFormatting(const QFont& f, QList<QTextLayout::FormatRange>& r)
{
    auto h = headMsg();
    if (h) {
        h->prepareCustomTitleFormatting(f, r);
    }
};

/*
bool ard::ethread::hasLabel(uint64_t filter)const
{
    bool rv = true;
    if (m_o)
        {
            rv = m_o->hasLabel(filter);
        }
    return rv;
};

bool ard::ethread::hasLimboLabel(uint64_t filter)const
{
    bool rv = false;
    if (m_o)
    {
        rv = m_o->hasLimboLabel(filter);
    }
    return rv;
};

bool ard::ethread::hasAllLabels(uint64_t filter)const
{
    bool rv = true;
    if (m_o)
        {
            rv = m_o->hasAllLabels(filter);
        }
    return rv;    
}*/

std::vector<mail_cache::label_ptr> ard::ethread::getLabels()const
{
    auto gm = ard::gmail();
    if (m_o && gm && gm->cacheRoutes())
    {
        return gm->cacheRoutes()->getThreadLabels(m_o.get());
    }

    std::vector<mail_cache::label_ptr> on_error;
    return on_error;
};

bool ard::ethread::canAcceptChild(cit_cptr it)const
{
    auto f = dynamic_cast<const ard::email*>(it);
    if (!f) {
        return false;
    }

    return ard::topic::canAcceptChild(it);
};

bool ard::ethread::isExpandedInList()const
{
    return isExpanded();
};

bool ard::ethread::hasColorByHashIndex()const 
{
    bool rv = false;
    auto h = headMsg();
    if (h) {
        rv = true;
    }
    return rv;
};

ard::ethread_ext* ard::ethread::ensureThreadExt()
{
    bool initExtenion = false;
    if (!m_EthreadExt) {
        if (!optr()) {
            ASSERT(0, "Expected linked thread wrapper") << dbgHint();
            return nullptr;
        }
        initExtenion = true;
    }

    ensureExtension(m_EthreadExt);
    if (initExtenion) {
        if (m_EthreadExt) {
            m_EthreadExt->m_thread_id = optr()->id();
            m_EthreadExt->m_account_email = dbp::configEmailUserId();
        }
    }
    return m_EthreadExt;
};

void ard::ethread::mapExtensionVar(cit_extension_ptr e)
{
    ard::topic::mapExtensionVar(e);
    if (e->ext_type() == snc::EOBJ_EXT::extThreadOfEmails) {
        ASSERT(!m_EthreadExt, "duplicate thread ext");
        m_EthreadExt = dynamic_cast<ethread_ext*>(e);
        ASSERT(m_EthreadExt, "expected thread ext");
    }
};



topic_ptr ard::ethread::produceMoveSource(ard::email_model* m) 
{
	auto p = parent();
	if (!p)
	{
		if (m) 
		{
			p = m->adoptThread(this);
			assert_return_null(p, "failed to adopt thread");
		}
	}
	return this;
	//auto r = m->a
};

/**
    thread is collection of messages
    1.invoke killSilently on each message including HEAD message
    2.
bool ard::ethread::killSilently(bool )
{
    ASSERT_VALID(this);
	dbp::removeTopics(this, items2delete);
	for (auto& t : items2delete) {
		auto p = t->parent();
		if (p) {
			p->remove_cit(t, false);
			t->release();
		}
	}
	return true;
};*/

bool ard::ethread::isAtomicIdenticalPropTo(const cit_primitive* _other, int& iflags)const
{
    assert_return_false(_other != nullptr, "expected item");
    auto other = dynamic_cast<const ard::ethread*>(_other);
    assert_return_false(other != nullptr, QString("expected item %1").arg(_other->dbgHint()));

    /// don't compare anything else if it's not a synchronizable
    if (!isSynchronizable())
        return true;

    COMPARE_ATTR(otype(), iflags, flOtype);
    COMPARE_ATTR(m_attr.Color, iflags, flFont);
    COMPARE_ATTR(m_annotation, iflags, flAnnotation);
    COMPARE_ATTR(m_attr.ToDo, iflags, flToDo);
    COMPARE_ATTR(m_attr.ToDoIsDone, iflags, flToDoDone);
    COMPARE_ATTR(m_attr.isRetired, iflags, flRetired);
    return true;
};

bool ard::ethread::isAdoptedInOutline()const
{
    bool rv = false;
    auto p = parent();
    if (p) {
        auto r = ard::db()->threads_root();
        if (r && p != r) {
            rv = true;
        }
    }
    return rv;
};

/**
    ethread_ext
*/
ard::ethread_ext::ethread_ext()
{

};

ard::ethread_ext::ethread_ext(ethread_ptr _owner, QSqlQuery& q)
{
    attachOwner(_owner);
    m_account_email = q.value(1).toString();
    m_thread_id = q.value(2).toString();
    m_mod_counter = q.value(3).toInt();
    _owner->addExtension(this);
    //_owner->ensureThreadExt(this);
};

ard::ethread_ext::~ethread_ext()
{

};

bool ard::ethread_ext::isAtomicIdenticalTo(const cit_primitive* _other, int&)const
{
    assert_return_false(_other, "expected item [1]");
    const ethread_ext* other = dynamic_cast<const ethread_ext*>(_other);
    assert_return_false(other, "expected item [2]");

    COMPARE_EXT_ATTR(accountEmail());
    COMPARE_EXT_ATTR(threadId());
    return true;
};

void ard::ethread_ext::assignSyncAtomicContent(const cit_primitive* _other)
{
    assert_return_void(_other, QString("expected ethread obj "));
    const ethread_ext* other = dynamic_cast<const ethread_ext*>(_other);
    assert_return_void(other, QString("expected prj root %1").arg(_other->dbgHint()));
    m_account_email = other->accountEmail();
    m_thread_id = other->threadId();
    ask4persistance(np_ATOMIC_CONTENT);
};

snc::cit_primitive* ard::ethread_ext::create()const
{
    return new ethread_ext();
};

QString ard::ethread_ext::calcContentHashString()const
{
    QString tmp = QString("%1 %2").arg(m_account_email).arg(m_thread_id);
    QString rv = QCryptographicHash::hash((tmp.toUtf8()), QCryptographicHash::Md5).toHex();
    return rv;
};

uint64_t ard::ethread_ext::contentSize()const
{
    uint64_t rv = m_account_email.size() + m_thread_id.size();
    return rv;
};


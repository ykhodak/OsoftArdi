#include <QCryptographicHash>
#include "blinks.h"
#include "snc-cdb.h"

ard::board_link::board_link()
{
	m_lflags.flag = 0;
    m_link_status = LinkStatus::normal;
};

ard::board_link::board_link(QString origin, QString target, int link_pindex, QString link_label)
    :m_origin(origin), m_target(target),
    m_link_label(link_label),
    m_link_status(LinkStatus::created)
{
	m_lflags.flag = 0;
	m_lflags.link_pindex = link_pindex;
};

ard::board_link::board_link(QSqlQuery* q)
{
    m_linkid = q->value(0).toInt();
    m_origin = q->value(2).toString();
    m_target = q->value(3).toString();
	m_link_syid = q->value(4).toString();
	m_lflags.link_pindex = q->value(5).toInt();
    m_link_label = q->value(6).toString();
    m_mod_counter = q->value(7).value<snc::COUNTER_TYPE>();
    m_link_status = LinkStatus::normal;
};

void ard::board_link::setLinkStatus(LinkStatus s)
{
    m_link_status = s;
}

void ard::board_link::setMdc(snc::COUNTER_TYPE v)
{
    m_mod_counter = v;
	if (m_link_status != LinkStatus::created) {
		setLinkStatus(LinkStatus::updated);
	}
};

void ard::board_link::setLinkPindex(int index)
{
	m_lflags.link_pindex = index;
	setLinkStatus(LinkStatus::updated);
};

void ard::board_link::setLinkLabel(QString s, snc::cdb* c)
{
    if (c) {
        m_mod_counter = c->db_mod_counter() + 1;
    }
	//qDebug() << "ykh-setLinkLabel" << s << "syid=" << m_link_syid << "mod_counter=" << m_mod_counter;
    //else {
    //  ASSERT(0, "expected syncDB for set-modified") << dbgHint();
    //}
    m_link_label = s;
	setLinkStatus(LinkStatus::updated);
};

void ard::board_link::markDeletedInSync() 
{
	setLinkStatus(LinkStatus::deleted_in_sync);
};

QString ard::board_link::dbgHint(QString)const
{
    QString rv = QString("blink:%1:%2:%3").arg(m_origin).arg(m_target).arg(m_lflags.link_pindex);
    return rv;
};

void ard::board_link::setupLinkIdFromDb(DB_ID_TYPE v)
{
    m_linkid = v;
};

ard::board_link* ard::board_link::cloneInSync(snc::COUNTER_TYPE mdc)const
{
    ard::board_link* lnk = new ard::board_link(m_origin, m_target, m_lflags.link_pindex, m_link_label);
    lnk->m_link_syid = m_link_syid;
    lnk->m_mod_counter = mdc;
	lnk->m_lflags.sync_processed_flag = 1;
    return lnk;
};

void ard::board_link::assignContentInSync(ard::board_link* lnk, snc::COUNTER_TYPE mdc)
{
    m_origin = lnk->origin();
    m_target = lnk->target();
	m_lflags.link_pindex = lnk->linkPindex();
    m_link_label = lnk->linkLabel();
	setLinkStatus(LinkStatus::updated);
	m_mod_counter = mdc;
	m_lflags.sync_processed_flag = 1;
};

void ard::board_link::prepare_link_sync()
{
	m_lflags.sync_processed_flag = 0;
};

QString	ard::board_link::toHashStr()const
{
	QString s = QString("%1%2%3%4").arg(m_origin).arg(m_target).arg(m_link_syid).arg(m_link_label);
	QString rv = QCryptographicHash::hash((s.toUtf8()), QCryptographicHash::Md5).toHex();
	return rv;
};

snc::LinkSyncInfo ard::board_link::toSyncInfo()const
{
	snc::LinkSyncInfo rv;
	rv.origin_syid = m_origin;
	rv.target_syid = m_target;
	rv.link_syid = m_link_syid;
	return rv;
};

/**
board_link_list
*/
ard::board_link_list::board_link_list()
{

};

ard::board_link_list::~board_link_list()
{
    for (auto l : m_links) {
        l->release();
    }
    for (auto l : m_removed_links) {
        l->release();
    }
};

size_t ard::board_link_list::size()const
{
    return m_links.size();
};

bool ard::board_link_list::empty()const
{
    return m_links.empty();
};

ard::board_link* ard::board_link_list::getAt(int pos)
{
    return m_links[pos];
};

const ard::board_link* ard::board_link_list::getAt(int pos)const
{
    return m_links[pos];
};

void ard::board_link_list::sortByPIndex()
{
    std::sort(m_links.begin(), m_links.end(), [](board_link* lnk1, board_link* lnk2)
    {
        return (lnk1->linkPindex() < lnk2->linkPindex());
    });
};

const ard::BLINK_LIST& ard::board_link_list::removeAllLinks()
{
    m_removed_links.insert(m_removed_links.end(), m_links.begin(), m_links.end());
    m_links.clear();
    return m_removed_links;
};

ard::BLINK_LIST ard::board_link_list::resetPIndex()
{
    ard::BLINK_LIST updated_links;
    int pindex = 0;
    for (auto i : m_links) {
        if (pindex != i->linkPindex()) {
            i->setLinkPindex(pindex);
            updated_links.push_back(i);
        }
        pindex++;
    }
    return updated_links;
};

void ard::board_link_list::removeBLinks(const std::unordered_set<board_link*>& links2remove)
{
    for (auto it = m_links.begin(); it != m_links.end();) {
        auto lnk = *it;
        if (links2remove.find(lnk) != links2remove.end())
        {
            it = m_links.erase(it);
            m_removed_links.push_back(lnk);
        }
        else
        {
            it++;
        }
    }
};


/*STRING_SET ard::board_link_list::compileUsedSyid()const
{
    STRING_SET rv;
    for (auto i : m_links) {
		auto syid = i->syid();
        if (IS_VALID_SYID(syid)) {
			auto j = rv.find(syid);
			if (j == rv.end()) {
				rv.insert(syid);
			}
			else {
				ard::error(QString("duplicate syid in link-list[%1]").arg(syid));
			}
        }
    }
    return rv;
};*/

void ard::board_link_list::prepare_link_sync()
{
	for (auto i : m_links) {
		i->prepare_link_sync();
	}
};
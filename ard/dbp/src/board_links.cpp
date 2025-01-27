#include "board_links.h"



/**
    board_link_map
*/
ard::board_link_map::~board_link_map() 
{
    for (auto i : m_o2targets) {
        for (auto j : i.second) {
            delete (j.second);
        }
    }
    m_o2targets.clear();
};

ard::board_link_list* ard::board_link_map::addNewBLink(ard::board_link* link)
{
    if (!IS_VALID_SYID(link->origin())) {
        ASSERT(0, "expected valid origin syid") << link->dbgHint();
        return nullptr;
    }

    if (!IS_VALID_SYID(link->target())) {
        ASSERT(0, "expected valid target syid") << link->dbgHint();
        return nullptr;
    }

    board_link_list* rv;

    auto i = m_o2targets.find(link->origin());
    if (i == m_o2targets.end()) {
        rv = new board_link_list;
        rv->m_links.push_back(link);
        std::unordered_map<QString, board_link_list* > mln;
        mln[link->target()] = rv;
        m_o2targets.emplace(link->origin(), std::move(mln));
    }
    else {
        auto j = i->second.find(link->target());
        if (j == i->second.end()) {
            rv = new board_link_list;
            rv->m_links.push_back(link);
            i->second[link->target()] = rv;
        }
        else {
            rv = j->second;
            rv->m_links.push_back(link);
        }
    }

    return rv;
};

void ard::board_link_map::removeMapLinks(const STRING_LIST& origins, QString target)
{
    for (auto& o : origins) {
        auto i = m_o2targets.find(o);
        if (i != m_o2targets.end()) {
            auto j = i->second.find(target);
            if (j != i->second.end()) {
                qDebug() << "lnk-scheduled2remove-1" << o << "->" << target;
                auto links = j->second;
                links->removeAllLinks();
            }
        }
    }

    auto i = m_o2targets.find(target);
    if (i != m_o2targets.end()) {
        for (auto j : i->second) {
            qDebug() << "lnk-scheduled2remove-2" << target << "->" << j.first;
            auto links = j.second;
            links->removeAllLinks();
        }
    }
};

size_t ard::board_link_map::totalLinksCount()const 
{
	size_t rv = 0;
	for (auto& i : m_o2targets) {
		for (auto& j : i.second) {
			rv += j.second->m_links.size();
		}
	}
	return rv;
};

ard::board_link_list* ard::board_link_map::findBLinkList(QString origin, QString target)
{
    auto i = m_o2targets.find(origin);
    if (i != m_o2targets.end()) {
        auto j = i->second.find(target);
        if (j != i->second.end()) {
            auto links = j->second;
            return links;
        }
    }
    return nullptr;
};

ard::board_link_map* ard::board_link_map::clone_links(QString board_syid, const SYID2SYID& source2clone)const
{   
    ard::board_link_map* rv = new ard::board_link_map(board_syid);
    for (auto& i : m_o2targets) {
        for (auto& j : i.second) {
            auto origin = i.first;
            auto target = j.first;

            auto c_o = source2clone.find(origin);
            auto c_t = source2clone.find(target);
            ASSERT(c_o != source2clone.end(), "failed to locate syid in map translation") << origin << source2clone.size();
            ASSERT(c_t != source2clone.end(), "failed to locate syid in map translation") << origin << source2clone.size();
            if (c_o != source2clone.end() && c_t != source2clone.end()) 
            {
                int link_num = j.second->size();
                for (int lnk_idx = 0; lnk_idx < link_num; lnk_idx++)
                {
                    auto lnk_source = j.second->getAt(lnk_idx);
                    auto lnk = new board_link(c_o->second, c_t->second, lnk_source->linkPindex(), lnk_source->linkLabel());

                    auto ii = rv->m_o2targets.find(c_o->second);
                    if (ii == rv->m_o2targets.end()) {
                        std::unordered_map<QString, board_link_list*> mln;
                        board_link_list* links = new board_link_list;
                        links->m_links.push_back(lnk);
                        mln.emplace(c_t->second, std::move(links));
                        rv->m_o2targets.emplace(c_o->second, std::move(mln));
                    }
                    else {
                        auto jj = ii->second.find(c_t->second);
                        if (jj != ii->second.end()) {
                            jj->second->m_links.push_back(lnk);
                        }
                        else {
                            board_link_list* links = new board_link_list;
                            links->m_links.push_back(lnk);
                            ii->second[c_t->second] = links;
                        }
                    }
                }
            }
        }
    }
    return rv;
};

void ard::board_link_map::add_link_from_db(QSqlQuery* q)
{
    ard::board_link* lnk = new ard::board_link(q);
    auto origin = lnk->origin();
    auto target = lnk->target();

    //qDebug() << "db-add-lnk" << origin << target;

    auto i = m_o2targets.find(origin);
    if (i == m_o2targets.end()) {       
        std::unordered_map<QString, board_link_list*> mln;
        board_link_list* links = new board_link_list;
        links->m_links.push_back(lnk);
        mln.emplace(target, std::move(links));
        m_o2targets.emplace(origin, std::move(mln));
    }
    else {
        auto j = i->second.find(target);
        if (j != i->second.end()) {
            j->second->m_links.push_back(lnk);
        }
        else {
            board_link_list* links = new board_link_list;
            links->m_links.push_back(lnk);
            i->second[target] = links;
        }
    }
};

void ard::board_link_map::ensurePersistantLinksMap(ArdDB* db)
{
    if (m_o2targets.empty())
        return;

    std::vector<ard::board_link*> links2create;
    std::vector<ard::board_link*> links2remove;
    std::vector<ard::board_link*> links2update;

    for (auto& i : m_o2targets) {
        for (auto& j : i.second) {
            for (auto& bl : j.second->m_removed_links) 
			{
                links2remove.push_back(bl);
				ard::trail(QString("schedule2remove-link [%1][%2][%3][%4][%5][%6]").arg(m_board_syid).arg(bl->link_syid()).arg(bl->linkid()).arg(bl->origin()).arg(bl->target()));
            }

            for (auto& bl : j.second->m_links) {
                switch (bl->linkStatus())
                {
                case LinkStatus::normal:break;
                case LinkStatus::created:
                {
                    links2create.push_back(bl);
                }break;
                case LinkStatus::updated:
                {
                    if (IS_VALID_DB_ID(bl->linkid())) {
                        links2update.push_back(bl);
                    }
                    else {
                        links2create.push_back(bl);
                    }
                }break;
                }
            }
        }
    }

	storeLinks(db, links2create, links2remove, links2update);

	if (!links2remove.empty()) {
		for (auto& i : m_o2targets) {
			for (auto& j : i.second) {
				j.second->m_removed_links.clear();
			}
		}
	}
};

void ard::board_link_map::storeLinks(ArdDB* db,
	std::vector<ard::board_link*>& links2create,
	std::vector<ard::board_link*>& links2remove,
	std::vector<ard::board_link*>& links2update) 
{
	if (!links2create.empty()) {
		if (!dbp::create_board_links(db, m_board_syid, links2create)) {
			ASSERT(0, "DB/failed to serialize new links");
		}
		for (auto lnk : links2create)lnk->setLinkStatus(LinkStatus::normal);
	}

	if (!links2remove.empty()) {
		if (!dbp::remove_board_links(db, links2remove)) {
			ASSERT(0, "DB/failed to remove links");
			for (auto lnk : links2remove)lnk->setLinkStatus(LinkStatus::normal);
		}
	}

	if (!links2update.empty()) {
		if (!dbp::update_board_links(db, links2update)) {
			ASSERT(0, "DB/failed to update links");
		}
		for (auto lnk : links2update)lnk->setLinkStatus(LinkStatus::normal);
	}
};


STRING_SET ard::board_link_map::selectUsedSyid()const 
{
	STRING_SET rv;
	for (auto& a : m_o2targets) 
	{
		for (auto& b : a.second)
		{
			auto links = b.second;
			for (auto i : links->m_links)
			{
				auto syid = i->link_syid();
				if (IS_VALID_SYID(syid)) 
				{
					auto j = rv.find(syid);
					if (j == rv.end()) {
						rv.insert(syid);
					}
					else {
						ard::error(QString("duplicate syid in link-list[%1][%2]").arg(m_board_syid).arg(syid));
					}
				}
			}

		}
	}

	return rv;
};

std::shared_ptr<ard::flat_links_sync_map> ard::board_link_map::produceFlatLinksMap(ArdDB* db, COUNTER_TYPE hist_counter)
{
	std::shared_ptr<flat_links_sync_map> rv(new ard::flat_links_sync_map);
	rv->m_link_map = this;
	rv->m_db = db;
	rv->m_hist_counter = hist_counter;
	for (auto& a : m_o2targets)
	{
		for (auto& b : a.second)
		{
			auto links = b.second;
			for (auto i : links->m_links)
			{
				rv->m_fm_links.push_back(i);
				rv->m_fm_syid2link[i->link_syid()] = i;				
			}
		}
	}
	return rv;
};

bool ard::flat_links_sync_map::synchronizeFlatMaps(ard::SLM_LIST& lst, snc::SyncProgressStatus* p)
{
	bool rv = false;
	for (auto& i : lst) {
		if (synchronizeTwoWayFlatLinkMaps(i.fm1.get(), i.fm2.get(), p))
		{
			rv = true;
		};
	}
	return rv;
};

bool ard::flat_links_sync_map::synchronizeTwoWayFlatLinkMaps(flat_links_sync_map* fm1, flat_links_sync_map* fm2, snc::SyncProgressStatus* p)
{
	bool changes_detected = false;
	if (ard::flat_links_sync_map::synchronizeOneWayFlatLinkMaps(fm1,
		fm2,
		fm1->m_hist_counter,
		fm2->m_hist_counter, 
		p))
	{
		changes_detected = true;
	}
	if (ard::flat_links_sync_map::synchronizeOneWayFlatLinkMaps(fm2,
		fm1,
		fm2->m_hist_counter,
		fm1->m_hist_counter, 
		p))
	{
		changes_detected = true;
	}
	if (changes_detected) {
		fm1->ensurePersistant();
		fm2->ensurePersistant();
	}
	return changes_detected;
};

bool ard::flat_links_sync_map::synchronizeOneWayFlatLinkMaps(ard::flat_links_sync_map* src,
	ard::flat_links_sync_map* target,
	COUNTER_TYPE hist_mod_counter,
	COUNTER_TYPE contra_db_hist_mod_counter,
	snc::SyncProgressStatus* p)
{
	bool rv = false;
	bool first_time_ever_this_db_in_synced = !IS_VALID_COUNTER(hist_mod_counter);
	std::function<bool(ard::board_link*)> is_modified_link = [hist_mod_counter, first_time_ever_this_db_in_synced](ard::board_link* lnk)
	{
		bool rv = (first_time_ever_this_db_in_synced || (lnk->mdc() > hist_mod_counter));
		return rv;
	};

	for (auto lnk_src : src->m_fm_links) 
	{

		if (!lnk_src->is_sync_processed())
		{
			auto j = target->m_fm_syid2link.find(lnk_src->link_syid());
			if (j == target->m_fm_syid2link.end())
			{
				if (is_modified_link(lnk_src))
				{
					//qDebug() << "slink/clone" << lnk_src->mdc() << lnk_src->link_syid() << lnk_src;
					if(p)p->m_cloned_lnk.push_back(lnk_src->toSyncInfo());
					auto l2 = lnk_src->cloneInSync(contra_db_hist_mod_counter + 1);
					target->m_link_map->addNewBLink(l2);
					target->m_fm_links.push_back(l2);
					target->m_fm_syid2link[l2->link_syid()] = l2;
					rv = true;
				}
				else 
				{
					//qDebug() << "slink/delete" << lnk_src->mdc() << lnk_src->link_syid();
					if(p)p->m_deleted_lnk.push_back(lnk_src->toSyncInfo());
					lnk_src->markDeletedInSync();
					rv = true;
				}
			}
			else 
			{
				if (is_modified_link(lnk_src))
				{
					//qDebug() << "slink/assign" << lnk_src->mdc() << lnk_src->link_syid();
					if(p)p->m_modified_lnk.push_back(lnk_src->toSyncInfo());
					auto lnk_target = j->second;
					lnk_target->assignContentInSync(lnk_src, contra_db_hist_mod_counter + 1);
					rv = true;
				}
			}
		}
	}
	return rv;
};

bool ard::flat_links_sync_map::cleanLinksModifications() 
{
	bool rv = false;
	for (auto& j : m_link_map->o2targets()) {
		for (auto& k : j.second) {
			auto links = k.second;
			for (int i = 0; i < static_cast<int>(links->size()); i++) {
				auto lnk = links->getAt(i);
				if (lnk->mdc() > m_hist_counter) {
					lnk->setMdc(m_hist_counter);
					rv = true;
				}
			}
		}
	}
	return rv;
};

void ard::flat_links_sync_map::ensurePersistant() 
{
	std::vector<ard::board_link*> links2create;
	std::vector<ard::board_link*> links2remove;
	std::vector<ard::board_link*> links2update;

	for (auto& bl : m_fm_links)
	{
		switch (bl->linkStatus())
		{
		case LinkStatus::normal:break;
		case LinkStatus::created:
		{
			links2create.push_back(bl);
		}break;
		case LinkStatus::updated:
		{
			if (IS_VALID_DB_ID(bl->linkid())) {
				links2update.push_back(bl);
			}
			else {
				links2create.push_back(bl);
			}
		}break;
		case LinkStatus::deleted_in_sync: 
		{
			links2remove.push_back(bl);
		}break;
		}
	}
	//qDebug() << "slink/ready2store"	<< "links2create=" << links2create.size() 
	//								<< "links2remove=" << links2remove.size()
	//								<< "links2update=" << links2update.size();
	m_link_map->storeLinks(m_db, links2create, links2remove, links2update);
};

QString	ard::flat_links_sync_map::toString()const 
{
	QString rv = QString("%1/%2 (%3=%4=%5)%6").arg(m_hist_counter)
								.arg(toHashStr())
								.arg(m_fm_links.size())
								.arg(m_fm_syid2link.size())
								.arg(m_link_map->totalLinksCount())
								.arg(m_db->sqldb().databaseName());
	return rv;
};

QString ard::flat_links_sync_map::toHashStr()const 
{
	QString rv("--");
	for (auto& i : m_fm_syid2link) {
		rv += i.second->toHashStr();
		rv = QCryptographicHash::hash((rv.toUtf8()), QCryptographicHash::Md5).toHex();
	}
	return rv;
};
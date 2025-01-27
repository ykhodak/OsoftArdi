#include <ctime>
#include <QDate>
#include <QCryptographicHash>
#include "snc-misc.h"
#include "snc-assert.h"
#include "snc.h"

using namespace snc;

#ifdef _DEBUG
static std::set<smart_counter_ptr> g_all_smart_counters;

template <class CounterIt>
void print_counters_container(CounterIt first, CounterIt last)
{
	for (CounterIt i = first; i != last; i++)
	{
		smart_counter_ptr c = *i;
		QString s = c->dbgHint();
		if (c->lock_frames().empty()) {
			s += "[new]";
		}
		else {
			for (auto& j : c->lock_frames()) {
				s += "[" + j + "]";
			}
		}
		qDebug() << c->counter() << s;
	}
}

void print_smart_counters_in_memory()
{
	qDebug() << "/ykh/print_smart_counters_in_memory/disabled";
	return;
	if (g_all_smart_counters.empty())
	{
		qDebug() << "smart-counter-set - OK";
	}
	else
	{
		qDebug() << "=========== begin counters map" << g_all_smart_counters.size() << "===============";
		print_counters_container(g_all_smart_counters.begin(), g_all_smart_counters.end());
		qDebug() << "=========== end counters map =================";

		CITEMS_SET top_parents_set;
		for (std::set<smart_counter_ptr>::iterator k = g_all_smart_counters.begin(); k != g_all_smart_counters.end(); k++)
		{
			cit_ptr it = dynamic_cast<cit_ptr>(*k);
			if (it)
			{
				cit_ptr p = it;
				cit_ptr p2 = p;
				CITEMS_SET parents_set;
				while (p)
				{
					p2 = p;
					p = p->cit_owner();
					if (parents_set.find(p) != parents_set.end())
					{
						ASSERT(0, "cross reference detected in tree") << p->dbgHint();
						break;
					}
					parents_set.insert(p);
				}
				if (p2)
					top_parents_set.insert(p2);
			}
		}
		qDebug() << "=========== begin top parents set" << top_parents_set.size() << "===============";
		print_counters_container(top_parents_set.begin(), top_parents_set.end());
		qDebug() << "=========== end top parents set =================";
	}
}

void smart_counter::lock(QString location)
{
    ASSERT_VALID(this);
    m_counter++;

    QString s = getLastStackFrames(10);
    m_lock_frames.push_back("<" + location + ">" + s);
};

#endif //_DEBUG

smart_counter::smart_counter():m_counter(1)
{
#ifdef _DEBUG
    m_magic_num = MAGIC_MEM_CHECK_NUMBER;
    g_all_smart_counters.insert(this);
#endif
}

smart_counter::~smart_counter()
{
    ASSERT(m_counter == 0, "not ZERO smart counter in destructor");
#ifdef _DEBUG
    m_magic_num = 0;
    std::set<smart_counter_ptr>::iterator k = g_all_smart_counters.find(this);
    if(k != g_all_smart_counters.end()){
        g_all_smart_counters.erase(k);
    }
#endif 
}

#ifndef _DEBUG
void smart_counter::lock()
{
    m_counter++;
};
#endif

void smart_counter::release()
{
    ASSERT_VALID(this);
    m_counter--;
    
    if(m_counter == 0){
        unregister();
        final_release();
    }
};

void smart_counter::final_release()
{
    delete this;
};

#ifdef _DEBUG
void smart_counter::print_lock_frames()
{
    qDebug() << "=== lock-frames" << lock_frames().size() << dbgHint();
    for(auto& i : lock_frames())
        {
            qDebug() << "l-f" << i;
        }
    qDebug() << "=== end-of-lock-frames ===";
};
#endif

snc::SHistEntry::SHistEntry():
    mod_counter(INVALID_COUNTER),
    last_sync_time(0),
    sync_point_id(syncpUknown)
{
};

QString formatDBID(DB_ID_TYPE id)
{
    QString rv = "#";
    if(IS_VALID_DB_ID(id))
        rv = QString("%1").arg(id);
    return rv;
};

QString formatSYID(snc::SYID id)
{
    QString rv = "#";
    if(IS_VALID_SYID(id))
        rv = QString("%1").arg(id);
    return rv;
};


#ifdef _SQL_PROFILER
void print_stack()
{
    STRING_LIST bt;
    get_stacktrace(bt);
    if(!bt.empty())
        {
            STRING_LIST::iterator k = bt.begin();
            k++;
            if(k != bt.end())
                {
                    QString s = *k;
                    qDebug() << "bt:" << s;
                }
        }
}
#endif

void print_stat_header(QString s)
{
#define HEADER_WIDTH 30
  
    int len = s.length();
    if(len < HEADER_WIDTH)
        {
            QString tmp = "=";
            while(len < HEADER_WIDTH)
                {
                    tmp += "=";len++;
                }
            s = tmp + " " + s;
        }
    sync_log(s);

#undef HEADER_WIDTH
}

void print_item_stat_list_with_pos(QString title, CITEMS& lst, QString desc)
{
    if(!lst.empty())
        {
            int list_size = lst.size();
            QString s = QString("%1 [%2]").arg(title).arg(list_size);
            print_stat_header(s);

            if(!desc.isEmpty())
                {
                    sync_log(desc);
                }

            int i = 1;
            int fieldSize = 1;
            if(list_size > 9)fieldSize=2;
            if(list_size > 99)fieldSize=3;
            if(list_size > 999)fieldSize=4;      
            STRING_LIST string_list;
            for(auto o : lst)
                {
                    cit_ptr p = o->cit_parent();
                    QString s_parent_info;      
                    if(p != NULL)
                        {
                            QString parent_obj_name = QString("%1%2")
                                .arg(p->otype())
                                .arg(p->objName().left(6));
                            s_parent_info = QString("[%1 %2 %3]")
                                .arg(parent_obj_name.left(6), 6)
                                .arg(p->syid(), 20)
                                .arg(p->indexOf_cit(o), 3);
                        }
                    else
                        {
                            s_parent_info = "[NO-PARENT]";
                        }
                    QString s2 = QString("%1.%2%3").arg(i, fieldSize).arg(s_parent_info, 24).arg(o->dbgHint());
                    string_list.push_back(s2);
                    //sync_log(s);
                    i++;
                }//for
            sync_log(string_list);
        }
}

bool areSyidEqual(snc::SYID id1, snc::SYID id2)
{
    bool rv = (id1.compare(id2, Qt::CaseInsensitive) == 0);
    return rv;
};

void ard::trail(QString s)
{
	auto o = qWarning();
	o.noquote();
	o << QDateTime::currentDateTime().toString("hh:mm:ss.zzz") << "[trail]" << s;
};

void ard::error(QString s)
{
	auto o = qWarning();
	o.noquote();
	o << QDateTime::currentDateTime().toString("hh:mm:ss.zzz") << "[error]" << s;
};

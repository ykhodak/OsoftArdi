#pragma once

#include "snc-misc.h"


namespace ardi_functional {
    template <class T> std::unordered_map<QString, T*> toSyidMap(std::vector<T*>& lst);
    template<class It> QString idsToStr(It begin, It end);
    template<class K, class V> std::vector<V> slice_map(const std::map<K, V>& m, const std::set<K>&& s);
};


namespace snc
{
  class SyncProcessor;

  template<class T>
  void clear_locked_vector(std::vector<T*>& v)
  {
	  typename std::vector<T*>::iterator i = v.begin();
	  for (; i != v.end(); i++) {
		  (*i)->release();
	  }
    v.clear();
  }
};

template <class T> std::unordered_map<QString, T*> ardi_functional::toSyidMap(std::vector<T*>& lst)
{
    std::unordered_map<QString, T*> rv;
    for (auto& f : lst) {
        rv[f->syid()] = f;
    }
    return rv;
};

template<class It> QString ardi_functional::idsToStr(It begin, It end)
{
    QString s_oids;

    for (It i = begin; i != end; i++) {
        DB_ID_TYPE dbid = *i;
        if (IS_VALID_DB_ID(dbid)) {
            s_oids += QString("%1").arg(dbid);
            s_oids += ",";
        }
    }
    if (!s_oids.isEmpty())
        s_oids.remove(s_oids.length() - 1, 1);
    return s_oids;
};

template<class K, class V>
std::vector<V> ardi_functional::slice_map(const std::map<K, V>& m, const std::set<K>&& s)
{
    std::vector<V> rv;
    typedef typename std::map<K, V>::const_iterator ITR;
    for (ITR i = m.begin(); i != m.end(); i++) {
        if (s.find(i->first) != s.end()) {
            rv.push_back(i->second);
        }
    }
    return rv;
};


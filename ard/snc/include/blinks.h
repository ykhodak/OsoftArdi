#pragma once

#include <QSqlQuery>
#include "snc.h"

namespace ard 
{
    class boards_root;
    class selector_board;
    class board_link_map;

    /**
    board links model

    so -> st                  *map level (string-based maps), sync/core model
    -----   \
    o->t   --[l]              *board level (pointer-based maps), higher abstruction model
    t->o    /
    *all three kind of maps point to same link-list, which is vector of links between two nodes
    */

    enum class LinkStatus
    {
        normal,
        created,
        updated,
		deleted_in_sync
    };

    /**
    one link from origin to target
    */
    class board_link :public snc::smart_counter
    {
    public:
        board_link();
        board_link(QString origin, QString target, int link_pindex, QString link_label);
        board_link(QSqlQuery* q);

        DB_ID_TYPE			linkid()const { return m_linkid; }
        QString				origin()const { return m_origin; }
        QString				target()const { return m_target; }
        QString				link_syid()const { return m_link_syid; }
		QString				toHashStr()const;
		snc::LinkSyncInfo	toSyncInfo()const;
        /// linkPindex is index of link in link list, list of links between same origin and target
        int					linkPindex()const { return m_lflags.link_pindex; }
        QString				linkLabel()const { return m_link_label; }
		bool				is_sync_processed()const { return (m_lflags.sync_processed_flag == 1); };
		void				prepare_link_sync();

        snc::COUNTER_TYPE   mdc()const { return m_mod_counter; }
        LinkStatus			linkStatus()const { return m_link_status; }
		void				markDeletedInSync();

        void				setLinkStatus(LinkStatus s);
        void				setMdc(snc::COUNTER_TYPE v);
        void				setLinkPindex(int index);
        void				setLinkLabel(QString s, snc::cdb* c);

        board_link*			cloneInSync(snc::COUNTER_TYPE)const;
        void				assignContentInSync(ard::board_link*, snc::COUNTER_TYPE);

        QString				dbgHint(QString s = "")const;
        void				setupLinkIdFromDb(DB_ID_TYPE v);
    protected:
        DB_ID_TYPE          m_linkid{ 0 };
        QString             m_origin;
        QString             m_target;
        QString             m_link_syid;
		union lflags {
			uint32_t  flag;
			struct {
				uint16_t  link_pindex;
				uint16_t  sync_processed_flag;
			};
		} m_lflags;
        
        QString             m_link_label;
        snc::COUNTER_TYPE   m_mod_counter{ 0 };
        LinkStatus          m_link_status;

        friend class selector_board;
    };

    using BLINK_LIST = std::vector<board_link*>;

    /**
        collection of links between origin and target
        we are wrapper around std::vector
    */
    class board_link_list
    {
    public:
        board_link_list();
        ~board_link_list();

        size_t              size()const;
        bool                empty()const;
        board_link*         getAt(int pos);
        const board_link*   getAt(int pos)const;
        void                sortByPIndex();
        BLINK_LIST          resetPIndex();
        const BLINK_LIST&   removeAllLinks();
        void                removeBLinks(const std::unordered_set<board_link*>& links2remove);
        //STRING_SET          compileUsedSyid()const;
		void				prepare_link_sync();

        /// rpos_index - index of list leading to the right from origin
        int                 rpos_index()const { return m_rpos_index;}
        /// rpos_index - index of list leading to the left from origin
        int                 lpos_index()const { return m_lpos_index; }
    protected:
        BLINK_LIST			m_links;
        BLINK_LIST			m_removed_links;
        int                 m_rpos_index{ 0 };
        int                 m_lpos_index{ 0 };
        friend class board_link_map;
        friend class selector_board;
    };

};
#pragma once
#include <memory>
#include <QSqlQuery>
#include "a-db-utils.h"
#include "anfolder.h"
#include "tooltopic.h"
#include "ansyncdb.h"
#include "blinks.h"

namespace ard {
	class flat_links_sync_map;

    class board_link_map 
    {
    public:
        ~board_link_map();

        board_link_list*						addNewBLink(ard::board_link* link);
        void									ensurePersistantLinksMap(ArdDB* db);
        QString									board_syid()const {return m_board_syid;}
        const O2TLINKS&							o2targets()const { return m_o2targets; }
        void									removeMapLinks(const STRING_LIST& origins, QString target);
        board_link_list*						findBLinkList(QString origin, QString target);
		STRING_SET								selectUsedSyid()const;
		std::shared_ptr<ard::flat_links_sync_map>produceFlatLinksMap(ArdDB* db, COUNTER_TYPE hist_counter);
		size_t									totalLinksCount()const;		
    protected:
        board_link_map(QString board_syid) :m_board_syid(board_syid) {};
        void                add_link_from_db(QSqlQuery* q);
        board_link_map*     clone_links(QString board_syid, const SYID2SYID& source2clone)const;

		void				storeLinks(ArdDB* db,
										std::vector<ard::board_link*>& links2create,
										std::vector<ard::board_link*>& links2remove,
										std::vector<ard::board_link*>& links2update);


        QString             m_board_syid;
        O2TLINKS            m_o2targets;

        friend class    boards_root;
        friend class    selector_board;
		friend class	flat_links_sync_map;
    };

    using S2BL = std::map<QString, std::unique_ptr<board_link_map>>;

	/// helper class to sync link maps
	struct link_map_sync_info
	{
		std::shared_ptr<ard::flat_links_sync_map> fm1{ nullptr };
		std::shared_ptr<ard::flat_links_sync_map> fm2{ nullptr };
	};
	using SLM_LIST = std::vector<ard::link_map_sync_info>;


	class flat_links_sync_map 
	{
	public:
		static bool synchronizeFlatMaps(SLM_LIST& lst, snc::SyncProgressStatus* p);
		QString	toString()const;
		QString toHashStr()const;
	protected:
		static bool synchronizeTwoWayFlatLinkMaps(flat_links_sync_map* fm1, flat_links_sync_map* fm2, snc::SyncProgressStatus* p);
		static bool synchronizeOneWayFlatLinkMaps(ard::flat_links_sync_map* src,
											ard::flat_links_sync_map* target,
											COUNTER_TYPE hist_mod_counter,
											COUNTER_TYPE contra_db_hist_mod_counter, 
											snc::SyncProgressStatus* p);

		bool		cleanLinksModifications();
		void		ensurePersistant();

		BLINK_LIST			m_fm_links;
		SYID2L_ORD			m_fm_syid2link;
		board_link_map*		m_link_map{ nullptr };
		ArdDB*				m_db{nullptr};
		COUNTER_TYPE		m_hist_counter{0};
		friend class board_link_map;
	};
};
#pragma once

#include "gmail/GmailRoutes.h"
#include "gmail/GmailCache.h"
#include "gcontact/GcontactCache.h"
#include "global-enums.h"

class anItem;
class OutlineView;
class PinInfo;
class FormFieldTopic;

namespace ard 
{
	class topic;
    class kring_model;
    class boards_model;
    class anKRingKeyExt;
    class email_draft_ext;
    class board_ext;
    class board_item_ext;
	class picture_ext;
    class contacts_model;
    class contact_ext;
    class ContactsRoot;
    class ContactGroupsRoot;
    class note_ext;
	class ethread;
    class ethread_ext;
    class q_param;
    class static_rule_root;
    class task_ring_observer;
    class local_search_observer;
    class notes_observer;
	class bookmarks_observer;
	class pictures_observer;
    class comments_observer;
    class color_observer;
    class rule_runner;
	class rules_root;
	class rule_ext;
	class rules_model;
	class email;
	class selector_board;
	class board_item;
	class board_item_ext;
	class board_link;
	class board_link_list;
	class contact;
	class contact_group;
	class picture;
	class email_draft;

	using O2TLINKS		= std::unordered_map<QString, std::unordered_map<QString, board_link_list*>>;
	using SYID2L_ORD	= std::unordered_map<QString, board_link*>;

	struct depth_topic
	{
		ard::topic*		topic{ nullptr };
		int				depth{ 0 };
	};
};

using SYID2SYID     = std::unordered_map<QString, QString>;

using IDS_SET       = std::unordered_set<DB_ID_TYPE>;
using IDS_LIST      = std::vector<DB_ID_TYPE>;

using TOPICS_LIST   = std::vector<ard::topic*>;
using DEPTH_TOPICS  = std::vector<ard::depth_topic>;
using CTOPICS_LIST  = std::vector<const ard::topic*>;
using TOPICS_SET    = std::unordered_set<ard::topic*>;
using TOPICS_MAP    = std::unordered_map<QString, ard::topic*>;
using CTOPICS_LIST  = std::vector<const ard::topic*>;
using TOPIC2TOPIC   = std::unordered_map<ard::topic*, ard::topic*>;
using FTYPE2TOPIC   = std::map<EFolderType, ard::topic*>;
using LOCUS_LIST	= std::vector<ard::locus_folder*>;

using S_2_CSET      = std::unordered_map<QString, std::set<contact_cptr>>;

using ELIST			= std::vector<ard::email*>;
using ESET			= std::unordered_set<ard::email*>;

using THREAD_VEC    = std::vector<ard::ethread*>;
using THREAD_SET    = std::unordered_set<ard::ethread*>;
using GTHREADS		= std::unordered_set<googleQt::mail_cache::ThreadData*>;
using S2TREADS		= std::unordered_map<QString, THREAD_VEC>;
using RULES			= std::vector<ard::q_param*>;

typedef std::map<QString, QVariant>             PARAM_MAP;
typedef std::unordered_map<DB_ID_TYPE, QString> ID2S;
typedef std::unordered_map<QString, QString>    S2S;

using TOPICS_WITH_LABELS = std::vector<std::pair<ard::topic*, googleQt::mail_cache::label_ptr>>;
using LABEL2FOLDER = std::unordered_map<googleQt::mail_cache::label_ptr, ard::topic*>;
using GLABEL_SET = std::set<googleQt::mail_cache::label_ptr>;

using GCONTACTS_LIST = std::vector<googleQt::gcontact::ContactInfo::ptr>;
using GGROUPS_MAP = std::map<QString, googleQt::gcontact::GroupInfo::ptr>;

using CONTACT_LIST = std::vector<contact_ptr>;
using CCONTACT_LIST = std::vector<contact_cptr>;
using CONTACT_GROUP_LIST = std::vector<ard::contact_group*>;
using CONTACT_CGROUP_LIST = std::vector<const ard::contact_group*>;
using GROUP_MAP = std::unordered_map<QString, ard::contact_group*>;
using PICTURES_LIST = std::vector<ard::picture*>;

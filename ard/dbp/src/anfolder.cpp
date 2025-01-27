#include <QApplication>
#include <QTime>
#include <QTextCursor>
#include <QTextDocument>
#include <QFontMetrics>
#include <QApplication>
#include <QDesktopWidget>
#include <QScreen>
#include <algorithm>
#include <ctime>

#include "a-db-utils.h"
#include "ansyncdb.h"
#include "email.h"
#include "gdrive/GdriveRoutes.h"
#include "Endpoint.h"
#include "ethread.h"
#include "extnote.h"
#include "anurl.h"


extern QString specialGtdFolderName(EFolderType type);
extern int meta_define_child_pindex(snc::CITEMS& items,  int index);
ard::AllocPool ard::topic::m_alloc_pool;

/**
   ard::topic
*/
ard::topic::topic()
{
	m_attr.flags = 0;
}

ard::topic::topic(QString title, 
                 EFolderType _ftype)
    :m_title(title)
{
	m_attr.flags = 0;
    m_attr.FolderType = static_cast<int>(_ftype);
}

ard::topic::~topic()
{
};

void ard::topic::unregister()
{
	if (isPersistant())
	{
		if (IS_VALID_DB_ID(m_id) && m_data_db)
		{
			m_data_db->unregisterObj(this);
			detachDb();
			m_id = INVALID_DB_ID;
		}
		//UNREGISTER_ME;
	}
};

snc::cdb* ard::topic::syncDb()
{
    if (!m_data_db)
    {
        ASSERT(0, "expected dataDB");
        return nullptr;
    }

    return m_data_db->syncDb();
};

const snc::cdb* ard::topic::syncDb()const
{
    if (!m_data_db)
    {
        ASSERT(0, "expected dataDB");
        return nullptr;
    }

    return m_data_db->syncDb();
};

void ard::topic::attachPdb(snc::persistantDB* pdb)
{
    m_data_db = dynamic_cast<ArdDB*>(pdb);
#ifdef _DEBUG
    ASSERT(m_data_db, "expected dataDB") << dbgHint();
#endif
};

QString ard::topic::title()const
{
    auto ft = folder_type();
    if (ft == EFolderType::folderBacklog) {
        return "Backlog";
    }

    QString rv = m_title;

    if (rv.isEmpty()) {
        if (isGtdSortingFolder())
        {
            rv = specialGtdFolderName(ft);
        }
    }

    return rv;
};


QString ard::topic::shortTitle()const
{
    QString rv = title();
    QRegExp r("[/]");
    auto idx = rv.indexOf(r);
    if(idx != -1){
        rv.remove(idx, rv.size());
    }
    
    return rv;
};

void ard::topic::setTitle(QString title, bool guiRecalc)
{
    title = title.trimmed();
    title.remove(QRegExp("[\\n\\t\\r]"));
    if (!canRename())
    {
        ASSERT(0, "can't change title") << dbgHint();
        return;
    }

    if (!canRenameToAnything()) {
        STRING_SET st = allowedTitles();
        if (!st.empty()) {
            auto it = st.find(title);
            if (it == st.end()) {
                ASSERT(0, "can't change title on title-limited") << dbgHint();
                return;
            }
        }
    }

    if (m_title.compare(title) == 0)
        return;

    forceOutlineHeightRecalc();
    m_title = title;
	if (IS_VALID_DB_ID(m_id)) {
		register_db_modification(this, dbmodTitle);
	}

    if (guiRecalc) {
        ard::update_topic_card(this);
    }
}

ard::topic* ard::topic::parent()
{
	return dynamic_cast<ard::topic*>(cit_parent());
};

const ard::topic* ard::topic::parent()const
{
	topic_cptr f = dynamic_cast<const ard::topic*>(cit_parent());
	return const_cast<ard::topic*>(f);
};


QString ard::topic::annotation()const 
{
    return m_annotation;
};

QString ard::topic::annotation4outline()const 
{
    return annotation();
};

void ard::topic::setAnnotation(QString s, bool gui_update)
{
    if (!canHaveAnnotation())
    {
        ASSERT(0, "can't change annotation") << dbgHint();
        return;
    }

    s = s.trimmed();

    if (m_annotation.compare(s) == 0)
        return;

    m_annotation = s;
    register_db_modification(this, dbmodAnnotation);

    if (gui_update) {
        ard::asyncExec(AR_BoardRebuildForRefTopic, this);
    }
};

bool ard::topic::canHaveAnnotation()const 
{
    bool rv = !IsUtilityFolder();
    return rv;
};

QString ard::topic::objName()const
{
	QString rv = "Unknown";
	EFolderType type = folder_type();
	if (isGtdSortingFolder())
	{
		rv = specialGtdFolderName(type);
	}
	else
	{
		switch (type)
		{
		case EFolderType::folderGeneric: rv = "topic"; break;
		case EFolderType::folderUserSorter:rv = "folder"; break;
		case EFolderType::folderUserSortersHolder:rv = "UHolder"; break;
		default:ASSERT(0, "unsupported folder type");
		}
	}
	return rv;
};

QString ard::topic::folderTypeAsString  ()const
{
#define CASE_TYPE(T) case EFolderType::T: rv = #T;break;
    
    QString rv = "";
    switch(folder_type())
        {
            CASE_TYPE(folderUnknown);
            CASE_TYPE(folderGeneric);
            CASE_TYPE(folderRecycle);
            CASE_TYPE(folderMaybe);
            CASE_TYPE(folderReference);
            CASE_TYPE(folderDelegated);
            CASE_TYPE(folderSortbox);
            CASE_TYPE(folderUserSortersHolder);
            CASE_TYPE(folderUserSorter);
            CASE_TYPE(folderBacklog);
            CASE_TYPE(folderBoardTopicsHolder);
        }
    return rv;
};

bool ard::topic::addItem(topic_ptr it)
{
    return insertItem(it, items().size());
};

bool ard::topic::insertItem(topic_ptr it, int index, bool raw_inner_insert /*= false*/)
{
    if (!canAcceptChild(it)) {
        ASSERT(0, "can not accept given child") << dbgHint() << "child=" << it->dbgHint();
        return false;
    }

    if (!it->canBeMemberOf(this)) {
        ASSERT(0, "The child can not be member of suggested parent=") << dbgHint() << "child=" << it->dbgHint();
        return false;
    }

    if (!raw_inner_insert)
    {
        if (isPersistant() &&
            it->isPersistant())
        {
            assign_child_pindex(it, index);
        }

        switch (folder_type())
        {
        case EFolderType::folderRecycle:
        {
            handle_child_retire(it);
        }break;
        default:break;
        }

    }

    insert_cit(index, it);

    if (!raw_inner_insert) 
	{
        if (isPersistant()) {
            register_db_modification(it, dbmodMoved);
        }
		setThumbDirty();
    }

    return true;
};

bool ard::topic::moveSubItemsFrom(topic_ptr parent)
{
    TOPICS_LIST subitems;
    for (auto it : parent->items()) {
        topic_ptr f = dynamic_cast<topic_ptr>(it);
        if (f) {
            subitems.push_back(f);
        }
    }

    for (auto it : subitems) {
        if (!canAcceptChild(it) || !it->canBeMemberOf(this)) {
            return false;
        }
    }

    parent->remove_all_cit(false);
    for (auto it : subitems) {
        addItem(it);
    }

    return true;
};

void ard::topic::assign_child_pindex(topic_ptr it, int index)
{
    it->m_pindex = meta_define_child_pindex(items(), index);
    it->ask4persistance(np_PINDEX);
};

void ard::topic::handle_child_retire(topic_ptr it)
{
    assert_return_void(it, "expected item"); 
    if(!it->isRetired())
        {
            it->setRetired(true);
        }

    for(auto& tp : it->items())
        {
            topic_ptr c = dynamic_cast<topic_ptr>(tp);
            handle_child_retire(c);
        }
};

void ard::topic::doDetachItem(topic_ptr it)
{
    remove_cit(it, false);
    it->setPindex(-1);
};

void ard::topic::killAllItemsSilently()
{
    TOPICS_LIST direct_children;
    TOPICS_LIST all_descendant;
    select4kill(all_descendant);

    for (auto& tp : items())
    {
		auto it = dynamic_cast<ard::topic*>(tp);
        ASSERT(it, "expected topic");
        if (it) {
            direct_children.push_back(it);
        }
    }

	for (auto& it : all_descendant) {
		it->removeExternalStorage();
	}


    bool rv = dbp::removeTopics(dataDb(), all_descendant);
    if (rv)
    {
        for (auto& it : direct_children)
        {
            if (it->isSingleton())
            {
                /// singleton - clean children but leave empty
                TOPICS_LIST subsingleton_direct_children;
                for (auto& tp : it->items())
                {
					auto it2 = dynamic_cast<ard::topic*>(tp);
                    ASSERT(it2, "expected topic");
                    if (it2) {
                        subsingleton_direct_children.push_back(it2);
                    }
                }
                for (auto& tp : subsingleton_direct_children)
                {
                    topic_ptr it2 = dynamic_cast<topic_ptr>(tp);
                    ASSERT(it2->parent() == it, "internal parent/child relation error");
                    it->remove_cit(it2, true);
                }
            }
            else
            {
                /// non-singleton - remove as it is
                remove_cit(it, true);
            }
        }
    }
    else
    {
        ASSERT(0, "failed to kill group of items") << id() << title();
    }
};

void ard::topic::detachItem(topic_ptr it)
{
    doDetachItem(it);
};

void ard::topic::emptyRecycle()
{
	assert_return_void(folder_type() == EFolderType::folderRecycle, "expected recycle bin");
	TOPICS_LIST items_list;

	for (auto& tp : m_items) {
		auto it = dynamic_cast<ard::topic*>(tp);
		ASSERT(it, "expected topic");
		if (it) {
			items_list.push_back(it);
		}
	}
	for (auto& it : items_list) {
		it->killSilently(true);
	}
};


int ard::topic::indexOf(topic_cptr it)const
{
    return cit::indexOf_cit(it);
};


bool ard::topic::isAtomicIdenticalTo(const cit_primitive* _other, int& iflags)const
{
    topic_cptr other = dynamic_cast<topic_cptr>(_other);
    COMPARE_SYID(this, other, iflags, flSyid);
    bool rv = isAtomicIdenticalPropTo(_other, iflags);
    if (rv)
    {
        auto OurParent = parent();
        auto OtherParent = other->parent();
#ifdef _DEBUG
		assert_return_false(OurParent, QString("expected our parent: %1").arg(dbgHint()));
		assert_return_false(OtherParent, QString("expected other parent: %1").arg(_other->dbgHint()));
#endif

        if (OurParent->isGtdSortingFolder() || OurParent->folder_type() == EFolderType::folderUserSorter) {
        ///skip position check on special folders
        } 
        else{
            int ourIdx = OurParent->indexOf(this);
            int otherIdx = OtherParent->indexOf(other);
            if (ourIdx != otherIdx)
            {
                qDebug() << "index-err our-idx=" 
                    << ourIdx << "other-idx" << otherIdx 
                    << "our-p-size=" << OurParent->items().size() 
                    << "other-p-size=" << OtherParent->items().size() 
                    << "our=" << dbgHint() 
                    << "other" << other->dbgHint();
                iflags ^= flPindex;
                return false;
            }
        }
    }
    return rv;
};

bool ard::topic::isAtomicIdenticalPropTo(const cit_primitive* _other, int& iflags)const
{
	assert_return_false(_other != nullptr, "expected item");
    topic_cptr other = dynamic_cast<topic_cptr>(_other);
	assert_return_false(other != nullptr, QString("expected item %1").arg(_other->dbgHint()));

    /// don't compare anything else if it's not a synchronizable
    if (!isSynchronizable())
        return true;

    COMPARE_ATTR(otype(), iflags, flOtype);
    COMPARE_ATTR(m_title, iflags, flTitle);
    COMPARE_ATTR(m_attr.Color, iflags, flFont);
    COMPARE_ATTR(m_annotation, iflags, flAnnotation);
    COMPARE_ATTR(m_attr.ToDo, iflags, flToDo);
    COMPARE_ATTR(m_attr.ToDoIsDone, iflags, flToDoDone);
    COMPARE_ATTR(m_attr.isRetired, iflags, flRetired);
    return true;
};

void ard::topic::assignContent(const cit* _other)
{
    assignSyncAtomicContent(_other);
};

void ard::topic::assignSyncAtomicContent(const cit_primitive* _other)
{
	auto other = dynamic_cast<const ard::topic*>(_other);
    assert_return_void(other, "expected topic");
    //m_FolderFlags.flags = other->m_FolderFlags.flags;
    m_attr.flags = other->m_attr.flags;
    if(other->isSingleton()){
        m_attr.FolderType = static_cast<int>(EFolderType::folderGeneric);
    }
    m_attr.ToDo = other->m_attr.ToDo;
    m_attr.ToDoIsDone = other->m_attr.ToDoIsDone;
    m_attr.isRetired = other->m_attr.isRetired;
	m_attr.inLocus = other->m_attr.inLocus;
	m_attr.Color = other->m_attr.Color;
    m_title = other->m_title;
    //m_font_prop = other->m_font_prop;
    m_annotation = other->m_annotation;
    //m_attr.Hotspot = other->m_attr.Hotspot;

    
    bool check_identity = false;
#ifdef _DEBUG
    check_identity = true;
#endif

#ifdef ARD_BETA
    check_identity = true;  
#endif

    if (check_identity)
    {
        int iflags = 0;
        if (!isAtomicIdenticalPropTo(other, iflags))
        {
            if (iflags != flPSyid)
            {
                //don't give warning on parent issue.. not really sure why..
                qWarning() << "identity-error-after-assign" << iflags << sync_flags2string(iflags) << "|" << dbgHint() << "|" << _other->dbgHint();
            }
        }
        else
        {
            //         qWarning() << "identity-OK-after-assign" << dbgHint();
        }

    }

    ask4persistance(np_ATOMIC_CONTENT);
};

topic_ptr ard::topic::clone()const
{
    topic_ptr rv = dynamic_cast<topic_ptr>(create());
    rv->m_title = "";
    rv->m_id = 0;

    rv->assignContent(this);

    if (m_note_ext) {
        auto e = rv->ensureNote();
        if (e) {
            e->assignSyncAtomicContent(m_note_ext);
        }
    }

    for(auto& tp : items()){
            topic_cptr it = dynamic_cast<topic_cptr>(tp);
            topic_ptr it_copy = it->clone();
            rv->insert_cit(rv->items().size(), it_copy);
        }

    return rv;  
};

void ard::topic::setExpanded(bool val )
{
    m_attr.isExpanded = val ? 1 : 0;
    ///do not store expanded flag
}

ENoteView ard::topic::noteViewType()const 
{ 
    if (!parent()) {
        return ENoteView::None;
    }

    auto ft = folder_type();
    switch (ft) {
    case EFolderType::folderGeneric:
//    case EFolderType::folderProject:
        return ENoteView::Edit;
    default:
        break;
    }

    return ENoteView::None;
}

void ard::topic::fatFingerSelect()
{
    ard::open_page(this);
};

void ard::topic::fatFingerDetails(const QPoint& pt)
{
    ard::show_selector_context_menu(this, pt);
};

bool ard::topic::isTopicRetired()
{
    bool rv = isRetired();
    if (rv)
    {
        for (auto& i : items()) {
            topic_ptr it = dynamic_cast<topic_ptr>(i);
			assert_return_false(it, "expected item");
            if (!it->isRetired())
            {
                return false;
            }

            if (!it->isTopicRetired())
            {
                return false;
            }
        }

    }
    return rv;
};

bool ard::topic::isTopicCompleted()
{
    bool rv = isToDoDone();
    if(rv)
        {
            for(auto& i : items())    {
                    topic_ptr it = dynamic_cast<topic_ptr>(i);
					assert_return_false(it != nullptr, "expected item");
                    if (it->canChangeToDo()) {
                        if (!it->isToDoDone()){
                            return false;
                        }

                        if (!it->isTopicCompleted()){
                            return false;
                        }
                    }
                }
        }
    return rv;
};



void ard::topic::select4kill(TOPICS_LIST& items_list)
{
	for (auto& i : items()) {
		auto it = dynamic_cast<ard::topic*>(i);
		assert_return_void(it != nullptr, "expected topic");
		if (!it->isSingleton() && IS_VALID_DB_ID(it->id())) {
			items_list.push_back(it);
		}
	};

	for (auto& i : items()) {
		auto it = dynamic_cast<ard::topic*>(i);
		it->select4kill(items_list);
	}
};

template<class ResContainer, class Sel>
void ard::topic::selectItemsBreadthFirst(ResContainer& items_list, Sel& sel)
{
    for (auto& i : items()) {
		auto it = dynamic_cast<ard::topic*>(i);
        if (it) {
            if (sel(it))items_list.push_back(it);
        }
    }

    for (auto& i : items()) {
		auto f = dynamic_cast<ard::topic*>(i);
        if (f)f->selectItemsBreadthFirst(items_list, sel);
    }
};

template<class Sel>
void ard::topic::selectItemsDepthFirst(DEPTH_TOPICS& items_list, Sel& sel, int depth)
{
	for (auto& i : items()) {
		auto f = dynamic_cast<ard::topic*>(i);
		if (f) {
			ard::depth_topic dt;
			dt.topic = f;
			dt.depth = depth;
			if (sel(f))items_list.push_back(dt);
			f->selectItemsDepthFirst(items_list, sel, depth+1);
		}
	}
};


class SelectThreads
{
public:
    bool operator ()(topic_ptr it)
    {
        bool select_it = (dynamic_cast<ard::ethread*>(it) != nullptr);
        return select_it;
    }
};

class SelectBookmarks
{
public:
    bool operator ()(topic_ptr it)
    {
        bool select_it = (dynamic_cast<ard::anUrl*>(it) != nullptr);
        return select_it;
    }
};

class SelectValid
{
public:
	bool operator ()(topic_ptr it)
	{
		bool select_it = it->isValid();
		return select_it;
	}
};

TOPICS_LIST ard::topic::selectAllThreads()
{
    TOPICS_LIST rv;
    SelectThreads sel;
	selectItemsBreadthFirst(rv, sel);
    return rv;
};

TOPICS_LIST ard::topic::selectAllBookmarks()
{
    TOPICS_LIST rv;
    SelectBookmarks sel;
	selectItemsBreadthFirst(rv, sel);
    return rv;
};

DEPTH_TOPICS ard::topic::selectDepth()
{
	DEPTH_TOPICS rv;
	SelectValid sel;
	selectItemsDepthFirst(rv, sel, 0);
	return rv;
};

bool ard::topic::hasCurrentCommand(ECurrentCommand c)const 
{
    bool rv = false;
    switch (c) 
        {
        case ECurrentCommand::cmdEmptyRecycle:
            {
                rv = (folder_type() == EFolderType::folderRecycle);
            }break;
        case ECurrentCommand::cmdSelect:
        case ECurrentCommand::cmdSelectMoveTarget:            
            {
                rv = true;
            }break;
        default: break;
        }
    return rv;
};


bool ard::topic::killSilently(bool gui_update)
{
    ASSERT_VALID(this);
	assert_return_false(!isRootTopic(), QString("can't kill root: %1").arg(dbgHint()));

    TOPICS_LIST items_list;
    items_list.push_back(this);
    select4kill(items_list);
	for (auto& it : items_list) {
		it->removeExternalStorage();
	}

    if (gui_update) {
        ard::close_popup(items_list);
    }

    if (!dataDb())
    {
        if (!m_owner2)
        {
            release();
            return true;
        }
    }

    bool rv = true;
    auto db = dataDb();
    if (db) {
        rv = dbp::removeTopics(db, items_list);
    }
    if (rv)
    {
        for (auto& i : items_list) {
            i->unregister();
        }

        if (!m_owner2)
        {
            release();
            return true;
        }
        auto t = parent();
        ASSERT(t, "expected topic");
        parent()->remove_cit(this, false);

        if (gui_update) {
            if (isInLocusTab()) {
                ard::asyncExec(AR_RebuildLocusTab);
            }
        }

        this->release();
    }
    else
    {
        ASSERT(0, "failed to kill") << id() << title();
    }

    return rv;
};

bool ard::topic::isEmptyTopic()const
{
    bool items_empty = items().empty();
    if (!items_empty)
        return false;

    bool title_empty = title().trimmed().isEmpty();
    if (!title_empty)
        return false;

    if (m_note_ext) {
        m_note_ext->queryGui();
        bool html_empty = m_note_ext->html().trimmed().isEmpty();
        if (!html_empty)
            return false;
    }

    return true;
};

void ard::topic::deleteEmptyTopics()
{
	TOPICS_LIST t2delete;
	for (auto& i : items()) {
		auto f = dynamic_cast<ard::topic*>(i);
		assert_return_void(f, "expected topic");
		if (f->isReadyToAutoClean())
		{
			t2delete.push_back(f);
		}
	}

	if (t2delete.size() > 0)
	{
		for (auto& f : t2delete) {
			qWarning() << "[cleanup empty]" << f->dbgHint();
			f->killSilently(true);
		}
	}

	for (auto& i : items()) {
		auto f = dynamic_cast<ard::topic*>(i);
		if (f->items().size() > 0) {
			f->deleteEmptyTopics();
		}
	}
};

bool ard::topic::canAcceptChild(const snc::cit* it)const
{
	ASSERT_VALID(this);
	ASSERT_VALID(it);

	if (m_items.size() > 15000) {
		ASSERT(0, "too many subitems") << m_items.size() << dbgHint();
		return false;
	}

	if (it == this)
	{
		ASSERT(0, "NA") << dbgHint();
		return false;
	}

	auto f = dynamic_cast<const ard::topic*>(it);
	if (f)
	{
		if (f->isAncestorOf(this))
		{
			dbg_print(QString("parent-child cross ref %1 %2").arg(dbgHint()).arg(f->dbgHint()));
			return false;
		}
	}

	const RootTopic* r = dynamic_cast<const RootTopic*>(it);
	if (r)
	{
		dbg_print(QString("can't accept root").arg(dbgHint()).arg(r->dbgHint()));
		return false;
	}

	if (isGtdSortingFolder())
	{
		if (f != NULL)
		{
			if (f->isGtdSortingFolder())
			{
				dbg_print(QString("no nested GTD").arg(dbgHint()).arg(f->dbgHint()));
				return false;
			}
		}
	}

	/// custom sorter root contains only sorting folders
	if (folder_type() == EFolderType::folderUserSortersHolder)
	{
		if (f && f->folder_type() != EFolderType::folderUserSorter)
		{
			dbg_print(QString("only U-sorter inside U-holder").arg(dbgHint()).arg(f->dbgHint()));
			return false;
		}
	}
	if (folder_type() == EFolderType::folderUserSorter)
	{
		if (
			(f && f->isGtdSortingFolder()) ||
			(f->folder_type() == EFolderType::folderUserSorter))
		{
			dbg_print(QString("no nested custom folders").arg(dbgHint()).arg(f->dbgHint()));
			return false;
		}
	}

	return true;
};

bool ard::topic::canMove()const
{
    bool rv = snc::cit::canMove();
    if(rv)
        {
            rv = !isGtdSortingFolder();
        }
    return rv;
};

bool ard::topic::canBeMemberOf(topic_cptr parent_folder)const
{
	// if(!anItem::canBeMemberOf(parent_folder))
	//     return false;

	 /// user custom sorters can reside only
	 /// inside custom sorters root
	if (folder_type() == EFolderType::folderUserSorter)
	{
		if (parent_folder->folder_type() != EFolderType::folderUserSortersHolder)
		{
			return false;
		}
	}

	return true;
};


bool ard::topic::need_pindex_rebalancing()const
{
	const snc::cit* prev_it = nullptr;
	int prev_pindex = -1;

	int step = 0;
	for (auto i = m_items.begin(); i != m_items.end(); i++)
	{
		auto it = *i;
		int curr_pindex = it->pindex();
		if (curr_pindex == -1)
		{
			return true;
		}

		if (prev_it)
		{
			if (curr_pindex - prev_pindex < PINDEX_MIN_STEP)
			{
				qDebug() << "need rebalancing [2][index-step-limit-exceeded]" << curr_pindex << prev_pindex << step << it->id() << it->title();
				return true;
			}
		}

		prev_it = it;
		prev_pindex = curr_pindex;
		step++;
	}

	return false;
};

void ard::topic::rebalance_pindex()
{
    if (IS_VALID_DB_ID(id()) && dataDb())
    {
        if (need_pindex_rebalancing())
        {
#ifdef ARD_BETA
            int subitemIndexInvalid = 0;
            int subitemIndexValid = 0;
            for (auto& i : items()) {
                topic_ptr it = dynamic_cast<topic_ptr>(i);
                int curr_pindex = it->pindex();
                if (curr_pindex == -1)
                {
                    subitemIndexInvalid++;
                }
                else
                {
                    subitemIndexValid++;
                }
            }
            if (true)
            {
                /*
                    qDebug() << "pidx-rbl id=" << id()
                        << "sz=" << items().size()
                        << "invalid=" << subitemIndexInvalid
                        << title().left(64);
                        */
            }
#endif
            dataDb()->updateChildrenPIndexesInBatch(this);
        }
    }
};


bool isPersistantRoot(topic_ptr f)
{
    bool rv = f->isRootTopic() && 
        (f->otype() == objDataRoot || 
        f->otype() == objEThreadsRoot ||
        f->otype() == objContactRoot ||
        f->otype() == objContactGroupRoot ||
        f->otype() == objKRingRoot ||
        f->otype() == objBoardRoot ||
		f->otype() == objQFilterRoot);
    return rv;
}

bool ard::topic::ensurePersistantItem(int depth, EPersBatchProcess pbatch /*= persAll*/)
{
    Q_UNUSED(depth);
    ASSERT_VALID(this);

	assert_return_false(isPersistant(), "expected persistant item");

    ArdDB* dbd = dataDb();
    if (!dbd)
    {
        auto p = parent();
        if (!p) {
            ASSERT(0, "anItem::ensurePersistant - expected valid parent object ") << dbgHint();
            return false;
        }
        dbd = p->dataDb();
        if (!dbd) {
            ASSERT(0, "anItem::ensurePersistant - expected valid parent dataDB object") << parent()->dbgHint();
            return false;
        }        
    }

	assert_return_false(dbd, QString("expected dataDB: %1").arg(dbgHint()));


    if (!IS_VALID_DB_ID(m_id))
    {
        bool rv = dbd->storeNewTopic(this);
        if (rv)
        {
            ensure_ExtensionPersistance(dbd);
            mark_gui_4_comments_queried();
        }
        else
        {
            return false;
        }
    }
    else
    {
        if (need_persistance(np_ATOMIC_CONTENT))
        {
            if (!dbp::updateItemAtomicContent(this, dbd))
                return false;
        }

        if (need_persistance(np_POS))
        {
            if (!dbp::updateItemPOS(this, dbd))
                return false;
        }

        if (need_persistance(np_PINDEX))
        {
            if (!dbp::updateItemPIndex(this, dbd))
                return false;
        }

        if (isSynchronizable())
        {
            if (need_persistance(np_SYNC_INFO))
            {
                if (!dbp::updateItemSyncInfo(this, dbd))
                    return false;
            }
        }
    }

    ASSERT(IS_VALID_DB_ID(m_id), "expected valid DBID");

    if (pbatch & persExtension)
        ensure_ExtensionPersistance(dbd);

    return true;
};


bool ard::topic::ensurePersistant(int depth, EPersBatchProcess pbatch /*= persAll*/)
{
    if (!IS_VALID_DB_ID(id()))
    {
        for (auto& i : items()) {
            topic_ptr it = dynamic_cast<topic_ptr>(i);
            if (it->isPersistant())
            {
                it->ask4persistance(np_POS);
            }
        }
    };

    bool storeCompoundData = true;
    if (isRootTopic())
    {
        storeCompoundData = isPersistantRoot(this);
        ASSERT(dataDb() != NULL, "expected dataDB") << dbgHint();
    }

    if (storeCompoundData)
    {
        if (!ensurePersistantItem(depth, pbatch))
        {
            return false;
        }
    }

    if (depth == 0)
        return true;

    rebalance_pindex();

    for (auto& i : items()) {
        topic_ptr it = dynamic_cast<topic_ptr>(i);
        if (it->isPersistant())
        {
            if (!it->ensurePersistant(depth - 1, pbatch)) {
                ASSERT(0, "failed-to-store") << it->dbgHint();
            }
        }
    }

    return true;
};


void ard::topic::detachDB()
{
    m_data_db = nullptr;
    for (auto& i : m_items) {
        topic_ptr it = dynamic_cast<topic_ptr>(i);
        if (it)
        {
            it->detachDB();
        }
    }
};



bool ard::topic::init_from_db(ArdDB* db, QSqlQuery& q)
{
    if (!isPersistant()) 
    {
        ASSERT(0, "expected persistant item");
        return true;
    }
    m_id = q.value(0).toInt();
    m_pindex = q.value(2).toInt();
    m_title = q.value(4).toString();
    m_attr.flags = 0;
    m_attr.ToDo = q.value(5).toInt();
    m_attr.ToDoIsDone = q.value(6).toInt();
    //m_attr.Hotspot = (q.value(7).toInt() > 0 ? 1 : 0);
   // m_attr.Retired = q.value(7).toInt();
	serial2mflag(q.value(7).toULongLong());
    //m_font_prop.setFontData(q.value(8).toInt());
    m_syid = q.value(8).toString();
    m_mod_counter = q.value(9).value<COUNTER_TYPE>();
    m_move_counter = q.value(10).value<COUNTER_TYPE>();
    int ftype = q.value(11).toInt();
    if (ftype == 2 || ftype == 10) 
    {
        ///deprecated projects
        ftype = static_cast<int>(EFolderType::folderGeneric);
    }
    if (ftype < static_cast<int>(EFolderType::folderBoardTopicsHolder) + 1 && ftype > -1) {
        m_attr.FolderType = ftype;
    }
    else {
        ASSERT(0, "invalid DB folder type valule") << id();
        ftype = static_cast<int>(EFolderType::folderGeneric);
    }


    m_annotation = q.value(12).toString();
	m_mod_time = q.value(13).toInt();
    bool rv = db->attach_data_db(this);
    return rv;
};


void ard::topic::setup_id_for_new_local_topic(ArdDB* db, DB_ID_TYPE _id)
{
    assert_return_void(isPersistant(), "expected persistant item");
    m_id = _id;
    m_data_db = db;
    if (IS_VALID_DB_ID(m_id)){
        db->registerObjId(this);
    }
}

void ard::topic::setPindex(int val)const
{
    m_pindex = val;
    topic_ptr ThisNotConst = const_cast<ard::topic*>(this);
    ThisNotConst->ask4persistance(np_PINDEX);
};

void ard::topic::ensure_ExtensionPersistance(ArdDB* db)
{
    if (dataDb()) {
        for (auto& i : m_extensions)i->ensureExtPersistant(db);
    }
};

void ard::topic::setupRootDbItem(ArdDB* db, QSqlQuery& q)
{
    EOBJ _otype = (EOBJ)q.value(3).toInt();
    assert_return_void(_otype == otype(), QString("invalid object type provided %1, expected:%2").arg(_otype).arg(otype()));
    ASSERT(m_id == 0, "expected uninitialized root record");
    m_id = q.value(0).toInt();
    //now, we are changing id..not sure
    //attachDataDb(db);
    db->attach_data_db(this);

    m_mod_counter = q.value(11).value<COUNTER_TYPE>();
    m_move_counter = q.value(12).value<COUNTER_TYPE>();
};

void ard::topic::setupAsRoot(ArdDB* db, QString title)
{
    m_id = 0;
    m_attr.flags = 0;
    m_title = title;
    db->attach_data_db(this);
};

QPixmap ard::topic::getIcon(OutlineContext c)const
{
    if (c == OutlineContext::grep2list)
    {
        return QPixmap();
    }

    if (c == OutlineContext::check2select) {
        return isInLocusTab() ? getIcon_CheckedBox() : getIcon_UnCheckedBox();
    }


    QPixmap rv;
    bool hasIcon = true;
    if (hasIcon)
    {
		if (m_items.size() > 0)
        {
            rv = isExpanded() ? getIcon_TopicExpanded() : getIcon_TopicCollapsed();
        }
    }

    return rv;
};

void ard::topic::applyOnVisible(std::function<void(ard::topic*)> fnc) 
{
	for (auto& i : m_items)
	{
		auto f = dynamic_cast<ard::topic*>(i);
		if (f)fnc(f);
	}
};

ESingletonType ard::topic::getSingletonType()const
{
    ESingletonType rv = ESingletonType::none;
    switch(folder_type())
        {
        case EFolderType::folderRecycle:                rv = ESingletonType::gtdRecycle;break;
        case EFolderType::folderMaybe:                  rv = ESingletonType::gtdInkubator;break;
        case EFolderType::folderReference:              rv = ESingletonType::gtdReference;break;
        case EFolderType::folderDelegated:              rv = ESingletonType::gtdDelegated;break;
        case EFolderType::folderSortbox:                rv = ESingletonType::gtdSortbox;break;
        case EFolderType::folderUserSortersHolder:      rv = ESingletonType::UFoldersHolder;break;
        case EFolderType::folderBacklog:                rv = ESingletonType::gtdDrafts; break;
        case EFolderType::folderBoardTopicsHolder:      rv = ESingletonType::boardTopicsHolder; break;
        default:break;
        }
    return rv;
};

bool ard::topic::isGtdSortingFolder()const
{
    bool rv = false;
    switch(folder_type())
        {
        case EFolderType::folderRecycle:
        case EFolderType::folderMaybe:
        case EFolderType::folderReference:
        case EFolderType::folderDelegated:
        case EFolderType::folderSortbox:
        case EFolderType::folderUserSortersHolder:
        case EFolderType::folderBacklog:
        case EFolderType::folderBoardTopicsHolder:
            rv = true;break;
        default:break;
        }
    return rv;
};

bool ard::topic::canHaveCard()const
{
    bool rv = true;
    if(parent() == NULL || isRootTopic() || isGtdSortingFolder())
        {
            rv = false;
        }
    return rv;
};

ard::locus_folder* ard::topic::getLocusFolder()
{
/*    assert_return_null(gui::isDBAttached(), "expected DB connection");

    if (isGtdSortingFolder())
    {
        return this;
    }

    topic_ptr rv = nullptr;
	*/
    auto p = parent();
    if (p)
    {
        /*if (p->isGtdSortingFolder() || p->folder_type() == EFolderType::folderUserSorter)
        {
            return p;
        }
        p = p->parent();
		*/
		return p->getLocusFolder();
    }

    /*if (rv == NULL)
    {
        rv = dbp::root();
    }*/

    return nullptr;
};


QPixmap ard::topic::getSecondaryIcon(OutlineContext)const
{
#ifdef _DEBUG
    ASSERT_VALID(this);
#endif

    QPixmap rv;
    switch(folder_type())
        {
        case EFolderType::folderSortbox:            rv = getIcon_Sortbox();break;
        case EFolderType::folderRecycle:            rv = getIcon_Recycle();break;
        case EFolderType::folderMaybe:              rv = getIcon_Maybe();break;
        case EFolderType::folderReference:          rv = getIcon_Reference();break;
        case EFolderType::folderDelegated:          rv = getIcon_Delegated();break;
        case EFolderType::folderBacklog:            rv = getIcon_Drafts(); break;
        case EFolderType::folderBoardTopicsHolder:  rv = getIcon_BoardTopicsFolder(); break;
        default: break;
        }
    return rv;
};

topic_ptr ard::topic::findTopicByTitle(QString title)
{
    topic_ptr rv = nullptr;
    snc::cit* it = findByTitleNonRecursive(title);
    if (it != NULL)
    {
		rv = dynamic_cast<ard::topic*>(it);
    }
    return rv;
};

topic_ptr ard::topic::findTopicBySyid(QString syid)
{
    topic_ptr rv = nullptr;
    snc::cit* it = findBySyidNonRecursive(syid);
    if (it != NULL)
    {
		rv = dynamic_cast<ard::topic*>(it);
    }
    return rv;
};


std::pair<TernaryIconType, int> ard::topic::ternaryIconWidth()const
{
    std::pair<TernaryIconType, int> rv{TernaryIconType::none, 0};
    return rv;
};

void ard::topic::formatNotes(QTextCharFormat& frm)
{
    if (m_note_ext)
    {
        m_note_ext->queryGui();
        QString st = m_note_ext->html().trimmed();
        if (!st.isEmpty())
        {
            QTextDocument* d = m_note_ext->document();
            QTextCursor cr(d);
            cr.select(QTextCursor::Document);
            cr.mergeCharFormat(frm);
            m_note_ext->setNoteHtml(d->toHtml().trimmed(),
                d->toPlainText().trimmed());
        }
    }

    for (auto& i : items()) {
		auto it = dynamic_cast<ard::topic*>(i);
        it->formatNotes(frm);
    }
};

void ard::topic::takeAllItemsFrom(topic_ptr from)
{
    assert_return_void(from != this, "can't merge into itself");

    while(!from->items().empty())
        {
            CITEMS::iterator i = from->items().begin();
			auto it2 = dynamic_cast<ard::topic*>(*i);
            from->detachItem(it2);
            addItem(it2);
        }
};

FieldParts ard::topic::fieldValues(EColumnType column_type, QString )const
{
    FieldParts rv;
    switch (column_type) {
    case EColumnType::Title: 
    {
        rv.add(FieldParts::Title, impliedTitle());
    }break;
    case EColumnType::Annotation: 
    {       
        rv.add(FieldParts::Annotation, annotation());
    }break;
    default:break;
    }
    return rv;
};

QString ard::topic::fieldMergedValue(EColumnType column_type, QString )const
{
    QString rv;
    switch (column_type) 
    {
    case EColumnType::Title: 
    {
        rv = impliedTitle();
    }break;
    default:break;
    }
    return rv;
};

TOPICS_LIST ard::topic::produceFormTopics(std::set<EColumnType>*, std::set<EColumnType>*)
{
    TOPICS_LIST lst;
    ASSERT(0, "NA");
    return lst;
};

void ard::topic::setFieldValues(EColumnType column_type, QString, const FieldParts& fp)
{
    switch (column_type)
    {
    case EColumnType::Title:
    {
        assert_return_void((fp.parts().size() == 1), QString("expected one part for title %1").arg(fp.parts().size()));
        assert_return_void((fp.parts()[0].first == FieldParts::Title), QString("expected title part for title %1").arg(static_cast<int>(fp.parts()[0].first)));
        QString str = fp.parts()[0].second;
        if (title().compare(str) != 0) {
            setTitle(str, true);
        }
    }break;    
    case EColumnType::Annotation:
    {
        assert_return_void((fp.parts().size() == 1), QString("expected one part for annotation %1").arg(fp.parts().size()));
        assert_return_void((fp.parts()[0].first == FieldParts::Annotation), QString("expected annotation part for annotation %1").arg(static_cast<int>(fp.parts()[0].first)));
        QString str = fp.parts()[0].second;
        if (annotation().compare(str) != 0) {
            setAnnotation(str, true);
        }
    }break;    
    default:ASSERT(0, "NA");
    }
};

void ard::topic::setImportant(bool set_it) 
{
    setToDo(-1, set_it ? ToDoPriority::important : ToDoPriority::notAToDo);
};

union serial_flag {
	uint64_t flags;
	struct {
		unsigned		isRetired	: 1;
		unsigned		inLocus		: 1;
		unsigned		Color		: 3;
	};
};

uint64_t ard::topic::mflag4serial()const
{
	serial_flag f;
	f.flags = 0;
	f.isRetired = m_attr.isRetired;
	f.inLocus = m_attr.inLocus;
	f.Color = m_attr.Color;
	if (f.Color > static_cast<int>(EColor::purple)) {
		f.Color = static_cast<int>(EColor::none);
	}
	return f.flags;
};

void ard::topic::serial2mflag(uint64_t v)
{
	serial_flag f;
	f.flags = v;
	m_attr.isRetired = f.isRetired;
	m_attr.inLocus = f.inLocus;
	m_attr.Color = f.Color;
	if (m_attr.Color > static_cast<int>(EColor::purple)) {
		m_attr.Color = static_cast<int>(EColor::none);
	}
};


void ard::topic::setColorIndex(EColor c)
{
	m_attr.Color = ard::cidx2char(c);
    register_db_modification(this, dbmodRetired);
}

ard::EColor ard::topic::colorIndex()const
{
	return static_cast<ard::EColor>(m_attr.Color);
};

bool ard::topic::isRetired() const
{
	return (m_attr.isRetired == 1);
}

bool ard::topic::isInLocusTab()const
{
    return m_attr.inLocus;
};

void ard::topic::setRetired(bool val)
{
    if (!canChangeToDo())
        return;

	m_attr.isRetired = val ? 1 : 0;
    register_db_modification(this, dbmodRetired);
}


void ard::topic::setInLocusTab(bool val) 
{
	m_attr.inLocus = val ? 1 : 0;
    register_db_modification(this, dbmodRetired);
};


void ard::topic::markAsModified()
{
    register_db_modification(this, dbmodForceModified);
};

void ard::topic::query_gui_4_comments()const
{
    if (!canAttachNote())
        return;

    if (isNoteLoadedFomDB())
        return;

    if (m_note_ext) {
        m_note_ext->queryGui();
    }

    topic_ptr ThisNonConst = const_cast<ard::topic*>(this);
    ThisNonConst->mark_gui_4_comments_queried();
};

bool ard::topic::TitleLess(topic_ptr it1, topic_ptr it2)
{
    bool rv = (it1->impliedTitle() < it2->impliedTitle());
    return rv;
};

void ard::topic::mark_gui_4_comments_queried()
{
	m_attr.optNoteLoadedFromDB = 1;
};

#ifdef ARD_BETA
void ard::topic::printInfo(QString _title)const
{
    qWarning() << "=== dbg-info ====" << _title << "======";
    qWarning() << dbgHint();
    qWarning() << "title=" << title();
    qWarning() << "id=" << id();
    qWarning() << "syid=" << syid();
    qWarning() << "ToDo=" << m_attr.ToDo;
    qWarning() << "ToDoIsDone=" << m_attr.ToDoIsDone;
    topic_cptr p = parent();
    if (p != NULL)
    {
        qWarning() << "parent=" << p->objName() << " syid=" << p->syid() << " title=" << p->title();
        qWarning() << "ppos=" << p->indexOf(this);
    }

    qWarning() << "=== end-dbg-info ======================";
};

bool should_watch_syid(QString syid)
{
    static QString syid2watch = "2F5090F-2FCE9ED";

    bool rv = false;
    if (!syid2watch.isEmpty() &&
        syid.compare(syid2watch) == 0)
    {
        rv = true;
    }
    return rv;
};
#endif

#ifdef _DEBUG
static QString printIt = "";
void ard::topic::dbgPrintPrjInfo()
{
};

bool ard::topic::hasDebugMark1()const
{
	return false;// (m_attr.debugMark1 == 1);
};

void ard::topic::setDebugMark1(bool )
{
	//m_attr.debugMark1 = bval ? 1 : 0;
};
#endif

QString ard::topic::calcContentHashString()const
{
    if (!isSynchronizable())
        return "";

    QString tmp1 = QString("%1%2%3%4")
        .arg(title())
        .arg(annotation())
        .arg(getToDoPriorityAsInt())
        .arg(getToDoDonePercent());
    QString tmp = QCryptographicHash::hash((tmp1.toUtf8()), QCryptographicHash::Md5).toHex();
    ASSERT_VALID(this);
    topic_ptr ThisNonConst = const_cast<ard::topic*>(this);
    snc::CompoundObserver o(ThisNonConst);
    for(auto& i : o.prim_map()){
        const cit_primitive* p = i.second;
        ASSERT_VALID(p);
        QString s = p->calcContentHashString();
        tmp += s;
#ifdef _DEBUG
        if (!printIt.isEmpty())
        {
            qDebug() << printIt << s << tmp << p->dbgHint();
        }
#endif
    }
    QString rv = QCryptographicHash::hash((tmp.toUtf8()), QCryptographicHash::Md5).toHex();
    return rv;
};

QString ard::topic::calcMergeAtomicHash()const 
{
    QString rv = "";
    QString title_str = "", annotation_str = "", note_text_str = "";
    if (m_note_ext) {
        m_note_ext->queryGui();
        note_text_str = m_note_ext->html();
    }

    title_str = title().trimmed();
    annotation_str = annotation().trimmed();
    note_text_str = note_text_str.trimmed();

    if (title_str.isEmpty() && annotation_str.isEmpty() && note_text_str.isEmpty()) {
        return "";
    }

    QString tmp1 = QString("%1%2%3")
        .arg(title_str)
        .arg(annotation_str)
        .arg(note_text_str);

    rv = QCryptographicHash::hash((tmp1.toUtf8()), QCryptographicHash::Md5).toHex();
    return rv;
};

topic_ptr ard::topic::cloneInMerge()const
{
    topic_ptr rv = dynamic_cast<topic_ptr>(create());
    rv->m_attr.FolderType = static_cast<int>(folder_type());

    QString title_str = "", annotation_str = "";
    if (m_note_ext) {
        m_note_ext->queryGui();
        auto source_note = mainNote();
        auto dest_note = rv->mainNote();
        dest_note->setNoteHtml(source_note->html(), source_note->plain_text());
    }

    title_str = title().trimmed();
    annotation_str = annotation().trimmed();

    rv->m_title = title_str;
    rv->m_annotation = annotation_str;
    return rv;
};

bool ard::topic::skipMergeImport()const 
{
    return (m_attr.mergeImportSkipMark == 1);
};

void ard::topic::setSkipMergeImport(bool val)
{
    m_attr.mergeImportSkipMark = val ? 1 : 0;
};


uint64_t ard::topic::contentSize()const
{
    uint64_t rv = title().size() + annotation().size() + sizeof(this);
    return rv;
};

#define MAKE_IDENT_INFO(T, O)       if(T->isAtomicIdenticalTo(O, flags)) \
        identInfo = "[ident-OK]";                                       \
    else                                                                \
        identInfo = QString("[ident-ERROR %1]").arg(flags);             \



void ard::topic::printIdentityDiff(topic_ptr _other)
{
    QString identInfo, compountInfo;
    int flags = 0;

    MAKE_IDENT_INFO(this, _other);

    bool diff_found = false;
    typedef std::set<const cit_primitive*> CPRIM_SET;

    //ASSERT(0, "stop");

    CPRIM_SET processed_prim;
    snc::CompoundObserver o_this(this);
    snc::CompoundObserver o_other(_other);
    compountInfo = QString("[o=%1/%2]").arg(o_this.prim_list().size()).arg(o_other.prim_list().size());
    //printIt = "h1";
    QString sh1 = calcContentHashString();
    //printIt = "h2";
    QString sh2 = _other->calcContentHashString();

    qDebug() << "[diff]" << compountInfo << identInfo << dbgHint() << sh1 << "|" << _other->dbgHint() << sh2;

    for(const auto p : o_this.prim_list())
    {
        QString cid = p->compoundSignature();
        STRING_2_PRIM::const_iterator j = o_other.prim_map().find(cid);
        if (j != o_other.prim_map().end())
        {
            const cit_primitive* p2 = j->second;
            processed_prim.insert(p2);
            if (p->calcContentHashString().compare(p2->calcContentHashString()) != 0)
            {
                MAKE_IDENT_INFO(p, p2);
                qDebug() << "[diff][c]" << identInfo << p->dbgHint() << p->calcContentHashString() << "|" << p2->dbgHint() << p2->calcContentHashString();
                diff_found = true;
            }
        }
        else
        {
            qDebug() << "[diff][c][missed-in-remote]" << p->dbgHint();
            diff_found = true;
        }
    }

    for(const auto p : o_other.prim_list())
    {
        CPRIM_SET::const_iterator j = processed_prim.find(p);
        if (j != processed_prim.end())
        {
            QString cid = p->compoundSignature();
            STRING_2_PRIM::const_iterator k = o_this.prim_map().find(cid);
            if (k != o_this.prim_map().end())
            {
                const cit_primitive* p2 = k->second;
                if (p->calcContentHashString().compare(p2->calcContentHashString()) != 0)
                {
                    MAKE_IDENT_INFO(p, p2);
                    qDebug() << "[diff][c]" << identInfo << p->dbgHint() << p->calcContentHashString() << "|" << p2->dbgHint() << p2->calcContentHashString();
                    diff_found = true;
                }
            }
            else
            {
                qDebug() << "[diff][c][missed-in-local]" << p->dbgHint();
                diff_found = true;
            }
        }//processed
    }

    if (diff_found)
    {
        qDebug() << "[diff][c]end";
    }
};

void ard::topic::makeTitleFromNote()
{
    if (m_note_ext)
    {
        m_note_ext->queryGui();

        QString stext = m_note_ext->plain_text();
        int nline = stext.indexOf(QRegExp("[\r\n]"));
        if (nline != -1)
        {
            stext = stext.mid(0, nline);
        }

        if (stext.length() > 80)
        {
            nline = stext.indexOf(QRegExp("\\b"), 80);
            if (nline != -1)
            {
                stext = stext.mid(0, nline);
            }
        }

        stext = stext.trimmed();
        qDebug() << "setting title to" << stext;
        setTitle(stext);
    }
};

topic_ptr ard::topic::createNewNote(QString _title /*= ""*/, QString _html /*= ""*/, QString _plain_text /*= ""*/)
{
    topic_ptr rv = new ard::topic(_title);
    auto e = rv->ensureNote();
    if (e) {
        e->setNoteHtml(_html, _plain_text);
    }
    return rv;
};

bool ard::topic::hasNote()const 
{
    return (m_note_ext != nullptr);
};

ard::note_ext* ard::topic::mainNote()
{
    auto rv = ensureNote();
    if (rv) {
        rv->queryGui();
    }
    return rv;
};

const ard::note_ext* ard::topic::mainNote()const
{
    ard::topic* ThisNonCost = (ard::topic*)this;
    auto rv = ThisNonCost->ensureNote();
    if (rv) {
        rv->queryGui();
    }
    return rv;
};

ard::note_ext* ard::topic::ensureNote()
{
    ASSERT_VALID(this);
    if (m_note_ext)
        return m_note_ext;

    ensureExtension(m_note_ext);
    return m_note_ext;
};

QString ard::topic::plainNoteText4Blackboard()const
{
    QString rv;
    if (m_note_ext) {
        m_note_ext->queryGui();
        rv = m_note_ext->plain_text4draw();
    }
    return rv;
};

topic_ptr ard::topic::prepareInjectIntoBBoard() 
{
   // qDebug() << dbgHint("prepareInjectIntoBBoard");

    topic_ptr rv = shortcutUnderlying();
    if (rv) {
        if (!IS_VALID_SYID(syid())) {
            auto db = rv->dataDb();
            assert_return_null(db, "expected DB");
            rv->guiEnsureSyid(db, "B");
            if (!rv->ensurePersistant(0)) {
                return nullptr;
            };
        }
    }
    return rv;
};

bool ard::topic::isExpandableInBBoard()const 
{
    return true;
};

QSize ard::topic::calcBlackboardTextBoxSize()const 
{
    auto s = impliedTitle().trimmed();
    if (s.isEmpty()) {
        s = "W";
    }

    static const int maxHInLines = 7;

    QSize rv{ 0,0 };
    QRect rc(0, 0, BBOARD_MAX_BOX_WIDTH, 3 * gui::lineHeight());
    QFontMetrics fm(*ard::defaultSmallFont());
    QRect rc2 = fm.boundingRect(rc, Qt::TextWordWrap, s);
    auto lineH = gui::lineHeight();
    auto h = rc2.height();
    if (h > maxHInLines * lineH)
        h = maxHInLines * lineH;
    if (h < lineH)
        h = lineH;
    rc2.setHeight(h);
    rv = rc2.size();
    const int min_w = gui::lineHeight() * 3;
    if (rv.width() < min_w) {
        rv.setWidth(min_w);
    }

    return rv;
};

QString asHtmlDocument(QString sPlainText)
{
    QString sHtml = QString("<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\" \"http://www.w3.org/TR/REC-html40/strict.dtd\"> <html><head><meta name=\"qrichtext\" content=\"1\" /><style type=\"text/css\"> p, li { white-space: pre-wrap; } </style></head><body style=\" font-family:'Sans Serif'; font-size:9pt; font-weight:400; font-style:normal;\"> <p style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\">%1</p></body></html>").arg(sPlainText);
    return sHtml;
};


void ard::topic::setMainNoteText(QString sPlainText)
{
	if (!canAttachNote()) {
		ASSERT(0, "can't attach note to") << dbgHint();
		return;
	}

    QString sHtml = asHtmlDocument(sPlainText);
    mainNote()->setNoteHtml(sHtml, sPlainText);
};

/*
bool ard::topic::hasAllLabels(uint64_t data)const
{
    static uint64_t theone = 1;
    bool rv = true;
    if (data != 0) {
        uint64_t label_mask = 0;
        if (isImportant()) {
            label_mask = theone;
        }
        if (isStarred()) {
            int starred_base = static_cast<int>(googleQt::mail_cache::SysLabel::STARRED);
            label_mask |= (theone << starred_base);
        }

        uint64_t f = data & label_mask;
        rv = (data == f);
    }
    return rv;
};*/

bool ard::topic::IsUtilityFolder()const
{
    bool rv = isSingleton() ||
        isGtdSortingFolder() ||
        folder_type() == EFolderType::folderUserSorter;
    return rv;
};

bool ard::topic::canHaveCryptoNote()const 
{
    bool rv = (folder_type() == EFolderType::folderGeneric);
    return rv;
};

QString ard::topic::impliedTitle()const
{
    QString rv = title();

    if (!rv.isEmpty())
        return rv;

    if (m_note_ext) 
    {
        m_note_ext->queryGui();
        QString text2draw = m_note_ext->plain_text4draw();
        if (text2draw.isEmpty()) {
            return "";
        }
        else {
            rv = "'";
            rv += text2draw;
        }
    }
    return rv;
};

QString ard::topic::altShortTitle()const
{
    QString rv = "----";
    auto stitle = impliedTitle();
    if (!stitle.isEmpty()) 
	{
		if (stitle.size() > 16) {
			auto idx = stitle.indexOf(" ", 16);
			if (idx != -1) {
				stitle += "..";
			}
		}
		/*
        auto slst = stitle.split(" ");
        QString tmp = slst.at(0);
        if (tmp.length() < 3 && slst.size() > 1) {
            tmp = slst.at(0) + " " + slst.at(1);
        }*/
        rv = stitle;
    }
    return rv;
};

bool ard::topic::hasText4SearchFilter(const TextFilterContext& fc)const
{
    bool rv = (title().indexOf(fc.key_str, 0, Qt::CaseInsensitive) != -1) ||
        (annotation().indexOf(fc.key_str, 0, Qt::CaseInsensitive) != -1);
    if (!rv && fc.include_expanded_notes) {
        if (m_note_ext) 
        {
            m_note_ext->queryGui();
            auto s = m_note_ext->plain_text4draw();
            rv = s.indexOf(fc.key_str, 0, Qt::CaseInsensitive) != -1;
        }
    }
    return rv;
};


void ard::topic::mapExtensionVar(cit_extension_ptr e)
{
    assert_return_void(e, "expected extension");
    switch (e->ext_type()) {
    case snc::EOBJ_EXT::extNote: 
    {
        ASSERT(!m_note_ext, "duplicate note-ext");
        m_note_ext = dynamic_cast<ard::note_ext*>(e);
        ASSERT(m_note_ext, "expected note-ext");
    }break;
    default:break;
    }
};

void ard::topic::toJson(QJsonObject& js)const 
{
    js["otype"] = QString("%1").arg(otype());
    js["ftype"] = QString("%1").arg(m_attr.FolderType);
    js["title"] = impliedTitle();
    auto str = impliedTitle();
    str.remove(QRegExp("[^a-zA-Z\\d\\s]"));
    js["hint"] = str;
    js["annotation"] = annotation();
    js["todo"] = QString("%1").arg(getToDoDonePercent());
    js["color"] = static_cast<int>(colorIndex());
    auto n = mainNote();
    if (n && !n->html().isEmpty()) {
        if (!n->html().isEmpty()) {
            QByteArray ba(n->html().toUtf8());
            QString str = ba.toBase64();
            js["html-note"] = str;
        }
        if (!n->plain_text().isEmpty()) {
            QByteArray ba(n->plain_text().toUtf8());
            QString str = ba.toBase64();
            js["plain-note"] = str;
        }

    }
    QJsonArray jarr;
    auto tlst = itemsAs<ard::topic>();
    for (auto& f : tlst) {
        if (EFolderType::folderGeneric != f->folder_type()) {
            if (f->items().empty())
                continue;
        }

        QJsonObject js2;
        f->toJson(js2);
        jarr.append(js2);
    }
    js["items"] = jarr;
};

void ard::topic::fromJson(const QJsonObject& js)
{
    //auto s_otype = js["otype"].toString();
    auto s_ftype = js["ftype"].toString();
    m_title = js["title"].toString();
    m_annotation = js["annotation"].toString();
    int tmp = js["todo"].toInt();
    if (tmp > 0 && tmp < 100) {
        setToDo(tmp, ToDoPriority::normal);
    }
    tmp = js["color"].toInt();
    if (tmp > 0 && tmp < 5) {
		EColor c = static_cast<EColor>(tmp);
        setColorIndex(c);
    }
    QString s_html, s_plain;
    auto ba_html = QByteArray::fromBase64(js["html-note"].toString().toLatin1());
    auto ba_plain = QByteArray::fromBase64(js["plain-note"].toString().toLatin1());
    if (ba_html.size() > 0) {
        s_html = ba_html;
        s_plain = ba_plain;
        s_html = s_html.trimmed();
        if (!s_html.isEmpty()) {
            auto n = mainNote();
            if (n) {
                n->setNoteHtml(s_html, s_plain);
            }
        }
    }

    int n_ftype = s_ftype.toInt();
    if (n_ftype >= 0 && n_ftype < 11) {
        m_attr.FolderType = n_ftype;
    }

    auto arr = js["items"].toArray();
    int Max = arr.size();
    for (int i = 0; i < Max; ++i) {
        auto js2 = arr[i].toObject();
        ard::topic* f = new ard::topic();
        f->fromJson(js2);
        addItem(f);
    }
};

void ard::topic::guiEnsureSyid(const ArdDB* db, QString prefix)
{
    if (!IS_VALID_SYID(syid())) {
        assert_return_void(db, "expected valid DB");

        auto rdb = db->getSyncCounters4NewRegistration();

        const SYID2TOPIC& registered_syid = db->syid2topic();
        m_syid = prefix + "-" + make_new_syid(rdb.first);
        auto i = registered_syid.find(m_syid);
        if (i != registered_syid.end()) {
            int idx = 1;
            while (i != registered_syid.end()) {
                qWarning() << "duplicate syid on generation" << m_syid << idx;
                m_syid = prefix + "-" + make_new_syid(rdb.first) + QString("D%1").arg(idx);
                i = registered_syid.find(m_syid);
                idx++;
            }
        }
        //setSyncModified();
        m_mod_counter = rdb.second;
        ask4persistance(np_SYNC_INFO);
        db->registerSyid(this);
    }
};

bool ard::topic::rollUpToRoot()
{
	bool _rebuildRequired = false;
	auto f = parent();
	if (f) {
		while (f) {
			if (!f->isExpanded()) {
				_rebuildRequired = true;
				f->setExpanded(true);
			}
			f = f->parent();
		}
	}

	return _rebuildRequired;
};

topic_ptr ard::topic::gtdGetSortingFolder()
{
	if (isGtdSortingFolder() || isRootTopic())
	{
		return this;
	}

	topic_ptr p2 = parent();
	while (p2)
	{
		if (p2->isGtdSortingFolder() || p2->isRootTopic())
		{
			return p2;
		}
		p2 = p2->parent();
	}
	return nullptr;
};

topic_cptr ard::topic::gtdGetSortingFolder()const
{
	topic_ptr ThisNonConst = const_cast<ard::topic*>(this);
	topic_ptr rv = ThisNonConst->gtdGetSortingFolder();
	return rv;
};


void ard::topic::setToDo(int done_percent, ToDoPriority prio)
{
	if (!canChangeToDo())
		return;

	if (done_percent == -1 && prio == ToDoPriority::unknown) {
		ASSERT(0, "nothing to change for ToDo");
		return;
	}

	unsigned int_prio = static_cast<unsigned>(prio);
	if ((m_attr.ToDoIsDone == static_cast<unsigned>(done_percent) || done_percent == -1) &&
		(int_prio == m_attr.ToDo || prio == ToDoPriority::unknown))
	{
		return;
	}

	//if (m_attr.ToDo == priority && m_attr.ToDoIsDone == (unsigned)done_percent)
	//return;
	if (prio != ToDoPriority::unknown) {
		m_attr.ToDo = int_prio;
	}

	if (done_percent > -1 && done_percent < 101)
	{
		m_attr.ToDoIsDone = done_percent;
		if (done_percent > 0)
		{
			if (m_attr.ToDo == 0) {
				m_attr.ToDo = 1;
			}
		}
	}
	register_db_modification(this, dbmodToDo);
}

std::pair<char, char> ard::topic::getColorHashIndex()const
{
	std::pair<char, char> rv = { 0,0 };
	rv.first = m_attr.colorHashIndex;
	rv.second = m_attr.colorHashChar;
	return rv;
};

bool ard::topic::isToDo() const
{
	return (m_attr.ToDo > 0);
}

bool ard::topic::isToDoDone()const
{
	return (m_attr.ToDoIsDone > 99);
}

int ard::topic::getToDoDonePercent()const
{
	int rv = m_attr.ToDoIsDone;
	if (rv > 100)
		rv = 100;
	return rv;
};


bool ard::topic::hasToDoPriority()const
{
	bool rv = (m_attr.ToDo > 1 && !isToDoDone());
	return rv;
};

ToDoPriority ard::topic::getToDoPriority()const
{
	ToDoPriority rv = ToDoPriority::notAToDo;
	switch (m_attr.ToDo)
	{
	case 1: rv = ToDoPriority::normal; break;
	case 2: rv = ToDoPriority::important; break;
	case 3: rv = ToDoPriority::critical; break;
	}
	return rv;
};

unsigned char ard::topic::getToDoPriorityAsInt() const
{
	return m_attr.ToDo;
}

void ard::topic::setTmpSelected(bool v)
{
	m_attr.isTmpSelected = v ? 1 : 0;
};

/**
	thumb_topic
*/
ard::thumb_topic::thumb_topic(QString title) :ard::topic(title) 
{
	m_thumb_flags.flags = 0;
};

QString	ard::thumb_topic::thumbnailFileName()const
{
	auto s = QString("%1%2-thumb-%3.png").arg(ard_dir(dirTMP)).arg(thumbnailFileNamePrefix()).arg(syid());
	return s;
};

const QPixmap* ard::thumb_topic::thumbnail()const
{
	if (!m_thumb_flags.isThumbnailQueried) {
		m_thumb_flags.isThumbnailQueried = 1;
		auto s = thumbnailFileName();
		if (QFile::exists(s)) {
			m_thumbnail.load(s);
		}
		else {
			m_thumbnail = emptyThumb();
			m_thumb_flags.isThumbnailEmpty = 1;
		}
	}

	return &m_thumbnail;
}

void ard::thumb_topic::setThumbnail(QPixmap pm)const
{
	m_thumbnail = pm.scaled(MP1_WIDTH, MP1_HEIGHT, Qt::KeepAspectRatio, Qt::SmoothTransformation);
	auto s = thumbnailFileName();
	if (!m_thumbnail.isNull()) {
		m_thumb_flags.isThumbnailQueried = 1;
		m_thumb_flags.isThumbnailEmpty = 0;
		m_thumbnail.save(s);
	}
	m_attr.isThumbDirty = 0;
	auto ThisNonConst = (ard::thumb_topic*)this;
	ard::asyncExec(AR_UpdateGItem, ThisNonConst);
};

void ard::thumb_topic::removeExternalStorage() 
{
	auto s = thumbnailFileName();
	if (QFile::exists(s))
	{
		if (!QFile::remove(s))
			qDebug() << "failed to delete thumbail" << s;
	}
};

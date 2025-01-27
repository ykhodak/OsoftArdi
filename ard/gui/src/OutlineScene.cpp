#include <QLineEdit>

#include "OutlineScene.h"
#include "OutlineView.h"
#include "OutlinePanel.h"
#include "anfolder.h"
#include "anGItem.h"
#include "ardmodel.h"
#include "TablePanel.h"
#include "ProtoTool.h"
#include "ContactFormPanel.h"
#include "ContactGroupPanel.h"
#include "ansyncdb.h"
#include "email.h"
#include "email_draft.h"
#include "contact.h"
#include "ansearch.h"
#include "rule.h"
#include "rule_runner.h"
#include "kring.h"
#include "board.h"
#include "locus_folder.h"

#define ADD_TOOL_GBUTTON(C, L, I)      if(true)     \
        {                                           \
            g = new ProtoToolItem(o_p);             \
            ProtoToolItem::BUTTONS_SET bset;        \
            ProtoToolItem::SetButton sb2(C, L, I);  \
            bset.push_back(sb2);                    \
            g->setButtonsSet(bset);                 \
            o_p->registerGI(g->topic(), g);         \
            g->setPos(o_p->panelIdent(), y_pos);    \
            y_pos += g->boundingRect().height();    \
        }

/**
   OutlineScene
*/
OutlineScene::OutlineScene(OutlineView* _view)
    :OutlineSceneBase(_view)
{
};

void OutlineScene::doRebuild()
{
#define ADD_CASE(C, F)case C:{F();}break;

    EOutlinePolicy p = outlinePolicy();
    switch(p)
        {
        case outline_policy_Pad:
        case outline_policy_PadEmail:
            {
                rebuildAsOutline();
            }break;

        ADD_CASE(outline_policy_TaskRing, rebuildAsTaskRing);
        ADD_CASE(outline_policy_Notes, rebuildAsNotesList);
		ADD_CASE(outline_policy_Bookmarks, rebuildAsBookmarksList);
		ADD_CASE(outline_policy_Pictures, rebuildAsPicturesList);
        ADD_CASE(outline_policy_Annotated, rebuildAsAnnotatedList);
        ADD_CASE(outline_policy_Colored, rebuildAsColoredList);
        ADD_CASE(outline_policy_2SearchView, rebuildAs2SearchView);
        ADD_CASE(outline_policy_KRingTable, rebuildAsKRingTableView);
        ADD_CASE(outline_policy_KRingForm, rebuildAsKRingFormView);
        ADD_CASE(outline_policy_BoardSelector, rebuildAsBoardSelectorView);
        case outline_policy_Uknown:break;
        default:ASSERT(0, "undefined outline policy type") << p; break;
        }  

#undef ADD_CASE
};

void OutlineScene::rebuildAsOutline()
{
    auto f = hoisted();
    assert_return_void(f, "expected hoisted folder");
    if (f == ard::Backlog()){
        f->deleteEmptyTopics();
    }

    EOutlinePolicy pol = outlinePolicy();
    switch(pol)
        {
        case outline_policy_Pad:rebuildTopicPanel<OutlinePanel>(pol, f);break;
        case outline_policy_PadEmail:rebuildEmailTopicPanel<OutlinePanel>(pol); break;
        default:ASSERT(0, "unexpected policy type");break;
        }
};

void OutlineScene::rebuildAs2SearchView()
{
    auto st = ard::db()->local_search();
    if (st) {
        outlineListPanel(outline_policy_2SearchView, st);
    }
}

void OutlineScene::rebuildAsBookmarksList()
{
	auto st = ard::db()->bookmarks();
	if (st) {
		outlineListPanel(outline_policy_Bookmarks, st);
	}
}

void OutlineScene::rebuildAsPicturesList()
{
	auto st = ard::db()->pictures();
	if (st) {
		outlineListPanel(outline_policy_Pictures, st);
	}
}

void OutlineScene::rebuildAsTaskRing() 
{
    auto st = ard::db()->task_ring();
    if (st) {
        outlineListPanel(outline_policy_TaskRing, st);
    }
};

void OutlineScene::rebuildAsAnnotatedList()
{
    auto st = ard::db()->annotated();
    if (st) {
        outlineListPanel(outline_policy_Annotated, st);
    }
};

void OutlineScene::rebuildAsNotesList()
{
    auto st = ard::db()->notes();
    if (st) {
        outlineListPanel(outline_policy_Notes, st);
    }
};

void OutlineScene::rebuildAsColoredList() 
{
    auto st = ard::db()->colored();
    if (st) {
        outlineListPanel(outline_policy_Colored, st);
    }
};


template <class O> void OutlineScene::rebuildTopicPanel(EOutlinePolicy pol, topic_ptr f)
{
    assert_return_void(f, "expected hoisted folder");

    if (!hasPanel())
    {
        O* p = new O(this);
        std::set<ProtoPanel::EProp> p2set, p2rem;

        SET_PPP(p2set, PP_Annotation);
		SET_PPP(p2set, PP_Thumbnail);

        if (pol == outline_policy_Pad)
        {
            SET_PPP(p2set, PP_FatFingerSelect);
        }
        p->setProp(&p2set, &p2rem);
        addPanel(p);
        p->applyParamMap();

        QSize sz = v()->v()->size();
        int w = sz.width();
        if (w > 0)
            p->setPanelWidth(w);
    }

    if (!hasPanel()) {
        assert_return_void(0, "expected valid panels in scene");
    }

    clearOutline();

    qreal y_pos = 0.0;
    O* o_p = dynamic_cast<O*>(panel());
    assert_return_void(o_p, "expected outline panel");
    assert_return_void(o_p->outlined().size() == 0, QString("expected clean panel:%1").arg(o_p->outlined().size()));
    if (!dbp::configFileSuppliedDemoDBUsed()) 
    {
        if (f == ard::Sortbox())
        {
            bool use_supplied = (f->items().size() == 0);

#ifdef _DEBUG
            if (!use_supplied) 
                use_supplied = true;
#endif

            if (use_supplied)
            {
                ProtoToolItem* g = nullptr;
                ADD_TOOL_GBUTTON(AR_ImportSuppliedOutline, "Import Demo Outline", ":ard/images/unix/button-add.png");
            }
        }
    }
    o_p->outlineTopic(f, y_pos);
    restoreMSelected();
};

template <class O> void OutlineScene::rebuildEmailTopicPanel(EOutlinePolicy pol)
{
    if (!ard::isDbConnected()) {
        return;
    }

    auto rr = ard::db()->gmail_runner();
    assert_return_void(rr, "expected gmail rule runner");

    if (!hasPanel())
    {
        O* p = new O(this);
        std::set<ProtoPanel::EProp> p2set, p2rem;
        SET_PPP(p2set, PP_Annotation);
        SET_PPP(p2set, PP_Labels);
        SET_PPP(p2set, PP_FatFingerSelect);
        p->setProp(&p2set, &p2rem);
        addPanel(p);
        p->applyParamMap();

        QSize sz = v()->v()->size();
        int w = sz.width();
        if (w > 0)p->setPanelWidth(w);
    }

    if (!hasPanel()) {
        assert_return_void(0, "expected valid panels in scene");
    }

    clearOutline();

    O* o_p = dynamic_cast<O*>(panel());
    assert_return_void(o_p, "expected outline panel");
    assert_return_void(o_p->outlined().size() == 0, QString("expected clean panel:%1").arg(o_p->outlined().size()));

    bool g_ok = ard::isGoogleConnected();
    if (!g_ok) {
        qreal y_pos = 0.0;
        ProtoToolItem* g = nullptr;
        g = new ProtoToolItem(o_p);
        g->setPixmap(QPixmap(":ard/images/unix/btn_google_light_normal_ios.png"), "Sign in with Google");
        g->setCommand(AR_GoogleConditianalyGetNewTokenAndConnect);
        o_p->registerGI(g->topic(), g);
        g->setPos(o_p->panelIdent(), y_pos);
        return;
    }

    if (pol == outline_policy_PadEmail)
    {
        if (!rr->isDefinedQ())
        {
            void rerun_q_param();
			rerun_q_param();
        }

        TOPICS_LIST lst;
        auto f = rr->outlineCompanion();
        if (f) {
            lst.push_back(f);
        }
        lst.push_back(rr);       

        o_p->outlineTopicsList(lst, 0);
		/*auto r = rr->rule();
		if (r) {
			if (r->isFilterTarget()) {
				HudButton* h = nullptr;
				h = new HudButton(this, HudButton::idFilter, "", ":ard/images/unix/search-filter.png");
				addTopRightHudButton(h);
			}
		}*/
    }
};

void OutlineScene::outlineListPanel(EOutlinePolicy pol, topic_ptr f)
{
    assert_return_void(f, "expected hoisted folder");
    Q_UNUSED(pol);

    if (!hasPanel())
    {
        auto p = new OutlinePanel(this);
        p->setOContext(OutlineContext::grep2list);
        std::set<ProtoPanel::EProp> p2set, p2rem;
        SET_PPP(p2set, PP_Annotation);

        //if (dbp::configFileGetDrawDebugHint()) 
        {
            SET_PPP(p2set, PP_Labels);
        }

        SET_PPP(p2set, PP_FatFingerSelect);
		SET_PPP(p2set, PP_Thumbnail);

        if (pol == outline_policy_TaskRing) {
            SET_PPP(p2set, PP_ActionButton);
        }

        if (pol == outline_policy_Notes) {
            SET_PPP(p2set, PP_ExpandNotes);
        }

        p->setProp(&p2set, &p2rem);
        addPanel(p);
        p->applyParamMap();

        QSize sz = v()->v()->size();
        int w = sz.width();
        if (w > 0)
            p->setPanelWidth(w);
    }

    if (!hasPanel()) {
        assert_return_void(0, "expected valid panels in scene");
    }

    clearOutline();

    auto o_p = dynamic_cast<OutlinePanel*>(panel());
    assert_return_void(o_p, "expected outline panel");
    assert_return_void(o_p->outlined().size() == 0, QString("expected clean panel:%1").arg(o_p->outlined().size())); 
    o_p->outlineTopicAsList(f);
    restoreMSelected();
};

void OutlineScene::rebuildAsContactTableView() 
{
    if (!hasPanel())
    {
        TablePanel* tp = new TablePanel(this);
        tp->addColumn(EColumnType::ContactTitle);
#ifdef ARD_BIG
        tp->addColumn(EColumnType::ContactEmail);
        tp->addColumn(EColumnType::ContactPhone);
        tp->addColumn(EColumnType::ContactAddress);
#endif //ARD_BIG

        std::set<ProtoPanel::EProp> p2remove;
        tp->setProp(nullptr, &p2remove);
        
        std::set<ProtoPanel::EProp> p2set;
        SET_PPP(p2set, PP_FatFingerSelect);
        tp->setProp(&p2set, nullptr);

        addPanel(tp);        
    }
    clearOutline();

    TablePanel* p = dynamic_cast<TablePanel*>(panel());
    assert_return_void(p, "expected contact panel");

    if (ard::root()){
        TOPICS_LIST selected;
        if (ard::isDbConnected()) {
            auto cr = ard::db()->cmodel()->croot();
            selected = cr->filteredItems(dbp::configFileContactHoistedGroupSyId(), dbp::configFileContactIndexStr());
        }
        std::sort(selected.begin(), selected.end(), ard::topic::TitleLess);
#ifdef _DEBUG
        TOPICS_SET selset;
        for (auto& j : selected) {
            auto k = selset.find(j);
            if (k != selset.end()) {
                ASSERT(0, "duplicate obj in list detected") << j->dbgHint();
            }
            else {
                selset.insert(j);
            }
        }
#endif            
        p->rebuildAsListPanel(selected);
    }
};


void OutlineScene::build_single_contact(OutlineScene* s, ard::contact* c)
{
	s->freePanels();
	auto p = new FormPanel(s);
	std::set<ProtoPanel::EProp> p2remove;
	p->setProp(nullptr, &p2remove);
	s->addPanel(p);
	p->build_form4topic(c);
};

void OutlineScene::rebuildAsContactGroupView() 
{
    if (!hasPanel()){
        ContactGroupPanel* tp = new ContactGroupPanel(this);
        addPanel(tp);
    }

    clearOutline();

    if (hasPanel()) {
        panel()->rebuildPanel();
    }
};

void OutlineScene::rebuildAsKRingTableView() 
{
    if (!hasPanel())
    {
        TablePanel* tp = new TablePanel(this);
        tp->addColumn(EColumnType::KRingTitle);
#ifdef ARD_BIG
        tp->addColumn(EColumnType::KRingLogin);
        tp->addColumn(EColumnType::KRingPwd);
#endif
        std::set<ProtoPanel::EProp> p2remove;
        tp->setProp(nullptr, &p2remove);

        std::set<ProtoPanel::EProp> p2set;
        SET_PPP(p2set, PP_FatFingerSelect);
        tp->setProp(&p2set, nullptr);

        addPanel(tp);
    }
    clearOutline();

    TablePanel* p = dynamic_cast<TablePanel*>(panel());
    assert_return_void(p, "expected contact panel");

    if (ard::root()) {
        TOPICS_LIST selected;
        if (ard::isDbConnected()) {
            auto cr = ard::db()->kmodel()->keys_root();
            selected = cr->filteredItems(/*dbp::configFileContactIndexStr()*/"*");
        }
        std::sort(selected.begin(), selected.end(), ard::topic::TitleLess);
#ifdef _DEBUG
        TOPICS_SET selset;
        for (auto& j : selected) {
            auto k = selset.find(j);
            if (k != selset.end()) {
                ASSERT(0, "duplicate obj in list detected") << j->dbgHint();
            }
            else {
                selset.insert(j);
            }
        }
#endif            
        p->rebuildAsListPanel(selected);
    }
};

void OutlineScene::rebuildAsKRingFormView() 
{
    if (!hasPanel())
    {
        auto tp = new FormPanel(this);
        std::set<ProtoPanel::EProp> p2remove;
        tp->setProp(nullptr, &p2remove);
        addPanel(tp);
    }


    clearOutline();
    auto p = dynamic_cast<FormPanel*>(panel());
    assert_return_void(p, "expected contact panel");
    auto k = model()->kringInFormView();
    if (!k) {
        ASSERT(0, "expected kring");
        return;
    }

    p->build_form4topic(k);
};

OutlinePanel* OutlineScene::prepareSelectorPanel()
{
    freePanels();
    OutlinePanel* p = nullptr;
    p = new OutlinePanel(this);

    auto so_context = enforced_ocontext();
    if (so_context == OutlineContext::none) {
        p->setOContext(OutlineContext::check2select);
    }
    else {
        p->setOContext(so_context);
    }
    std::set<ProtoPanel::EProp> p2set1, p2rem;
    SET_PPP(p2rem, PP_ToDoColumn);
    SET_PPP(p2rem, PP_CurrSpot);
    //SET_PPP(p2set1, PP_CustomHotspot);

    p->setProp(&p2set1, &p2rem);

    addPanel(p);
    p->applyParamMap();

    QSize sz = v()->v()->size();
    int w = sz.width();
    if(w > 0)
        p->setPanelWidth(w);

    return p;
};

void OutlineScene::build_folders(OutlineScene* s)
{
    OutlinePanel* o_p = s->prepareSelectorPanel();
    assert_return_void(o_p, "expected outline panel");

    o_p->setIncludeRoot(false);

	TOPICS_LIST list2outline;
    auto inbox = ard::Sortbox();
    assert_return_void(inbox, "expected 'sortbox' folder");

    auto ref = ard::Reference();
    assert_return_void(ref, "expected 'reference' folder");
    auto maybe = ard::Maybe();
    assert_return_void(maybe, "expected 'maybe' folder");
    auto btopics = ard::BoardTopicsHolder();
    assert_return_void(btopics, "expected 'btopics' folder");
    auto trash = ard::Trash();
    assert_return_void(trash, "expected 'trash' folder");
    auto delegate = ard::Delegated();
    assert_return_void(delegate, "expected 'delegated' folder");
    auto backlog = ard::Backlog();
    assert_return_void(backlog, "expected 'backlog' folder");

    list2outline.clear();
    list2outline.push_back(inbox);
    list2outline.push_back(ref);
    list2outline.push_back(maybe);
    list2outline.push_back(btopics);
    list2outline.push_back(trash);
    list2outline.push_back(delegate);
    list2outline.push_back(backlog);
    o_p->listMTopics(list2outline);
    list2outline.clear();

	LOCUS_LIST flst;
    ard::selectCustomFolders(flst);
    TOPICS_LIST prj_list2outline = ard::shortcut::wrapList(flst.begin(), flst.end(), outline_policy_Pad);
    o_p->listMTopics(prj_list2outline);
}

void OutlineScene::rebuildAsBoardSelectorView()
{
    OutlinePanel* o_p = prepareSelectorPanel();
    assert_return_void(o_p, "expected outline panel");

    if (isMainScene()) {
        std::set<ProtoPanel::EProp> p2set;
        SET_PPP(p2set, PP_FatFingerSelect);
        SET_PPP(p2set, PP_CurrSpot);
		SET_PPP(p2set, PP_Thumbnail);
        o_p->setProp(&p2set, nullptr);
        o_p->setOContext(OutlineContext::normal);
    }
    o_p->setIncludeRoot(false);

	auto d = ard::db();
	if (!d || !d->isOpen())
		return;

    qreal y_pos = 0.0;
    ProtoToolItem* g = nullptr;
    TOPICS_LIST list2outline;

	/// files board	
	auto bb = d->boards_model()->folders_board();
	auto sh = new ard::shortcut(bb);
	o_p->produceOutlineItems(sh,
		0,
		y_pos);
	

    ADD_TOOL_GBUTTON(AR_insertBBoard, "Create New Board", ":ard/images/unix/button-add.png");
    ADD_TOOL_GBUTTON(AR_none, "Select Existing Board", "");

    TOPICS_LIST prj_list;
    auto br = d->boards_model()->boards_root();
    if (br) {
        for (auto& i : br->items()) {
            auto b = dynamic_cast<ard::selector_board*>(i);
            ASSERT(b, "expected board");
            if (b) {
                prj_list.push_back(b);
            }
        }
    }

    list2outline.clear();
    list2outline = ard::shortcut::wrapList(prj_list, outline_policy_Uknown);
    o_p->listMTopics(list2outline);
};

void OutlineScene::build_email_attachements(OutlineScene* s, ard::email* em)
{
    OutlinePanel* o_p = s->prepareSelectorPanel();
    assert_return_void(o_p, "expected outline panel");

    o_p->setIncludeRoot(false);

    qreal y_pos = 0.0;
    ProtoToolItem* g = nullptr;

    ADD_TOOL_GBUTTON(AR_none, "Attachments", "");
    TOPICS_LIST list2outline;
    auto lst = em->getAttachments();
    for (auto & att : lst) {
        auto it = new ard::email_attachment(att);
        list2outline.push_back(it);
    }
    o_p->listMTopics(list2outline);
};

void OutlineScene::build_draft_attachements(OutlineScene* s, ard::email_draft* dm)
{
    auto dext = dm->draftExt();
    assert_return_void(dext, "expected draft extension");

    OutlinePanel* o_p = s->prepareSelectorPanel();
    assert_return_void(o_p, "expected outline panel");
    o_p->setIncludeRoot(false);

    qreal y_pos = 0.0;
    ProtoToolItem* g = nullptr;
    ADD_TOOL_GBUTTON(AR_none, "Attachments", "");
    TOPICS_LIST list2outline;
    auto& att_lst = dext->attachementList();
    for (auto & att : att_lst) 
	{
        auto it = new ard::draft_attachment_item(att);
        list2outline.push_back(it);
    }
    o_p->listMTopics(list2outline);
};

void OutlineScene::build_gmail_accounts(OutlineScene* s)
{
    OutlinePanel* o_p = s->prepareSelectorPanel();
    assert_return_void(o_p, "expected outline panel");
    o_p->setIncludeRoot(false);

    std::set<ProtoPanel::EProp> p;
    SET_PPP(p, PP_InplaceEdit);
    SET_PPP(p, PP_CurrSelect);
    SET_PPP(p, PP_CurrSpot);
    SET_PPP(p, PP_RTF);    
    o_p->setProp(&p);

    qreal y_pos = 0.0;
    ProtoToolItem* g = nullptr;
    ADD_TOOL_GBUTTON(AR_none, "GMail Accounts", "");
    TOPICS_LIST list2outline;
    
    if (!ard::isGoogleConnected()) {
        if (!ard::hasGoogleToken()) {
            return;
        }
    }

    auto storage = ard::gstorage();
    if (storage) {
        auto lst = storage->getAccounts();
        if (storage) {
            for (auto& i : lst) {
                auto acc = new ard::email_account_info(i);
                list2outline.push_back(acc);
            }
        }
    }

    o_p->listMTopics(list2outline);
};

void OutlineScene::build_files(OutlineScene* s)
{
	OutlinePanel* o_p = s->prepareSelectorPanel();
	assert_return_void(o_p, "expected outline panel");
	o_p->setIncludeRoot(false);

	std::set<ProtoPanel::EProp> p;
	SET_PPP(p, PP_CurrOpen);
	o_p->setProp(&p);

	qreal y_pos = 0.0;
	ProtoToolItem* g = nullptr;
	ADD_TOOL_GBUTTON(AR_none, "Open existing file", "");

	COMMANDS_SET cset;
	cset.insert(ECurrentCommand::cmdOpen);

	bool currDBfileFound = false;
	TOPICS_LIST& list2outline = model()->getDBFilesList();
	//for (TOPICS_LIST::iterator i = list2outline.begin(); i != list2outline.end(); i++)
	for(auto& i : list2outline)
	{
		auto f = dynamic_cast<ard::tool_topic*>(i);
		f->setCommands(cset);
		QString fileName = f->title();
		if (ard::isDbConnected() && fileName == dbp::currentDBName())
		{
			o_p->setCurrentSelected(f);
			f->setIconPixmap(getIcon_GreenCheck());
			currDBfileFound = true;
		}
	}

	if (!currDBfileFound)
	{
		ADD_TOOL_GBUTTON(AR_none, QString("DB:%1").arg(dbp::currentDBName()), "");
	}

	o_p->listMTopics(list2outline);
};

void OutlineScene::build_rules(OutlineScene* s) 
{
	OutlinePanel* o_p = s->prepareSelectorPanel();
	assert_return_void(o_p, "expected outline panel");
	o_p->setIncludeRoot(false);
	TOPICS_LIST lst;
	auto d = ard::db();
	if (d && d->isOpen()) {
		auto r = d->rmodel()->rroot();
		r->applyOnVisible([&](ard::rule* r)
		{
			lst.push_back(r);
		});
	}

	std::sort(lst.begin(), lst.end(), [](const ard::topic* f1, const ard::topic* f2)
	{
		return (f1->title() < f2->title());
	});

	o_p->listMTopics(lst);
};

void OutlineScene::build_contact_groups(OutlineScene* s)
{
    OutlinePanel* o_p = s->prepareSelectorPanel();
    assert_return_void(o_p, "expected outline panel");
    o_p->setIncludeRoot(false);
    auto gr_list = ard::db()->cmodel()->groot()->groups();
    o_p->rebuildAsListPanel(gr_list);
};

void OutlineScene::build_contacts(OutlineScene* s, QString group_id, QString search_str)
{
	s->freePanels();
	auto tp = new TablePanel(s);
	s->addPanel(tp);
	tp->addColumn(EColumnType::ContactTitle);
#ifdef ARD_BIG
	tp->addColumn(EColumnType::ContactEmail);
#endif
	if (ard::isDbConnected()) {
		if (ard::gmail())
		{
			auto c_list = ard::db()->cmodel()->croot()->contacts();

			if (group_id.isEmpty()) {
				/// all contacts
				std::sort(c_list.begin(), c_list.end(), ard::topic::TitleLess);
				tp->rebuildAsListPanel(c_list, search_str);
			}
			else {
				TOPICS_LIST group_selected;
				for (auto f : c_list) {
					auto c = dynamic_cast<ard::contact*>(f);
					ASSERT(c, "expected contact");
					if (c) {
						auto gmem = c->groupsMember();
						auto it = gmem.find(group_id);
						if (it != gmem.end()) {
							group_selected.push_back(c);
						}
					}
				}
				std::sort(group_selected.begin(), group_selected.end(), ard::topic::TitleLess);
				tp->rebuildAsListPanel(group_selected, search_str);
			}
		}
	}
};

void OutlineScene::build_from_list(OutlineScene* s, TOPICS_LIST& list2outline)
{
	OutlinePanel* o_p = s->prepareSelectorPanel();
	assert_return_void(o_p, "expected outline panel");
	o_p->setIncludeRoot(false);

	std::set<ProtoPanel::EProp> p;
	SET_PPP(p, PP_InplaceEdit);
	SET_PPP(p, PP_CurrSelect);
	o_p->setProp(&p);
	o_p->listMTopics(list2outline);
};

void OutlineScene::outlineHudButtonPressed(int _id, int _data)
{
	qDebug() << "<<OutlineScene::outlineHudButtonPressed" << _id << _data;
};
#include <QPushButton>
#include "contact.h"
#include "ardmodel.h"
#include "ContactGroupMembershipDlg.h"
#include "OutlineView.h"
#include "OutlineScene.h"
#include "OutlineSceneBase.h"
#include "ansyncdb.h"
#include "TablePanel.h"


/**
table with 1 column to show contacts details as a form view
*/
class ContactFormPanelIn1Col : public TablePanel
{
public:
    ContactFormPanelIn1Col(ProtoScene* s) :TablePanel(s)
    {
        addColumn(EColumnType::FormFieldValue);
    };
    void rebuild4contact(ard::contact* c)
    {
        attachTopic(c);
        std::set<EColumnType> exclude_columns;
        exclude_columns.insert(EColumnType::ContactNotes);
        TOPICS_LIST lst = c->produceFormTopics(nullptr, &exclude_columns);
        rebuildAsListPanel(lst, false);
    };
};


/**
EditContactGroupMembership
*/
void EditContactGroupMembership::editMembership(ard::contact* c)
{
    EditContactGroupMembership d(c);
    d.exec();
};


EditContactGroupMembership::EditContactGroupMembership(ard::contact* c)
{
    if (ard::isDbConnected()) {
        auto cr = ard::db()->cmodel()->croot();
        m_contacts_list = cr->filteredItems(dbp::configFileContactHoistedGroupSyId());
    }
    ASSERT(!m_contacts_list.empty(), "expected contacts list");
    std::sort(m_contacts_list.begin(), m_contacts_list.end(), ard::topic::TitleLess);
    m_curr_it = m_contacts_list.begin();
    while (m_curr_it != m_contacts_list.end() && *m_curr_it != c) {
        m_curr_it++;
    }
    if (m_curr_it == m_contacts_list.end()) {
        QString msg = QString("failed to locate contact in group")
            .arg(dbp::configFileContactHoistedGroupSyId())
            .arg(m_contacts_list.size())
            .arg(c->dbgHint());
            ASSERT(m_curr_it != m_contacts_list.end(), msg);
    }

	m_contact = c;
	/*
    m_item = c;
    m_dest = m_selTopic = nullptr;
    m_CanChangeBuilderType = false;
    m_HasOKbuton = false;
    m_destType = destinationUnknown;
	*/

    setWindowTitle(m_contact->title());
    m_ref_label_width = utils::calcWidth("Cancel Select Select2..");

    std::set<QString> groups_member = c->groupsMember();


    //** outline
    //createScene();
	m_view = new OutlineView(this);
	m_scene = new OutlineSceneBase(m_view);
	m_scene->setOutlinePolicy(outline_policy_Pad);
	m_scene->attachHoisted(dbp::root());
	m_view->setAlignment(Qt::AlignLeft | Qt::AlignTop);
	m_view->setMinimumHeight(300);

	std::set<ProtoPanel::EProp> prop;
	prop.insert(ProtoPanel::PP_InplaceEdit);
	m_contact_view = scene_view::create_with_builder([=](OutlineScene* s)
	{
		auto o_p = new ContactFormPanelIn1Col(s);
		s->addPanel(o_p);
		assert_return_void(o_p, "expected outline panel");
		o_p->setIncludeRoot(false);
		auto gr_list = ard::db()->cmodel()->groot()->groups();
		o_p->rebuildAsListPanel(gr_list);
	}, prop, this);

    m_contact_view->view->setMinimumHeight(gui::lineHeight() * 4);
    m_contact_view->view->setNoScrollBars();

    QVBoxLayout *v_main = new QVBoxLayout;
    v_main->addWidget(m_contact_view->view);
#ifdef _DEBUG
    auto b = new QPushButton(this);
    b->setText("re-init");
    QObject::connect(b, &QPushButton::released, [=]()
    {
        ard::messageBox(this,"press ok to reinit");
        initPostBuild();
    });
    v_main->addWidget(b);
#endif
    v_main->addWidget(m_view);
    setLayout(v_main);
	rebuildScene();
	setModal(true);

	gui::resizeWidget(this, QSize(600, 200));
    //initDlg();
};

void EditContactGroupMembership::rebuildScene()
{
    rebuildGroupScene();
    rebuildContactScene();
    QTimer::singleShot(10, this, SLOT(initPostBuild()));
};

void EditContactGroupMembership::rebuildContactScene() 
{
    assert_return_void(ard::isDbConnected(), "expected open DB");
    assert_return_void(m_curr_it != m_contacts_list.end(), "iterator at the end of contacts list");
    auto c = dynamic_cast<ard::contact*>(*m_curr_it);
    assert_return_void(c, "expected contact");

    m_contact_view->scene->clearOutline();

    if (c && m_contact_view->scene->hasPanel()) {
        ContactFormPanelIn1Col* p = dynamic_cast<ContactFormPanelIn1Col*>(m_contact_view->scene->panel());
		assert_return_void(p, "expected group panel");
        p->rebuild4contact(c);

        HudButton* h = nullptr;
        h = new HudButton(this, HudButton::idNext, " > ", "");
        m_contact_view->scene->addTopRightHudButton(h);

        h = new HudButton(this, HudButton::EHudType::Break);
        m_contact_view->scene->addTopRightHudButton(h);

        h = new HudButton(this, HudButton::idPrev, " < ", "");
        m_contact_view->scene->addTopRightHudButton(h);
    }


    ///---- update group selection
    std::set<QString> groups_member = c->groupsMember();
    auto gr_list = ard::db()->cmodel()->groot()->groups();
    for (auto g : gr_list) {
        //auto g = dynamic_cast<ard::contact_group*>(t);
        //ASSERT(g, "expected group");
        bool is_member = (groups_member.find(g->syid()) != groups_member.end());
        g->setTmpSelected(is_member ? true : false);
    }
};

void EditContactGroupMembership::rebuildGroupScene() 
{
    assert_return_void(ard::isDbConnected(), "expected open DB");
    if (!ard::google()) {
        ASSERT(0, "expected g-module");
        return;
    }

    TablePanel* tp = new TablePanel(m_scene);
    std::set<ProtoPanel::EProp> p2add, p2remove;
    SET_PPP(p2remove, PP_InplaceEdit);
    //SET_PPP(p2remove, PP_LabelFilter);
    tp->setProp(&p2add, &p2remove);
    tp->addColumn(EColumnType::Selection)
        .addColumn(EColumnType::Title);

    m_scene->freePanels();
    m_scene->addPanel(tp);
    
    auto gr_list = ard::db()->cmodel()->groot()->groups();
    tp->rebuildAsListPanel(gr_list);

    HudButton* h = nullptr;
    h = new HudButton(this, HudButton::idOK, "OK", "");
    m_scene->addTopRightHudButton(h);

    h = new HudButton(this, HudButton::EHudType::Break);
    h->setWidth(m_ref_label_width);
    m_scene->addTopRightHudButton(h);

    h = new HudButton(this, HudButton::idCancel, "Close", "");
    m_scene->addTopRightHudButton(h);

    /*
    h = new HudButton(this, HudButton::typeLineBreak);
    h->setWidth(m_ref_label_width);
    m_scene->addTopRightHudButton(h);

    h = new HudButton(this, HudButton::idCreateObject, "NewGroup", "");
    m_scene->addTopRightHudButton(h);
    */
};


bool EditContactGroupMembership::onHudCmd(HudButton::E_ID cid)
{
    bool processedContacts = false;

    switch (cid)
    {
        case HudButton::idNext: 
        {
            storeContactMembershipSelection();
            if (m_curr_it == m_contacts_list.end()) {
                m_curr_it = m_contacts_list.begin();
            }
            else {
                m_curr_it++;
            }
            if (m_curr_it == m_contacts_list.end()) {
                m_curr_it = m_contacts_list.begin();
            }

            processedContacts = true;
        }break;
        case HudButton::idPrev: 
        {
            storeContactMembershipSelection();
            if (m_curr_it == m_contacts_list.begin()) {
                m_curr_it = m_contacts_list.end();
                m_curr_it--;
            }
            else {
                m_curr_it--;
            }

            processedContacts = true;
        }break;
        default:break;
    }

    if (processedContacts) {
        rebuildContactScene();
        QTimer::singleShot(10, this, SLOT(initPostBuild()));
    }

    return processedContacts;
};

void EditContactGroupMembership::outlineTmpSelectionBoxPressed(int _id, int)
{
    auto g = ard::lookupAs<ard::contact_group>(_id);
    if (g) {
        if (g->syid().isEmpty()) {
            QString s = QString("Selected group '%1' has empty ID. If it is a new group please synchronize data to cloud before adding contact").arg(g->title());
			ard::errorBox(this, s);
            g->setTmpSelected(false);
            ProtoGItem* git = m_scene->findGItem(g);
            if (git) {
                git->g()->update();
            }
        }
    }
    else {
        ASSERT(0, "failed to locate group by dbid") << _id;
    }
};

void EditContactGroupMembership::storeContactMembershipSelection()
{
    assert_return_void(ard::isDbConnected(), "expected open DB");
    assert_return_void(m_curr_it != m_contacts_list.end(), "iterator at the end of contacts list");
    auto c = dynamic_cast<ard::contact*>(*m_curr_it);
    assert_return_void(c, "expected contact");

    std::vector<QString> selected_groups;

    auto gr_list = ard::db()->cmodel()->groot()->items();
    for (auto t : gr_list) {
        auto* g = dynamic_cast<ard::contact_group*>(t);
        if (g && g->isTmpSelected()) {
            selected_groups.push_back(g->syid());
        }
    }

    if (!selected_groups.empty()) {
        qDebug() << "selected groups" << selected_groups.size();
        for (auto g : selected_groups) {
            qDebug() << "sel-g" << g;
        }

        ard::db()->cmodel()->croot()->setContactGroupMembership(c, selected_groups);
        /*
        if (ard::db()) {
            ard::db()->contact_root()->setContactGroupMembership(c, selected_groups);
        }*/
    }

};

void EditContactGroupMembership::acceptWindow()
{
    if (!m_contacts_list.empty() && m_curr_it != m_contacts_list.end()) {
        storeContactMembershipSelection();
    }

    accept();
}

void EditContactGroupMembership::initPostBuild()
{
	m_scene->resetSceneBoundingRect();
	m_scene->updateAuxItemsPos();

    if (m_contact_view && m_contact_view->scene) {
        m_contact_view->scene->resetSceneBoundingRect();
        m_contact_view->scene->updateAuxItemsPos();
    }
};

void EditContactGroupMembership::outlineHudButtonPressed(int _id, int _data)
{
	Q_UNUSED(_data);

	HudButton::E_ID bid = (HudButton::E_ID)_id;
	if (onHudCmd(bid))
		return;

	switch (bid)
	{
	case HudButton::idCancel:    reject(); return;
	case HudButton::idOK:        acceptWindow(); return;
	case HudButton::idCreateObject:
	{

	}break;
	default: ASSERT(0, "the command is not implemented");
	}

	m_scene->freePanels();
	rebuildScene();
};


/**
    ImportContactsSelect
*/
class ImportContactsSelect : public QDialog
{
protected:
    ImportContactsSelect(ard::contacts_merge_result& mr);
    void    rebuildScene();
    void    uncheckAll() 
    {
        for (auto i : m_mresult.contacts)i->setTmpSelected(false);      
        m_import_view->view->viewport()->update();
    };

protected:
    scene_view::ptr                 m_import_view;
    ard::contacts_merge_result&     m_mresult;
    bool                            m_accepted{ false };
    friend bool guiSelectContacts2Import(ard::contacts_merge_result& mr);
};

bool guiSelectContacts2Import(ard::contacts_merge_result& mr) 
{
    ImportContactsSelect d(mr);
    d.exec();
    return d.m_accepted;
};

ImportContactsSelect::ImportContactsSelect(ard::contacts_merge_result& mr):m_mresult(mr)
{
    m_import_view = scene_view::create(nullptr);
    //m_view = new OutlineView(this);
    //m_scene = new OutlineSceneBase(m_view);
    m_import_view->scene->setOutlinePolicy(outline_policy_Pad);

    QVBoxLayout *v_main = new QVBoxLayout;
    QString msg = QString("Found '%1' new contacts, skipped '%2' contacts, already in database.")
        .arg(m_mresult.contacts.size())
        .arg(m_mresult.skipped_existing_contacts);
    
    if (m_mresult.skipped_tmp_contacts > 0) {
        msg += QString("Skipped '%1' temporary contacts").arg(m_mresult.skipped_tmp_contacts);
    }
    QLabel* lb = new QLabel(msg);
    v_main->addWidget(lb);
    v_main->addWidget(m_import_view->view);
    QHBoxLayout *h_btns = new QHBoxLayout;
    QPushButton* b = nullptr;
    ADD_BUTTON2LAYOUT(h_btns, "Uncheck All", [=]() { uncheckAll(); });
    ADD_BUTTON2LAYOUT(h_btns, "OK", [=]() {m_accepted = true; close(); });
    ADD_BUTTON2LAYOUT(h_btns, "Cancel", &QPushButton::close);
    v_main->addLayout(h_btns);
    setLayout(v_main);
    rebuildScene();
    gui::resizeWidget(this, QSize(500, 800));
};

void ImportContactsSelect::rebuildScene()
{
    TablePanel* tp = new TablePanel(m_import_view->scene);
    std::set<ProtoPanel::EProp> p2add, p2remove;
    SET_PPP(p2remove, PP_InplaceEdit);
    tp->setProp(&p2add, &p2remove);
    tp->addColumn(EColumnType::Selection)
        .addColumn(EColumnType::ContactTitle)
        .addColumn(EColumnType::ContactEmail)
        .addColumn(EColumnType::ContactPhone);

    m_import_view->scene->freePanels();
    m_import_view->scene->addPanel(tp);
    tp->rebuildAsListPanel(m_mresult.contacts);

    QTimer::singleShot(10, this, [=]() 
    {
        m_import_view->scene->updateAuxItemsPos();
    });
};

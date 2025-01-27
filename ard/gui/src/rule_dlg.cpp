#include <QVBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QListWidget>
#include <QRadioButton>
#include <QButtonGroup>
#include <QCheckBox>
#include <QListView>
#include "rule.h"
#include "rule_dlg.h"
#include "address_book.h"
#include "contact.h"
#include "OutlineScene.h"
#include "OutlineView.h"

/**
rule_add_opt - give option to create rule from email
*/
class rule_add_opt : public QDialog 
{
public:
	static std::pair<bool, ard::rule*> run_opt(QWidget* parent, std::set<QString>* from)
	{
		std::pair<bool, ard::rule*> rv;
		rule_add_opt d(parent, from);
		d.exec();

		rv.first	= d.m_accepted;
		rv.second	= d.m_selected_rule;
		return rv;
	}

	rule_add_opt(QWidget* parent, std::set<QString>* from):QDialog(parent)
	{
		auto h_main = new QHBoxLayout;
		auto v_select = new QVBoxLayout;
		auto v_buttons = new QVBoxLayout;
		
		if (from && !from->empty()) {
			auto s = *(from->begin());
			v_select->addWidget(new QLabel(QString("%1").arg(s)));
		}

		m_bg = new QButtonGroup(this);
		m_bcreate = new QRadioButton("Create new mail filter rule");
		m_bcreate->setChecked(true);
		m_bg->addButton(m_bcreate, 0);
		v_select->addWidget(m_bcreate);
		m_add = new QRadioButton("Add sender to existing rule");
		m_bg->addButton(m_add, 1);
		v_select->addWidget(m_add);

		m_cb = new QComboBox;
		v_select->addWidget(m_cb);
		auto r = ard::db()->rmodel()->rroot();
		r->applyOnVisible([&](ard::rule* r)
		{
			m_cb->addItem(r->title(), r->id());
		});
		m_cb->setEnabled(false);

		connect(m_cb, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), [=](int idx) {update_rule_details(idx); });

		m_fast_from = new QListView;
		v_select->addWidget(m_fast_from);
		m_fast_from->hide();
		//m_fast_from->setEnabled(false);
		//m_fast_from->setMaximumHeight(100);

		connect(m_bcreate, &QRadioButton::toggled, [=](bool){update_selection();});
		connect(m_add, &QRadioButton::toggled, [=](bool) {update_selection(); });

		h_main->addLayout(v_select);

		ard::addBoxButton(v_buttons, "OK", [&]()
		{
			if (m_bg->checkedId() == 1) 
			{
				auto idx = m_cb->currentIndex();
				if (idx != -1)
				{
					topic_ptr it = ard::lookup(m_cb->itemData(idx).toInt());
					m_selected_rule = dynamic_cast<ard::rule*>(it);					
				}
			}
			m_accepted = true;
			close();
		});
		ard::addBoxButton(v_buttons, "Cancel", [&]() {close(); });
		v_buttons->addWidget(ard::newSpacer(true));
		h_main->addLayout(v_buttons);
		h_main->addWidget(ard::newSpacer(true));
		setLayout(h_main);
	}

	void update_selection()
	{
		if (m_bg->checkedId() == 1){
			m_cb->setEnabled(true);
			update_rule_details(0);
			m_fast_from->show();
		}else{
			m_cb->setEnabled(false);
			m_fast_from->hide();
		}
	}

	void update_rule_details(int)
	{
		auto rid = m_cb->currentData().toInt();
		auto rr = ard::db()->rmodel()->rroot();
		auto r1 = rr->findRule(rid);
		if (r1)
		{
			auto r = dynamic_cast<ard::rule*>(r1);
			if (r) 
			{
				qDebug() << "selected rule" << r->title();
				auto e = r->rext();
				if (e) {
					auto& lst = e->fastFrom();
					QStandardItemModel* m = new QStandardItemModel();
					for (auto& s : lst) {
						auto si = new QStandardItem(s);
						m->appendRow(si);
					}
					m_fast_from->setModel(m);
				}
			}
		}
	}

	QButtonGroup*	m_bg{nullptr};
	QComboBox*		m_cb{nullptr};
	QListView*		m_fast_from{ nullptr };
	QRadioButton	*m_bcreate{ nullptr }, *m_add{nullptr};
	ard::rule*		m_selected_rule{ nullptr };
	bool			m_accepted{false};
};

bool ard::rule_dlg::addRule(QString name, std::set<QString>* from)
{
	assert_return_false(isDbConnected(), "expected connected DB");
	if (from) {
		auto rr = ard::db()->rmodel()->rroot();
		int rnum = 0;
		rr->applyOnVisible([&](ard::rule*){rnum++;});
		if (rnum > 0) 
		{
			auto res = rule_add_opt::run_opt(gui::mainWnd(), from);
			if (!res.first)
				return false;
			if (res.second) {
				return rule_dlg::editRule(res.second, from);
			}
		}
	}

	rule_dlg d(nullptr, name, from);
	d.exec();
	if (d.m_accepted) 
	{
	//	auto db = ard::db();
	//	if (db && db->isOpen()) {
	//		db->rmodel()->rroot()->resetRules();
	//	}
		if (d.m_rule) {
			d.m_rule->ensure_q();
			auto db = ard::db();
			if (db && db->isOpen()) {
				db->rmodel()->rroot()->resetRules();
				db->rmodel()->scheduleRuleRun(d.m_rule, 1);
			}
		}
		ard::asyncExec(AR_RebuildLocusTab);
		ard::rebuildMailBoard(nullptr);
	}

	return d.m_accepted;
};

bool ard::rule_dlg::editRule(ard::rule* r, std::set<QString>* from)
{
	assert_return_false(isDbConnected(), "expected connected DB");
	rule_dlg d(r, "", from);
	d.exec();
	return d.m_accepted;
};

ard::rule_dlg::rule_dlg(ard::rule* r, QString name, std::set<QString>* from):m_rule(r)
{
	QPushButton* b = nullptr;
	auto v_main = new QVBoxLayout;
	///name
	auto h_name = new QHBoxLayout;
	auto lb = new QLabel("Name");
	h_name->addWidget(lb);
	h_name->addWidget(ard::newSpacer());
	m_exclusion_filter = new QCheckBox("Don't show filtered messages in INBOX.");
	h_name->addWidget(m_exclusion_filter);
	v_main->addLayout(h_name);

	m_name = new QLineEdit();
	v_main->addWidget(m_name);

	/// senders
	auto h_sender = new QHBoxLayout;
	h_sender->addWidget(new QLabel("Senders"));
	h_sender->addWidget(ard::newSpacer());
	ADD_BUTTON2LAYOUT(h_sender, "Add Sender", &rule_dlg::addSender);
	v_main->addLayout(h_sender);
	buildSendersView(r, from, v_main);

	/// subject
	auto h_subject = new QHBoxLayout;
	h_subject->addWidget(new QLabel("Words in subject"));
	h_subject->addWidget(ard::newSpacer());
	ADD_BUTTON2LAYOUT(h_subject, "Add Word", &rule_dlg::addSubject);
	v_main->addLayout(h_subject);
	buildSubjectView(r, v_main);

	v_main->addWidget(new QLabel("Phrase in message body"));
	m_msg_phrase = new QPlainTextEdit;
	m_msg_phrase->setMaximumHeight(100);
	//m_msg_phrase->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
	v_main->addWidget(m_msg_phrase);

	if (m_rule) 
	{
		m_name->setText(m_rule->title());
		auto e = m_rule->rext();
		if (e)
		{
			m_msg_phrase->setPlainText(e->exact_phrase());
		}

		m_exclusion_filter->setChecked(m_rule->isFilter());
	}
	else 
	{
		if (!name.isEmpty()) {
			m_name->setText(name);
		}
		m_exclusion_filter->setChecked(true);
	}

	auto h = new QHBoxLayout;
	ADD_BUTTON2LAYOUT(h, "OK", &rule_dlg::processOK);
	ADD_BUTTON2LAYOUT(h, "Cancel", &rule_dlg::processCancel);
	v_main->addLayout(h);
	m_name->setFocus(Qt::OtherFocusReason);

	MODAL_DIALOG_SIZE(v_main, QSize(620, 720));
};

void ard::rule_dlg::processOK() 
{
	m_accepted = false;

	QString name = m_name->text().trimmed();
	if (name.isEmpty())
	{
		ard::errorBox(this, "Please enter rule name.");
		return;
	}

	if (!m_rule) 
	{
		auto rt = ard::db()->rmodel()->rroot();
		m_rule = rt->addRule(name);
	}

	if (m_rule) 
	{
		ORD_STRING_SET lst_from, lst_subject;
		for (auto& i : m_senders_list)lst_from.insert(i->title());
		for (auto& i : m_subject_list)lst_subject.insert(i->title());

		m_rule->setTitle(name);

		auto e = m_rule->rext();
		if (e) {
			e->setupRule(lst_subject,
				m_msg_phrase->toPlainText().trimmed(),
				lst_from,
				m_exclusion_filter->isChecked());
		}
	}
	m_accepted = true;
	close();
};

void ard::rule_dlg::processCancel() 
{
	m_accepted = false;
	close();
};


void ard::rule_dlg::addSender()
{
	auto r = ard::address_book_dlg::select_contacts(this);
	for (auto& i : r.to_list)
	{
		auto s = i.toEmailAddress();
		bool exist_in_list = false;
		auto s1 = ard::recoverEmailAddress(s).toLower();
		for (auto& i : m_senders_list) {
			auto s2 = i->title().toLower();
			auto j = s2.indexOf(s1);
			if (j != -1) 
			{
				exist_in_list = true;
				break;
			}
		}

		if (!exist_in_list) 
		{
			auto f = new ard::tool_topic(s);
			COMMANDS_SET cset;
			cset.insert(ECurrentCommand::cmdDelete);
			cset.insert(ECurrentCommand::cmdEdit);
			f->setCommands(cset);
			f->setCanRename(true);
			m_senders_list.push_back(f);
			m_senders_view->run_scene_builder();
		}
	}
};

/*
void ard::rule_dlg::remove_sender() 
{
	auto lst = m_senders->selectedItems();
	for (auto& i : lst) {
		m_senders->takeItem(m_senders->row(i));
	}
};*/

void ard::rule_dlg::addSubject()
{
	auto f = new ard::tool_topic("");
	COMMANDS_SET cset;
	cset.insert(ECurrentCommand::cmdDelete);
	cset.insert(ECurrentCommand::cmdEdit);
	f->setCommands(cset);
	f->setCanRename(true);
	m_subject_list.push_back(f);
	m_subject_view->run_scene_builder();
	auto g = m_subject_view->scene->findGItem(f);
	if (g) {
		m_subject_view->view->ensureVisibleGItem(g);
		m_subject_view->view->renameSelected();
	}
	//m_subject_view->view->ensureVisibleGItem(f);

	/*
	auto r = gui::edit("", "Phrase", this);
	if (r.first) {
		auto lst = m_subject->findItems(r.second, Qt::MatchFixedString);
		if (lst.empty()) {
			m_subject->addItem(r.second);
		}
	}*/
};

/*
void ard::rule_dlg::remove_subject() 
{
	auto lst = m_subject->selectedItems();
	for (auto& i : lst) {
		m_subject->takeItem(m_subject->row(i));
	}
};*/

void ard::rule_dlg::buildSendersView(ard::rule* r, std::set<QString>* from, QBoxLayout* box)
{
	ORD_STRING_SET rset;
	if (r){
		auto e = r->rext();
		if (e)rset = e->from();
	}

	if (from){
		for (auto& s : *from)rset.insert(s);
	}

	COMMANDS_SET cset;
	cset.insert(ECurrentCommand::cmdDelete);
	cset.insert(ECurrentCommand::cmdEdit);

	m_senders_list = ard::tool_topic::wrapList(rset.begin(), rset.end(), cset, true);

	std::set<ProtoPanel::EProp> prop;
	prop.insert(ProtoPanel::PP_CurrSelect);
	prop.insert(ProtoPanel::PP_CurrEdit);
	prop.insert(ProtoPanel::PP_CurrDelete);	
	m_senders_view = scene_view::create_with_builder([=](OutlineScene* s)
	{
		OutlineScene::build_from_list(s, m_senders_list);
	}, prop, this, OutlineContext::grep2list);
	box->addWidget(m_senders_view->view);
};

void ard::rule_dlg::buildSubjectView(ard::rule* r, QBoxLayout* box) 
{
	COMMANDS_SET cset;
	cset.insert(ECurrentCommand::cmdDelete);
	cset.insert(ECurrentCommand::cmdEdit);

	ORD_STRING_SET rset;
	if (r) {
		auto e = r->rext();
		auto lst = e->subject();
		for (auto& s : lst)rset.insert(s);
	}

	m_subject_list = ard::tool_topic::wrapList(rset.begin(), rset.end(), cset, true);

	std::set<ProtoPanel::EProp> prop;
	prop.insert(ProtoPanel::PP_CurrSelect);
	prop.insert(ProtoPanel::PP_CurrEdit);
	prop.insert(ProtoPanel::PP_CurrDelete);
	m_subject_view = scene_view::create_with_builder([=](OutlineScene* s)
	{
		OutlineScene::build_from_list(s, m_subject_list);
	}, prop, this, OutlineContext::grep2list);
	box->addWidget(m_subject_view->view);
};

void ard::rule_dlg::currentMarkPressed(int c, ProtoGItem* g) 
{
	auto f = g->topic();
	if (f)
	{
		ECurrentCommand ec = (ECurrentCommand)c;
		switch (ec)
		{
        default:break;
		case ECurrentCommand::cmdEdit:
		{
			auto v = g->p()->s()->v();
			v->renameSelected();
		}break;
		case ECurrentCommand::cmdDelete:
		{
			auto v = g->p()->s()->v();
			if (v == m_senders_view->view) 
			{
				if (!ard::confirmBox(this, QString("Please confirm removing sender '%1'").arg(f->title()))) {
					return;
				}
				auto i = m_senders_list.begin();
				while (i != m_senders_list.end()) {
					if (*i == f) {
						m_senders_list.erase(i);
						m_senders_view->run_scene_builder();
						return;
					}
					i++;
				}
			}
			else if (v == m_subject_view->view) 
			{
				if (!ard::confirmBox(this, QString("Please confirm removing words '%1'").arg(f->title()))) {
					return;
				}
				auto i = m_subject_list.begin();
				while (i != m_subject_list.end()) {
					if (*i == f) {
						m_subject_list.erase(i);
						m_subject_view->run_scene_builder();
						return;
					}
					i++;
				}
			}
		}break;
		}
	}
};

void ard::rule_dlg::currentGChanged(ProtoGItem* ) 
{

};

/**
rules_dlg
*/
void ard::rules_dlg::run_it(ard::q_param* r2select)
{
	rules_dlg d(r2select);
	d.exec();
	if (d.m_modified)
	{
		ard::asyncExec(AR_RebuildLocusTab);
		ard::rebuildMailBoard(nullptr);
	}
};

ard::rules_dlg::rules_dlg(ard::q_param* r2select)
{
	addRulesTab();
	selectRule(r2select);
	setupDialog(QSize(550, 600), false);
	QPushButton* b;
	b = new QPushButton("Add");
	m_buttons_layout->addWidget(b);
	connect(b, &QPushButton::released,
		[=]()
	{
		if (ard::rule_dlg::addRule())
		{
			rebuild_current_scene_view();
			m_modified = true;
		};
	});

	/*b = new QPushButton("Edit");
	m_buttons_layout->addWidget(b);
	connect(b, &QPushButton::released,
		[=]()
	{
		editRule();
	});*/

	b = new QPushButton("Del");
	m_buttons_layout->addWidget(b);
	connect(b, &QPushButton::released,
		[=]()
	{
		removeRule();
	});

	ADD_BUTTON2LAYOUT(m_buttons_layout, "Close", &ard::scene_view_dlg::close);
};

void ard::rules_dlg::addRulesTab()
{
	if (gui::isDBAttached())
	{
		std::set<ProtoPanel::EProp> prop;
		prop.insert(ProtoPanel::PP_CurrSelect);
		prop.insert(ProtoPanel::PP_CurrEdit);
		
		auto vtab = scene_view::create_with_builder([=](OutlineScene* s)
		{
			OutlineScene::build_rules(s);
		}, prop, this, OutlineContext::grep2list);
		m_main_tab->addTab(vtab->view, "Mail Rules");
		m_rule_str = new QPlainTextEdit;
		m_rule_str->setMaximumHeight(100);
		m_rule_str->setReadOnly(true);
		m_rule_str->setStyleSheet("QPlainTextEdit {background-color: rgb(192,192,192)}");
		auto fnt = m_rule_str->font();
		fnt.setBold(true);
		m_rule_str->setFont(fnt);

		m_main_layout->addWidget(m_rule_str);
		int idx = m_main_tab->count() - 1;
		m_index2view[idx] = std::move(vtab);
	}
};

void ard::rules_dlg::editRule()
{
	auto f = current_v_topic<ard::rule>();
	if (f && ard::rule_dlg::editRule(f))
	{
		rebuild_current_scene_view();
	};
	m_modified = true;
};

void ard::rules_dlg::removeRule() 
{
	topic_ptr f = current_v_topic<>();
	if (!f) {
		ard::errorBox(this, "Please select topic to proceed.");
		return;
	}


	if (ard::confirmBox(this, QString("Please confirm removing mail rule '%1'").arg(f->title())))
	{
		f->killSilently(false);
		auto d = ard::db();
		if (d && d->isOpen()) {
			auto r = d->rmodel()->rroot();
			if (r) {
				r->resetRules();
				rebuild_current_scene_view();
			}
		}
	};
	m_modified = true;
};

void ard::rules_dlg::selectRule(ard::q_param* r2select) 
{
	auto v = current_scene_view();
	if (v)
	{
		if (r2select)
		{
			auto g = v->scene->findGItemByUnderlying(r2select);
			if (g) {
				v->scene->selectGI(g);
				g->g()->ensureVisible(QRectF(0, 0, 1, 1));
			}
		}
		else
		{
			///select first one
			auto p = v->scene->panel();
			if (p) {
				auto& lst = p->outlined();
				if (!lst.empty()) {
					auto g = *(lst.begin());
					v->scene->selectGI(g);
					g->g()->ensureVisible(QRectF(0, 0, 1, 1));
				}
			}
		}
	}
};

void ard::rules_dlg::currentMarkPressed(int c, ProtoGItem* g)
{
	auto r = dynamic_cast<ard::rule*>(g->topic());
	if (r)
	{
		ECurrentCommand ec = (ECurrentCommand)c;
		switch (ec)
		{
        default:break;
		case ECurrentCommand::cmdEdit:
		{
			editRule();
		}break;
		case ECurrentCommand::cmdDelete: 
		{
			removeRule();
		}break;
		case ECurrentCommand::cmdSelect:
		{
			auto f = current_v_topic<ard::rule>();
			if (f)
			{
				dbp::configFileSetELabelHoisted(f->id());
				gui::rebuildOutline(outline_policy_PadEmail);
				close();
			}
		}break;
		}
	}
};

void ard::rules_dlg::currentGChanged(ProtoGItem* g)
{
	auto r = dynamic_cast<ard::rule*>(g->topic());
	if (r && m_rule_str)
	{
		m_rule_str->setPlainText(r->qstr());
	}
};

#include <QPushButton>
#include <QComboBox>
#include <QCheckBox>
#include <QProgressBar>

#include "address_book.h"
#include "ard-db.h"
#include "ansyncdb.h"
#include "OutlineScene.h"
#include "ContactFormPanel.h"
#include "ContactGroupMembershipDlg.h"

static void doAddContact(ard::EitherContactOrEmail c, QTextEdit* e)
{
    QString s_title;
    QString s_email;

    if (c.contact1) {
        s_title = c.contact1->impliedTitle();
        s_email = c.contact1->contactEmail();
    }
    else {
        s_title = c.display_email.name();
        s_email = c.display_email.email_addr;
    }
    s_title.remove(QRegExp("[\\n\\t\\r]"));

    QTextCursor cur(e->textCursor());
    cur.movePosition(QTextCursor::End);
    QTextCharFormat fmt(cur.charFormat());
    fmt.setFontUnderline(true);
    fmt.setAnchorHref(s_email);

    cur.insertText(s_title, fmt);

    fmt.setFontUnderline(false);
    fmt.setAnchorHref("");
    cur.insertText("; ", fmt);
};

QTextEdit* ard::buildEmailListEditor()
{
    QTextEdit *e = new QTextEdit;
    e->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    e->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    e->setFont(*ard::defaultFont());
    e->setWordWrapMode(QTextOption::NoWrap);
    e->setLineWrapMode(QTextEdit::NoWrap);

    QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    sizePolicy.setVerticalStretch(0);
    e->setSizePolicy(sizePolicy);


    auto h = utils::calcHeight("W", &(e->font())) + ARD_MARGIN;
    e->setMaximumHeight(h);
    return e;
};

void ard::lookupList2editor(const ard::LOOKUP_LIST& lookup_list, QTextEdit* e)
{
    e->clear();
    for (auto c : lookup_list) {
        addContactToListEditor(c, e);
    }
};


void ard::editor2lookupList(QTextEdit* e,
    STRING_SET* emails_set,
    ard::LOOKUP_LIST* lookup_list)
{
    std::function<void(QString, QString)> add_parsed = [&emails_set, &lookup_list](QString name, QString email)
    {
        if (email.indexOf("@") != -1) {
            if (emails_set) {
                if (!email.isEmpty()) {
                    emails_set->insert(email);
                }
            }
            if (lookup_list) {
                ard::EitherContactOrEmail c(name, email);
                lookup_list->push_back(c);
            }
        }
    };

    QTextDocument* doc = e->document();
    if (doc) {
        int block_idx = 0;
        QTextBlock b = doc->begin();
        while (b.isValid()) {
            QTextBlock::iterator it;
            for (it = b.begin(); !(it.atEnd()); ++it) {
                QTextFragment fragment = it.fragment();
                QString display_name = fragment.text().trimmed();
                if (fragment.isValid()) {
                    auto chfm = fragment.charFormat();
                    QString href = chfm.anchorHref();
                    if (!href.isEmpty()) {
                        add_parsed(display_name, href);
                    }
                    else {
                        ///try to parse block as plain string with display name and email
                        auto lst = display_name.split(QRegExp(";"), QString::SkipEmptyParts);
                        for (auto s1 : lst) {
                            int idx = s1.indexOf("<");
                            if (idx != -1) {
                                QString name = s1.left(idx).trimmed();
                                QString href = s1.mid(idx + 1).remove(">").trimmed();
                                add_parsed(name, href);
                            }
                            else {
                                if (display_name.indexOf("@") != -1) {
                                    add_parsed(display_name, display_name);
                                }
                            }
                        }
                    }
                }
            }
            b = b.next();
            block_idx++;
        }
    }
};

void ard::addContactToListEditor(ard::EitherContactOrEmail c, QTextEdit* e)
{
    QString new_email; //c.toEmailAddress();

    if (c.contact1) {
        new_email = c.contact1->contactEmail();
    }
    else {
        new_email = c.display_email.email_addr;
    }

    if (!c.isValid()) {
        ASSERT(0, "invalid email formatting") << c.display_email.display_name << c.display_email.email_addr;
        return;
    }

    STRING_SET emails_set;
    editor2lookupList(e, &emails_set, nullptr);

    auto i = emails_set.find(new_email);
    if (i != emails_set.end()) {
        qDebug() << "email already in list" << new_email;
        return;
    }

    doAddContact(c, e);
};

ard::ContactsLookup ard::address_book_dlg::select_receiver(QWidget* parent, ard::ContactsLookup* predef_lst, bool attached_option)
{
    auto db = ard::db();
    if (!db ||
        !ard::isDbConnected()) {
		ard::errorBox(parent, "Contact model no initiated.");
        ard::ContactsLookup empty_result;
        empty_result.lookup_succeeded = false;
        return empty_result;
    }

    ard::address_book_dlg d(false, attached_option);
    if (predef_lst) {
        ard::db()->cmodel()->croot()->enrichLookupListFromCache(*predef_lst);
        std::function<void(const ard::LOOKUP_LIST&, QTextEdit*)> add_list = [](const ard::LOOKUP_LIST& lst, QTextEdit* e)
        {
            ard::lookupList2editor(lst, e);
        };

        add_list(predef_lst->to_list, d.m_edit_to);
        add_list(predef_lst->cc_list, d.m_edit_cc);
    }
    d.exec();
    return d.m_result;
};


ard::ContactsLookup ard::address_book_dlg::select_contacts(QWidget* parent)
{
	auto db = ard::db();
	if (!db ||
		!ard::isDbConnected()) {
		ard::errorBox(parent, "Contact model not initiated.");
		ard::ContactsLookup empty_result;
		empty_result.lookup_succeeded = false;
		return empty_result;
	}

	ard::address_book_dlg d(false, false, false);
	d.m_btnTo->setText("select");
	d.exec();
	return d.m_result;
};

ard::contact* ard::address_book_dlg::open_book()
{
    assert_return_null(ard::isDbConnected(), "expected open DB");

    ard::address_book_dlg d(true);
    d.exec();

    if (d.m_result.lookup_succeeded) {
        if (!d.m_result.to_list.empty()) {
            auto it = d.m_result.to_list.begin();
            auto c = (*it).contact1;
            return c;
        }
    }
    return nullptr;
};


void ard::address_book_dlg::addABTab()
{
    if (gui::isDBAttached())
    {
        std::set<ProtoPanel::EProp> p;
        if (m_single_contact_select) {            
            SET_PPP(p, PP_CurrSpot);
            SET_PPP(p, PP_CurrSelect);
        }

		auto vtab = scene_view::create_with_builder([=](OutlineScene* s)
		{
			OutlineScene::build_contacts(s, "", m_str_search);
		}, p, this);
        m_main_tab->addTab(vtab->view, "All Contacts");
        int idx = m_main_tab->count() - 1;
		m_index2view[idx] = std::move(vtab);

        if (ard::isDbConnected()) {
            if (ard::gmail())
            {
                std::set<QString> groups_with_contacts;
                auto c_list = ard::db()->cmodel()->croot()->items();
                for (auto f : c_list) {
                    auto c = dynamic_cast<ard::contact*>(f);
                    if (c) {
                        auto gmem = c->groupsMember();
                        groups_with_contacts.insert(gmem.begin(), gmem.end());
                    }
                }

                auto gr_list = ard::db()->cmodel()->groot()->items();
                for (auto t : gr_list) {
                    auto* g = dynamic_cast<ard::contact_group*>(t);
                    if (g && groups_with_contacts.find(g->syid()) != groups_with_contacts.end()) {
						vtab = scene_view::create_with_builder([=](OutlineScene* s)
						{
							OutlineScene::build_contacts(s, g->syid(), m_str_search);
						}, p, this);
                        m_main_tab->addTab(vtab->view, g->title());
                        int idx = m_main_tab->count() - 1;
						m_index2view[idx] = std::move(vtab);
                    }
                }
            }
        }
    }
};

ard::address_book_dlg::address_book_dlg(bool single_select, bool attached_option, bool has_cc):m_single_contact_select(single_select)
{
    //..
    ASSERT(m_main_tab, "expected main tab");
    QHBoxLayout* l_search = new QHBoxLayout;
    m_main_layout->insertLayout(0, l_search);
    l_search->addWidget(new QLabel("Search:"));
    m_search_edit = new QLineEdit;
    l_search->addWidget(m_search_edit);
    connect(m_search_edit, &QLineEdit::textChanged, this, [&](const QString& str)
    {
        if (m_str_search != str) {
            m_str_search = str.trimmed();

			auto v = current_scene_view();
			if (v) 
			{
				v->run_scene_builder();
			}
        }
    });

    addABTab();
    setupDialog(QSize(550, 700), false);


    QPushButton* btn = nullptr;
    if (m_single_contact_select) {
		btn = create_new_contact_button();
		m_buttons_layout->addWidget(btn);
		btn = create_edit_contact_button();
		m_buttons_layout->addWidget(btn);
		btn = create_delete_contact_button();
		m_buttons_layout->addWidget(btn);

        btn = new QPushButton("Close");
		m_buttons_layout->addWidget(btn);
        //m_buttons_layout->addWidget(btn, 0, Qt::AlignRight);
        connect(btn, &QPushButton::released, [&]()
        {
            m_result.lookup_succeeded = false;
            close();
        });
    } 
    else{
        QGridLayout* gl = new QGridLayout;
        m_buttons_layout->addLayout(gl);

        m_edit_to = ard::buildEmailListEditor();
		m_btnTo = new QPushButton;
		m_btnTo->setText("To:");
        gl->addWidget(m_btnTo, 0, 0);
        connect(m_btnTo, &QPushButton::released, [&]()
        {
            auto c = selected_contact();
            if (c) {
                ard::addContactToListEditor(c, m_edit_to);
            }
        });
		gl->addWidget(m_edit_to, 0, 1);

		if (has_cc)
		{
			m_edit_cc = ard::buildEmailListEditor();
			btn = new QPushButton;
			btn->setText("CC:");
			gl->addWidget(btn, 1, 0);
			connect(btn, &QPushButton::released, [&]()
			{
				auto c = selected_contact();
				if (c) {
					ard::addContactToListEditor(c, m_edit_cc);
				}
			});
			gl->addWidget(m_edit_cc, 1, 1);
		}

        QHBoxLayout* l_close = new QHBoxLayout;
        m_main_layout->addLayout(l_close);

		if (attached_option) 
		{
			m_chk_attached = new QCheckBox("Include attachements");
			l_close->addWidget(m_chk_attached);
		}

        l_close->addStretch();
		btn = create_edit_contact_button();
		l_close->addWidget(btn, 0, Qt::AlignRight);

        btn = new QPushButton("OK");
        l_close->addWidget(btn, 0, Qt::AlignRight);
        connect(btn, &QPushButton::released, [&]()
        {
            ard::editor2lookupList(m_edit_to, nullptr, &(m_result.to_list));
			if (m_edit_cc) {
				ard::editor2lookupList(m_edit_cc, nullptr, &(m_result.cc_list));
			}
            m_result.lookup_succeeded = true;
			if (m_chk_attached && m_chk_attached->isChecked()) {
				m_result.include_attached4forward = true;
			}
            close();
        });

        btn = new QPushButton("Cancel");
        l_close->addWidget(btn, 0, Qt::AlignRight);
        connect(btn, &QPushButton::released, [&]()
        {
            m_result.lookup_succeeded = false;
            close();
        });
    }
}

QPushButton* ard::address_book_dlg::create_edit_contact_button() 
{
	auto btn = new QPushButton("Edit");	
	connect(btn, &QPushButton::released, [&]()
	{
		auto c = selected_contact();
		if (c) {
			ard::contact_dlg::runIt(c);
		}

		auto v = current_scene_view();
		if (v)
		{
			v->run_scene_builder();
		}
	});
	return btn;
};

QPushButton* ard::address_book_dlg::create_delete_contact_button()
{
	auto btn = new QPushButton("Delete");
	connect(btn, &QPushButton::released, [&]()
	{
		auto c = selected_contact();
		if (c) {
			if (ard::confirmBox(this, QString("Delete selected contact '%1'?").arg(c->title()))) {
				c->killSilently(true);
			}
		}

		auto v = current_scene_view();
		if (v)
		{
			v->run_scene_builder();
		}
	});
	return btn;
};


QPushButton* ard::address_book_dlg::create_new_contact_button() 
{
	auto btn = new QPushButton("New Contact");
	connect(btn, &QPushButton::released, [&]()
	{
		//auto c = selected_contact();
		assert_return_void(ard::isDbConnected(), "expected open DB");
		auto cr = ard::db()->cmodel()->croot();
		auto c = cr->addContact("new", "contact");
		if (c) {
			ard::contact_dlg::runIt(c);
		}

		auto v = current_scene_view();
		if (v)
		{
			v->run_scene_builder();
		}
	});
	return btn;
};

ard::contact* ard::address_book_dlg::selected_contact()
{
	auto g = currentGI();
	if (g) {
		return dynamic_cast<ard::contact*>(g->topic()->shortcutUnderlying());
	}
	return nullptr;
};


void ard::address_book_dlg::currentMarkPressed(int c, ProtoGItem* g)
{
    if (g) {
        ECurrentCommand ec = (ECurrentCommand)c;
        qDebug() << "got-command" << c;
        switch (ec)
        {
        case ECurrentCommand::cmdSelect:
        {
            auto c = dynamic_cast<ard::contact*>(g->topic());
            assert_return_void(c, "expected contact object");
            //auto c2 = c->optr();
            //assert_return_void(c2, "expected contact underlying object");
            QString name = c->contactGivenName();
            QString last = c->contactFamilyName();
            if (name.isEmpty() || last.isEmpty()) {
				ard::errorBox(this, "Please select contact with filled 'First' & 'Last' names. Both 'Last' && 'First' names should be defined. You can edit contact in contact view if needed.");
                return;
            }

            m_result.to_list.push_back(c);
            m_result.lookup_succeeded = true;
            close();
            return;
        }break;
        default:break;
        }
    }
}

void ard::address_book_dlg::topicDoubleClick(void* p) 
{
    if (p) {
        topic_ptr f = (topic_ptr)p;
        auto c = dynamic_cast<ard::contact*>(f);
        if (c) {
            ard::addContactToListEditor(c, m_edit_to);
        }
    }
};

/**
	contact_dlg
*/
void ard::contact_dlg::runIt(ard::contact* c)
{
	ard::contact_dlg d(c);
	d.exec();
};

ard::contact_dlg::contact_dlg(ard::contact* c):m_c(c)
{
	add_contact_tab();
	setupDialog(QSize(550, 600));
	QPushButton* b;
	b = new QPushButton("Set Group");
	//gui::setButtonMinHeight(b);
	m_buttons_layout->insertWidget(0, b);
	connect(b, &QPushButton::released,
		[=]()
	{
		EditContactGroupMembership::editMembership(c);
	});
};

void ard::contact_dlg::add_contact_tab() 
{
	if (gui::isDBAttached())
	{
		std::set<ProtoPanel::EProp> prop;
		auto vtab = scene_view::create_with_builder([=](OutlineScene* s)
		{
			OutlineScene::build_single_contact(s, m_c);
		}, prop, this);
		m_main_tab->addTab(vtab->view, "Contact");
		int idx = m_main_tab->count() - 1;
		m_index2view[idx] = std::move(vtab);
	}
};
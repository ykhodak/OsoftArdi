#include <QLineEdit>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QToolButton>
#ifdef ARD_CHROME
#include <QWebEngineProfile>
#endif
#include <QFileDialog>
#include <QProgressBar>
#include <QMenu>
#include <QShortcut>
#include <QApplication>

#include "EmailPreview.h"
#include "utils.h"
#include "anfolder.h"
#include "email.h"
#include "ethread.h"
#include "gmail/GmailCache.h"
#include "MainWindow.h"
#include "small_dialogs.h"
#include "ardmodel.h"
#include "address_book.h"
#include "custom-menus.h"
#include "NoteFrameWidget.h"
#include "PopupCard.h"
#include "extnote.h"
#include "CardPopupMenu.h"
#include "custom-boxes.h"

class EmailTitleLineWidget : public QWidget
{
public:
	EmailTitleLineWidget();
};


#define ADD_TOOL_BTN(I, T, L)     btn = new QToolButton;    \
    btn->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);   \
    btn->setIcon(utils::fromTheme(I));                      \
    btn->setIconSize(QSize(32, 32));                        \
    btn->setText(T);                                        \
    L->addWidget(btn);                                      \

#ifdef ARD_CHROME
ArdQWebEnginePage::ArdQWebEnginePage() :
    QWebEnginePage(QWebEngineProfile::defaultProfile())
{

};

bool ArdQWebEnginePage::acceptNavigationRequest(const QUrl & url, QWebEnginePage::NavigationType type, bool)
{
    if (type == QWebEnginePage::NavigationTypeLinkClicked)
    {
        QDesktopServices::openUrl(url);
        return false;
    }
    return true;
}

QWebEnginePage* ArdQWebEnginePage::createWindow(WebWindowType)
{
    return new ArdQWebEnginePage();
};
#endif

class StaticEmailTitleWidget : public QWidget
{
	enum class eselection 
	{
		none,
		subject,
		to,
		from
	};

public:
    StaticEmailTitleWidget(ard::TopicWidget* w):m_twidget(w)	
	{
		QShortcut *sh = new QShortcut(QKeySequence("Ctrl+c"), this);
		connect(sh, &QShortcut::activated, [&]() 
		{
			QString str2copy;
			switch (m_selection)
			{
            case eselection::none:break;
			case eselection::subject: if (m_email)str2copy = m_email->plainSubject(); break;
			case eselection::from:
			{
				str2copy = m_from;
				auto idx = str2copy.indexOf("from:");
				if (idx == 0) {
					str2copy = str2copy.mid(5);
				}
			}break;
			case eselection::to:
			{
				str2copy = m_to;
				auto idx = str2copy.indexOf("to:");
				if (idx == 0) {
					str2copy = str2copy.mid(4);
				}
			}break;
			}
			if (!str2copy.isEmpty()) 
			{
				auto c = QApplication::clipboard();
				if (c) {
					c->setText(str2copy);
				}
			}
		});

	}

    void attachEmail(email_ptr e)
    {
        auto h = utils::calcHeight("AA", ard::defaultBoldFont())
            + utils::calcHeight("AA", ard::defaultFont())
            + utils::calcHeight("AA", ard::defaultSmallFont());

        m_email = e;
        m_to = QString("to: %1").arg(m_email->to());
        m_from = QString("from: %1").arg(m_email->from());
        auto cc = m_email->plainCC();
        if (!cc.isEmpty())
        {
            m_cc = QString("cc: %1").arg(cc);
            h += utils::calcHeight("AA", ard::defaultSmallFont());
        }
        m_date = m_email->dateColumnLabel();
        setMinimumHeight(h);
        update();
    }

protected:
    void  paintEvent(QPaintEvent*)override
    {
        QPainter p(this);
        QRect rc = rect();
        //auto rgb = qRgb(94, 94, 94);
        auto def_bk = QBrush(qRgb(128, 128, 128));
		auto sel_bk = model()->brushSelectedItem();
        p.setBrush(def_bk);
        p.drawRect(rc);
		QRect brc;
		QRect rcd = rc;

		std::function<void(QString, eselection)> draw_sel_text = [&](QString text, eselection sel)
		{
			PGUARD(&p);
			if (m_selection == sel) {
				p.setBackgroundMode(Qt::OpaqueMode);
				p.setBackground(sel_bk);
			}
			p.drawText(rcd, Qt::AlignLeft, text, &brc);
		};
			
        if (m_email)
        {
            p.setFont(*ard::defaultBoldFont());
			draw_sel_text(m_email->plainSubject(), eselection::subject);
            rcd.setTop(rcd.top() + brc.height());
			m_subject_rc = brc;

            p.setFont(*ard::defaultFont());
			draw_sel_text(m_from, eselection::from);
            rcd.setTop(rcd.top() + brc.height());
			m_from_rc = brc;

            p.setFont(*ard::defaultSmallFont());
			draw_sel_text(m_to, eselection::to);
			m_to_rc = brc;
            if (!m_cc.isEmpty())
            {
                rcd.setTop(rcd.top() + brc.height());
                p.drawText(rcd, Qt::AlignLeft, m_cc, &brc);
                rcd.setTop(rcd.top() + brc.height());				
            }

            m_email_button_rc = rc;
            unsigned h = utils::calcHeight(m_date, ard::defaultSmallFont());
            m_email_button_rc.setBottom(m_email_button_rc.bottom() - h);
            m_email_button_rc.setLeft(m_email_button_rc.right() - gui::lineHeight());
            m_email_button_rc.setTop(m_email_button_rc.bottom() - gui::lineHeight());
            p.drawPixmap(m_email_button_rc, getIcon_Email());

            if (m_email->hasAttachment())
            {
                m_attachment_button_rc = m_email_button_rc.translated(-m_email_button_rc.width(), 0);
                p.drawPixmap(m_attachment_button_rc, getIcon_AttachmentFile());
				m_mark_button_rc = m_attachment_button_rc.translated(-m_attachment_button_rc.width(), 0);
            }
			else {
				m_mark_button_rc = m_email_button_rc.translated(-m_email_button_rc.width(), 0);
			}
			p.drawPixmap(m_mark_button_rc, getIcon_AnnotationWhite());

            QPen penText(color::Red);
            p.setPen(penText);
            p.drawText(rc, Qt::AlignRight | Qt::AlignBottom, m_date);

			//..........
			auto th = m_email->parent();
			if (th)
			{
				auto clidx = th->colorIndex();
				if (clidx != ard::EColor::none)
				{
					auto pm = utils::colorMarkSelectorPixmap(clidx);
					if (pm)
					{
						QRect rc_clr = m_mark_button_rc;
						//rc_clr.setRight(rc_clr.left() + 100);
						rc_clr.setRight(m_mark_button_rc.left());
						rc_clr.setLeft(rc_clr.right() - 100);



						p.drawPixmap(rc_clr, *pm);

					}
				}

				if (th->isToDo())
				{
					QRectF rc1(rc.topRight().x() - gui::lineHeight(),
						rc.topRight().y(),
						gui::lineHeight(),
						gui::lineHeight());
					utils::drawCompletedBox(m_email, rc1, &p);
				}
			}
        }
    };

    void  mousePressEvent(QMouseEvent *e)override
    {
		m_selection = eselection::none;
		update();
        auto pt = e->pos();
        if (m_email_button_rc.contains(pt)) 
        {			
            processButtonClick();
            return;
        }
        else if (m_attachment_button_rc.contains(pt)) 
        {
			ard::attachements_dlg::runIt(m_email);
            return;
        }
		else if (m_mark_button_rc.contains(pt)) 
		{
			TOPICS_LIST lst;
			lst.push_back(m_email);
			QPoint pt = QCursor::pos();
			ard::SelectorTopicPopupMenu::showSelectorTopicPopupMenu(pt, lst, true);
			return;
		}
		else if (m_subject_rc.contains(pt)) 
		{
			m_selection = eselection::subject;
		}
		else if (m_from_rc.contains(pt))
		{
			m_selection = eselection::from;
		}
		else if (m_to_rc.contains(pt))
		{
			m_selection = eselection::to;
		}

		setFocus();

        QWidget::mousePressEvent(e);
    };

    void processButtonClick()
    {
        QAction* a = nullptr;
        QMenu m(this);
        ard::setup_menu(&m);
        ADD_MENU_ACTION("Reply", ard::menu::MCmd::email_reply);
        ADD_MENU_ACTION("Reply-All", ard::menu::MCmd::email_reply_all);
        ADD_MENU_ACTION("Forward", ard::menu::MCmd::email_forward);
        m.addSeparator();
        ADD_MENU_ACTION("New", ard::menu::MCmd::email_new);
        //m.addSeparator();
        //ADD_MENU_ACTION("Delete", ard::menu::MCmd::email_delete);
        connect(&m, &QMenu::triggered, [&](QAction* a)
        {
            googleQt::GmailRoutes* r = ard::gmail();
            if (r && m_email)
            {
                if (!m_email->optr()) {
					ard::errorBox(this, "Incomplete Email object");
                    return;
                }

                auto t = (ard::menu::MCmd)a->data().toInt();
                switch (t)
                {
                case ard::menu::MCmd::email_new:
                {
                    extern void createNewDraftNote();
                    createNewDraftNote();
                }break;
                case ard::menu::MCmd::email_reply:
                {
                    ard::edit_note(ard::email_draft::replyDraft(m_email), true);
                }break;
                case ard::menu::MCmd::email_reply_all:
                {
                    ard::edit_note(ard::email_draft::replyAllDraft(m_email), true);
                }break;
                case ard::menu::MCmd::email_forward:
                {
                    extern void createForwardDraftNote(ard::email* );
                    createForwardDraftNote(m_email);
                }break;
                default:break;
                }
            }
        });
        m.exec(QCursor::pos());
    };

protected:
    email_ptr           m_email{ nullptr };
    ard::TopicWidget*   m_twidget{nullptr};
    QString             m_to, m_from, m_cc, m_date;
    QRect               m_email_button_rc, m_attachment_button_rc, m_mark_button_rc,
						m_subject_rc, m_from_rc, m_to_rc;
	eselection			m_selection{ eselection::none };
};

/**
    EmailPreview
*/
EmailPreview::EmailPreview(ard::TopicWidget* w)//:m_twidget(w)
{
    QVBoxLayout* l = new QVBoxLayout();
    utils::setupBoxLayout(l);

    m_title_widget = new StaticEmailTitleWidget(w);//EmailTitleWidget(m_twidget);
    l->addWidget(m_title_widget);
#ifdef ARD_CHROME
    m_web_view = new QWebEngineView;
    utils::expandWidget(m_web_view);
    /*    QSizePolicy sp4preview(QSizePolicy::Expanding, QSizePolicy::Expanding);
    sp4preview.setHorizontalStretch(0);
    sp4preview.setVerticalStretch(0);    
    m_web_view->setSizePolicy(sp4preview);    
    */
    l->addWidget(m_web_view);
    ArdQWebEnginePage* pg = new ArdQWebEnginePage();
    m_web_view->setPage(pg);    
    qreal z = dbp::zoomChromeBrowser();
    if (z > 0 && z <= 5.0) {
        m_web_view->setZoomFactor(z);
    }
#else
    m_web_view = new ArdQTextBrowser;
    l->addWidget(m_web_view);
    int z = dbp::zoomTextBrowser();
    if (z > 0) {
        m_web_view->zoomIn(z);
    }
#endif
    setLayout(l);
};

EmailPreview::~EmailPreview() 
{
    detach();
};

void EmailPreview::attachEmail(email_ptr e) 
{
    detach();
    m_email = e;
    if (m_email)
    {
        LOCK(m_email);
        m_title_widget->attachEmail(m_email);
        auto c = m_email->mainNote();
        assert_return_void(c, "expected note");
#ifdef ARD_CHROME
        m_web_view->setHtml(c->html());
#else
        m_web_view->setPlainText(c->plain_text());
#endif
    }
};

void EmailPreview::reloadContent()
{	
    if (m_email)
    {
        auto c = m_email->mainNote();
        assert_return_void(c, "expected note");
#ifdef ARD_CHROME
        m_web_view->setHtml(c->html());
#else
        m_web_view->setHtml(c->plain_text());
#endif
		updateTitle();
    }
};

void EmailPreview::updateTitle() 
{
	m_title_widget->update();
};

void EmailPreview::zoomView(bool zoom_in) 
{
    if (m_email)
    {
#ifdef ARD_CHROME
        auto f = m_web_view->zoomFactor();
        if (zoom_in) {
            f += 0.25;
        }
        else {
            f -= 0.25;
        }
        m_web_view->setZoomFactor(f);
        dbp::setZoomChromeBrowser(f);
#else   
        int z = dbp::zoomTextBrowser();
        if (zoom_in) {
            z += 2;
            m_web_view->zoomIn(2);
        }
        else {
            z -= 2;
            m_web_view->zoomOut(2);
        }
        dbp::setZoomTextBrowser(z);
//      m_email->markAsZoomed();
#endif
    }
};

void EmailPreview::detach() 
{
    if (m_email)
    {
        m_email->release();
        m_email = nullptr;
    }
};


void EmailPreview::findText() 
{
	auto s = dbp::configFileLastSearchStr();
	s = ard::find_box::findWhat(s);
	while (!s.isEmpty()) {
		dbp::configFileSetLastSearchStr(s);
		m_web_view->findText(s);
		s = ard::find_box::findWhat(s);
	}
};

/**
    EmailTitleLineWidget
*/
static int _line_h = 4;
EmailTitleLineWidget::EmailTitleLineWidget() 
{
    setFixedHeight(_line_h);
    //setStyleSheet("background-color:black;");
    QPalette pal = palette();
    pal.setColor(QPalette::Background, Qt::black);
    setAutoFillBackground(true);
    setPalette(pal);
};

ard::email_draft_ext::attachement_file_list select_attachement_files_from_shell() 
{
	ard::email_draft_ext::attachement_file_list rv;

	QStringList filesList = QFileDialog::getOpenFileNames(main_wnd(),
		"Select one or more files to attach",
		dbp::configFileLastShellAccessDir());
	QString s = QString("selected %1 files").arg(filesList.size());

	if (filesList.size() > 0) {
		dbp::configFileSetLastShellAccessDir(filesList[0], true);
		std::set<QString> unique_str;
		for (auto& i : filesList) {
			if (unique_str.find(i) == unique_str.end())
			{
				ard::email_draft_ext::attachement_file af = { i, "" };

				rv.push_back(af);
				unique_str.insert(i);
			}
		}
	}
	return rv;
}

/**
    DraftEmailTitleWidget
*/
DraftEmailTitleWidget::DraftEmailTitleWidget(NoteFrameWidget* text_edit):m_text_edit(text_edit)
{
    QVBoxLayout* main_l = new QVBoxLayout();
    QHBoxLayout* send_l = new QHBoxLayout();
    utils::setupBoxLayout(main_l);
    utils::setupBoxLayout(send_l);
    setStyleSheet("QLineEdit { border: none }");    

    m_edit_subject = new HintLineEdit("Subject");
    main_l->addWidget(m_edit_subject);

    QGridLayout* gl = new QGridLayout;
    send_l->addLayout(gl);

    QToolButton* btn = nullptr;


    ADD_TOOL_BTN("attach", "Attach", send_l);
    m_att_btn = btn;
    connect(btn, &QPushButton::released, [&]()
    {
        if (m_attachementList.empty())
		{
			m_attachementList = select_attachement_files_from_shell();
			if (!m_attachementList.empty()) {
				m_attachment_info_modified = true;
			}
            saveTitleModified();
        }
        else {
            if (m_draft){
				ard::draft_attachements_dlg::runIt(m_draft, this);
                auto ext = m_draft->draftExt();
                m_attachementList = ext->attachementList();
                m_attachment_info_modified = true;
            }
        }
        updateAttachmentButton();
    });

    ADD_TOOL_BTN("send-email", "Send", send_l);
    connect(btn, &QPushButton::released, [&]()
    {
        if (m_draft)
        {
            saveTitleModified();
            if (m_draft->title().isEmpty())
            {
                ard::messageBox(this,"Please type in subject");
                return;
            }
            auto ext = m_draft->draftExt();
            ASSERT(ext, "expected extension");
            if (ext && ext->to().isEmpty())
            {
                ard::messageBox(this,"Please enter recipient");
                return;
            }

            if (!ext->verifyAttachments()) {
                if (!ard::confirmBox(ard::mainWnd(), "Some of the attachments can't be located. Do you want to proceed?")) {
                    return;
                }
            }

            m_text_edit->saveModified();
            m_draft->ensurePersistant(1);
            
			ard::close_popup(m_draft);
			ard::email_model* gm = ard::gmail_model();
			if (gm)
			{
				gm->schedule_draft(m_draft);
			}
			else 
			{
				ard::messageBox(this,"Gmail is not connected, cannot send email. Message will stored in 'Backlog'.");
			}
        }
    });    


    main_l->addLayout(send_l);

    
    m_edit_to = ard::buildEmailListEditor();
    m_edit_cc = ard::buildEmailListEditor();
    gl->addWidget(m_edit_to, 0, 1);
    gl->addWidget(m_edit_cc, 1, 1);
    

    QToolButton* pb = nullptr;
    pb = new QToolButton;
    pb->setText("To:");
    gl->addWidget(pb, 0, 0);
    connect(pb, &QToolButton::released, [&]()
    {
        runAB();
    });


    pb = new QToolButton;
    pb->setText("CC:");
    gl->addWidget(pb, 1, 0);
    connect(pb, &QToolButton::released, [&]()
    {
        runAB();
    });
    

    EmailTitleLineWidget* l = new EmailTitleLineWidget;
    main_l->addWidget(l);

    m_edit_subject->setFont(*utils::defaultBoldFont());

    setLayout(main_l); 
};


void DraftEmailTitleWidget::runAB() 
{
    ard::ContactsLookup predef_lst;
    if (m_edit_to) {
        ard::editor2lookupList(m_edit_to, nullptr, &predef_lst.to_list);
    }
    if (m_edit_cc) {
        ard::editor2lookupList(m_edit_cc, nullptr, &predef_lst.cc_list);
    }
    auto r = ard::address_book_dlg::select_receiver(ard::mainWnd(), &predef_lst);
    if (r.lookup_succeeded) {
        if (m_edit_to) {
            ard::lookupList2editor(r.to_list, m_edit_to);
        }
        if (m_edit_cc) {
            ard::lookupList2editor(r.cc_list, m_edit_cc);
        }
    }
};

void DraftEmailTitleWidget::updateAttachmentButton()
{
    assert_return_void(m_draft, "expected draft email object");
    assert_return_void(m_att_btn, "expected email attach button");
    
    QString s = "Attach";
    if (m_attachementList.size() > 1) {
        s = QString("[%1]").arg(m_attachementList.size());
    }
    else if (m_attachementList.size() == 1) 
	{
		auto att = m_attachementList[0];
		if (att.not_downloaded_att_id.isEmpty())
		{
			QFileInfo fi(att.file_path);
			s = googleQt::size_human(fi.size());
		}
		else 
		{
			s = QString("[%1]").arg(m_attachementList.size());
		}
    }
    m_att_btn->setText(s);
};

void DraftEmailTitleWidget::saveTitleModified() 
{
    if (m_draft)
    {
        auto ext = m_draft->draftExt();
        assert_return_void(ext, QString("expected draft extension %1").arg(m_draft->dbgHint()));

        if (m_edit_subject->isModified())
        {
            m_draft->setTitle(m_edit_subject->text());
            ext->setContentModified();
        }

        if (m_edit_to->document()->isModified() ||
            m_edit_cc->document()->isModified() ||
            m_attachment_info_modified)
        {
            ard::ContactsLookup res;
            ard::editor2lookupList(m_edit_to, nullptr, &res.to_list);
            ard::editor2lookupList(m_edit_cc, nullptr, &res.cc_list);
            auto str_to = ard::ContactsLookup::toAddressStr(res.to_list);
            auto str_cc = ard::ContactsLookup::toAddressStr(res.cc_list);

            ext->set_draft_data(str_to, str_cc, "", &m_attachementList);
        }

        m_edit_subject->setModified(false);
        m_edit_to->document()->setModified(false);
        m_edit_cc->document()->setModified(false);
        m_attachment_info_modified = false;
    }
};

void DraftEmailTitleWidget::attachDraft(ard::email_draft* d) 
{
    m_attachment_info_modified = false;
    detachDraft();
    m_draft = d;
    if (m_draft)
    {
        LOCK(m_draft);
        m_edit_subject->setText(m_draft->title());
        if (m_draft->draftExt())
        {
            auto ext = m_draft->draftExt();
            if (m_edit_to) {
                m_edit_to->setText(ext->to());
                m_edit_to->document()->setModified(false);
            }
            if (m_edit_cc) {
                m_edit_cc->setText(ext->cc());
                m_edit_cc->document()->setModified(false);
            }
            m_attachementList = ext->attachementList();
        }
        else 
        {
            ASSERT(0, "expected draft extension");
        }
        m_edit_subject->setModified(false);               

        m_edit_subject->setFocus();
        updateAttachmentButton();
    }
};

void DraftEmailTitleWidget::detachDraft() 
{
    if (m_draft){
        m_draft->release();
        m_draft = nullptr;
        m_attachementList.clear();
    };
};


#undef ADD_TOOL_BTN

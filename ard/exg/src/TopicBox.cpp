#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QRadioButton>
#include <QDebug>
#include <QButtonGroup>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QStandardItemModel>
#include <QTableView>
#include <QHeaderView>
#include <QClipboard>
#include <QTabWidget>
#include <QFileInfo>
#include <QPlainTextEdit>
#include <QTimer>
#include <QDesktopServices>
#include <QMenu>
#include <QApplication>


#include "TopicBox.h"
#include "email.h"
#include "custom-widgets.h"
#include "snc-tree.h"
#include <sstream>
#include "google/demo/ApiTerminal.h"
#include "gdrive/GdriveRoutes.h"
#include "Endpoint.h"
#include "contact.h"
#include "ethread.h"
#include "gmail/GmailRoutes.h"
#include "EmailSearchBox.h"
#include "ansearch.h"
#include "extnote.h"
#include "anurl.h"
#include "email_draft.h"
#include "picture.h"

extern QString size_human(float num);

void TopicBox::showTopic(topic_ptr f) 
{
    assert_return_void(f, "expected topic");

    TopicBox d(f);
    d.exec();
};

TopicBox::TopicBox(topic_ptr f)
{
    connect(this, &QDialog::finished, this, [=](int )
            {
                m_prop_layout = nullptr;
            });
    
    
    m_topic = f;
    LOCK(m_topic);
    
    QVBoxLayout *h_1 = new QVBoxLayout;
    QHBoxLayout *h_btns = new QHBoxLayout;
    m_tabs = new QTabWidget();
    m_tabs->setTabPosition(QTabWidget::East);
    h_1->addWidget(m_tabs);
    QStandardItemModel* sm = generateCurrentTopicModel();
    if (sm) {
        QTableView* v = createTableView(sm);
        QWidget* w = new QWidget();
        m_prop_layout = new QVBoxLayout;
        gui::setupBoxLayout(m_prop_layout);
        w->setLayout(m_prop_layout);
        m_prop_layout->addWidget(v);
        addTab(v, "Properties", w);
    }

    sm = generateCurrentEmailThreadModel();
    if (sm) {
        QTableView* v = createTableView(sm);
        QWidget* w = new QWidget();
        auto h = new QVBoxLayout;
        gui::setupBoxLayout(h);
        w->setLayout(h);
        h->addWidget(v);

        auto t = ard::as_ethread(m_topic);
        if (t) {
            auto t1 = t->optr();
            if (t1) {
                auto h1 = new QHBoxLayout;
                gui::setupBoxLayout(h1);
                h->addLayout(h1);

                QPushButton* b = nullptr;
                ADD_BUTTON2LAYOUT(h1, "FindInEList", &TopicBox::locateEThread);
            }
        }
        addTab(v, "EThread", w);
    }

    sm = generateCurrentEmailModel();
    if (sm) {
        QTableView* v = createTableView(sm);
        addTab(v, "Email");

        QString s = generateCurrentEmailSnapshotFromCloud();
        if (!s.isEmpty()) {
            m_email_snapshot = new QPlainTextEdit();
            m_tabs->addTab(m_email_snapshot, "Details");
            m_email_snapshot->setPlainText(s);
        }
    }

    sm = generateNoteModel();
    if (sm) {
        //QTableView* v = createTableView(sm);
        //addTab(v, "Note");

        //..
        QTableView* v = createTableView(sm);
        QWidget* w = new QWidget();
        auto vlayout = new QVBoxLayout;
        gui::setupBoxLayout(vlayout);
        w->setLayout(vlayout);
        vlayout->addWidget(v);

        auto n = m_topic->mainNote();
        if (n) {
            auto e_html = new QPlainTextEdit(n->html());
            auto e_plain = new QPlainTextEdit(n->plain_text());
            vlayout->addWidget(e_html);
            vlayout->addWidget(e_plain);
        }
        //auto e_html = 

        addTab(v, "Note", w);
        //..
    }

    auto u = dynamic_cast<ard::anUrl*>(m_topic);
    if (u) {
        QWidget* w = new QWidget();
        auto ul = new QHBoxLayout;
        gui::setupBoxLayout(ul);
        w->setLayout(ul);
        m_tabs->addTab(w, "Bookmark");
        m_url = new QPlainTextEdit;
        m_url->setPlainText(u->url());

        QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
        sizePolicy.setVerticalStretch(0);
        m_url->setSizePolicy(sizePolicy);

        ul->addWidget(m_url);
        QPushButton* b;
        b = ard::addBoxButton(ul, "set url", [&]() {
            u->setUrl(m_url->toPlainText().trimmed());
        });
    }

	////
	auto p = dynamic_cast<ard::picture*>(m_topic);
	if (p) {
		QWidget* w = new QWidget();
		auto ul = new QHBoxLayout;
		gui::setupBoxLayout(ul);
		w->setLayout(ul);
		m_tabs->addTab(w, "Image");
		auto media = new QPlainTextEdit;
		media->setPlainText(p->imageFileName());
		media->setReadOnly(true);

		QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
		sizePolicy.setVerticalStretch(0);
		media->setSizePolicy(sizePolicy);

		ul->addWidget(media);
	}

	///


    sm = generateCurrentContactModel();
    if (sm) {
        QTableView* v = createTableView(sm);
        addTab(v, "Contact");
    }

    sm = generateDraftModel();
    if (sm) {
        QTableView* v = createTableView(sm);
        addTab(v, "Email Draft");
    }


    h_1->addLayout(h_btns);
    ard::addBoxButton(h_btns, "Cancel", [&]() { close(); });
    if(QApplication::clipboard()){
        ard::addBoxButton(h_btns, "Copy", [&]()
                          { 
                              int idx = m_tabs->currentIndex();
                              auto it = m_idx2tableview.find(idx);
                              ASSERT(it != m_idx2tableview.end(), "Invalid tab index") << idx;
                              if(it != m_idx2tableview.end()){
                                  QTableView* pv = it->second;
                                  gui::copyTableViewSelectionToClipbard(pv);
                                  /*
                                  QModelIndexList si_lst = pv->selectionModel()->selectedIndexes();
                                  QString clipboardString;
                                  for(int i = 0; i < si_lst.count(); i++){
                                      auto mi = si_lst[i];
                                      clipboardString += mi.data().toString();
                                      if(i != si_lst.count() - 1){
                                          clipboardString += ", ";
                                      }
                                  }
                                  QApplication::clipboard()->setText(clipboardString);
                                  */
                              }
                          });
    }

    MODAL_DIALOG_SIZE(h_1, QSize(600, 900));
};

TopicBox::~TopicBox() 
{
    m_topic->release();
};

QTableView* TopicBox::createTableView(QStandardItemModel* m) 
{
    return gui::createTableView(m);
};

void TopicBox::addTab(QTableView* v, QString tab_label, QWidget* wrapper_widget)
{
    int idx = 0;
    if (wrapper_widget) {
        idx = m_tabs->addTab(wrapper_widget, tab_label);
    }
    else {
        idx = m_tabs->addTab(v, tab_label);
    }
    m_idx2tableview[idx] = v;
};

void TopicBox::locateEThread()
{
    auto t = ard::as_ethread(m_topic);
    if (t) {
        if (gui::ensureVisibleInOutline(t)) {
            ard::messageBox(this,"EThread located");
        }
        else {
            ard::messageBox(this,"EThread not found");
        }
        close();
    }
};

void TopicBox::setEmailDetails(QString s)
{
    if (m_email_snapshot) {
        m_email_snapshot->setPlainText(s);
        //auto m = generateGlobalLabelCacheModel();
        //m_label_view->setModel(m);
    }
};

QString TopicBox::generateCurrentEmailSnapshotFromCloud()
{
    auto m = ard::as_email(m_topic);

    auto route = ard::gmail();
    if (m && m->optr() && route)
    {
        std::set<QString> headers_to_print = { "From", "To", "Subject", "CC", "BCC", "References" };
        
        QString msg_id = m->optr()->id();
        QString userId = dbp::configEmailUserId();
        googleQt::gmail::IdArg mid(userId, msg_id);
        auto r = route->getMessages()->get_Async(mid);
        r->then([=](std::unique_ptr<googleQt::messages::MessageResource> r)
        {
            std::stringstream ss;

            ss << "id=" << r->id() << std::endl
                << "tid=" << r->threadid() << std::endl
                << "snippet=" << r->snippet() << std::endl;

            auto& labels = r->labelids();
            if (labels.size() > 0) {
                ss << "labels=";
                for (auto lb : labels) {
                    ss << lb << " ";
                }
                ss << std::endl;
            }

            auto p = r->payload();
            ss << "type: " << p.mimetype() << std::endl;
            auto header_list = p.headers();
            ss << "headers count: " << header_list.size() << std::endl;
            for (auto h : header_list)
            {
                if (headers_to_print.find(h.name()) != headers_to_print.end())
                    ss << h.name().leftJustified(20, ' ') << h.value() << std::endl;
            }

            auto& payload_body = p.body();
            if (!payload_body.attachmentid().isEmpty() || payload_body.size() > 0)
            {
                ss << "payload_body "
                    << " attId:" << payload_body.attachmentid()
                    << " size:" << payload_body.size()
                    << " data:" << payload_body.data().constData()
                    << std::endl;
            }

            if (p.mimetype().compare("text/html") == 0)
            {
                QByteArray payload_body = QByteArray::fromBase64(p.body().data(), QByteArray::Base64UrlEncoding);
                ss << payload_body.constData() << std::endl;
            }
            else
            {
                auto parts = p.parts();
                ss << "parts count: " << parts.size() << std::endl;
                for (auto pt : parts)
                {
                    auto& pt_body = pt.body();
                    QString partID = pt.partid();
                    ss << "------------------------------------------------"
                        << "part" << partID << "---------------------------------"
                        << std::endl;
                    ss << pt.mimetype() << " body-size:" << pt_body.size() << std::endl;
                    if (!pt_body.attachmentid().isEmpty()) {
                        ss << "attachmentid:" << pt_body.attachmentid() << std::endl;
                    }

                    bool is_plain_text = false;
                    bool is_html_text = false;
                    auto pt_headers = pt.headers();
                    for (auto h : pt_headers)
                    {
                        if (h.name() == "Content-Type") {
                            is_plain_text = (h.value().indexOf("text/plain") == 0);
                            is_html_text = (h.value().indexOf("text/html") == 0);
                        }
                        ss << "" << h.name().leftJustified(20, ' ') << " " << h.value() << std::endl;
                    }
                    if (is_plain_text || is_html_text)
                    {
                        ss << QByteArray::fromBase64(pt_body.data(), QByteArray::Base64UrlEncoding).constData() << std::endl;
                    }
                }
            }

            if (m_box) {
                QMetaObject::invokeMethod(m_box, "setEmailDetails", Qt::QueuedConnection, Q_ARG(QString, ss.str().c_str()));
            }

        }, [=](std::unique_ptr<googleQt::GoogleException> ex)
        {
            if (m_box) {
                QString s = ex->what();
                QMetaObject::invokeMethod(m_box, "setEmailDetails", Qt::QueuedConnection, Q_ARG(QString, s));
            }
        }
        );
    }

    return "loading email details..";
};

#define ADD_P(L, V)sm->setItem(row, 0, new QStandardItem(L));   \
    sm->setItem(row, 1, new QStandardItem(V));                  \
    row++;


QStandardItemModel* TopicBox::generateCurrentTopicModel() 
{
    QStandardItemModel* sm{nullptr};
    ArdDB* db = ard::db();
    if (m_topic && db)
    {
        auto f = m_topic;

        QStringList column_labels;
        column_labels.push_back("Property");
        column_labels.push_back("Value");
        sm = new QStandardItemModel();
        sm->setHorizontalHeaderLabels(column_labels);

        int row = 0;

        ADD_P("Type", f->objName() + "/" + f->folderTypeAsString());
        ADD_P("Title", f->title());

        QString ptrStr = QString("0x%1").arg((quintptr)f, QT_POINTER_SIZE * 2, 16, QChar('0'));
        ADD_P("PTR", ptrStr);

        //ADD_P("GdriveID/cropped", QString("%1").arg(f->cloudID(CloudIdType::GDriveId, false)));
       // ADD_P("GdriveID/original", QString("%1").arg(f->cloudID(CloudIdType::GDriveId, true)));
       // ADD_P("LocalID", QString("%1").arg(f->cloudID(CloudIdType::LocalDbId, false)));
        ADD_P("DBID", QString("%1").arg(f->id()));
        QString parent_str;
        auto p = f->parent();
        if (p) 
        {
            parent_str = QString("%1/%2/%3").arg(p->id()).arg(p->objName()).arg(p->title());
        }
        ADD_P("parent", parent_str);
        ADD_P("SYID", f->syid());
        ADD_P("ref-#", QString("%1").arg(f->counter()));
        ADD_P("mod-#", QString("%1").arg(f->modCounter()));
        ADD_P("mov-#", QString("%1").arg(f->moveCounter()));
        //isRetired()
        ADD_P("retired", QString("%1").arg(f->isRetired() ? "TRUE" : "FALSE"));
        size_t lsize = db->lookupMapSize();
        QString s_lookup = "";
        auto f2 = ard::lookup(f->id());
        if (f2 != NULL)
        {
            if (f2 != f)
            {
                s_lookup = "ERR-DIFF-ITEM";
            }
            else
            {
                s_lookup = "OK";
            }
        }
        else
        {
            s_lookup = "FAILED";
        }

        s_lookup += QString("[%1]").arg(lsize);
        ADD_P("mem-lookup", s_lookup);

        auto t = ard::as_ethread(f);
        if (t) {
            auto td = t->optr();
            if (td) {
                QString sbits(QString::number(td->labelsBitMap(), 2));
                ADD_P("LabelsBMap", QString("%1/%2").arg(td->labelsBitMap()).arg(sbits));
                auto labels = t->getLabels();
                for (auto& i : labels) {
                    ADD_P(QString("l:%1").arg(i->labelId()), QString("%1/%2").arg(i->labelMask()).arg(i->labelName()));
                }
            }
            else {
                ADD_P("TID", "ETHREAD-NOT-ATTACHED");

                auto e = t->getThreadExt();
                if (e) {
                    ADD_P("unresolved-TID", QString("%1").arg(e->threadId()));
                }
            }
        }

        TOPICS_LIST ans;
        ans.push_back(f);
        p = f->parent();
        while (p)
        {
            ans.push_back(p);
            p = p->parent();
        }
        QString s = "/";
        for (TOPICS_LIST::reverse_iterator i = ans.rbegin(); i != ans.rend(); ++i)
        {
            topic_ptr it = *i;
            s += it->title() + QString("(%1)").arg(it->id());
            if (i != ans.rend())
                s += "/";
        }
    }
    return sm;
};

QStandardItemModel* TopicBox::generateCurrentEmailModel()
{
    QStandardItemModel* sm{ nullptr };
    auto m = ard::as_email(m_topic);
    if (m && m->optr())
    {
        QStringList column_labels;
        column_labels.push_back("Property");
        column_labels.push_back("Value");
        sm = new QStandardItemModel();
        sm->setHorizontalHeaderLabels(column_labels);
        
        int row = 0;

        googleQt::mail_cache::msg_ptr mp = m->optr();
        QString lbl_string = "";
        auto lb_list = m->getLabels();
        for (auto & lb : lb_list) {
            lbl_string += lb->labelId();
            lbl_string += " ";
        }

        qlonglong d1 = mp->internalDate();
        QDateTime dt = QDateTime::fromMSecsSinceEpoch(d1);

		ADD_P("id", mp->id());
        ADD_P("labels", lbl_string);
        QString sbits(QString::number(mp->labelsBitMap(), 2));
        ADD_P("labelsBMap", QString("%1/%2").arg(mp->labelsBitMap()).arg(sbits));
        ADD_P("attachments", QString("%1").arg(m->getAttachments().size()));
        ADD_P("date", dt.toString());
        ADD_P("from", mp->from());
        ADD_P("to", mp->to());
        ADD_P("cc", mp->cc());
        ADD_P("bcc", mp->bcc());
        ADD_P("references", mp->references());
        ADD_P("subject", mp->subject());
        ADD_P("snippet", mp->snippet());
        ADD_P("plain", mp->plain());
        ADD_P("html", mp->html());

    }
    return sm;
};

QStandardItemModel* TopicBox::generateDraftModel() 
{
    QStandardItemModel* sm{ nullptr };
    auto d = dynamic_cast<ard::email_draft*>(m_topic);
    if (d) 
    {
        auto e = d->draftExt();
        assert_return_null(e, "expected draft extension");
        auto c = d->mainNote();
        assert_return_null(e, "expected draft note");

        QStringList column_labels;
        column_labels.push_back("Property");
        column_labels.push_back("Value");
        sm = new QStandardItemModel();
        sm->setHorizontalHeaderLabels(column_labels);

        int row = 0;
        ADD_P("To", e->to());
        ADD_P("InReplyTo", e->originalEId());
        ADD_P("references", e->references());
        ADD_P("threadId", e->threadId());
        ADD_P("CC", e->cc());
        ADD_P("BCC", e->bcc());
        ADD_P("userid", e->userIdOnDraft());
        ADD_P("attachments", e->attachmentsAsString());
        ADD_P("att.host", e->attachmentsHost());
        ADD_P("Title", d->title());
        ADD_P("Text", c->plain_text());

        QString str;
        auto lst = e->labelList();
        for (auto& i : lst){
            str += i;
            str += " ";
        }
        ADD_P("labels", str);
    }       

    return sm;
};

QStandardItemModel* TopicBox::generateCurrentEmailThreadModel()
{
    QStandardItemModel* sm{ nullptr };
    auto t = ard::as_ethread(m_topic);
    if (t && t->optr())
    {
        QStringList column_labels;
        column_labels.push_back("Property");
        column_labels.push_back("Value");
        sm = new QStandardItemModel();
        sm->setHorizontalHeaderLabels(column_labels);

        int row = 0;

        QString lbl_string = "";
        auto lb_list = t->getLabels();
        for (auto & lb : lb_list) {
            lbl_string += lb->labelId();
            lbl_string += " ";
        }

        auto tp = t->optr();
        ADD_P("id", tp->id());
        ADD_P("history_id", tp->historyId());
        ADD_P("messages_count", QString("%1").arg(tp->messagesCount()));
        ADD_P("emails", t->items().size() + 1);
        ADD_P("labels", lbl_string);

		googleQt::GmailRoutes* gm = ard::gmail();
		if (gm && gm->cacheRoutes()) {
			auto st = gm->cacheRoutes()->storage();
			auto qst = st->qstorage();			
			auto fmask = tp->filterMask();
			QString str = QString("%1").arg(fmask);
			auto qlst = qst->find_q_by_mask(fmask);
			for (auto q : qlst) {
				str += QString("[%1]").arg(q->name());
			}
			ADD_P("filters", str);
		}
    }
    return sm;
};

QStandardItemModel* TopicBox::generateNoteModel() 
{
    QStandardItemModel* sm{ nullptr };
    if (m_topic)
    {
        auto et = ard::as_ethread(m_topic);
        if (et) {
            return nullptr;
        }

        auto u = dynamic_cast<ard::anUrl*>(m_topic);
        if(u) {
            return nullptr;
        }
		auto p = dynamic_cast<ard::picture*>(m_topic);
		if (p) {
			return nullptr;
		}

        auto n = m_topic->mainNote();
        if (n) {
            QStringList column_labels;
            column_labels.push_back("Property");
            column_labels.push_back("Value");
            sm = new QStandardItemModel();
            sm->setHorizontalHeaderLabels(column_labels);

            int row = 0;
            ADD_P("PlainTextSize", QString("%1").arg(n->plain_text().size()));
            ADD_P("TextSize", QString("%1").arg(n->html().size()));
        }
    }
    return sm;
};

QStandardItemModel* TopicBox::generateCurrentContactModel()
{
    QStandardItemModel* sm{ nullptr };
    auto c = dynamic_cast<ard::contact*>(m_topic);
    if (c)
    {
        QStringList column_labels;
        column_labels.push_back("Property");
        column_labels.push_back("Value");
        sm = new QStandardItemModel();
        sm->setHorizontalHeaderLabels(column_labels);

        int row = 0;

        ADD_P("Title", c->title());
        ADD_P("DBID/cache", c->id());
        ADD_P("ID", c->syid());
        //ADD_P("etag", m->optr()->etag());
        //ADD_P("status", googleQt::gcontact::ContactXmlPersistant::status2string(m->optr()->status()));
        ADD_P("Email", c->contactEmail());
        ADD_P("Phone", c->contactPhone());
        ADD_P("Address", c->contactAddress());

        /*
        auto cache = ard::contacts_cache();
        if (cache) {
            QString imageFile = cache->getPhotoMediaPath(m->optr()->id());
            if (QFile::exists(imageFile)) {
                ADD_P("PhotoFile", imageFile);
            }
        }
        */
    }
    return sm;
};




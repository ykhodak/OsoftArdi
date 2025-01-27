#include <QXmlStreamReader>
#include "a-db-utils.h"
#include "email_draft.h"
#include "email.h"
#include "gmail/GmailRoutes.h"
#include "GoogleClient.h"
#include "contact.h"
#include "extnote.h"
#include "locus_folder.h"

extern QString gmail_access_last_exception;
extern QString gmail_send_last_exception;

bool ard::gui_check_and_notify_on_gmail_access_error()
{
    bool error_detected = false;
    bool show_next_msg = true;
    if (!gmail_access_last_exception.isEmpty()) {
		ard::messageBox(gui::mainWnd(), QString("Error detected during last server access. Please restart Ardi and check your network connection. Details: %1").arg(gmail_access_last_exception));
        gmail_access_last_exception = "";
        show_next_msg = false;
        error_detected = true;
    }

    if (!gmail_send_last_exception.isEmpty()) {
        error_detected = true;
        if (show_next_msg) {
			ard::messageBox(gui::mainWnd(), QString("Send Email - Error detected during last server access. Please restart Ardi and check your network connection. Details: %1").arg(gmail_send_last_exception));
            gmail_send_last_exception = "";
        }
    }

    return error_detected;
}

/**
    email_draft
*/
ard::email_draft::email_draft(QString title) :ard::topic(title)
{
    ensureDraftExt();
};

ard::email_draft::~email_draft() 
{
	if (attachements_forward_origin) {
		attachements_forward_origin->release();
	}
};

QString ard::email_draft::objName()const
{
    return "draft";
};

ard::email_draft* ard::email_draft::newDraft(QString userId, contact_ptr c /*= nullptr*/, QString email_address /*= ""*/)
{
    assert_return_null(ard::isDbConnected(), "expected DB connection");
    auto drafts = ard::Backlog();
    assert_return_null(drafts, "expected 'backlog' folder");
    email_draft* m = new email_draft();
    auto ext = m->ensureDraftExt();
    ext->setUserIdOnDraft(userId);
    QString lbl_name_personal = googleQt::mail_cache::sysLabelId(googleQt::mail_cache::SysLabel::CATEGORY_PERSONAL);
    ext->m_labelList.push_back(lbl_name_personal);
    if (c) {
        if (email_address.isEmpty()) {
            ext->m_to = c->contactEmail();
        }
        else {
            ext->m_to = email_address;
        }
    }
    drafts->insertItem(m, 0);
    drafts->ensurePersistant(1);
    return m;
};

ard::email_draft* ard::email_draft::prepareReplyDraft(email_ptr e, QString forceSubject /*= ""*/)
{
    assert_return_null(e, "expected email");
    assert_return_null(e->optr(), "expected completed email");
    email_draft* d = newDraft(dbp::configEmailUserId());
    assert_return_null(d, "expected 'draft' obj");
    QString subject;
    if (!forceSubject.isEmpty()) {
        subject = forceSubject;
    }
    else {
        subject = e->plainSubject();
        int re_index = subject.indexOf("Re:", 0, Qt::CaseInsensitive);
        if (re_index != 0)
        {
            subject = "Re:" + subject;
        }
    }
    d->setTitle(subject);

    if (e->optr()->isLoaded(googleQt::EDataState::body)) 
    {
        auto n = e->mainNote();
        if (n) {
            auto html = n->html();
            auto plain = n->plain_text();
            auto dn = d->mainNote();
            if (dn) {
                dn->setNoteHtml(html, plain);
            }
        }
    }
    else 
    {
        QString s_reply = "<br/><hr><br/>";
        s_reply += QString("<b>From:</b>%1<br/>").arg(e->from());
        s_reply += QString("<b>To:</b>%1<br/>").arg(e->to());
        s_reply += QString("<b>Cc:</b>%1<br/>").arg(e->CC());
        s_reply += QString("<b>Subject:</b>%1<br/>").arg(e->plainSubject());
        s_reply += "<br/><br/>";
        s_reply += e->snippet();
        d->setMainNoteText(s_reply);
    }

    auto e_ext = d->draftExt();
    assert_return_null(e_ext, "expected email ext");
    auto lb_list = e->getLabels();
    for(auto & lb : lb_list){
        if(lb->isPersonalCategory()){
            e_ext->m_labelList.push_back(lb->labelId());
            qDebug() << "adding label to draft-ext" << lb->labelId();
        }
    }
    
    return d;
};

ard::email_draft* ard::email_draft::replyDraft(email_ptr e)
{
    email_draft* d = prepareReplyDraft(e);
    assert_return_null(d, "expected draft");
    QString s_from = e->from();
    email_draft_ext* ext = d->draftExt();

    assert_return_null(ext, "expected draft extension");
    ext->setDraftFromEmail(e, s_from, "", "");
    return d;
};

ard::email_draft* ard::email_draft::replyAllDraft(email_ptr e)
{
    email_draft* d = prepareReplyDraft(e);
    assert_return_null(d, "expected draft");
    QString s_from = e->plainFrom();
    QString s_cc = e->plainCC();
    email_draft_ext* ext = d->draftExt();
    assert_return_null(ext, "expected draft extension");
    ext->setDraftFromEmail(e, s_from, s_cc, "");
    return d;
};

ard::email_draft* ard::email_draft::forward_draft(email_ptr e)
{
    assert_return_null(e, "expected email");
    assert_return_null(e->optr(), "expected completed email");
    QString subject = e->plainSubject();
    subject = "FW:" + subject;
    email_draft* d = prepareReplyDraft(e, subject);
    assert_return_null(d, "expected 'draft' obj");
    email_draft_ext* ext = d->draftExt();
    assert_return_null(ext, "expected draft extension");
    ext->setDraftFromEmail(e, "", "", "");
	d->attachements_forward_origin = e;
	LOCK(d->attachements_forward_origin);
    return d;
};

void ard::email_draft::mapExtensionVar(cit_extension_ptr e)
{
    ard::topic::mapExtensionVar(e);
    if (e->ext_type() == snc::EOBJ_EXT::extEmailDraftDetail) {
        ASSERT(!m_draftExt, "duplicate draft ext");
        m_draftExt = dynamic_cast<email_draft_ext*>(e);
        ASSERT(m_draftExt, "expected draft ext");
    }
};

ard::email_draft_ext* ard::email_draft::ensureDraftExt(/*email_draft_ext* pex*/)
{
    ensureExtension(m_draftExt);
    return m_draftExt;
};

ENoteView ard::email_draft::noteViewType()const
{
    return ENoteView::EditEmail;
}

bool ard::email_draft::hasDrafExt()const
{
    bool rv = false;
    if (m_draftExt) {
        rv = true;
    }
    return rv;
};

QString ard::email_draft::impliedTitle()const
{
    QString rv = title().trimmed();
    if (!rv.isEmpty()) {
        return rv;
    }

    rv = "DRAFT Email";
    return rv;
};

bool ard::email_draft::canAcceptChild(cit_cptr)const
{
    return false;
};

ard::email_draft_ext* ard::email_draft::draftExt()
{
    if (!m_draftExt)
    {
        m_draftExt = ensureDraftExt();
        ensurePersistant(1);
    }

    return m_draftExt;
};


bool ard::email_draft::isReadyToAutoClean()const
{
    bool rv = true;
    if (m_draftExt) {
        /// is draf was sent no need to keep it around any more
        rv = m_draftExt->m_sent_time.isValid();
        if (!rv) {
            if (!m_draftExt->isContentModified()) {
                auto c = mainNote();
                if (c) {
                    if (c->modTime().isValid()) {
                        auto d = c->modTime().daysTo(QDateTime::currentDateTime());
                        if (d > 2) {
                            rv = true;
                            qWarning() << "ready to clean up old empty email draft: " << id() << d << title();
                        }
                    }
                    else {
                        rv = true;
                    }
                }
                else {
                    rv = true;
                }
            }
        }
    }
    return rv;
};

topic_ptr ard::email_draft::cloneInMerge()const
{
    topic_ptr f = ard::topic::cloneInMerge();
    auto d = dynamic_cast<ard::email_draft*>(f);
    if (d) {
        if (m_draftExt) {
            auto e2 = d->ensureDraftExt();
            if (e2) {
                e2->assignSyncAtomicContent(m_draftExt);
            }
        }
    }
    return f;
}

void ard::email_draft::send_draft(googleQt::gclient_ptr gc)
{
    assert_return_void(m_draftExt, "expected draft extension.");
	assert_return_void(gc, "expected gmail client.");
	auto gm = gc->gmail();
	assert_return_void(gm, "expected gmail client routes.");

    m_draftExt->verifyAttachments();
    if (!m_draftExt->verifyAttachmentsOriginHost()) {
#if (QT_VERSION >= QT_VERSION_CHECK(5, 6, 0))
        ASSERT(0, "attachments hosts are different") << m_draftExt->attachmentsHost() << QSysInfo::machineHostName();
#endif
        return;
    }

	assert_return_void(m_draftExt->are_all_attachements_locally_resolved(), "Unresolved attachements found. Can't send draft");
	
	auto c = mainNote();
    assert_return_void(c, "expected note");

#ifdef _DEBUG
    qDebug() << "<<< === sending email" << m_draftExt->to();
    qDebug() << title();
    qDebug() << c->plain_text();
    qDebug() << "labels:" << m_draftExt->labelList().size();
    qDebug() << "InReplyTo:" << m_draftExt->originalEId();
    qDebug() << "references:" << m_draftExt->references();
    qDebug() << "threadId:" << m_draftExt->threadId();
    qDebug() << "<<< === end";
#endif

    googleQt::gmail::SendMimeMessageArg arg(ard::google()->userId(),
        m_draftExt->to(),
        title(),
        c->plain_text(),
        c->html());
    if (!m_draftExt->attachementList().empty()) {
		std::vector<QString> lst;
		for (auto& i : m_draftExt->attachementList()) {
			if (i.not_downloaded_att_id.isEmpty()) {
				lst.push_back(i.file_path);
			}
		}
		arg.addAttachments(lst);
        //arg.addAttachments(m_draftExt->attachementList());
    }

    arg.setInReplyToMsgId(m_draftExt->originalEId());
    arg.setReferences(m_draftExt->references());
    arg.setThreadId(m_draftExt->threadId());

    gm->getMessages()->send_Async(arg)->then([=](std::unique_ptr<googleQt::messages::MessageResource> msg)
    {
        //apply label to sent email
        //googleQt::GmailRoutes* gm2 = ard::gmail();
        //if (!gm2)
        //    return;
		auto gm2 = gc->gmail();
        auto labels_on_draf = m_draftExt->labelList();
        qDebug() << "labelListSize" << m_draftExt->labelList().size();
        if (!labels_on_draf.empty()) {
            auto it = labels_on_draf.begin();
            QString mlsb = *it;
            googleQt::gmail::ModifyMessageArg arg_lbl(ard::google()->userId(), msg->id());
            arg_lbl.addAddLabel(mlsb);
            arg_lbl.addRemoveLabel(googleQt::mail_cache::sysLabelId(googleQt::mail_cache::SysLabel::INBOX));
            arg_lbl.addRemoveLabel(googleQt::mail_cache::sysLabelId(googleQt::mail_cache::SysLabel::UNREAD));
            gm2->getMessages()->modify_Async(arg_lbl)->then([=](std::unique_ptr<googleQt::messages::MessageResource>)
            {
                //have to update label on email cache locally
                m_draftExt->markAsSent();
            },
                [](std::unique_ptr<googleQt::GoogleException> ex)
            {
                ASSERT(0, "Failed to modify label on 'sent' email") << ex->what();
            });
        }
        else {
            m_draftExt->markAsSent();
        }
    },
        [](std::unique_ptr<googleQt::GoogleException> ex)
    {           
        ASSERT(0, "Failed to send email") << ex->what();
        gmail_send_last_exception = ex->what();
        ard::asyncExec(AR_GmailErrorDetected, 1);
    });
};

void ard::email_draft::onModifiedNote()
{
    email_draft_ext* e_ext = draftExt();
    assert_return_void(e_ext, "expected email ext");
    e_ext->setContentModified();
};


/**
email_draft_ext
*/
ard::email_draft_ext::email_draft_ext()
{

};

ard::email_draft_ext::email_draft_ext(email_draft* _owner, QSqlQuery& q)
{
    attachOwner(_owner);
    m_original_eid = q.value(1).toString();
    m_mod_counter = q.value(2).toInt();
    m_to = q.value(3).toString();
    m_cc = q.value(4).toString();
    m_bcc = q.value(5).toString();  

    ///load attachments
    QString s_att = q.value(6).toString();
	if (!s_att.isEmpty())
	{
		auto s_list = s_att.split(",", QString::SkipEmptyParts);
		for (auto& s : s_list)
		{
			auto s2 = s.split("|", QString::KeepEmptyParts);
			if (s2.size() == 2)
			{
				auto att = attachement_file{ s2[0], s2[1] };
				m_attachementList.push_back(att);
				//qDebug() << "-recovered-db-attachements" << att.file_path << att.not_downloaded_att_id;
			}
			else 
			{
				qWarning() << "Error. Attachement info not properly loaded" << s_att;
			}
			//m_attachementList.push_back(s);
		}
	}
    //}
    m_attachementHost = q.value(7).toString();

///load labels
    QString s_lbl = q.value(8).toString();
    if (!s_lbl.isEmpty()) {
        QStringList s_list = s_lbl.split(",", QString::SkipEmptyParts);
        for(auto& s : s_list){
            m_labelList.push_back(s);
        }
    }

    m_content_modified = (q.value(9).toInt() > 0 ? true : false);
    m_references = q.value(10).toString();
    m_thread_id = q.value(11).toString();
    m_draft_userId = q.value(12).toString();
    _owner->addExtension(this);
    //_owner->ensureDraftExt(this);
}

snc::cit_primitive* ard::email_draft_ext::create()const
{
    return new ard::email_draft_ext();
};

void ard::email_draft_ext::assignSyncAtomicContent(const cit_primitive* _other)
{
    const email_draft_ext* other = dynamic_cast<const email_draft_ext*>(_other);
    assert_return_void(other, QString("expected prj root %1").arg(_other->dbgHint()));
    m_original_eid = other->originalEId();
    m_references = other->references();
    m_thread_id = other->threadId();
    m_to = other->to();
    m_cc = other->cc();
    m_bcc = other->bcc();
    m_attachementList = other->attachementList();
    m_attachementHost = other->attachmentsHost();
    m_labelList = other->labelList();
    m_content_modified = other->isContentModified();
    ask4persistance(np_ATOMIC_CONTENT);
};


void ard::email_draft_ext::setDraftFromEmail(email_ptr e, QString to, QString cc, QString bcc)
{
    assert_return_void(owner(), "expected owner");
    if (e->optr()) {
        m_original_eid = e->wrappedId();
        m_references = e->optr()->references();
        m_thread_id = e->optr()->threadId();
    }
    m_to = to;
    m_cc = cc;
    m_bcc = bcc;
    m_content_modified = false;
    ask4persistance(np_ATOMIC_CONTENT);
    ensureExtPersistant(owner()->dataDb());
};

void ard::email_draft_ext::set_draft_data(QString to, QString cc, QString bcc, const attachement_file_list* attachements)
{
    assert_return_void(owner(), "expected owner");
    if (!m_attachementHost.isEmpty()) {
#if (QT_VERSION >= QT_VERSION_CHECK(5, 6, 0))
        if (m_attachementHost != QSysInfo::machineHostName()) {
            ASSERT(0, "Can't remove attachment created on different machine") << QSysInfo::machineHostName() << m_attachementHost;
            return;
        }
#endif
    }

    m_to = to;
    m_cc = cc;
    m_bcc = bcc;
	if (attachements) {
		m_attachementList = *attachements;
	}
    if (!m_attachementList.empty()) {
#if (QT_VERSION >= QT_VERSION_CHECK(5, 6, 0)) 
        m_attachementHost = QSysInfo::machineHostName();
#endif
    }
    m_content_modified = true;
    ask4persistance(np_ATOMIC_CONTENT);
    if (owner() && owner()->dataDb()) {
        ensureExtPersistant(owner()->dataDb());
    }
};

void ard::email_draft_ext::setContentModified()
{
    assert_return_void(owner(), "expected owner");
    m_content_modified = true;
    ask4persistance(np_ATOMIC_CONTENT);
    ensureExtPersistant(owner()->dataDb());
};

void ard::email_draft_ext::remove_attachment(const email_draft_ext::attachement_file& att)
{
    assert_return_void(owner(), "expected owner");
#if (QT_VERSION >= QT_VERSION_CHECK(5, 6, 0))
    if (m_attachementHost != QSysInfo::machineHostName()) {
        ASSERT(0, "Can't remove attachment created on different machine") << QSysInfo::machineHostName() << att.file_path << att.not_downloaded_att_id;
        return;
    }
#endif	
	m_attachementList.erase(std::remove_if(m_attachementList.begin(), 
        m_attachementList.end(), 
        [&](attachement_file s) 
	{
		return (s.not_downloaded_att_id.compare(att.not_downloaded_att_id, Qt::CaseInsensitive) == 0 &&
			s.file_path.compare(att.file_path, Qt::CaseInsensitive) == 0); 
	}),
		m_attachementList.end());

    ask4persistance(np_ATOMIC_CONTENT);
    ensureExtPersistant(owner()->dataDb());
};

void ard::email_draft_ext::add_attachments(const attachement_file_list& attachements) 
{
	for (auto& i : attachements) {
		auto idx = find_attachment(i);
		if (idx == -1) {
			qDebug() << "ykh - adding new att" << i.file_path;
			m_attachementList.push_back(i);
		}
	}
	ask4persistance(np_ATOMIC_CONTENT);
	ensureExtPersistant(owner()->dataDb());
};

int ard::email_draft_ext::find_attachment(const attachement_file& att) 
{
	int idx = 0;
	for (auto& i : m_attachementList) {
		if (i.file_path == att.file_path &&
			i.not_downloaded_att_id == att.not_downloaded_att_id)
			return idx;
		idx++;
	}

	return -1;
};

QString ard::email_draft_ext::attachmentsAsString()const
{
	QString rv = "";
	if (!m_attachementList.empty())
	{
		for (auto& i : m_attachementList)
		{
			rv += i.file_path;
			rv += "|";
			rv += i.not_downloaded_att_id;
			rv += ",";
		}
		rv = rv.left(rv.length() - 1);
	}
	return rv;
    //return googleQt::slist2commalist(m_attachementList);
};

bool ard::email_draft_ext::are_all_attachements_locally_resolved()
{
	for (auto& i : m_attachementList) {
		if (!i.not_downloaded_att_id.isEmpty()) {
			return false;;
		}
	}
	return true;
};

bool ard::email_draft_ext::rebuild_unresolved_attachements(googleQt::gclient_ptr gc, ard::email* att_msg)
{
	assert_return_false(gc, "expected gclient");
	assert_return_false(att_msg, "expected forward origin");
	auto mst = gc->gmail_storage();
	assert_return_false(mst, "expected gmail storage");

	using ID2ATT = std::unordered_map<QString, googleQt::mail_cache::att_ptr>;
	ID2ATT id2att_completed;
	auto lst = att_msg->getAttachments();
	for (auto a : lst) 
	{
		if (a->status() == googleQt::mail_cache::AttachmentData::EStatus::statusDownloaded) {
			id2att_completed[a->attachmentId()] = a;
		}
	}

	for (auto& i : m_attachementList) 
	{
		if (!i.not_downloaded_att_id.isEmpty()) {
			auto j = id2att_completed.find(i.not_downloaded_att_id);
			if (j != id2att_completed.end()) 
			{
				QString filePath = mst->findAttachmentFile(j->second);
				if (!filePath.isEmpty()) 
				{
					i.not_downloaded_att_id = "";
					i.file_path = filePath;
					qDebug() << "ykh-resolved-att-file" << filePath;
				}		
				//i.file_path = a->lo
			}
		}
	}

	ask4persistance(np_ATOMIC_CONTENT);
	ensureExtPersistant(owner()->dataDb());

	bool rv = are_all_attachements_locally_resolved();
	if (rv) {
		qDebug() << "ykh-all-att-locally-resolved";
	}
	return rv;
};

QString ard::email_draft_ext::labelsAsString()const
{
    return googleQt::slist2str(m_labelList.begin(), m_labelList.end());
};


void ard::email_draft_ext::markAsSent()
{
    m_sent_time = QDateTime::currentDateTime();
    if (owner() && owner()->dataDb()) {
        dbp::updateSentTime(this, owner()->dataDb());
    }
};

bool ard::email_draft_ext::isAtomicIdenticalTo(const cit_primitive* _other, int& )const
{
    const email_draft_ext* other = dynamic_cast<const email_draft_ext*>(_other);
    ASSERT(other, "invalid other object pointer");

    COMPARE_EXT_ATTR(originalEId());
    COMPARE_EXT_ATTR(to());
    COMPARE_EXT_ATTR(cc());
    COMPARE_EXT_ATTR(bcc());
    COMPARE_EXT_ATTR(attachmentsAsString());
    COMPARE_EXT_ATTR(attachmentsHost());
    COMPARE_EXT_ATTR(labelsAsString());
    return true;
};

QString ard::email_draft_ext::calcContentHashString()const
{
    QString tmp = QString("%1 %2 %3").arg(m_to).arg(m_cc).arg(m_bcc);
    QString rv = QCryptographicHash::hash((tmp.toUtf8()), QCryptographicHash::Md5).toHex();
    return rv;
};

uint64_t ard::email_draft_ext::contentSize()const
{
    uint64_t rv = m_to.size() + m_cc.size() + m_bcc.size();
    return rv;
};

bool ard::email_draft_ext::verifyAttachments()const
{
    bool rv = true;
    if (!m_attachementList.empty())
    {
        for (auto& i : m_attachementList) 
		{
			if (i.not_downloaded_att_id.isEmpty()) {
				if (!QFile::exists(i.file_path)) {
					ASSERT(0, "attachment file not found") << i.file_path;
					return false;
				}
			}
        }
    }
    return rv;
};

bool ard::email_draft_ext::verifyAttachmentsOriginHost()const
{
    bool rv = true;
    if (!m_attachementList.empty())
    {
#if (QT_VERSION >= QT_VERSION_CHECK(5, 6, 0))
        rv = (m_attachementHost == QSysInfo::machineHostName());
#endif
    }
    return rv;
};

/**
    draft_attachment_item
*/
ard::draft_attachment_item::draft_attachment_item(const email_draft_ext::attachement_file& att)
	:m_att_file(att)
{
	if (att.not_downloaded_att_id.isEmpty()) {
		m_title = att.file_path;
	}
	else{
		m_title = QString("loading..%1").arg(att.not_downloaded_att_id);
	}
    //m_title = file_path;
};

QPixmap ard::draft_attachment_item::getIcon(OutlineContext)const
{
    return QPixmap();
};

QPixmap ard::draft_attachment_item::getSecondaryIcon(OutlineContext)const
{
    return QPixmap();
};

bool ard::draft_attachment_item::hasCurrentCommand(ECurrentCommand c)const
{
    bool rv = false;
    if (m_att_file.not_downloaded_att_id.isEmpty())
	{
        switch (c)
        {
        case ECurrentCommand::cmdOpen:
        case ECurrentCommand::cmdFindInShell:
        case ECurrentCommand::cmdDelete:
            rv = true;
            break;
        default:break;
        }
    }
	else 
	{
		/// we haven't downloaded attachement yet..
		switch (c)
		{
		case ECurrentCommand::cmdDelete:
			rv = true;
			break;
		default:break;
		}
	}
    return rv;
};

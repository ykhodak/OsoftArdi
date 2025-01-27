#pragma once

#include "anfolder.h"
#include "tooltopic.h"

class SyncAutoTest;

namespace ard {
    extern bool gui_check_and_notify_on_gmail_access_error();

    class email_draft_ext;
    class contact;

    class email_draft : public ard::topic
    {
    public:
        email_draft() {};
		~email_draft();

        static email_draft*   newDraft(QString userId, contact_ptr c = nullptr, QString email_address = "");
        static email_draft*   replyDraft(email_ptr e);
        static email_draft*   replyAllDraft(email_ptr e);
        static email_draft*   forward_draft(email_ptr e);

        cit_primitive*      create()const override { return new email_draft; }
        topic_ptr           cloneInMerge()const override;
        QString             objName()const override;
        email_draft_ext*    ensureDraftExt();
        EOBJ                otype()const override { return objEmailDraft; };
        ENoteView           noteViewType()const override;
        bool                isReadyToAutoClean()const override;

        QString             impliedTitle()const override;
        ///we can't have any subitems
        bool                canAcceptChild(cit_cptr it)const override;

        ///will return existing extension or create a new one
        email_draft_ext*    draftExt();
        ///will not autocreate new extension
        bool                hasDrafExt()const;

        void                onModifiedNote()override;
    protected:
        void					mapExtensionVar(cit_extension_ptr e)override;
        email_draft_ext*		m_draftExt{ nullptr };
		ard::email*				attachements_forward_origin{nullptr};

        static email_draft*		prepareReplyDraft(email_ptr e, QString forceSubject = "");
		void					send_draft(googleQt::gclient_ptr);
    private:
        email_draft(QString title);
		friend class ::SyncAutoTest;
		friend class ard::email_model;
    };


    /**
    email_draft_ext - contains details on draft email topic
    */
    class email_draft_ext : public ardiExtension<email_draft_ext, email_draft>
    {
        friend class email_draft;
        DECLARE_DB_EXTENSION_PERSISTANT(snc::EOBJ_EXT::extEmailDraftDetail, "email-draft-detail", "ard_ext_draft");
    public:

		struct attachement_file
		{
			QString file_path;
			QString not_downloaded_att_id;
		};
		using attachement_file_list = std::vector<attachement_file>;

        ///default constructor
        email_draft_ext();
        ///for recovering from DB
        email_draft_ext(email_draft* _owner, QSqlQuery& q);

        QString userIdOnDraft()const { return m_draft_userId; }
        void setUserIdOnDraft(QString userId) { m_draft_userId = userId; }

        QString to()const { return m_to; }
        QString cc()const { return m_cc; }
        QString bcc()const { return m_bcc; }
        QString originalEId()const { return m_original_eid; }
        const QDateTime& sentTime()const { return m_sent_time; }
        QString references()const { return m_references; }
        QString threadId()const { return m_thread_id; }

        bool    isContentModified()const { return m_content_modified; }
        void    setContentModified();

        snc::cit_primitive* create()const override;
        void   assignSyncAtomicContent(const cit_primitive* other)override;
        bool   isAtomicIdenticalTo(const cit_primitive* other, int& iflags)const override;
        QString calcContentHashString()const override;
        uint64_t contentSize()const override;

        void set_draft_data(QString to, QString cc, QString bcc = "", const attachement_file_list* attachements = nullptr);
        void remove_attachment(const email_draft_ext::attachement_file& att);
		void add_attachments(const attachement_file_list& attachements);
		int  find_attachment(const attachement_file& att);

        const attachement_file_list& attachementList()const { return m_attachementList; }
        QString attachmentsAsString()const;
        QString attachmentsHost()const { return m_attachementHost; }
        bool	verifyAttachments()const;
        bool	verifyAttachmentsOriginHost()const;
		bool	are_all_attachements_locally_resolved();
		bool	rebuild_unresolved_attachements(googleQt::gclient_ptr gc, ard::email* att_msg);

        const std::vector<QString>& labelList()const { return m_labelList; }
        QString labelsAsString()const;
    protected:
        void setDraftFromEmail(email_ptr e, QString to, QString cc, QString bcc);
        void markAsSent();

    protected:
        QString					m_draft_userId;
        QString					m_to, m_cc, m_bcc, m_original_eid, m_references, m_thread_id;
        bool					m_is_sent;
        bool					m_content_modified{ false };
        QDateTime				m_sent_time;
		attachement_file_list	m_attachementList;  //list of files with full path
        QString					m_attachementHost;  //machine name with attachment files
        std::vector<QString>	m_labelList;        //list of labels to set on sending email

		friend class email_model;
    };

    /**
    draft_attachment_item - to display attachement info in scene (upload/attachment dialog)
    */
    class draft_attachment_item : public ard::tool_topic
    {
    public:
        draft_attachment_item(const email_draft_ext::attachement_file& att);

        QPixmap     getIcon(OutlineContext c)const override;
        QPixmap     getSecondaryIcon(OutlineContext c)const override;
        bool        hasCurrentCommand(ECurrentCommand c)const override;

        QString     filePath()const { return m_att_file.file_path; }
		const email_draft_ext::attachement_file& att_file()const { return	m_att_file; }

    protected:
		email_draft_ext::attachement_file	m_att_file;
    };
};//mail
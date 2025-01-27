#pragma once

#ifdef ARD_GD
#include <memory>
#include <QTimer>
#include "tooltopic.h"

namespace ard {
    class email_model;
	class email_draft;
    googleQt::gclient_ptr google();
    /**
        email - wrapper around googleQt class
        !email - we can't really serialize objects when m_d or storage are invalid
    */
    class email : public ard::topic
    {
        DECLARE_IN_ALLOC_POOL(email);
    public:
        virtual ~email();

		googleQt::mail_cache::msg_ptr		optr() { return m_o; }
		const googleQt::mail_cache::msg_ptr optr()const { return m_o; }
        topic_ptr       produceLabelSubject() override;
        /// create new object that wrappes underlying email but is not owned by gmail cache
        static email_ptr    create_out_of_shadow_obj(googleQt::mail_cache::msg_ptr& d);

        ///parent thread
        ethread_ptr         parent();
        ethread_cptr        parent()const;
		bool				isValid()const override;

        QString     snippet()const;
        QString     title()const override;
        QString     objName()const override;

        QPixmap     getIcon(OutlineContext c)const override;
        QPixmap     getSecondaryIcon(OutlineContext c)const override;
        virtual cit_base*   create()const override;

        bool        isContentLoaded()const override;
 
        ard::note_ext*             mainNote()override;
        const ard::note_ext*       mainNote()const override;
        bool                isWrapper()const override { return true; }

        ENoteView   noteViewType()const override;

        EOBJ        otype()const override { return objEmail; };
        bool        isSynchronizable()const override { return false; }
        bool        isPersistant()const override { return false; }

        int         accountId()const;
        QString     wrappedId()const override;
        QString     plainSubject()const;
        QString     plainFrom()const;
        QString     plainTo()const;
        QString     plainCC()const;
        QString     plainBCC()const;

        QString     from()const;
        QString     to()const;
        QString     CC()const;
        QString     BCC()const;
        QString     references()const;

        qlonglong   internalDate()const;
        QString     dateColumnLabel()const override;
        std::vector<googleQt::mail_cache::label_ptr> getLabels()const override;
        bool        isStarred()const;
        bool        isImportant()const override;
        bool        isUnread()const;
        bool        hasPromotionLabel()const;
        bool        hasSocialLabel()const;
        bool        hasUpdateLabel()const;
        bool        hasForumsLabel()const;
        bool        hasAttachment()const override;
        googleQt::mail_cache::ATTACHMENTS_LIST getAttachments();

        void        setUnread(bool set_it);
        void        setStarred(bool set_it)override;
        void        setImportant(bool set_it)override;

        bool        canRename()const override { return false; }
        bool        canMove()const override { return false; };
        bool        canHaveCard()const override { return true; }
        bool        hasCustomTitleFormatting()const override { return true; }
        void        prepareCustomTitleFormatting(const QFont&, QList<QTextLayout::FormatRange>&)override;
        topic_ptr   prepareInjectIntoBBoard()override;
        void        downloadAttachment(googleQt::mail_cache::att_ptr a);
		void        download_all_attachments();

        QString     dbgHint(QString s = "")const override;

        //bool        kill(bool silently = false)override;
        bool        killSilently(bool) override;
        DB_ID_TYPE      underlyingDbid()const override;

        ///todo section
        void            setToDo(int done_percent, ToDoPriority prio)override;
        bool            isToDo()const override;
        bool            isToDoDone()const override;
        int             getToDoDonePercent()const override;
        int             getToDoDonePercent4GUI()const override;
        unsigned char   getToDoPriorityAsInt()const override;
        ToDoPriority    getToDoPriority()const override;
        bool            hasToDoPriority()const override;
        topic_ptr       getToDoContext()override;

        void            setColorIndex(EColor c)override;
		EColor			colorIndex()const override;

        bool            canBeMemberOf(const topic_ptr)const override;
        bool            canAcceptChild(cit_cptr it)const override;
        topic_ptr       produceMoveSource(ard::email_model*)override;

        /// annotation section
        QString         annotation()const override;
        QString         annotation4outline()const override;
        void            setAnnotation(QString s, bool gui_update = false)override;
        bool            canHaveAnnotation()const override;

        /// dont want to draw content on blackboard
        QString         plainNoteText4Blackboard()const override { return ""; };
        bool            canHaveCardToolOptButton()const override{ return false; };

    protected:
        void        calc_headers()const;
        bool        hasLabelId(QString label_id)const;
        bool        hasSysLabel(googleQt::mail_cache::SysLabel)const;
        ard::note_ext* ensureNote()override;

        mutable union UFlags {
            int64_t flags;
            struct {
                unsigned from_length : 11;
                unsigned subject_length : 11;
                unsigned snippet_length : 11;
                unsigned calculated : 1;
                unsigned attachments_queried : 1;
                unsigned has_attachments : 1;
            };
        }            m_emailFlags;
		googleQt::mail_cache::msg_ptr m_o;
        mutable QString m_calc_title;
        mutable QString m_calc_plain_subject;
        mutable QString m_calc_date_label;
    private:
        ///outline object constructor
        //email();
        ///shadow object constructor
        email(googleQt::mail_cache::msg_ptr d);

        friend class email_model;
        friend class ethread;
        friend class ArdDB;
    };

    /**
        email_model - utility class for googleQt/gmail
    */
    class email_model : public QObject
    {
        Q_OBJECT
    public:
		email_model();
        virtual ~email_model();

        /// this is to list wrapped thread
        ethread_ptr lookupFirstWrapperOrWrapGthread(googleQt::mail_cache::thread_ptr& d);

        /// we accept map [threadid -> [emailid]] -> [email_ptr]
        std::vector<email_ptr> lookupOrWrapGthreadMessages(std::unordered_map<QString, std::vector<QString>>);

        /// if thread is outside outline we will add it to threads root
        topic_ptr		adoptThread(ethread_ptr t);
        THREAD_VEC      linkThreadWrappers(THREAD_VEC& th_vec); ///returns list of unresolvable threads - threads without extensions

        void            releaseWrapper(googleQt::mail_cache::thread_ptr o);

        googleQt::mail_cache::GMailSQLiteStorage* gstorage();

        void detachModelGui();
        bool isGoogleConnected();
		bool isGmailConnected()const { return m_gmail_cache_attached; };
        void disableGoogle();
        bool connectGoogle();
        bool reconnectGoogle(QString userId);
        bool authAndConnectNewGoogleUser();

		void	schedule_draft(ard::email_draft*);
		void    fire_all_attachments_downloaded(ard::email*);

		void	trashThreadsSilently(const THREAD_SET& lst);
		void	markUnread(const ESET& lst, bool set_it);
		void	markTrashed(const ESET& lst);

#ifdef _DEBUG
        void debugFunction();
#endif
	signals:
		void all_attachments_downloaded(ard::email*);

	public slots:
		void	download_progress(qint64, qint64);
		void	upload_progress(qint64, qint64);

    protected:
        /// we attach gmail object to ardi object that alredy injected in space (has parent owning it)
        void    attachAdoptedThread(ethread_ptr t, googleQt::mail_cache::thread_ptr d);
        void    registerAdopted(ethread_ptr t);
        bool    setupGmailCache();
        bool    initGClientUsingExistingToken();
        bool    guiInitGClientByAcquiringNewToken();
        void    releaseGClient();
        void    asyncQueryCloudAndLinkThreadWrappers(const std::vector<googleQt::HistId>& id_list2load);
		void    doRegister(ethread_ptr t);

        googleQt::gclient_ptr	m_gclient;
		bool					m_gmail_cache_attached{ false };
		S2TREADS				m_threads2resolve;
        QTime                   m_last_q_time;
		THREAD_VEC				m_wrapped;

		/// drafts scheduler - we might wait for event(attachement downloaded) before sending draft///
		struct draft_schedule_point 
		{
			googleQt::gclient_ptr	gclient{nullptr};
			ard::email*				attachements_forward_origin{nullptr};
		};
		using DRAFT_SCHEDULE = std::unordered_map<ard::email_draft*, draft_schedule_point>;
		DRAFT_SCHEDULE				m_scheduled_drafts;
		QTimer						m_drafts_scheduler_timer;
		void						install_drafts_scheduler_timer();
		void						run_drafts_scheduler();

		/// gclient - we release only when all downloades finished ///
		using GCLIENTS = std::vector<googleQt::gclient_ptr>;
		GCLIENTS					m_clients2release;
		QTimer						m_clients_release_timer;
		void						install_clients_release_timer();

		friend googleQt::gclient_ptr ard::google();
    };


    /**
        email_attachment - to display attachement info in scene (download dialog)
    */
    class email_attachment : public ard::tool_topic
    {
    public:
        email_attachment(googleQt::mail_cache::att_ptr att);
        googleQt::mail_cache::att_ptr attachment() { return m_att; }

        QPixmap     getIcon(OutlineContext c)const override;
        QPixmap     getSecondaryIcon(OutlineContext c)const override;
        bool        hasCurrentCommand(ECurrentCommand c)const override;

    protected:
        googleQt::mail_cache::att_ptr m_att;
        mutable QPixmap     m_local_file_icon;
    };

    /**
    anAccountInfoItem - to display gmail-account info in dialog
    */
    class email_account_info : public ard::tool_topic
    {
    public:
        email_account_info(googleQt::mail_cache::acc_ptr acc);
        bool canRename()const override { return false; }
        bool hasCurrentCommand(ECurrentCommand c)const override;
        void updateAttr();
        int accountId()const;
        QPixmap     getIcon(OutlineContext)const override;
    protected:
        bool m_isCurrent;
        googleQt::mail_cache::acc_ptr m_acc;
    };

};//mail

#endif //ARD_GD

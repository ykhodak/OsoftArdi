#pragma once

#ifdef ARD_GD
#include <memory>
#include "anfolder.h"
#include "tooltopic.h"
#include "gmail/GmailRoutes.h"

class ArdDB;
namespace ard {
    class email_model;
    class email;
};

namespace ard {
    class ethread_ext;

    /**
        ethread - wrapper around googleQt email thread
    */
    class ethread : public ard::topic
    {
        DECLARE_IN_ALLOC_POOL(ethread);
    public:
        virtual ~ethread();

        topic_ptr   tspaceTopic()override;

        QString         title()const override;
        QString         objName()const override;
        EOBJ            otype()const override { return objEThread; };
        cit_prim_ptr    create()const override;
        QString         serializable_title()const override { return QString(""); }

        qlonglong       internalDate()const;
        QString         dateColumnLabel()const override;

        bool            isExpandedInList()const override;

        topic_ptr       produceLabelSubject() override;
        email_ptr       headMsg();
        email_cptr      headMsg()const;
        googleQt::mail_cache::thread_ptr optr();
        QString         threadId()const;
        ethread_ext*    getThreadExt() { return m_EthreadExt; }
        const ethread_ext*  getThreadExt()const { return m_EthreadExt; }

        email_ptr       lookupMessage(googleQt::mail_cache::msg_ptr d);

        bool            hasCustomTitleFormatting()const override;
        void            prepareCustomTitleFormatting(const QFont&, QList<QTextLayout::FormatRange>&)override;
        std::vector<googleQt::mail_cache::label_ptr> getLabels()const override;
        QPixmap         getSecondaryIcon(OutlineContext)const override { return QPixmap(); };
        bool            hasColorByHashIndex()const override;

        bool			canAcceptChild(cit_cptr it)const override;
        bool			canAttachNote()const override { return false; };
		topic_ptr       produceMoveSource(ard::email_model*)override;


        QString     impliedTitle()const override;
        ENoteView   noteViewType()const override;
        ard::note_ext*    mainNote()override;
        const ard::note_ext*    mainNote()const override;
        bool        hasSynchronizableItems()const override { return false; }
        bool        isWrapper()const override { return true; }
        QString     wrappedId()const override;
        topic_ptr   popupTopic()override;

        bool        hasAttachment()const override;
        bool        canHaveCard()const override;
        bool        canRename()const override { return false; }

        void        clear()override;

        topic_ptr       prepareInjectIntoBBoard()override;
        bool            isExpandableInBBoard()const override{ return false; };

        ///todo section
        void            setToDo(int done_percent, ToDoPriority prio)override;
        /// annotation
        void            setAnnotation(QString s, bool gui_update = false)override;
        /// color
        void            setColorIndex(EColor c)override;

        ethread_ext*    ensureThreadExt();

        bool			isAtomicIdenticalPropTo(const cit_primitive* other, int& iflags)const override;
        bool			isAdoptedInOutline()const;
        void			resetThread();
		bool			isValid()const override;

    protected:
        void        clear_thread();
        bool        injectInOutlineSpace();
        void        mapExtensionVar(cit_extension_ptr e)override;

        email_ptr   m_headMsg1{ nullptr };
        ethread_ext* m_EthreadExt{ nullptr };
        googleQt::mail_cache::thread_ptr m_o;
    private:
        ///outline object constructor
        ethread();
        ///shadow object constructor
        ethread(googleQt::mail_cache::thread_ptr d);

        friend class ard::email_model;
        friend class ::ArdDB;
    };

    /**
    ethread_ext - ID and details of email thread
    */
    class ethread_ext : public ardiExtension<ethread_ext, ethread>
    {
        DECLARE_DB_EXTENSION_PERSISTANT(snc::EOBJ_EXT::extThreadOfEmails, "ethread", "ard_ext_ethread");
    public:

        ///default constructor
        ethread_ext();
        ///for recovering from DB
        ethread_ext(ethread_ptr _owner, QSqlQuery& q);

        ~ethread_ext();

        QString     accountEmail()const { return m_account_email; }
        QString     threadId()const { return m_thread_id; }

        snc::cit_primitive* create()const override;
        void        assignSyncAtomicContent(const cit_primitive* other)override;
        bool        isAtomicIdenticalTo(const cit_primitive* other, int& iflags)const override;
        QString     calcContentHashString()const override;
        uint64_t    contentSize()const override;
    protected:
        QString     m_account_email;
        QString     m_thread_id;
        friend class ethread;
    };
};
#endif //ARD_GD

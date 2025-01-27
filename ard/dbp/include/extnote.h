#pragma once
#include "anfolder.h"

namespace ard
{
    class note_ext : public ardiExtension<note_ext, ard::topic>
    {
        DECLARE_DB_EXTENSION_PERSISTANT(snc::EOBJ_EXT::extNote, "note-ext", "ard_ext_note");
    public:
        ///default constructor
        note_ext();
        ///for recovering from DB
        note_ext(ard::topic* _owner, QSqlQuery& q);
        ~note_ext();

        ///html - text of the note
        QString             html()const;
        ///plain_text - plain text with stripped out HTML tags
        QString             plain_text()const;
        ///plain_text4draw - used for drawing part of note in boxes
        QString             plain_text4draw()const;
        ///plain_text4title - used for puttin on title
        QString             plain_text4title()const;
        ///setHtml - sets HTML & plaint text
        void                setNoteHtml(QString html, QString plain_text);

        ///document - return QTextDocument object responsible for presenting text in view
        QTextDocument*      document();
        ///hasDocument - returns true is Document object is attached
        bool                hasDocument()const { return (m_document != nullptr); };
        ///detachDocument - detaches internal Document object
        void                detachDocument() { m_document = nullptr; }

        void                setupFromDb(QString text, QString plain_test);

        const QDateTime&    modTime()const { return m_mod_time; }

        bool                isAtomicIdenticalTo(const cit_primitive* other, int& iflags)const override;
        void                assignSyncAtomicContent(const cit_primitive* other)override;
        snc::cit_primitive* create()const override;
        QString             calcContentHashString()const override;
        uint64_t            contentSize()const override;
        
        bool                saveIfModified();
        void                docVScroll(int& vscroll, int& width)const { vscroll = m_DocVScroll; width = m_DocWidth; };
        void                setDocVScroll(int vscroll, int width) { m_DocVScroll = vscroll; m_DocWidth = width; }
        void                dropTextFile(const QUrl& url, QTextCursor& cr);
        void                dropHtml(const QString& ml, QTextCursor& cr);
        void                dropText(const QString& str, QTextCursor& cr);
        void                dropUrl(const QUrl& u, QTextCursor& cr);
    protected:
        void                queryGui()const;
        void                doSetNoteHtml(QString html, QString plain_text);

        int                 m_DocVScroll;
        int                 m_DocWidth;
        QString             m_html, m_plain_text;
        mutable QString     m_plain_text4draw;
        QDateTime           m_mod_time;
        QTextDocument*      m_document{nullptr};

        mutable union NoteFlags {
            unsigned char flags;
            struct {
                unsigned loaded : 1;
            };
        } m_noteFlags;

        friend class ard::topic;
    };
};

#include <QTextEdit>
#include "a-db-utils.h"
#include "extnote.h"

ard::note_ext::note_ext() 
{
    m_noteFlags.flags = 0;
};

ard::note_ext::note_ext(ard::topic* _owner, QSqlQuery& q) 
{
    m_noteFlags.flags = 0;
    attachOwner(_owner);
    m_mod_counter = q.value(1).toInt();
    m_mod_time = QDateTime::fromTime_t(q.value(2).value<uint>());
    _owner->addExtension(this);
};

ard::note_ext::~note_ext() 
{
    if (m_document)m_document->deleteLater();
};

/*ard::topic* ard::note_ext::owner() 
{
	auto rv = dynamic_cast<ard::topic*>(cit_owner());
	return rv;
};

const ard::topic* ard::note_ext::owner()const 
{
	auto rv = dynamic_cast<const ard::topic*>(cit_owner());
	return rv;
};*/

QString ard::note_ext::html()const
{
    queryGui();
    return m_html;
}

QString ard::note_ext::plain_text()const
{
    queryGui();
    return m_plain_text;
}

QString ard::note_ext::plain_text4draw()const
{
    queryGui();
    if (m_plain_text4draw.isEmpty())
    {
        if (!m_plain_text.isEmpty())
        {
            m_plain_text4draw = m_plain_text.left(512).trimmed();
            m_plain_text4draw.replace("\n\n\n", "\n");
            m_plain_text4draw.replace("\n\n", "\n");
        }
    }

    return m_plain_text4draw;
}

QString ard::note_ext::plain_text4title()const
{
    queryGui();
    QString s = m_plain_text.left(128).trimmed();
    auto idx = s.indexOf("\n");
    if (idx > 4) {
        s = s.left(idx);
    }
    else {
        s.replace("\n\n\n", "\n");
        s.replace("\n\n", "\n");
    }
    return s;
};

void ard::note_ext::setupFromDb(QString text, QString plain_test)
{
    m_html = text;
    m_plain_text = plain_test;
    m_noteFlags.loaded = 1;
}

void ard::note_ext::queryGui()const
{
    if (!m_noteFlags.loaded)
    {
        m_plain_text4draw = "";
        ///!not sure how it will work with emails..		
        if (IS_VALID_DB_ID(owner()->id()))
        {           
            ard::note_ext* ThisNonConst = const_cast<ard::note_ext*>(this);
            dbp::load_note(ThisNonConst);
        }
        m_noteFlags.loaded = 1;
    }
};

void ard::note_ext::setNoteHtml(QString html, QString plain_text)
{
    if (html.compare(m_html) == 0)
        return;

    doSetNoteHtml(html, plain_text);

    if (hasDocument())
    {
        m_document->setHtml(html);
        m_document->setModified(false);
    }
}

void ard::note_ext::doSetNoteHtml(QString html, QString plain_text)
{
    assert_return_void(owner(), "expected owner");
    ASSERT_VALID(this);
    if (html.compare(m_html) == 0)
        return;

    m_html = html;
    m_plain_text = plain_text;
    m_plain_text4draw = "";
    m_noteFlags.loaded = 1;

    if (IS_VALID_DB_ID(owner()->id())) 
    {
        setSyncModified();
        ask4persistance(np_SYNC_INFO);
        ask4persistance(np_ATOMIC_CONTENT);
        ensureExtPersistant(owner()->dataDb());
    }
};


QTextDocument* ard::note_ext::document()
{
	auto w = gui::mainWnd();
    assert_return_null(w, "expected main window");

    if (!m_document)
    {
        m_document = new QTextDocument;
        auto f = ard::defaultNoteFont();
        m_document->setDefaultFont(*f);
        m_document->setHtml(html());
        m_document->setModified(false);
    }
    return m_document;
};


bool ard::note_ext::isAtomicIdenticalTo(const cit_primitive* _other, int& )const 
{
    assert_return_false(_other, "expected item [1]");
    auto other = dynamic_cast<const ard::note_ext*>(_other);
    assert_return_false(other, "expected item [2]");
    bool rv = (m_html == m_html && m_plain_text == m_plain_text);
    return rv;
};

void ard::note_ext::assignSyncAtomicContent(const cit_primitive* _other) 
{
    assert_return_void(_other, QString("expected note "));
    auto other = dynamic_cast<const ard::note_ext*>(_other);
    assert_return_void(other, QString("expected note %1").arg(_other->dbgHint()));
    m_html = other->m_html;
    m_plain_text = other->m_plain_text;
    m_noteFlags.loaded = 1;
    ask4persistance(np_ATOMIC_CONTENT);
};

snc::cit_primitive* ard::note_ext::create()const 
{
    return new note_ext;
};

QString ard::note_ext::calcContentHashString()const 
{
    QString rv = QCryptographicHash::hash((m_html + m_plain_text).toUtf8(), QCryptographicHash::Md5).toHex();
    return rv;
};

uint64_t ard::note_ext::contentSize()const 
{
    uint64_t rv = m_html.size() + m_plain_text.size();
    return rv;
};

bool ard::note_ext::saveIfModified()
{
    bool rv = false;
    if (gui::isDBAttached() && hasDocument()) {
        rv = document()->isModified();
        if (rv)
        {
            doSetNoteHtml(document()->toHtml().trimmed(), document()->toPlainText().trimmed());
            document()->setModified(false);

            topic_ptr it = dynamic_cast<topic_ptr>(cit_owner());
            if (it) {
                it->onModifiedNote();
            }
        }
    }
    return rv;
};

void ard::note_ext::dropTextFile(const QUrl& url, QTextCursor& cr)
{
    QString sfile = url.toLocalFile();

    QFile file(sfile);

    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        cr.insertText(file.readAll());
        document()->setModified(true);
        saveIfModified();
    }
    else
    {
        ASSERT(0, QString("failed to open text file: %1 %2").arg(sfile).arg(file.errorString()));
    }
};

void ard::note_ext::dropHtml(const QString& ml, QTextCursor& cr)
{
    cr.insertHtml(ml);
    document()->setModified(true);
    saveIfModified();
};

void ard::note_ext::dropText(const QString& str, QTextCursor& cr)
{
    cr.insertText(str);
    document()->setModified(true);
    saveIfModified();
};

void ard::note_ext::dropUrl(const QUrl& u, QTextCursor& cr)
{
    cr.insertText(u.toString());
    document()->setModified(true);
    saveIfModified();
};

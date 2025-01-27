#include <QPainter>
#include <QScrollBar>
#include <QVBoxLayout>
#include <QScroller>
#include <QApplication>

#include "NoteEdit.h"
#include "utils.h"
#include "MainWindow.h"
#include "ardmodel.h"
#include "OutlineMain.h"
#include "global-media.h"
#include "NoteFrameWidget.h"
#include "TabControl.h"
#include "popup-widgets.h"
#include "extnote.h"

#define VSCROL_TIMER_TIMEOUT 100

/**
WorkingNoteEdit
*/
WorkingNoteEdit::WorkingNoteEdit() 
{
    m_VScrolTimer = new QTimer(this);
	connect(m_VScrolTimer, &QTimer::timeout, this, &WorkingNoteEdit::VscrolTimerTimeout);
    //connect(m_VScrolTimer, SIGNAL(timeout()),
    //    this, SLOT(VscrolTimerTimeout()));

    auto vbar = verticalScrollBar();
    if (vbar) {
        m_vscroll_width = style()->pixelMetric(QStyle::PM_ScrollBarExtent);

        connect(vbar, &QScrollBar::rangeChanged,
            [&](int, int)
        {
            if (m_topic) {
                if (m_VScrolTimer->isActive()) {
                    m_VScrolTimer->stop();
                    m_VScrolTimer->start(VSCROL_TIMER_TIMEOUT);
                }
            }
        });
    }

    //installLocationAnimator();

    connect(this, &QTextEdit::textChanged,
        [&]() {m_hasEmptyNoteImage = false; });

};

WorkingNoteEdit::~WorkingNoteEdit() 
{
    detachTopic();
};

void WorkingNoteEdit::attachTopic(topic_ptr f)
{
    assert_return_void(f, "expected valid topic");
    auto n = f->mainNote();
    assert_return_void(n, "expected valid topic");
    if (f != m_topic)
    {
        detachTopic();
        m_topic = f;
        LOCK(f);

        m_hasEmptyNoteImage = false;
        int len = n->plain_text().size();
        setDocument(n->document());

        if (len > 0)
        {
            m_hasEmptyNoteImage = false;
        }
        else
        {
            m_hasEmptyNoteImage = true;
        }

        if (globalTextFilter().isActive())
        {
            find(globalTextFilter().fcontext().key_str);
        }
        else
        {
            m_VScrolTimer->start(VSCROL_TIMER_TIMEOUT);
        }
    }
};

void WorkingNoteEdit::detachTopic(bool saveChanges)
{
    if (gui::isDBAttached() && m_topic)
    {
        auto n = m_topic->mainNote();
        assert_return_void(n, "expected valid topic");

        if (saveChanges)
        {
            saveIfModified();
        }
        else
        {
            ///revert back changes if any
            if (n->hasDocument() && n->document()->isModified())
            {
                n->document()->setHtml(n->html());
                n->document()->setModified(false);
            }
        }
        m_topic->release();
        m_topic = nullptr;
        QTextDocument* d = new QTextDocument(this);
        setDocument(d);
        d->setPlainText("- This buffer is for notes you don't want to save\n- If you want to save text select existing or create a new note");
    }

    if (!gui::isDBAttached() && m_topic)
    {
        //this should happened only when main windows is closing
        //and there are still popups
        auto n = m_topic->mainNote();
        assert_return_void(n, "expected valid topic");
        n->detachDocument();
        m_topic->release();
        m_topic = nullptr;
    }
};

bool WorkingNoteEdit::saveIfModified()
{
    bool rv = false;
    if (m_topic)
    {
        auto n = m_topic->mainNote();
        if (n)
        {
            rv = n->saveIfModified();

            int vscrol = verticalScrollBar()->value();
            int dwidth = verticalScrollBar()->maximum();
            n->setDocVScroll(vscrol, dwidth);
        }
    }
    return rv;
};

void WorkingNoteEdit::VscrolTimerTimeout()
{
    m_VScrolTimer->stop();

    auto c = comment();
    if (c)
    {
        int vscroll = 0;
        int dwidth = 0;
        c->docVScroll(vscroll, dwidth);
        if (vscroll > 0 && dwidth > 0 && verticalScrollBar())
        {
            int curr_max = verticalScrollBar()->maximum();
            if (curr_max != dwidth)
            {
                int new_scroll = (int)((qreal)vscroll * curr_max / dwidth);
                vscroll = new_scroll;
            }
            verticalScrollBar()->setValue(vscroll);
        }
    }
};

ard::note_ext* WorkingNoteEdit::comment() 
{
    ard::note_ext* rv = nullptr;
    if (m_topic){
        rv = m_topic->mainNote();
    }
    return rv;
};

const ard::note_ext* WorkingNoteEdit::comment()const 
{
    const ard::note_ext* rv = nullptr;
    if (m_topic) {
        rv = m_topic->mainNote();
    }
    return rv;
};

void WorkingNoteEdit::paintEvent(QPaintEvent *e)
{
    if (m_hasEmptyNoteImage)
    {
        QWidget* v = viewport();
        QPainter p(v);
        QRect rc = v->geometry();
        p.setFont(*utils::groupHeaderFont());
        QPen pn(Qt::gray);
        p.setPen(pn);
        p.drawText(rc, Qt::AlignHCenter | Qt::AlignVCenter | Qt::TextWordWrap, "Start typing to create a new note");
    }

    NoteEditBase::paintEvent(e);
};

void WorkingNoteEdit::keyPressEvent(QKeyEvent *e)
{
    auto km = QApplication::keyboardModifiers();

    switch (e->key()) 
    {
    case Qt::Key_D: {
        if (km & Qt::AltModifier) {
            auto str = dbp::currentDateStamp() + " ";
            insertPlainText(str);
            return;
        }
    }break;
    case Qt::Key_T: {
        if (km & Qt::AltModifier) {
            auto str = dbp::currentTimeStamp() + " ";
            insertPlainText(str);
            return;
        }
    }break;

    }

    NoteEditBase::keyPressEvent(e);
};


void WorkingNoteEdit::mergeFormatOnWordOrSelection(const QTextCharFormat &format)
{
    QTextCursor cursor = textCursor();
    if (!cursor.hasSelection())
        cursor.select(QTextCursor::WordUnderCursor);
    cursor.mergeCharFormat(format);
    mergeCurrentCharFormat(format);
}


void WorkingNoteEdit::pastePlainText() 
{
#ifndef QT_NO_CLIPBOARD
    QClipboard *clipboard = QApplication::clipboard();
    const QMimeData *md = clipboard->mimeData();
    if (md && md->hasText())
    {
        auto s = md->text();
        insertPlainText(s);
        //rv = md->text().left(128).trimmed();
    }
#endif
};

/**
    CardEdit
*/
CardEdit::CardEdit(ard::TopicWidget* w) :m_w(w)
{

};

void CardEdit::focusInEvent(QFocusEvent * e)
{
    QTextEdit::focusInEvent(e);
    if (m_w) {
        m_w->makeActiveInPopupLayout();
    }
};

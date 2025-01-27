#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QColorDialog>
#include <QComboBox>
#include <QFontComboBox>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFontDatabase>
#include <QMenuBar>
#include <QShortcut>
#include <QTextCodec>
#include <QTextEdit>
#include <QToolBar>
#include <QTextCursor>
#include <QTextDocumentWriter>
#include <QTextList>
#include <QtDebug>
#include <QCloseEvent>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QDesktopWidget>

#include "NoteFrameWidget.h"
#include "utils.h"
#include "anfolder.h"
#include "MainWindow.h"
#include "ardmodel.h"
#include "NoteEdit.h"
#include "OutlineMain.h"
#include "OutlineSceneBase.h"
#include "TabControl.h"
#include "OutlineTitleEdit.h"
#include "NoteToolbar.h"
#include "EmailPreview.h"
#include "custom-boxes.h"
#include "extnote.h"

/**
   NoteFrameWidget
*/

NoteFrameWidget::NoteFrameWidget(ard::TopicWidget* w)
{
    QVBoxLayout* main_l = new QVBoxLayout();
    utils::setupBoxLayout(main_l);

    m_text_edit1 = new CardEdit(w);
    utils::expandWidget(m_text_edit1);
    connect(m_text_edit1, SIGNAL(currentCharFormatChanged(QTextCharFormat)),
        this, SLOT(currentCharFormatChanged(QTextCharFormat)));
    m_email_title_edit_widget = new DraftEmailTitleWidget(this);
    main_l->addWidget(m_email_title_edit_widget);


    m_tb = new NoteTextToolbar(this, w);
    main_l->addWidget(m_tb->editToolbar());
    m_tb->attachEditor(this);
    m_tb->showToolbar(false);


    main_l->addWidget(m_text_edit1);
    setStyleSheet("QToolBar{background:palette(Window);}");  
    
    setLayout(main_l);
    m_text_edit1->setFocus();

    fontChanged(m_text_edit1->font());
    colorChanged(QColor(Qt::black), QColor(Qt::white));
    alignmentChanged(m_text_edit1->alignment());
}

NoteFrameWidget::~NoteFrameWidget()
{
}

bool NoteFrameWidget::saveModified()
{
	if (m_email_title_edit_widget && m_email_title_edit_widget->isEnabled())
	{
		m_email_title_edit_widget->saveTitleModified();
	}

	return m_text_edit1->saveIfModified();
}

void NoteFrameWidget::detachOwner()
{
    m_text_edit1->detachTopic();

    if (m_email_title_edit_widget && m_email_title_edit_widget->isEnabled())
    {
        m_email_title_edit_widget->detachDraft();
    }
};

void NoteFrameWidget::attachTopic(topic_ptr it)
{
    assert_return_void(it, "Expected owner");
    assert_return_void(it->mainNote(), "Expected comment");

    saveModified();

    switch (it->noteViewType())
    {
    case ENoteView::Edit:
    {
        ENABLE_OBJ(m_email_title_edit_widget, false);
    }break;
    case ENoteView::EditEmail:
    {
        auto d = dynamic_cast<ard::email_draft*>(it);
        if (d)
        {
            ENABLE_OBJ(m_email_title_edit_widget, true);
            m_email_title_edit_widget->attachDraft(d);
        }
        else
        {
            ASSERT(0, "expected draft" << it->dbgHint());
        }
    }break;
    default:ASSERT(0, "unsupported edit type") << (int)it->noteViewType() << it->dbgHint();
    }
    m_text_edit1->attachTopic(it);

#ifdef ARD_BIG
    m_tb->attachEditor(this);
#endif
};

void NoteFrameWidget::detachGui()
{
    detachOwner();
};

void NoteFrameWidget::setFocusOnEdit(/*bool animate*/)
{
    m_text_edit1->setFocus();
    /*
    QPointF pt(0,0);
    auto e = new QMouseEvent(QEvent::MouseButtonPress, pt, Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    QCoreApplication::postEvent(m_text_edit1, e);
    e = new QMouseEvent(QEvent::MouseButtonRelease, pt, Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    QCoreApplication::postEvent(m_text_edit1, e);
    */
    //m_text_edit1->setFocus(Qt::MouseFocusReason);
    //m_text_edit1->animateLocator();
    //if (animate) {
    //    m_text_edit1->animateLocator();
    //}
};

void NoteFrameWidget::locateFilterText() 
{
    if (globalTextFilter().isActive())
    {
        QString s = globalTextFilter().fcontext().key_str;
        if (!m_text_edit1->find(s))
        {
            QTextCursor c = m_text_edit1->textCursor();
            c.movePosition(QTextCursor::Start, QTextCursor::MoveAnchor, 0);
            m_text_edit1->find(s);
            m_text_edit1->setTextCursor(c);
        };        
    }
    m_text_edit1->animateLocator();
};

void NoteFrameWidget::searchForText(QString s) 
{
    if (!m_text_edit1->find(s))
    {
        QTextCursor c = m_text_edit1->textCursor();
        c.movePosition(QTextCursor::Start, QTextCursor::MoveAnchor, 0);
        m_text_edit1->find(s);
        m_text_edit1->setTextCursor(c);
    };

    m_text_edit1->animateLocator();
};

void NoteFrameWidget::openSearchWindow() 
{
    auto s = dbp::configFileLastSearchStr();
    s = ard::find_box::findWhat(s);
    while (!s.isEmpty()) {
        dbp::configFileSetLastSearchStr(s);
        searchForText(s);
        s = ard::find_box::findWhat(s);
    }
};

void NoteFrameWidget::selectAll() 
{
    m_text_edit1->setFocus();
    m_text_edit1->selectAll();
};

int NoteFrameWidget::replaceText(QString sFrom, QString sTo) 
{
    extern int replaceTextInEditor(QString sFrom, QString sTo, QTextEdit& working_edit);

    auto cm = m_text_edit1->comment();
    int replaced = replaceTextInEditor(sFrom, sTo, *m_text_edit1);
    cm->setNoteHtml(m_text_edit1->toHtml().trimmed(), m_text_edit1->toPlainText().trimmed());
    return replaced;
};

void NoteFrameWidget::formatText(const QTextCharFormat *fmt) 
{
    extern void formatTextInEditor(const QTextCharFormat* fmt, QTextEdit& working_edit);

    auto cm = m_text_edit1->comment();
    formatTextInEditor(fmt, *m_text_edit1);
    cm->setNoteHtml(m_text_edit1->toHtml().trimmed(), m_text_edit1->toPlainText().trimmed());
};

void zoomDocFont(QTextDocument* d, qreal z)
{
    if (d) {
        QTextBlock b = d->begin();

        while (b.isValid()) {
            QTextCursor c(b);
            c.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
            QTextCharFormat fm = c.charFormat();
            qreal fsz = fm.fontPointSize();
            fm.setFontPointSize(fsz + z);
            c.setCharFormat(fm);
            b = b.next();
        }
    }
};

void NoteFrameWidget::zoomIn()
{
    zoomDocFont(m_text_edit1->document(), 2.0);
};

void NoteFrameWidget::zoomOut()
{
    zoomDocFont(m_text_edit1->document(), -2.0);
};


ard::topic_tab_page* ard::edit_note(topic_ptr n, bool in_new_tab)
{
    assert_return_null(n, "expected topic");
    auto c = n->mainNote();
    assert_return_null(c, "expected note");
    return ard::open_page(n, in_new_tab ? true : false);
};


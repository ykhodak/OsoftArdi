#include <QPainter>
#include "anfolder.h"
#include "board.h"
#include "email.h"
#include "email_draft.h"
#include "fileref.h"
#include "popup-widgets.h"
#include "NoteFrameWidget.h"
#include "CardPopupMenu.h"
#include "BlackBoard.h"
#include "EmailPreview.h"
#include "extnote.h"
#include "anurl.h"
#include "picture.h"
#include "MainWindow.h"
#include "tda_view.h"

#ifdef ARD_BIG
extern bool bPopupLayoutON;

QRect defaultPopupGeometry(){return QRect (10, 20, 600, 300);}

const int annotation_drawing_flags = Qt::AlignTop | Qt::AlignLeft | Qt::TextWordWrap | Qt::TextExpandTabs;

class EmbededAnnotationEdit :  public QTextEdit,
                        public LocationAnimator
{
public:
    EmbededAnnotationEdit(EmbededAnnotationCard *c):m_annotation_card(c)
    {
        setParent(c);
		//color::getColorStringByClrIndex(11)
        setStyleSheet(QString("QTextEdit {border: 1px solid rgb(255,255,192);background-color:rgb(%1)}").arg("232,232,175"));
        setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        setFont(*ard::defaultFont());


        auto f = c->topic();
        if (f) {
            setPlainText(f->annotation().trimmed());
        }

        installLocationAnimator();
    }

    void storeAnnotation() 
    {
        auto f = m_annotation_card->topic();
        if (f) {
            auto str = toPlainText().trimmed();
            if (str != f->annotation()) {
                f->setAnnotation(str);
                m_annotation_card->resizeToText();
                m_annotation_card->twidget()->resetAnnotationCardPos();
                gui::rebuildOutline();
            }

            if (str.isEmpty()) {
                m_annotation_card->hide();
            }
        }
    }

    void focusOutEvent(QFocusEvent *e)override 
    {
        storeAnnotation();
        close();
        QTextEdit::focusOutEvent(e);
    }

    void paintEvent(QPaintEvent *e)override
    {
        QPainter p(viewport());
        drawLocator(p);
        PGUARD(&p);
        QRect rc_ico = rect();
        int ico_w = gui::lineHeight();
        rc_ico.setLeft(rc_ico.right() - ico_w);
        rc_ico.setBottom(rc_ico.top() + ico_w);
        p.setOpacity(0.7);
        p.drawPixmap(rc_ico, getIcon_PinBtn());
        QTextEdit::paintEvent(e);
    }

    void mousePressEvent(QMouseEvent *e)override 
    {
        auto pt = e->pos();
        QRect rc_ico = rect();
        int ico_w = gui::lineHeight();
        rc_ico.setLeft(rc_ico.right() - ico_w);
        rc_ico.setBottom(rc_ico.top() + ico_w);
        if (rc_ico.contains(pt)) {          
            auto f = m_annotation_card->topic();
            if (f) {
                storeAnnotation();
                ard::popup_annotation(f);
                close();
                return;
            }
        }
        QTextEdit::mousePressEvent(e);
    };

    QRect locatorCursorRect()const override
    {
        QRect rc(0,0,gui::lineHeight(),gui::lineHeight());
        return rc;
    }

    void locatorUpdateRect(const QRect& rc)override
    {
        viewport()->update(rc);
    };

protected:
    EmbededAnnotationCard *m_annotation_card{nullptr};
};

/**
* EmbededAnnotationCard
*/
EmbededAnnotationCard::EmbededAnnotationCard(ard::TopicWidget* w) :m_w(w)
{
    ASSERT(m_w->topic(), "expected topic");
    setParent(m_w);
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowSystemMenuHint);
    setAttribute(Qt::WA_DeleteOnClose);
    setObjectName("annotationCard");
    resizeToText();
};

void EmbededAnnotationCard::resizeToText()
{
    QSize sz(200, 100);
    auto annotation = m_w->topic()->annotation();
    if (!annotation.isEmpty()) {
        QRect rc2(QPoint(0, 0), sz);
        QFontMetrics fm(*ard::defaultFont());
        QRect rc3 = fm.boundingRect(rc2, annotation_drawing_flags, annotation);
        if (rc3.height() > sz.height()) {
            sz.setWidth(200);
            rc3 = fm.boundingRect(rc2, annotation_drawing_flags, annotation);
            if (rc3.height() > sz.height()) {
                sz.setHeight(300);
            }
            //sz.setHeight(300);
        }
    }
    resize(sz);
    /*
    QSize sz(200, 100);
    auto annotation = m_w->topic()->annotation();
    if (!annotation.isEmpty()) {
        QRect rc2(QPoint(0, 0), sz);
        QFontMetrics fm(*ard::defaultFont());
        QRect rc3 = fm.boundingRect(rc2, annotation_drawing_flags, annotation);
        sz = rc3.size();
        sz.setWidth(sz.width() + 10 * ARD_MARGIN);
        if (sz.height() > 300) {
            sz.setWidth(300);
            rc2 = QRect(QPoint(0, 0), sz);
            rc3 = fm.boundingRect(rc2, annotation_drawing_flags, annotation);
            sz = rc3.size();
            sz.setWidth(sz.width() + 10 * ARD_MARGIN);
        }
    }

    auto wrc = m_w->rect();

    const int min_w = 100;
    const int min_h = 2 * gui::lineHeight();
    const int max_h = wrc.height() - 3 * gui::lineHeight();

    if (sz.width() < min_w) {
        sz.setWidth(min_w);
    }
    if (sz.height() < min_h) {
        sz.setHeight(min_h);
    }

    if (sz.height() > max_h) {
        sz.setHeight(max_h);
    }

    resize(sz);
    */
};

void EmbededAnnotationCard::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setFont(*ard::defaultFont());

    QRect rc = rect();
	QColor cl_btn = qRgb(232,232,175);//color::Yellow;
    QBrush br(cl_btn);
    p.setBrush(br);

    cl_btn = cl_btn.darker(200);
    QPen pn(cl_btn);
    p.setPen(pn);

    rc.setRight(rc.right() - 1);
    rc.setBottom(rc.bottom() - 1);
    p.setPen(Qt::gray);
    //p.drawRect(rc);

	static int radius = 2;
	p.drawRoundedRect(rc, radius, radius);

    p.setPen(Qt::black);
    if (m_w->topic()) 
    {
        /*
        int ico_w = gui::lineHeight();
        auto rcText = rc;
        QRect rc_ico = rc;
        rc_ico.setLeft(rc.right() - ico_w);
        rc_ico.setBottom(rc_ico.top() + ico_w);
        p.drawPixmap(rc_ico, getIcon_PinBtn());
        */

        auto rcText = rc;
        rcText.translate(ARD_MARGIN, ARD_MARGIN);
        rcText.setWidth(rcText.width() - 2 * ARD_MARGIN);
        rcText.setHeight(rcText.height() - 2 * ARD_MARGIN);
        auto str = m_w->topic()->annotation();
        p.drawText(rcText, annotation_drawing_flags, str);
    }
};

void EmbededAnnotationCard::mousePressEvent(QMouseEvent *e)
{
    auto pt = e->globalPos();
    editAnnotation(false, &pt);
};

void EmbededAnnotationCard::editAnnotation(bool animate_pos, const QPoint* pt)
{
    auto f = m_w->topic();
    if (f) 
	{
		auto c = ard::AnnotationPopupCard::findAnnotationCard(f);
		if (c) {
			QTimer::singleShot(100, [=]() {
				c->setFocusOnContent();
				c->locateFilterText();
			});
			return;
		}

        auto e = new EmbededAnnotationEdit(this);
        auto rc = rect();
        rc.translate(1, 1);
        rc.setWidth(rc.width() - 2);
        rc.setHeight(rc.height() - 2);
        e->setGeometry(rc);
        e->setEnabled(true);
        e->show();
        e->setFocus();
        if (animate_pos) {
            e->animateLocator();
        }
        if (pt) {
            auto pt2 = e->mapFromGlobal(*pt);
            QTextCursor cpos = e->cursorForPosition(pt2);
            e->setTextCursor(cpos);
        }
    }
};

ard::selector_board* ard::currentBoard() 
{
	auto b = ard::wspace()->currentPageAs<ard::BlackBoard>();
	if (b) {
		return b->board();
	}
	return nullptr;
};

ard::topic_tab_page* ard::open_page(topic_ptr f1, bool focusOnContext)
{
	assert_return_null(f1, "expected topic");
	topic_ptr f = f1->shortcutUnderlying();
	auto f2 = f->popupTopic();
	assert_return_null(f2, "expected popup topic owner");
	auto nt = f2->noteViewType();
	if (nt == ENoteView::None) {
		auto u = dynamic_cast<ard::anUrl*>(f2);
		if (u) {
			u->openUrl();
		}
		return nullptr;
	}

	auto ts = ard::wspace();
	assert_return_null(ts, "expected tab space");
	auto idx = ts->indexOfPage(f2);
	if (idx != -1)
	{
		return ts->selectPage(idx);
	}
	else
	{
		topic_tab_page* pg = ts->createPage(f2);
		assert_return_null(pg, "expected page object");
		ts->replacePage(pg);
		if (!ts->isVisible())ts->show();

		ts->locateFilterText();
		if (focusOnContext) {
			QTimer::singleShot(200, [=]() {
				ts->setFocusOnContent();
			});
		}
		return pg;
	}

	return nullptr;
};

void ard::popup_annotation(topic_ptr f1) 
{
    assert_return_void(f1, "expected topic");
    topic_ptr f = f1->shortcutUnderlying();
    auto f2 = f->popupTopic();
    assert_return_void(f2, "expected popup topic owner");
    if (!f2->canHaveAnnotation()) {
        ASSERT(0, "annotation is not possible for") << f2->dbgHint();
        return;
    }

	auto c = AnnotationPopupCard::findAnnotationCard(f2);
    if (c) {
        QTimer::singleShot(100, [=]() {
            c->setFocusOnContent();
            c->locateFilterText();
        });
    }
    else {
        auto c = new ard::AnnotationPopupCard(f2);
        c->init_popup();
        c->show();
        //pl->registerCard(c);        
        //pl->makeActive(c);
        c->locateFilterText();
    }
};

void ard::save_all_popup_content()
{
	ard::AnnotationPopupCard::saveAllCards();
};

void ard::close_popup(topic_ptr f1)
{
	assert_return_void(f1, "expected topic");
	topic_ptr f = f1->shortcutUnderlying();
	auto f2 = f->popupTopic();
	assert_return_void(f2, "expected popup topic owner");
	auto ts = ard::wspace();
	assert_return_void(ts, "expected tab space");
	auto idx = ts->indexOfPage(f2);
	if (idx != -1)
	{
		ts->closePage(idx);
	}
};

void ard::close_popup(TOPICS_LIST lst) 
{
    for (auto f : lst) {
        close_popup(f);
    }
};

void ard::update_topic_card(topic_ptr f1)
{
	assert_return_void(f1, "expected topic");
	topic_ptr f = f1->shortcutUnderlying();
	auto f2 = f->popupTopic();
	assert_return_void(f2, "expected popup topic owner");

	auto ts = ard::wspace();
	assert_return_void(ts, "expected tab space");
	auto idx = ts->indexOfPage(f2);
	if (idx != -1)
	{
		ts->updatePageTab(idx);
	}
};

/**
* EmailTabPage
*/
ard::EmailTabPage::EmailTabPage(ard::email* e):topic_tab_page(QIcon(":ard/images/unix/email-closed-env"))
{
    m_email_view = new EmailPreview(this);
    m_page_context_box->addWidget(m_email_view);
    attachTopic(e);
};

void ard::EmailTabPage::attachTopic(ard::email* e) 
{
    m_email = e;
    m_email_view->attachEmail(m_email);
    LOCK(m_email);
};

topic_ptr ard::EmailTabPage::topic()
{
    return m_email;
};

void ard::EmailTabPage::refreshTopicWidget()
{
    m_email_view->updateTitle();
};

void ard::EmailTabPage::reloadContent()
{
    m_email_view->reloadContent();
};

void ard::EmailTabPage::zoomView(bool zoom_in)
{
    m_email_view->zoomView(zoom_in);
};

void ard::EmailTabPage::detachCardOwner()
{
    if (m_email) {
        m_email->release();
        m_email = nullptr;
    }
};

void ard::EmailTabPage::findText()
{
	m_email_view->findText();
};

/**
NoteTabPage
*/
ard::NoteTabPage::NoteTabPage(topic_ptr f) :topic_tab_page(QIcon(":ard/images/unix/select-note"))
{
    m_text_edit = new NoteFrameWidget(this);
    m_page_context_box->addWidget(m_text_edit); 
    attachTopic(f);
    m_text_edit->showToolbar(true);
};

void ard::NoteTabPage::attachTopic(topic_ptr f) 
{
    m_topic = f;
    if (m_topic)
    {
        LOCK(m_topic);
        m_text_edit->attachTopic(m_topic);
    }
};

void ard::NoteTabPage::closeEvent(QCloseEvent *e) 
{
    auto f = m_topic;
    if (f) {
        LOCK(f);
    }
    detachCardOwner();
    if (f) {
        if (m_modifications_saved) {
			ard::wspace()->updateBlackboards(f);
        }
        f->release();
    }

    topic_tab_page::closeEvent(e);
};

void ard::NoteTabPage::detachCardOwner()
{
    if (m_topic) 
    {
        saveModified();
        if (m_topic->isEmptyTopic()) 
        {
            ///if note is empty, we clean it away
            m_topic->killSilently(false);
            //ard::asyncExec(AR_rebuildOutline);
            m_topic = nullptr;
        }
        else
        {
            m_topic->release();
            m_topic = nullptr;
        }
        if (m_modifications_saved) 
        {
            ard::asyncExec(AR_rebuildOutline);
        }
        //m_topic->release();
        //m_topic = nullptr;
    }

	if (m_text_edit)
		m_text_edit->detachOwner();
};

void ard::NoteTabPage::zoomView(bool zoom_in) 
{
    if (zoom_in) {
        m_text_edit->zoomIn();
}
    else {
        m_text_edit->zoomOut();
    }
};

void ard::NoteTabPage::saveModified() 
{
    if (m_text_edit) 
    {
        if (m_text_edit->saveModified()) {
            m_modifications_saved = true;
        }

        ///if title is empty, we put title on
        auto s = m_topic->title().trimmed();
        if (s.isEmpty()) {
            auto c = m_topic->mainNote();
            if (c) {
                s = c->plain_text4title();
                m_topic->setTitle(s);
            }
        }
    }
};

void ard::NoteTabPage::reloadContent()
{	
    if (m_text_edit && m_topic)
    {
		ard::trail(QString("reload-note-content [%1][%2]").arg(m_topic->id()).arg(m_topic->title().left(20)));
        m_text_edit->attachTopic(m_topic);
    }
};

void ard::NoteTabPage::locateFilterText() 
{
    m_text_edit->locateFilterText();
};

void ard::NoteTabPage::setFocusOnContent() 
{
    m_text_edit->setFocusOnEdit();
};

/**
	EmailDraftTabPage
*/
ard::EmailDraftTabPage::EmailDraftTabPage(ard::email_draft* f) :topic_tab_page(QIcon(":ard/images/unix/email-new")), m_draft(f)
{
    m_text_edit = new NoteFrameWidget(this);
    m_page_context_box->addWidget(m_text_edit);

    m_text_edit->attachTopic(m_draft);
    m_text_edit->showToolbar(true);

    LOCK(m_draft);
};

void ard::EmailDraftTabPage::closeEvent(QCloseEvent *e) 
{
    auto f = m_draft;
    if (f) {
        LOCK(f);
    }
    detachCardOwner();
    if (f) {
        if (m_modifications_saved) {
			ard::wspace()->updateBlackboards(f);
        }
        f->release();
    }
    topic_tab_page::closeEvent(e);
};

topic_ptr ard::EmailDraftTabPage::topic(){ return m_draft; };

void ard::EmailDraftTabPage::detachCardOwner()
{
    if (m_draft) {
        m_draft->release();
        m_draft = nullptr;
    }
};

void ard::EmailDraftTabPage::zoomView(bool zoom_in)
{
    if (zoom_in) {
        m_text_edit->zoomIn();
    }
    else {
        m_text_edit->zoomOut();
    }
};

void ard::EmailDraftTabPage::saveModified()
{
    if (m_text_edit) {
        if (m_text_edit->saveModified()) {
            m_modifications_saved = true;
        }
    }
};

void ard::EmailDraftTabPage::reloadContent() 
{
    if (m_text_edit && m_draft)
    {
        m_text_edit->attachTopic(m_draft);
    }
};

void ard::EmailDraftTabPage::locateFilterText()
{
    m_text_edit->locateFilterText();
};

void ard::EmailDraftTabPage::setFocusOnContent()
{
    m_text_edit->setFocusOnEdit();
};

/**
	PicturePage
*/
class PicturePreview : public QWidget
{
public:
	void attachPicture(ard::picture* p);
	void detachPicture();
protected:
	void	paintEvent(QPaintEvent*)override;
	void	mouseDoubleClickEvent(QMouseEvent *e)override;

	ard::picture*   m_pic{ nullptr };
};

void PicturePreview::attachPicture(ard::picture* p) 
{
	detachPicture();
	m_pic = p;
	LOCK(m_pic);
};

void PicturePreview::detachPicture() 
{
	if (m_pic) {
		m_pic->release();
		m_pic = nullptr;
	}
}

void PicturePreview::mouseDoubleClickEvent(QMouseEvent*) 
{
	if (m_pic)
	{
		QDesktopServices::openUrl(QUrl(m_pic->imageFileName()));
		model()->registerPictureWatcher(m_pic);
	}
};


void PicturePreview::paintEvent(QPaintEvent*)
{
	QPainter p(this);
	if (m_pic)
	{
		QRect rc = contentsRect();
		//QPoint ptLeftTop = rc.topLeft();
		QPixmap pm = m_pic->image();
		if (!pm.isNull())
		{
			/*
			QSize szPM = pm.size();
			if (szPM.width() < rc.width()) {
				rc.setLeft((rc.width() - szPM.width()) / 2);
			}
			if (szPM.height() < rc.height()) {
				rc.setTop((rc.height() - szPM.height()) / 2);
			}
			rc.setWidth(szPM.width());
			rc.setHeight(szPM.height());
			*/
			//QRect rc_source(0,0,pm.width(),pm.height());
			//p.drawPixmap(rc, pm, rc_source);

			utils::drawImagePixmap(pm, &p, rc, nullptr);
		}
		//p.drawPixmap(rc, pm);
	}
};


ard::PicturePage::PicturePage(ard::picture* p) :topic_tab_page(QIcon(":ard/images/unix/select-image")), m_pic(p)
{
	m_pview = new PicturePreview;
	m_pview->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	m_page_context_box->addWidget(m_pview);
	attachTopic(p);
};


topic_ptr ard::PicturePage::topic() 
{
	return m_pic;
};


void ard::PicturePage::attachTopic(ard::picture* p) 
{
	m_pic = p;
	m_pview->attachPicture(p);
	LOCK(m_pic);
};

void ard::PicturePage::detachCardOwner()
{
	if (m_pic) {
		m_pic->release();
		m_pic = nullptr;
	}
	m_pview->detachPicture();
};


void ard::PicturePage::reloadContent()
{
	m_pview->update();
};

//...
ard::TdaPage::TdaPage(ard::fileref* p) :topic_tab_page(QIcon(":ard/images/unix/select-image")), m_ref(p)
{
	m_tda_view = new ard::tda_view;
	m_tda_view->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	m_page_context_box->addWidget(m_tda_view);
	attachTopic(p);
};


topic_ptr ard::TdaPage::topic()
{
	return m_ref;
};


void ard::TdaPage::attachTopic(ard::fileref* p)
{
	m_ref = p;
	m_tda_view->attachFileRef(p);
};
//...

#else
ard::selector_board* ard::currentBoard() {return nullptr;}
#endif //ARD_BIG

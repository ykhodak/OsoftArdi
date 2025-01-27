#include <QTextDocument>
#include "ard-reporter.h"
#include "anfolder.h"
#include "kring.h"
#include "extnote.h"
#include "locus_folder.h"

class ReportTopic : public ard::topic 
{
public:
    ReportTopic(QString title) :ard::topic(title) {}
    ENoteView   noteViewType()const override {return ENoteView::Edit;};
};

void ard::TexDocReportBuilder::mergeTopics(TOPICS_LIST& lst)
{
    TexDocReportBuilder b;
    b.merge_topics(lst);
};

void ard::TexDocReportBuilder::mergeToFlexbox(TOPICS_LIST& lst)
{
    TexDocReportBuilder b;
    b.merge_to_flexbox(lst);
};

void ard::TexDocReportBuilder::guiMergeKRingKeys()
{
    assert_return_void(ard::isDbConnected(), "expected open DB");
    auto r = ard::db()->kmodel()->keys_root();
    if (r->isRingLocked()) {
        if (!r->guiUnlockRing()) {
            return;
        }
    }

    TexDocReportBuilder b;
    b.merge_KRingKeys();
};

void ard::TexDocReportBuilder::add_line()
{
    m_wrk_cursor.movePosition(QTextCursor::End);
    QTextFrameFormat frameFormat;
    frameFormat.setHeight(5);
    frameFormat.setWidth(300);
    frameFormat.setBackground(Qt::black);
    m_wrk_cursor.insertFrame(frameFormat);
    m_wrk_cursor.movePosition(QTextCursor::End);
};

void ard::TexDocReportBuilder::add_html(QString html)
{
    m_wrk_cursor.movePosition(QTextCursor::End);
    m_wrk_cursor.insertHtml(html);
    m_wrk_cursor.movePosition(QTextCursor::End);
};

void ard::TexDocReportBuilder::add_break()
{
    m_wrk_cursor.movePosition(QTextCursor::End);
    add_html("<br />");
    m_wrk_cursor.movePosition(QTextCursor::End);
};

void ard::TexDocReportBuilder::add_title(topic_ptr f)
{
    add_text(f->title(), true);
};

void ard::TexDocReportBuilder::add_note(ard::note_ext* n)
{
    add_text(n->plain_text(), false);
};

void ard::TexDocReportBuilder::add_text(QString txt, bool bold) 
{
    m_wrk_cursor.movePosition(QTextCursor::End);
    QTextCharFormat fmt;
    fmt.setFont(bold ? *ard::defaultBoldFont() : *ard::defaultFont());
    m_wrk_cursor.insertText(txt, fmt);
    m_wrk_cursor.movePosition(QTextCursor::End);
};

void ard::TexDocReportBuilder::add_space() 
{
    m_wrk_cursor.movePosition(QTextCursor::End);
    m_wrk_cursor.insertHtml("&nbsp;");
    m_wrk_cursor.movePosition(QTextCursor::End);
};

topic_ptr ard::TexDocReportBuilder::make_report(QString title)
{
    assert_return_null(m_wrk_doc, "expected utility document");

    auto drafts = ard::Backlog();
    if (drafts) {
        auto f = new ard::topic(title);
        drafts->addItem(f);
        drafts->ensurePersistant(1);
        f->setMainNoteText(m_wrk_doc->toHtml());
        return f;
    }
    return nullptr;
};

void ard::TexDocReportBuilder::merge_topics(TOPICS_LIST& lst)
{
    m_wrk_doc = gui::helperTextDocument();
    if (m_wrk_doc) {
        m_wrk_doc->setHtml("");

        m_wrk_cursor = QTextCursor(m_wrk_doc);
        TOPICS_LIST::iterator i = lst.begin();
        add_title(*i);
        add_break();
        auto n = (*i)->mainNote();
        if (n) {
            add_note(n);
        }
        while (i != lst.end()) {
            add_line();
            add_title(*i);
            add_break();
            auto n = (*i)->mainNote();
            if (n) {
                add_note(n);
            }
            i++;
        }

        i = lst.begin();
        QString report_title = QString("Report on '%1' Topics. %2").arg(lst.size()).arg((*i)->title());
        auto f = make_report(report_title);
        if (f) {
            ard::open_page(f);
        }
    }//hd
};

void ard::TexDocReportBuilder::merge_KRingKeys()
{
    assert_return_void(ard::isDbConnected(), "expected open DB");
    auto r = ard::db()->kmodel()->keys_root();
    assert_return_void(!r->isRingLocked(), "expected unlocked kring root");

    m_wrk_doc = gui::helperTextDocument();
    if (!m_wrk_doc) {
        ASSERT(0, "expected reported wdoc");
        return;
    }

    m_wrk_doc->setHtml("");
    m_wrk_cursor = QTextCursor(m_wrk_doc);

    auto c_list = r->items();
    for (auto& i : c_list)
    {
        auto k = dynamic_cast<ard::KRingKey*>(i);
        if (k) {
            add_text(k->keyTitle(), true); add_space(); add_space();
            add_text(k->keyLogin(), false); add_space(); add_space();
            add_text(k->keyPwd(), false); add_space(); add_space();
            add_text(k->keyNote(), false); add_space(); add_space();
            add_break();
        }
    }

    QString report_title = QString("Report on '%1' Keys").arg(r->items().size());
    auto f = new ReportTopic(report_title);
    f->setMainNoteText(m_wrk_doc->toHtml());
    ard::open_page(f);
};

void ard::TexDocReportBuilder::merge_to_flexbox(TOPICS_LIST& lst)
{
/*
<style>
.flex - container{
display: flex;
background - color: DodgerBlue;
}

.flex - container > div{
background - color: #f1f1f1;
margin: 10px;
padding: 20px;
font - size: 30px;
}
< / style>
*/

    QString html_header = R"(
<div class="flex-container">
  <div>1</div>
  <div>2</div>
  <div>3</div>  
</div>
<p>A Flexible Layout must have a parent element with the <em>display</em> property set to <em>flex</em>.</p>
<p>Direct child elements(s) of the flexible container automatically becomes flexible items.</p>
)";


    QString flex_css = R"(
<style type=\"text/css\">
.flex - container{
display: flex;
background - color: DodgerBlue;
}

.flex - container > div{
background - color: #f1f1f1;
margin: 10px;
padding: 20px;
font - size: 30px;
}
</style>
    )";


    m_wrk_doc = gui::helperTextDocument();
    if (m_wrk_doc) {
        auto ss = m_wrk_doc->defaultStyleSheet();               
        m_wrk_doc->setHtml(flex_css + html_header);
        //m_wrk_doc->setDefaultStyleSheet(flex_css);
        qDebug() << "<<ykh-css:" << m_wrk_doc->defaultStyleSheet();
        qDebug() << "<<ykh-reading-html:" << html_header;
        qDebug() << "<<ykh-setup-to-html:" << m_wrk_doc->toHtml();

        TOPICS_LIST::iterator i = lst.begin();
        QString report_title = QString("Report on '%1' Topics. %2").arg(lst.size()).arg((*i)->title());
        auto f = make_report(report_title);
        if (f) {
            ard::open_page(f);
        }
        m_wrk_doc->setDefaultStyleSheet(ss);
    }
};

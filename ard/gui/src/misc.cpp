#include "a-db-utils.h"
#include "utils.h"
#include "anfolder.h"
#include "ansyncdb.h"
#include "NoteEdit.h"
#include "NoteFrameWidget.h"
#include "ardmodel.h"
#include "snc-tree.h"
#include "extnote.h"


void gui::searchLocalText(QString local_search)
{
    model()->selectByText(local_search);
};

int replaceTextInEditor(QString sFrom, QString sTo, QTextEdit& working_edit)
{
  QTextCursor c = working_edit.textCursor();
  c.setPosition(0);
  working_edit.setTextCursor(c);
  int replaced = 0;
  while(working_edit.find(sFrom))
    {
      if(working_edit.textCursor().hasSelection())
    {
      working_edit.textCursor().insertText(sTo);
    }
      replaced++;
    }

  return replaced;
}

static int replaceTextInNotes(topic_ptr it, QString sFrom, QString sTo, QTextEdit& working_edit)
{
    int replaced = 0;
    auto n = it->mainNote();
    if(n)
    {
        QString s = n->html();
        if (!s.isEmpty())
        {
            working_edit.setHtml(s);
            int r = replaceTextInEditor(sFrom, sTo, working_edit);
            if (r > 0)
            {
                n->setNoteHtml(working_edit.toHtml().trimmed(), working_edit.toPlainText().trimmed());
                replaced += r;
            }
        }
    }

    return replaced;
};

static void replaceTextInTopic(topic_ptr f, QString sFrom, QString sTo, QTextEdit& working_edit)
{
    snc::MemFindAllPipe mp;
    f->memFindItems(&mp);
    QString s = QString("Running replace on %1 item(s)").arg(mp.items().size());
    int replaced = 0, idx = 0;
    for (CITEMS::iterator i = mp.items().begin(); i != mp.items().end(); i++)
    {
        topic_ptr it = dynamic_cast<topic_ptr>(*i);
        replaced += replaceTextInNotes(it, sFrom, sTo, working_edit);
        idx++;
    }
    if (f == dbp::root())
    {
		ard::messageBox(gui::mainWnd(), QString("Replaced %1 occurrence(s) in %2 topic(s)").arg(replaced).arg(mp.items().size()));
    }
    else
    {
		ard::messageBox(gui::mainWnd(), QString("Replaced %1 occurrence(s) in %2 topic(s) of '%3' project").arg(replaced).arg(mp.items().size()).arg(f->title()));
    }
}

void gui::replaceGlobalText(QString sFrom, QString sTo, ESearchScope sc, DB_ID_TYPE id)
{
    assert_return_void(gui::isDBAttached(), "expected attached DB");
    WaitCursor wait;

    QTextEdit work_edit;
    ard::save_all_popup_content();
    if (sc == search_scopeNone)
        sc = search_scopeOpenDatabase;

    switch (sc)
    {
    case search_scopeTopic:
    {
        ASSERT(0, "NA");
    }break;
    case search_scopeOpenDatabase:
    {
        auto r = dbp::root();
        assert_return_void(r, "expected root topic");
        replaceTextInTopic(r, sFrom, sTo, work_edit);
    }break;
    case search_scopeProject:
    case search_scopeGtdFolder:
    {
        assert_return_void(IS_VALID_DB_ID(id), "expected valid ID");
        auto f = ard::lookup(id);
        assert_return_void(f, QString("failed to lookup topic: %1").arg(id));
        qDebug() << "running project scope" << f->dbgHint();

        replaceTextInTopic(f, sFrom, sTo, work_edit);
    }break;
    case search_scopeNone:
    {
        ASSERT(0, "NA");
    }break;
    }
};

void formatTextInEditor(const QTextCharFormat* fmt, QTextEdit& working_edit)
{
  QTextCursor cursor = working_edit.textCursor();
  cursor.select(QTextCursor::Document);
  cursor.mergeCharFormat(*fmt);
}

static int formatTextInComments(topic_ptr it, const QTextCharFormat* fmt, QTextEdit& working_edit)
{
    int formated = 0;
    auto n = it->mainNote();
    if(n)
    {
        QString s = n->html();
        if (!s.isEmpty())
        {
            working_edit.setHtml(s);
            formatTextInEditor(fmt, working_edit);
            n->setNoteHtml(working_edit.toHtml().trimmed(), working_edit.toPlainText().trimmed());
            formated++;
        }
    }

    return formated;
};


void ard::formatNotes(TOPICS_LIST& lst, const QTextCharFormat* fmt) 
{
    QTextEdit work_edit;
    for (auto& f : lst) {
        formatTextInComments(f, fmt, work_edit);
    }
};


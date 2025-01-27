#include <QDesktopServices>
#include <QFileDialog>
#include "a-db-utils.h"
#include "anurl.h"
#include "extnote.h"
#include "locus_folder.h"

IMPLEMENT_ALLOC_POOL(anUrl);

ard::anUrl::anUrl(QString title) :ard::topic(title) 
{

};

ard::anUrl* ard::anUrl::createUrl()
{
    return new ard::anUrl();
};

ard::anUrl* ard::anUrl::createUrl(QString title, QString ref) 
{
    auto rv = new anUrl(title);
    auto e = rv->ensureNote();
    e->setNoteHtml(ref, ref);
    return rv;
};

cit_prim_ptr ard::anUrl::create()const
{
    return createUrl();
};

QPixmap ard::anUrl::getSecondaryIcon(OutlineContext)const 
{
    return getIcon_Url();
};

void ard::anUrl::fatFingerSelect() 
{
    openUrl();
};

void ard::anUrl::openUrl()
{
    QString url, url2open;
    auto n = mainNote();
    if (n) {
        url = n->plain_text().trimmed();
    }

    url2open = url;
    if (!url2open.isEmpty()) {
        auto idx = url2open.indexOf("http://");
        if (idx == -1) {
            idx = url2open.indexOf("https://");
        }
        if (idx == -1) {
            url2open = "https://" + url;
        }
    }

    if (!url2open.isEmpty()) {
        QDesktopServices::openUrl(url2open);
    }
};

QString ard::anUrl::url()const 
{
    QString rv;
    auto n = mainNote();
    if (n) {
        rv = n->plain_text().trimmed();
    }
    return rv;
};

void ard::anUrl::setUrl(QString u) 
{
    auto n = mainNote();
    if (n) {
        n->setNoteHtml(u, u);
    }
};


QSize ard::anUrl::calcBlackboardTextBoxSize()const 
{
    auto rv = ard::topic::calcBlackboardTextBoxSize();
    rv.setWidth(rv.width() + gui::lineHeight());
    return rv;
};

//// gui_import_html_bookmarks ////

static QString get_tag_text(QString html_tag)
{
    QString text;
    auto i = html_tag.indexOf(">");
    if (i != -1)
    {
        auto i2 = html_tag.indexOf("<", i + 1);
        if (i2 != -1)
        {
            text = html_tag.mid(i + 1, i2 - i - 1);
        }
    }
    return text;
}

static QString get_href(QString html_tag)
{
    QString text;
    auto i = html_tag.indexOf("\"");
    if (i != -1)
    {
        auto i2 = html_tag.indexOf("\"", i + 1);
        if (i2 != -1)
        {
            text = html_tag.mid(i + 1, i2 - i - 1);
        }
    }
    return text;
}

static topic_ptr import_subfolder(QTextStream& in, QString folder_title)
{
    qDebug() << "s-down" << folder_title;
    topic_ptr rv = new ard::topic(folder_title);

    QString sub_folder_title;
    while (!in.atEnd())
    {
        QString line = in.readLine().trimmed();
        if (!line.isEmpty())
        {
            auto i = line.indexOf("<DT><H3");
            if (i == 0)
            {
                sub_folder_title = get_tag_text(line.mid(i + 10));
                continue;
            }

            i = line.indexOf("<DL><p>");
            if (i == 0)
            {
                auto f = import_subfolder(in, sub_folder_title);
                rv->addItem(f);
            }
            else
            {
                auto i = line.indexOf("</DL>");
                if (i == 0)
                {
                    qDebug() << "s-up" << folder_title;
                    return rv;
                }
                else
                {
                    i = line.indexOf("<DT><A");
                    if (i == 0)
                    {
                        auto bmk_title = get_tag_text(line.mid(i + 10));
                        auto href = get_href(line.mid(i + 10));
                        auto u = ard::anUrl::createUrl(bmk_title, href);
                        if (u) {
                            rv->addItem(u);
                        }
                        qDebug() << "bmk" << bmk_title << href;
                    }
                }
            }
        }
    }

    return rv;
}

void ard::gui_import_html_bookmarks()
{
    auto sb = ard::Sortbox();
    assert_return_void(sb, "expected inbox");

    if (!ard::confirmBox(ard::mainWnd(), "Please confirm bookmarks import. In next window you will select Bookmarks File created in Google Chrome and others popular browsers."))
        return;

    auto fpath = QFileDialog::getOpenFileName(gui::mainWnd(),
        QObject::tr("Open Bookmarks File"),
        dbp::configFileLastShellAccessDir(),
        QString("Bookmarks (*.html)"));
    if (!fpath.isEmpty())
    {
        dbp::configFileSetLastShellAccessDir(fpath, true);
        qDebug() << "opening boomkark file" << fpath;

        QFile f(fpath);
        if (f.open(QIODevice::ReadOnly))
        {
            QString folder_title;
            QTextStream in(&f);
            while (!in.atEnd())
            {
                QString line = in.readLine().trimmed();
                if (!line.isEmpty())
                {
                    auto i = line.indexOf("<H1>");
                    if (i == 0)
                    {
                        folder_title = get_tag_text(line.mid(i + 3));
                    }

                    i = line.indexOf("<DL><p>");
                    if (i == 0)
                    {
                        auto f = import_subfolder(in, folder_title);
                        if (f) 
                        {
                            sb->addItem(f);

                            sb->ensurePersistant(-1);
                            gui::rebuildOutline(outline_policy_Pad);
                            gui::ensureVisibleInOutline(f);

                            auto imported = f->selectAllBookmarks();
                            QString msg(QString("Imported %1 bookmarks.").arg(imported.size()));
							ard::messageBox(gui::mainWnd(), msg);
                        }
                    }
                    else
                    {
                        qWarning() << "HTML-BMK-import, ignoring line" << line;
                    }
                }
            }
            f.close();
        }
    }
}

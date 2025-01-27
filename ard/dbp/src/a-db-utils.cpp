#include <QApplication>
#include <QWaitCondition>
#include <QDir>
#include <QPainter>
#include <QUrl>
#include <QTextEdit>
#include <QPushButton>
#include <csetjmp>
#include <csignal>
#include <QNetworkInterface>
#include <QPlainTextEdit>
#include <QDesktopServices>
#include <QSqlError>
#include <QDesktopWidget>
#include <QStyleOptionButton>
#include <QProcess>
#include <QMessageBox>
#include <QTableView>
#include <QStandardItemModel>
#include <QHeaderView>
#include <QClipboard>
#include "email.h"
#include "ethread.h"
#include "a-db-utils.h"
#include "anfolder.h"
#include "snc-tree.h"
#include "google/endpoint/ApiUtil.h"
#include "mpool.h"
#include "contact.h"

#ifdef Q_OS_WIN
#include <windows.h>
#endif

extern QTableView* NewArdTableView();

QColor gui::invertColor(QColor clr)
{
    QColor rv;
    if(clr.black() == 255)
        {
            rv = QColor(255, 255, 255);
        }
    else
        {
            rv = clr.lighter(300);
        }

    return rv;
};

/*
dbp_exception::dbp_exception(int _code)throw()
    :m_code(_code)
{
    //qWarning() << "RAISING EXCEPTION " << getLastStackFrames(7);
    //m_s = getLastStackFrames(3);
};*/

/*
static QString get_program_appdata_name()
{
    return "OsoftArdi";
    }*/




bool copyOverFile(QString sourceFile, QString destFile)
{
    if(QFile::exists(destFile))
        {
            if(!QFile::remove(destFile))
                {
                    ASSERT(0, "failed to delete while copying over") << destFile;
                    return false;
                }
        };

    bool rv = QFile::copy(sourceFile, destFile);
    ASSERT(rv, "failed to copy file") << sourceFile << destFile;
    return rv;
};

bool renameOverFile(QString sourceFile, QString destFile)
{
    if(QFile::exists(destFile))
        {
            if(!QFile::remove(destFile))
                {
                    ASSERT(0, "failed to delete while renaming over") << destFile;
                    return false;
                }
        };

    bool rv = QFile::rename(sourceFile, destFile);
    ASSERT(rv, "failed to rename file") << sourceFile << destFile;
    return rv;
};

/**
   Delete a directory along with all of its contents
*/
bool removeDir(const QString &dirName)
{
    bool result = true;
    QDir d(dirName);
    if(d.exists())
        {
            result = d.removeRecursively();
            ASSERT(result, "failed to delete recursively") << dirName;
        }
    return result;
};


bool has_ppos_field(int flags)
{
    bool rv = (flags & flPindex);
    return rv;
};

/*bool has_content_field(int flags)
{
    bool rv = (flags & flTitle) || (flags & flFont) || (flags & flHotSpot) || 
        (flags & flToDo) || (flags & flToDoDone) || (flags & flRetired) ||
        (flags & flContent) || (flags & flAnnotation) || (flags & flSourceRef) ||
        (flags & flUploadTime) || (flags & flCloudId) || (flags & flCloudIdType) ||
        (flags & flDuration) || (flags & flCost);
    return rv;
};*/

static QString flagDesc(enField f, const snc::cit_base* o, const snc::cit_base* other)
{
    QString rv = "";
    if(o && other)
        {
            const snc::cit* o_cit = dynamic_cast<const snc::cit*>(o);
            const snc::cit* other_cit = dynamic_cast<const snc::cit*>(other);
            if(o_cit && other_cit)
                {
                    switch(f)
                        {
                        case flPindex:
                            {
                                const snc::cit* OurParent = o_cit->cit_parent();
                                const snc::cit* OtherParent = other_cit->cit_parent();
                                if(OurParent && OtherParent)
                                    {
                                        int ourIdx = OurParent->indexOf_cit(o_cit);
                                        int otherIdx = OtherParent->indexOf_cit(other_cit);
                                        rv = QString("(%1/%2)").arg(ourIdx).arg(otherIdx);
                                    }       
                            }break;
                        case flTitle:
                            {
                                rv = QString("(%1/%2)").arg(o_cit->title()).arg(other_cit->title());
                            }break;
                        default:break;
                        }//switch
                }//cit-check
        }
    return rv;
};

QString sync_flags2string(int flags, const cit_base* o, const cit_base* other)
{
#define CHECK_FIELD(F, S)if(flags & F)rv += QString("|%1%2").arg(S).arg(flagDesc(F, o, other))

    QString rv = QString("%1").arg(flags);
    CHECK_FIELD(flSyid, "syid");
    CHECK_FIELD(flOtype, "otype");
    CHECK_FIELD(flPSyid, "psyid");
    CHECK_FIELD(flTitle, "title");
    CHECK_FIELD(flFont, "font");
    CHECK_FIELD(flAnnotation, "annotation");
    CHECK_FIELD(flToDo, "todo");
    CHECK_FIELD(flToDoDone, "done");
    CHECK_FIELD(flHotSpot, "hspot");
    CHECK_FIELD(flRetired, "retired");
    //CHECK_FIELD(flDuration, "duration");
    CHECK_FIELD(flSourceRef, "source-ref");
    CHECK_FIELD(flFileName, "file-name");
    CHECK_FIELD(flContent, "content");
    CHECK_FIELD(flPindex, "ppos");
   // CHECK_FIELD(flUploadTime, "uploadTime");
    CHECK_FIELD(flCloudId, "cloudId");
   // CHECK_FIELD(flCloudIdType, "cloudIdType");

#undef CHECK_FIELD
    return rv;
}


/**
   WaitCursor
*/
WaitCursor::WaitCursor()
{
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
};

WaitCursor::~WaitCursor()
{
    QApplication::restoreOverrideCursor();
};


/**
   ard_dir
*/
typedef std::map<EARD_DIR, QString> DIR_POOL;
static DIR_POOL dir_pool;
static QString __dir_curr_root = "";
static QString __db_custom_path_name = "";
void ard_dir_set_curr_root(QString dir_path)
{
    dir_pool.clear();
    __dir_curr_root = dir_path;
    int len = __dir_curr_root.length();
    if(len > 0)
        {
            if(__dir_curr_root[len -1] != '/')
                __dir_curr_root += "/";
        }  
  
    QDir dir(__dir_curr_root);
    ASSERT(dir.exists(), "expected valid directory") << __dir_curr_root;
};

void setCustomDBPathName(QString s)
{
    __db_custom_path_name = s;
}

QString ard_dir_curr_root()
{
    return __dir_curr_root;
};

QString get_db_file_path()
{
    QString rv = ard_dir_curr_root() + DB_FILE_NAME;
    if(!__db_custom_path_name.isEmpty())
        {
            rv = __db_custom_path_name;
        }
    return rv;
}

QString get_root_db_file_path()
{
    QString rv = ard_dir_curr_root() + ROOT_DB_FILE_NAME;
    return rv;
}

QString get_ard_folder(QString parent_path, QString sub_folder)
{
    QString rv = parent_path + sub_folder;
    QDir dir(rv);
    if(!dir.exists()){
            bool bok = dir.mkpath(rv);
            if(!bok){
                ASSERT(bok, "Failed to create directory:") << rv;
            }
        }
    return rv;
}

QString ard_dir(EARD_DIR d)
{
    QString rv = "";
  
    DIR_POOL::iterator k = dir_pool.find(d);
    if(k != dir_pool.end())
        {
            return k->second;
        }

    switch(d)
        {
        case dirMediaAux:     rv = get_ard_folder(ard_dir_curr_root(), "media-thumb-cache/");break;
		case dirTMP:          rv = get_ard_folder(ard_dir_curr_root(), "tmp/"); break;
		case dirMedia:        rv = get_ard_folder(ard_dir_curr_root(), "media/"); break;
        case dirMP:           rv = get_ard_folder(ard_dir_curr_root(), "mp/");break;
        case dirMP1:          rv = get_ard_folder(ard_dir(dirMediaAux), "mp1/");break;
        case dirMP2:          rv = get_ard_folder(ard_dir(dirMediaAux), "mp2/");break;        
        default:ASSERT(0, "invalid directory type provided");
        }

    dir_pool[d] = rv;

    return rv;
};

void rectf2rect(const QRectF& rc_in, QRect& rc_out)
{
    rc_out.setLeft(static_cast<int>(rc_in.left()));
    rc_out.setRight(static_cast<int>(rc_in.right()));
    rc_out.setTop(static_cast<int>(rc_in.top()));
    rc_out.setBottom(static_cast<int>(rc_in.bottom()));
};

QString defaultRepositoryPath()
{
    // don't do any qDebug or other output from here
    // because it's used in output handler in release
    // it will crash because of recoursive call
    static QString appdata_root = "";
    if(appdata_root.isEmpty())
        {      
            const char* p = getenv("ARD_ROOT_DIR");
            if(p != NULL)
                {
                    appdata_root = p;
                    appdata_root += "/";
                }
            else
                {
                    appdata_root = QDir::homePath();
#ifdef Q_OS_WIN32
                    appdata_root += "/appdata/roaming";
#endif

#ifdef Q_OS_MAC
                    appdata_root += "/Library/Preferences";
#endif

#ifdef Q_OS_ANDROID
                    appdata_root += "/";
#else
                    
                    appdata_root += "/OsoftArdi/ardi-space/";
                    //                    appdata_root += "/" + get_program_appdata_name() + "/";
#endif
                }

            QDir dir(appdata_root);
            if(!dir.exists())
                {
                    bool bok = dir.mkpath(appdata_root);
                    ASSERT(bok, "Failed to create appdata_root");
                }
        }
    return appdata_root;
};

void loadDBsList(QStringList& lst)
{
    lst.clear();

    lst.push_back(DEFAULT_DB_NAME);

    QString sub_dbs = defaultRepositoryPath() + "dbs/";
    QDir d(sub_dbs);
    QStringList _list = d.entryList(QDir::AllDirs|QDir::NoDotAndDotDot);
    for(int i = 0; i < _list.size(); ++i)
        {
            QString dbPath = sub_dbs + _list.at(i) + "/" + DB_FILE_NAME;
            if(QFile::exists(dbPath))
                {
                    lst.push_back(_list.at(i));
                }
            else
                {
                    ASSERT(0, "database folder empty") << _list.at(i);
                }
        }
};



void gui::drawArdPixmap(QPainter * p, QString resStr, const QRect& rc)
{
    QPixmap pm(resStr);
    p->drawPixmap(rc, pm);
};

void gui::drawArdPixmap(QPainter * p, QPixmap pm, const QRect& rc) 
{
    p->drawPixmap(rc, pm);
};

QString gui::policy2name(EOutlinePolicy p)
{
#define RV_CASE(P, L)case P:rv = L;break;
    QString rv = QString("%1").arg(p);
    switch(p)
        {
            RV_CASE(outline_policy_Uknown, "unknown");
            RV_CASE(outline_policy_Pad, "pad");
            RV_CASE(outline_policy_PadEmail, "pad-email");
            RV_CASE(outline_policy_TaskRing, "task-ring");
            RV_CASE(outline_policy_Notes, "notes");
			RV_CASE(outline_policy_Bookmarks, "bookmarks");
			RV_CASE(outline_policy_Pictures, "pictures");
            RV_CASE(outline_policy_Annotated, "annotated");
            RV_CASE(outline_policy_Colored, "colored");
            RV_CASE(outline_policy_2SearchView, "search");            
            RV_CASE(outline_policy_KRingTable, "k-table");
            RV_CASE(outline_policy_KRingForm, "k-form");
            RV_CASE(outline_policy_BoardSelector, "b-select");
        default:break;
        }
    return rv;
#undef RV_CASE
};


void gui::setButtonMinHeight(QPushButton* b)
{
#ifdef ARD_BIG
    Q_UNUSED(b);
#else
    QSize textSize = b->fontMetrics().size(Qt::TextShowMnemonic, "XX");
    QStyleOptionButton opt;
    opt.initFrom(b);
    opt.rect.setSize(textSize);

    QSize sz2 = b->style()->sizeFromContents(QStyle::CT_PushButton,
                                             &opt,
                                             textSize,
                                             b);
    b->setMinimumHeight(sz2.height() * 2);
#endif
};

void gui::resizeWidget(QWidget* w, QSize sz)
{
#ifdef ARD_BIG
    if(!sz.isEmpty())
        w->resize(sz);
#else
#ifdef ARD_HOME_BUILD
    Q_UNUSED(sz);
    w->resize(QApplication::desktop()->availableGeometry().size());
#else
    if(!sz.isEmpty())
        w->resize(sz);
#endif  //ARD_HOME_BUILD
#endif  //ARD_BIG
};

bool gui::isConnectedToNetwork()
{
#ifdef ARD_GD
    return googleQt::isConnectedToNetwork();
#endif
};

QString get_cloud_support_folder_name() 
{
    QString rv = "Ardi-support";
    return rv;
};

QString get_compressed_remote_db_file_name(QString db_prefix)
{
    QString rv = "ardi.qpk";
    db_prefix = db_prefix.trimmed();
    if(!db_prefix.isEmpty())
        {
            rv = QString("ardi-%1.qpk").arg(db_prefix);
        }
    return rv;
};

QString get_temp_uncompressed_file_suffix()
{
    return ".tmp-db.sqlite";
}

QString gui::imagesFilter()
{
    QString rv = "Images (*.png *.xpm *.jpg *.bmp *.PNG *.XPM *.JPG *.BMP)";
    return rv;
};

QString gui::recoverUrl(QString s)
{
    if(s.isEmpty())
        return "";

    QString rv = "";

    int end_of_line = s.indexOf("\n");
    if(end_of_line != -1)
        {
            s = s.left(end_of_line);
        }

    int www_pos = s.indexOf("http://");
    if(www_pos == -1)
        {
            www_pos = s.indexOf("https://");
        }
    if(www_pos == -1)
        {
            www_pos = s.indexOf("www.");
        }
    if(www_pos != -1)
        {
            int end_pos = s.indexOf(QRegExp("\\s"), www_pos);
            if(end_pos != -1)
                {
                    s = s.mid(www_pos, end_pos - www_pos);
                }

            QUrl u(s);
            if(u.isValid())
                {
                    rv = s;
                    //rv = u.path();
                }
        }
    return rv;
};

void gui::openUrl(QString surl)
{
    int idx = surl.indexOf("www.");
    if(idx == 0)
        {
            surl = "http://" + surl;
        }
    QDesktopServices::openUrl(QUrl(surl));
};

/**
   db-profyling
*/

#ifdef _SQL_PROFILER

class SqlProfileInfo
{
public:
    int calls_number;
    int total_duration;
};

typedef std::map<QString, SqlProfileInfo> FUN_2_PROFILE_INFO;
static FUN_2_PROFILE_INFO fun2profile_info;
static int print_time = time(NULL);

void SqlQuery::runProfiler(int duration, STRING_LIST& bt)
{
    QString top_fun = *bt.begin();
    if(!top_fun.isEmpty())
        {
            STRING_LIST::iterator k = bt.begin();
            while(top_fun[0] == '/' && k != bt.end())
                {
                    top_fun = *k;
                    ++k;
                }

            FUN_2_PROFILE_INFO::iterator i = fun2profile_info.find(top_fun);
            if(i != fun2profile_info.end())
                {
                    SqlProfileInfo& _pi = i->second;
                    _pi.calls_number++;
                    _pi.total_duration += duration;
                }
            else
                {
                    SqlProfileInfo _pi;
                    _pi.calls_number = 1;
                    _pi.total_duration = duration;
                    fun2profile_info[top_fun] = _pi;
                }
        }

    //if(time(NULL) - print_time > 10)
    if(false)
        {
            //      qDebug() << "--------SQL-profiler-begin---------------";
            QString s = QString("%1%2%3")
                .arg(QString("==== SQL-profiler ====").leftJustified(40, ' '))
                .arg(QString("%1").arg("cols").leftJustified(5, ' '))
                .arg(QString("%1").arg("sec").leftJustified(5, ' '));
            qDebug() << s;
            for(FUN_2_PROFILE_INFO::iterator i = fun2profile_info.begin(); i != fun2profile_info.end(); ++i)
                {
                    s = QString("%1%2%3")
                        .arg(i->first.leftJustified(40, ' '))
                        .arg(QString("%1").arg(i->second.calls_number).leftJustified(5, ' '))
                        .arg(QString("%1").arg(i->second.total_duration).leftJustified(5, ' '));
                    qDebug() << s;
                }
            qDebug() << "=== SQL-profiler-end ===";
            print_time = time(NULL);
        }
};

#endif //_SQL_PROFILER

bool SqlQuery::exec()
{
#ifdef _SQL_PROFILER
    QTime t;
    t.start();
#endif //_SQL_PROFILER

    //checkCache();

    bool rv = QSqlQuery::exec();
    if(!rv)    
        {
            ASSERT(0, "SQL-ARD_ERROR") << lastQuery() << lastError().text();
        }
#ifdef _SQL_PROFILER
    if(rv)
        {
            int duration = t.elapsed();

            STRING_LIST bt;
            get_stacktrace(bt);
            if(!bt.empty())
                {
                    runProfiler(duration, bt);
                }
        }
#endif //_SQL_PROFILER
    return rv;
}

bool SqlQuery::execBatch()
{
#ifdef _SQL_PROFILER
    QTime t;
    t.start();
#endif //_SQL_PROFILER

    //checkCache();

    bool rv = QSqlQuery::execBatch();
    if(!rv)    
        {
            ASSERT(0, "SQL-ARD_ERROR") << lastQuery();
        }

#ifdef _SQL_PROFILER
    if(rv)
        {
            int duration = t.elapsed();

            STRING_LIST bt;
            get_stacktrace(bt);
            if(!bt.empty())
                {
                    runProfiler(duration, bt);
                }
        }
#endif //_SQL_PROFILER

    return rv;
};


SqlQuery::~SqlQuery()
{
};


ArdNIL& ArdNIL::nil()
{
    static ArdNIL n;
    return n;
};

void gui::showInGraphicalShell(QWidget *parent, const QString &pathIn) 
{
    Q_UNUSED(parent)
    // Mac, Windows support folder or file.
#if defined(Q_OS_WIN)
    Q_UNUSED(parent)
    /*
    const QString explorer = QProcessEnvironment::systemEnvironment().searchInPath(QLatin1String("explorer.exe"));
    if (explorer.isEmpty()) {
        QMessageBox::warning(parent,
            tr("Launching Windows Explorer failed"),
            tr("Could not find explorer.exe in path to launch Windows Explorer."));
        return;
    }
    QString param;
    if (!QFileInfo(pathIn).isDir())
        param = QLatin1String("/select,");
    param += QDir::toNativeSeparators(pathIn);
    QString command = explorer + " " + param;
    QProcess::startDetached(command);
    */
    QStringList args;
    args << "/select," << QDir::toNativeSeparators(pathIn);
    QProcess::startDetached("explorer", args);
#elif defined(Q_OS_MAC)

#ifdef Q_OS_IOS
    //@todo: this might be universal code
    QUrl url = QUrl::fromLocalFile(pathIn);
    QDesktopServices::openUrl(url);
#else
    Q_UNUSED(parent)
        QStringList scriptArgs;
    scriptArgs << QLatin1String("-e")
        << QString::fromLatin1("tell application \"Finder\" to reveal POSIX file \"%1\"")
        .arg(pathIn);
    QProcess::execute(QLatin1String("/usr/bin/osascript"), scriptArgs);
    scriptArgs.clear();
    scriptArgs << QLatin1String("-e")
        << QLatin1String("tell application \"Finder\" to activate");
    QProcess::execute("/usr/bin/osascript", scriptArgs);
#endif//Q_OS_IOS
    
#else
    QUrl url = QUrl::fromLocalFile(pathIn);
    QDesktopServices::openUrl(url);
    /*
    // we cannot select a file here, because no file browser really supports it...
    const QFileInfo fileInfo(pathIn);
    const QString folder = fileInfo.absoluteFilePath();
    const QString app = Utils::UnixUtils::fileBrowser(Core::ICore::instance()->settings());
    QProcess browserProc;
    const QString browserArgs = Utils::UnixUtils::substituteFileBrowserParameters(app, folder);
    if (debug)
        qDebug() << browserArgs;
    bool success = browserProc.startDetached(browserArgs);
    const QString error = QString::fromLocal8Bit(browserProc.readAllStandardError());
    success = success && error.isEmpty();
    if (!success)
        showGraphicalShellError(parent, app, error);
    */
#endif
};

int meta_define_child_pindex(snc::CITEMS& items, int index)
{
    int child_pindex = -1;
    int items_count = items.size();
#ifdef _DEBUG
    QString s_route = "[-]";
#endif
    if (index == 0)
    {
        if (items_count == 0)
        {
            child_pindex = PINDEX_STEP;
#ifdef _DEBUG
            s_route = "[0]";
#endif
        }
        else
        {
            auto v = items[0];
            child_pindex = v->pindex() / 2;
#ifdef _DEBUG
            s_route = "[1]";
#endif
        }
    }
    else if (index == items_count)
    {
        if (items_count == 0)
        {
            ASSERT(0, QString("Inner logic error. invalid index: %1").arg(index).arg(items_count));
            return false;
        }
        else
        {
            auto v = items[items_count - 1];
            int idx2 = v->pindex();
            if (idx2 < 0)
            {
                idx2 = 0;
                ASSERT(0, "invalid item index") << idx2 << v->dbgHint();
            }
            child_pindex = idx2 + PINDEX_STEP;
#ifdef _DEBUG
            s_route = "[2]";
#endif
        }
    }
    else
    {
        if (index > items_count)
        {
            ASSERT(0, QString("Inner logic error. invalid index: %1/%2").arg(index).arg(items.size()));
            return false;
        }

        auto v_before = items[index - 1];
        auto v_after = items[index];

        int pindex_before = v_before->pindex();
        int pindex_after = v_after->pindex();
        if (pindex_before != -1 && pindex_after != -1)
        {
            //we are good
            child_pindex = (pindex_before + pindex_after) / 2;
#ifdef _DEBUG
            s_route = QString("[3](%1-%2-%3)").arg(pindex_before).arg(pindex_after).arg(index);
#endif
        }
    }
#ifdef _DEBUG
    // qDebug() << "meta-assign-child-index" << s_route << child_pindex;
#endif
    return child_pindex;
};


bool gui::isValidEmailAddress(QString email) 
{
    static QRegExp mailREX("\\b[A-Z0-9._%+-]+@[A-Z0-9.-]+\\.[A-Z]{2,4}\\b");
    mailREX.setCaseSensitivity(Qt::CaseInsensitive);
    mailREX.setPatternSyntax(QRegExp::RegExp);
    bool rv = mailREX.exactMatch(email);
    return rv;
};

QString gui::makeValidFileName(QString fileName)
{
   // QString rv = fileName.replace(QRegExp("[" + QRegExp::escape("\\/:*?\"<>|") + "]"), QString("_"));
    return googleQt::makeValidFileName(fileName);
}

QString gui::makeUniqueFileName(QString path, QString name, QString ext) 
{
    QString name1 = makeValidFileName(name);
    QString full_path = path + name1 + "." + ext;
    int idx = 0;
    while (QFile::exists(full_path))
    {
        name1 += QString("%1").arg(idx);
        idx++;
        full_path = path + name1 + "." + ext;
        if (idx > 200) {
            ASSERT(0, "potentially endless loop on file name generation") << full_path << idx;
        }
    }

    return name1;
};

bool PolicyCategory::isGmailDepending(EOutlinePolicy p)
{
    bool rv = false;
    switch (p) 
    {
    case outline_policy_PadEmail:
        rv = true;
    default:break;
    }
    return rv;
};


bool PolicyCategory::isKRingView(EOutlinePolicy p) 
{
    bool rv = false;
    switch (p)
    {
    case outline_policy_KRingTable:
    case outline_policy_KRingForm:
        rv = true;
    default:break;
    }
    return rv;
};

bool PolicyCategory::isHoistedBased(EOutlinePolicy p)
{
    bool rv = false;
    switch (p)
    {
    case outline_policy_Pad:
    case outline_policy_PadEmail:
        rv = true;
    default:break;
    }

    return rv;
};

bool PolicyCategory::is_observable_policy(EOutlinePolicy p) 
{
	bool rv = false;
	auto pp = parentPolicy(p);
	if (pp == outline_policy_2SearchView)
		rv = true;
	return rv;
};

bool PolicyCategory::is_shortcut_wrapping_policy(EOutlinePolicy p) 
{
	bool rv = false;
	switch (p) 
	{
	case outline_policy_BoardSelector: rv = true; break;
    default:break;
	}
	return rv;
};

EOutlinePolicy PolicyCategory::parentPolicy(EOutlinePolicy sub_policy)
{
    switch (sub_policy) {
    case outline_policy_KRingTable:
    case outline_policy_KRingForm:
        return outline_policy_KRingTable;

    case outline_policy_Pad:
        return outline_policy_Pad;

    
    case outline_policy_Notes:
	case outline_policy_Bookmarks:
	case outline_policy_Pictures:
    case outline_policy_Annotated:
    case outline_policy_2SearchView:
        return outline_policy_2SearchView;

    default:break;
    }

    return outline_policy_Uknown;
};

/**
    FieldParts
*/
bool FieldParts::isEmptyData()const 
{
    for (auto p : m_parts) {
        if (!p.second.isEmpty())
            return false;
    }
    return true;
};

QString FieldParts::type2label(FieldParts::EType pt)
{
#define FIELD_LABEL(F, L) case FieldParts::F: rv = L;break;

    QString rv;
    switch (pt) 
    {
        FIELD_LABEL(Title, "Title");
        FIELD_LABEL(FirstName, "First");
        FIELD_LABEL(LastName, "Last");
        FIELD_LABEL(Email, "Email");
        FIELD_LABEL(Phone, "Phone");
        FIELD_LABEL(AddrStreet, "Street");
        FIELD_LABEL(AddrCity, "City");
        FIELD_LABEL(AddrRegion, "Region");
        FIELD_LABEL(AddrZip, "Zip");
        FIELD_LABEL(AddrCountry, "Country");
        FIELD_LABEL(OrganizationName, "Name");
        FIELD_LABEL(OrganizationTitle, "Title");
        FIELD_LABEL(Notes, "Notes");
        FIELD_LABEL(Annotation, "Annotation");
        FIELD_LABEL(KRingTitle, "Title");
        FIELD_LABEL(KRingLogin, "Login");
        FIELD_LABEL(KRingPwd, "Password");
        FIELD_LABEL(KRingLink, "Link");
        FIELD_LABEL(KRingNotes, "Notes");
        default:ASSERT(0, "NA");
    }
    return rv;
};

void FieldParts::add(EType t, QString str) 
{
    std::pair<EType, QString> p;
    p.first = t;
    p.second = str;
    m_parts.push_back(p);
};

QString columntype2label(EColumnType t, bool ) 
{
#define CASE_COL(C, L)case EColumnType::C:                 rv = L;break;

    QString rv = "";
    switch (t)
    {
//        CASE_COL(Node, "#");
        CASE_COL(Title, "Title");
//        CASE_COL(ShortName, "Short");
        CASE_COL(ContactTitle, "Name");
        CASE_COL(ContactEmail, "Email");
        CASE_COL(ContactPhone, "Phone");
        CASE_COL(ContactAddress, "Address");
        CASE_COL(ContactOrganization, "Organization");
        CASE_COL(ContactNotes, "Notes");
        CASE_COL(FormFieldName, "Name");
        CASE_COL(FormFieldValue, "Value");
        CASE_COL(KRingTitle, "Title");
        CASE_COL(KRingLogin, "Login");
        CASE_COL(KRingPwd, "Pwd");
        CASE_COL(KRingLink, "Link");
        CASE_COL(KRingNotes, "Notes");
        CASE_COL(Selection, "x");
    default:ASSERT(0, "NA"); break;
    }
    return rv;
};

/**
LastSyncInfo
*/
LastSyncInfo::LastSyncInfo()
{
    invalidate();
};

void LastSyncInfo::invalidate()
{
    syncTime = QDateTime();
    syncDbSize = 0;
};

bool LastSyncInfo::isValid()const
{
    return (syncTime.isValid() && syncDbSize > 100);
};

QString LastSyncInfo::toString()const
{
    QString rv;
    if (isValid()) {
        rv = QString("1|%1|%2").arg(syncDbSize).arg(syncTime.toString(Qt::ISODate));
    }
    return rv;
};

QString LastSyncInfo::toString4Human()const
{
    extern QString size_human(float num);
    QString rv;
    if (isValid()) {
        rv = QString("%1 - %2").arg(size_human(syncDbSize)).arg(syncTime.toString());
    }
    return rv;
};

bool LastSyncInfo::fromString(QString s)
{
    invalidate();

    QStringList lst = s.split("|");
    if (lst.size() == 3) {
        int v = lst[0].toInt();
        if (v != 1) {
            return false;
        }

        syncDbSize = lst[1].toULongLong();
        syncTime = QDateTime::fromString(lst[2], Qt::ISODate);        
    }

    return isValid();
};


/**
*   TopicsByType
*/
void TopicsByType::pipe(snc::cit* it)
{
    topic_ptr t = dynamic_cast<ard::topic*>(it);
    if (t) 
    {
        m_topics.push_back(t);
        int tmp = (int)t->folder_type();
        if (tmp > 0) {
            m_by_type[tmp] = t;
        }
    }
}

ard::topic* TopicsByType::findGtdSortingFolder(EFolderType ft)
{
    auto it = m_by_type.find((int)ft);
    if (it != m_by_type.end()) {
        return it->second;
    }
    return nullptr;
};

void gui::sleep(int milliseconds) 
{
    QTime endTime = QTime::currentTime().addMSecs(milliseconds);
    while (QTime::currentTime() < endTime)
    {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
    }
};

QTableView* gui::createTableView(QStandardItemModel* m, QTableView* tv /*= nullptr*/)
{   
    //QTableView* pv = 
    //QTableView* pv = NewArdTableView();
    if (!tv) {
        tv = NewArdTableView();
    }
    tv->setModel(m);

    QHeaderView* h = tv->horizontalHeader();
    int columns = m->columnCount();
    for (int i = 0; i < columns; i++) {
        h->setSectionResizeMode(i, QHeaderView::Stretch);
    }

    tv->setWordWrap(true);
    tv->setTextElideMode(Qt::ElideMiddle);
    tv->resizeRowsToContents();
    return tv;
};

void gui::copyTableViewSelectionToClipbard(QTableView* v) 
{
    auto cb = QApplication::clipboard();
    if (cb) {
        QModelIndexList si_lst = v->selectionModel()->selectedIndexes();
        QString clipboardString;
        for (int i = 0; i < si_lst.count(); i++) {
            auto mi = si_lst[i];
            clipboardString += mi.data().toString();
            if (i != si_lst.count() - 1) {
                clipboardString += ", ";
            }
        }
        cb->setText(clipboardString);
    }
};

email_ptr ard::as_email(topic_ptr f) 
{ 
    auto m = dynamic_cast<email_ptr>(f);
    if (!m) {
        auto t = dynamic_cast<ethread_ptr>(f);
        if (t) {
            m = t->headMsg();
        }
    }
    return m;
};
email_cptr ard::as_email(topic_cptr f) 
{
    auto m = dynamic_cast<email_cptr>(f);
    if (!m) {
        auto t = dynamic_cast<ethread_cptr>(f);
        if (t) {
            m = t->headMsg();
        }
    }
    return m;
};

ethread_ptr ard::as_ethread(topic_ptr f) 
{ 
    auto t = dynamic_cast<ethread_ptr>(f);
    if (t)
        return t;
    auto m = dynamic_cast<email_ptr>(f);
    if (m && m->parent()) {
        return dynamic_cast<ethread_ptr>(m->parent());
    }
    return nullptr;
};

ethread_cptr ard::as_ethread(topic_cptr f) 
{ 
    auto t = dynamic_cast<ethread_cptr>(f);
    if (t)
        return t;
    auto m = dynamic_cast<email_cptr>(f);
    if (m && m->parent()) {
        return dynamic_cast<ethread_cptr>(m->parent());
    }
    return nullptr;
};

QRgb ard::cidx2color(ard::EColor c) 
{
	switch (c)
	{
	case ard::EColor::red:return color::Red;
	case ard::EColor::green:return color::Green;
	case ard::EColor::blue:return color::Blue;
	case ard::EColor::purple:return color::Purple;
	default:
		break;
	}
	return color::Black;
};

QString ard::cidx2color_str(EColor c) 
{
	return color::getColorStringByClrIndex(ard::cidx2color(c), color::palTrue1);
};

char ard::cidx2char(ard::EColor c) 
{
	switch (c) 
	{
	case ard::EColor::red: return 1;
	case ard::EColor::green: return 2;
	case ard::EColor::blue: return 3;
	case ard::EColor::purple: return 4;
    default:break;
	}
	return 0;
};

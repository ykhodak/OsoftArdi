#include <QDir>
#include <QPainter>
#include <QPixmap>
#include <QBitmap>
#include <time.h>
#include <QCoreApplication>
#include <QApplication>
#include <QHBoxLayout>
#include <QPushButton>
#include <QToolBar>
#include <QPalette>
#include <QProgressBar>
#include <QMenu>
#include <QMessageBox>

#include "utils.h"
#include "anfolder.h"
#include "MainWindow.h"
#include "ChoiceBox.h"
#include "ProtoScene.h"
#include "OutlineScene.h"
#include "OutlineMain.h"
#include "syncpoint.h"
#include "contact.h"
#include "ansearch.h"
#include "ardmodel.h"
#include "email.h"
#include "custom-boxes.h"
#include "ethread.h"
#include "rule_runner.h"

static QFont*   _defaultFont = nullptr;
static QFont*   _defaultNoteFont = nullptr;
static QFont*   _defaultBoldFont = nullptr;
static QFont*   _defaultSmallFont = nullptr;
static QFont*   _defaultSmall2Font = nullptr;
static QFont*   _defaultSmallBoldFont = nullptr;
static QFont*   _NHeaderFont = nullptr;
static QFont*   _defaultLabelFont = nullptr;
static QFont*   _defaultTinyFont = nullptr;
static QString  _defaultFontFamily = "";
static int      _defaultFontSize = 0;
static QFont*   _groupHeaderFont = nullptr;


static QFont*   _boardNormalTextFont = nullptr;
static QFont*   _boardNormalItalicFont = nullptr;
static QFont*   _boardNormalBoldFont = nullptr;

static unsigned _defaultFontHeight = 0;
static unsigned _defaultSmallFontHeight = 0;
static unsigned _outlineHeight = 0;
static unsigned _outlineSmallHeight = 0;
static unsigned _tinyFontHeight = 0;
static unsigned _outlineShortDateWidth = 0;
static unsigned _graphNodeHeaderHeight = 0;
static unsigned _outlineGroupHeaderHeight = 0;
static unsigned _nheader_height = 0;


extern QString get_ard_folder(QString parent_path, QString sub_folder);

int utils::outlinePadding(){return 5;};
int progressHeight() { return gui::lineHeight() / 3; }


QString get_tmp_code_dir_path()
{
    QString rv = get_ard_folder(defaultRepositoryPath(), "tmp_code_area/");
    return rv;
};

QString get_backup_dir_path()
{
    static QString backup_dir = "";
    if(backup_dir.isEmpty())
        {
            backup_dir = get_ard_folder(defaultRepositoryPath(), "backup/");
        }
    return backup_dir;
};


bool isDefaultDBName(QString db_name)
{
    bool rv = false;
    if(db_name.compare(DEFAULT_DB_NAME, Qt::CaseInsensitive) == 0)
        {
            rv = true;
        }
    return rv;
};

QString get_tmp_sync_dir_prefix()
{
    return "tmp_sync_area4";
}

QString get_tmp_sync_dir_path(QString composit_db_prefix)
{
    QString rv = "";
    if(composit_db_prefix.isEmpty())
        {
            rv = get_ard_folder(ard::logDir(), QString("%1MyData-%2/")
                .arg(get_tmp_sync_dir_prefix())
                .arg(dbp::configEmailUserId()));
        }
    else
        {
            rv = get_ard_folder(ard::logDir(), QString("%1%2-%3/")
                .arg(get_tmp_sync_dir_prefix())
                .arg(composit_db_prefix)
                .arg(dbp::configEmailUserId()));
        }
    return rv;
};

QString get_tmp_import_dir_path() 
{
    QString rv = get_ard_folder(ard::logDir(), "tmp_import_area/");
    return rv;
};

QString get_sync_log_path(QString db_prefix)
{
    QString rv = get_tmp_sync_dir_path(db_prefix);
    rv += "sync.log";
    return rv;  
};

QString get_sync_log_archives_path()
{
    static QString archives_dir = "";
    if(archives_dir.isEmpty())
        {
            archives_dir = get_ard_folder(ard::logDir(), "sync-log-archives/");
        }
    return archives_dir;  
};

QString get_db_tmp_file_path(QString repositoryPath = "")
{
    if(repositoryPath.isEmpty())
        {
            repositoryPath = defaultRepositoryPath();
        }

    QString rv = repositoryPath + "temporary-db.tmp2";
    return rv;
}

void check_cleanup()
{
    utils::removeEmptySubfolders(ard_dir(dirMP));
    utils::removeEmptySubfolders(ard_dir(dirMP1));
    utils::removeEmptySubfolders(ard_dir(dirMP2));
}

QString get_curr_db_weekly_backup_file_path()
{
	assert_return_empty(gui::isDBAttached(), "expected attached DB");

    QString backup_file_path = "";
    QString db_path_name = get_db_file_path();
    QDate dt = QDate::currentDate();
    int ynum = 0;
    int wnum = dt.weekNumber(&ynum);
    QFileInfo fi(db_path_name);
    QString cpath = fi.canonicalPath();
    int idx = cpath.lastIndexOf("/");//have to test on Windows
    if(idx != -1)
        {
            idx = cpath.lastIndexOf("/", idx-1);
            if(idx != -1)
                {
                    QString sub_path = db_path_name.mid(idx+1, cpath.size() - idx - 1);
                    sub_path = sub_path.replace("/", "--");
                    backup_file_path = get_backup_dir_path() + QString("%1.%2w%3.qpk").arg(sub_path).arg(ynum).arg(wnum);
                }
        }

    return backup_file_path;
};

QString programName()
{
    return "Ardi";
}

QString utils::get_program_appdata_log_dir_path()
{
    static QString log_dir = "";
    if(log_dir.isEmpty())
        {
            log_dir = get_ard_folder(defaultRepositoryPath(), "log/");
        }
    return log_dir;
}

QString ard::get_crypto_config_dir()
{
    static QString db_file = "";
    if (db_file.isEmpty())
    {
        db_file = get_ard_folder(defaultRepositoryPath(), ".iid2");
    }
    return db_file;
};

QString ard::logDir() 
{
    return utils::get_program_appdata_log_dir_path();
};

QString get_program_appdata_sync_autotest_log_file_name() 
{
    QString log_file = utils::get_program_appdata_log_dir_path() + "sync_auto_test.log";
    return log_file;
};

QString get_program_appdata_log_file_name()
{
    QString log_file = utils::get_program_appdata_log_dir_path() + "ardi-on-the-run.log";
    return log_file;
};

QString get_program_appdata_bak_log_file_name()
{
    QString log_file = get_program_appdata_log_file_name() + ".bak";
    return log_file;
};

QString get_program_appdata_bak2_log_file_name()
{
    QString log_file = get_program_appdata_log_file_name() + ".bak2";
    return log_file;
};

int ard::defaultFallbackFontSize()
{
    int defDropDown = 24;

#ifdef ARD_BIG
#ifdef Q_OS_MAC
    defDropDown = 22;
#else
    defDropDown = 14;
#endif
#endif
    return defDropDown;
};

static void doInitFonts(int custom_fontSize = -1)
{
    _defaultFont = new QFont(QApplication::font());
    ard::resetNotesFonts();

    _defaultFontSize = _defaultFont->pointSize();
    if (custom_fontSize != -1)
        {
            _defaultFontSize = custom_fontSize;
        }

    int defDropDown = ard::defaultFallbackFontSize();

  
    if (_defaultFontSize < MIN_CUST_FONT_SIZE || _defaultFontSize > MAX_CUST_FONT_SIZE)
    {
        _defaultFontSize = defDropDown;
    }
    
    int smallFontSize = _defaultFontSize - 2;
    int bigFontSize = _defaultFontSize + 4;
    //int hugeIndexHintFontSize = _defaultFontSize + 30;
    _defaultFont->setPointSize(_defaultFontSize);
    _defaultFontFamily = _defaultFont->family();

    _defaultSmallFont = new QFont(*_defaultFont);
    _defaultSmallFont->setPointSize(smallFontSize);

    _defaultSmall2Font = new QFont(*_defaultFont);
    _defaultSmall2Font->setPointSize(smallFontSize - 4);

    _defaultSmallBoldFont = new QFont(*_defaultSmallFont);
    _defaultSmallBoldFont->setBold(true);

    _NHeaderFont = new QFont(*_defaultFont);
    _NHeaderFont->setPointSize(smallFontSize);
    _NHeaderFont->setBold(true);

    _defaultLabelFont = new QFont(*_defaultSmallFont);
    _defaultLabelFont->setPointSize(smallFontSize-2);
    _defaultLabelFont->setBold(true);

    _defaultTinyFont = new QFont(*_defaultSmallFont);
    _defaultTinyFont->setPointSize(smallFontSize-6);
    _defaultTinyFont->setBold(false);

    
    _groupHeaderFont = new QFont(*_defaultFont);
    _groupHeaderFont->setPointSize(bigFontSize);
    _groupHeaderFont->setBold(true);
    //    _groupHeaderFont->setUnderline(true);

    _defaultBoldFont = new QFont(*_defaultFont);
    _defaultBoldFont->setBold(true);

    ///..
    
    _boardNormalTextFont = new QFont(*_defaultSmallFont);
    _boardNormalItalicFont = new QFont(*_defaultFont);
    _boardNormalItalicFont->setFamily("Forte");
   // _boardNormalItalicFont->setItalic(true);
    _boardNormalBoldFont = new QFont(*_defaultFont);
    //_boardNormalBoldFont->setPointSize(bigFontSize);
    //_boardNormalBoldFont->setBold(true);
    ///..

    utils::applySettings();
}

/*
void utils::reset(int customFontSize)
{
    doInit(customFontSize);
};*/

bool utils::resetFonts()
{
    int customFontSize = dbp::configFileCustomFontSize();  
    doInitFonts(customFontSize);
    return true;
}

void utils::clean()
{
    FREE_OBJ(_defaultFont);
    FREE_OBJ(_groupHeaderFont);
    FREE_OBJ(_defaultLabelFont);
    FREE_OBJ(_defaultSmallFont);
    FREE_OBJ(_defaultSmall2Font);
    FREE_OBJ(_NHeaderFont);
    FREE_OBJ(_defaultSmallBoldFont);
    FREE_OBJ(_boardNormalTextFont);
    FREE_OBJ(_boardNormalItalicFont);
    FREE_OBJ(_boardNormalBoldFont);
}

QFont* utils::defaultNoteFont()
{
    return _defaultNoteFont;
}

QFont* utils::defaultSmall2Font()
{
    return _defaultSmall2Font;
}

QFont* utils::defaultSmallBoldFont()
{
    return _defaultSmallBoldFont;
}


QString utils::defaultFontFamily()
{
    return _defaultFontFamily;
};

int utils::defaultFontSize()
{
    return _defaultFontSize;
};

QFont* utils::groupHeaderFont()
{
    return _groupHeaderFont;
}

QFont* utils::annotationFont() 
{
    return _groupHeaderFont;
};

QFont* utils::nheaderFont()
{
    return _NHeaderFont;
};

QFont* utils::defaultTinyFont()
{
    return _defaultTinyFont;
};

qreal utils::nheaderFontHeight()
{
    return _nheader_height;
};

QFont* utils::defaultBoldFont()
{
    return _defaultBoldFont;
};

QFont* utils::graphNodeHeaderFont()
{
    return defaultSmallBoldFont();
};

#define DEFINE_ICON(N, R)QPixmap N(){static QPixmap pm(QString(":ard/images/unix/%1").arg(R));return pm;}

DEFINE_ICON(getIcon_Pad, "note.png");
DEFINE_ICON(getIcon_TopicExpanded, "icon-topic-open.png");
DEFINE_ICON(getIcon_TopicCollapsed, "icon-topic-closed.png");
DEFINE_ICON(getIcon_Folder, "open_folder_gray.png");
DEFINE_ICON(getIcon_Sortbox, "sortbox.png");
DEFINE_ICON(getIcon_Recycle, "x-trash.png");
DEFINE_ICON(getIcon_Reference, "shelf.png");
DEFINE_ICON(getIcon_Maybe, "empty-box.png");
DEFINE_ICON(getIcon_Delegated, "hresource.png");
DEFINE_ICON(getIcon_Bookmark, "icon-bookmark.png");
DEFINE_ICON(getIcon_NotLoadedEmail, "one-dot.png");
DEFINE_ICON(getIcon_InProgress, "wait-blue.png");
DEFINE_ICON(getIcon_AttachmentFile, "attach.png");
DEFINE_ICON(getIcon_CheckSelect, "check-black.png");
DEFINE_ICON(getIcon_CheckedBox, "check-white.png");
DEFINE_ICON(getIcon_CheckedGrayedBox, "check-gray.png");
DEFINE_ICON(getIcon_UnCheckedBox, "uncheck-white.png");
DEFINE_ICON(getIcon_GreenCheck, "check.png");
DEFINE_ICON(getIcon_Url, "blue-globe.png");
DEFINE_ICON(getIcon_RulesFilter, "search-filter.png");
DEFINE_ICON(getIcon_Drafts, "email-closed-env.png");
DEFINE_ICON(getIcon_Email, "email-new.png");
DEFINE_ICON(getIcon_CloseBtn, "close-tab.png");
DEFINE_ICON(getIcon_PinBtn, "gray-pin.png");
DEFINE_ICON(getIcon_TabPin, "tab-pin.png");
DEFINE_ICON(getIcon_MoveTabsBtn, "organize-gray.png");
DEFINE_ICON(getIcon_Annotation, "annotation.png");
DEFINE_ICON(getIcon_AnnotationWhite, "annotation-white.png");
DEFINE_ICON(getIcon_Star, "star_on.png");
DEFINE_ICON(getIcon_StarOff, "star_off.png");
DEFINE_ICON(getIcon_Important, "important_on.png");
DEFINE_ICON(getIcon_ImportantOff, "important_off.png");
DEFINE_ICON(getIcon_Open, "bkg-text.png");
DEFINE_ICON(getIcon_Insert, "add.png");
DEFINE_ICON(getIcon_Remove, "remove.png");
DEFINE_ICON(getIcon_LinkArrow, "arrow-link.png");
DEFINE_ICON(getIcon_BoardShape, "board-shape.png");
DEFINE_ICON(getIcon_BoardTemplate, "b-template.png");
DEFINE_ICON(getIcon_Rename, "rename.png");
DEFINE_ICON(getIcon_Locate, "target-mark.png");
DEFINE_ICON(getIcon_EmailLocate, "email-forward.png");
DEFINE_ICON(getIcon_BoardTopicsFolder, "unix/board-shape.png");
DEFINE_ICON(getIcon_PopupUnlocked, "tab-slide-on.png");
DEFINE_ICON(getIcon_PopupLocked, "tab-slide-off.png");
DEFINE_ICON(getIcon_Find, "find.png");
DEFINE_ICON(getIcon_Search, "search-view-gray.png");
DEFINE_ICON(getIcon_Details, "dots.png");
DEFINE_ICON(getIcon_VDetails, "dots-v.png");
DEFINE_ICON(getIcon_LockTabRoll, "x-lock-closed.png");
DEFINE_ICON(getIcon_UnlockTabRoll, "x-lock-open.png");
DEFINE_ICON(getIcon_EmptyBoard, "empty-board.png");
DEFINE_ICON(getIcon_EmptyPicture, "empty-picture.png");
DEFINE_ICON(getIcon_Copy, "edit-copy.png");
DEFINE_ICON(getIcon_Paste, "edit-paste.png");
DEFINE_ICON(getIcon_EmailAsRead, "email-in-env.png");
//DEFINE_ICON(getIcon_MailBoard, "mail-board.png");
DEFINE_ICON(getIcon_MailBoard, "email-closed-env.png");
DEFINE_ICON(getIcon_SelectorBoard, "selector-board.png");
DEFINE_ICON(getIcon_RuleFilter, "rule-filter.png");
DEFINE_ICON(getIcon_SearchGlass, "search-view-gray.png");

int utils::projectHeaderHeight()
{
    int rv = (int)(2 * _defaultFontHeight);
    return rv;
};


int utils::outlineDefaultHeight()
{
    int rv = (int)_defaultFontHeight;
    return rv;
};

int utils::outlineTinyHeight()
{
    int rv = (int)_tinyFontHeight;
    return rv;
};

int utils::outlineSmallHeight() 
{
    int rv = (int)_defaultSmallFontHeight;
    return rv;
};

int utils::outlineShortDateWidth() 
{
    int rv = (int)_outlineShortDateWidth + ARD_MARGIN;
    return rv;
};

int ard::graphNodeHeaderHeight()
{
    int rv = (int)_graphNodeHeaderHeight;
    return rv;
};

unsigned utils::calcWidth(QString s, QFont* font)
{
    QFontMetrics fm(font ? *font : *_defaultFont );
    return fm.boundingRect(s).width();
}

unsigned utils::calcHeight(QString s, const QFont* font)
{
    QFontMetrics fm(font ? *font : *_defaultFont);
    return fm.boundingRect(s).height();
}

QSize utils::calcSize(QString s, QFont* font)
{
    QFontMetrics fm(font ? *font : *_defaultFont);
    return fm.size(Qt::TextSingleLine, s);
};

void utils::applySettings()
{
    _defaultFontHeight = calcHeight("X", _defaultFont);
    _defaultSmallFontHeight = calcHeight("X", _defaultSmallFont);
    _outlineHeight = _defaultFontHeight + 1 + utils::outlinePadding();
    _outlineSmallHeight = _defaultSmallFontHeight + 1 + utils::outlinePadding();
    _outlineGroupHeaderHeight = calcHeight("X", _groupHeaderFont) + 1;
    _graphNodeHeaderHeight = calcHeight("X", _defaultSmallBoldFont);
    _nheader_height = calcHeight("X", _NHeaderFont);
    _outlineShortDateWidth = utils::calcWidth("2/2/2016", _defaultSmallFont);
    _tinyFontHeight = calcHeight("X", _defaultTinyFont);
}


QIcon ard::iconFromTheme(QString name) 
{
    return utils::fromTheme(name);
};

QIcon utils::fromTheme(QString name)
{
    QString s = ":ard/images/unix/" + name;
    return QIcon(s);
};

QAction* utils::actionFromTheme(QString theme, QString label, QObject* parent, QAction::Priority priority /*= QAction::LowPriority*/)
{
    QAction* a = new QAction(fromTheme(theme), label, parent);
    a->setPriority(priority);
    return a;
};

QPixmap utils::colorMarkIcon(ard::EColor clr_idx, bool as_checked)
{
    QPixmap pm(QSize(48,48));
    QPainter p(&pm);
    QBrush b(ard::cidx2color(clr_idx));
    QRect rc(0,0,48,48);
    p.setBrush(b);
    p.setPen(Qt::gray);
    p.drawRect(rc);
    p.setPen(Qt::black);
    p.setFont(*ard::defaultFont());
    p.drawText(rc, Qt::AlignCenter | Qt::AlignVCenter, as_checked ? "[x]" : "[ ]");
    return pm;
};

QPixmap* utils::colorMarkSelectorPixmap(ard::EColor c)
{
    static QPixmap pm_red;
    static QPixmap pm_green;
    static QPixmap pm_blue;
    static QPixmap pm_purple;

#define CASE_CLR(I, V, N)\
    case I:\
        if (V.isNull())\
        {\
            V = QPixmap(QString(":ard/images/unix/%1.png").arg(N));\
        }\
        pm = &V;\
        break;\

    QPixmap* pm = nullptr;
    switch (c)
    {
        CASE_CLR(ard::EColor::purple, pm_purple, "brush-stroke-purple");
        CASE_CLR(ard::EColor::red, pm_red, "brush-stroke-red");
        CASE_CLR(ard::EColor::blue, pm_blue, "brush-stroke-blue");
        CASE_CLR(ard::EColor::green, pm_green, "brush-stroke-green");
    default:break;
    }

    return pm;
};

static void draw_pazzle(QPainter *painter, const QRect& rc)
{
    //painter->fillRect(rc, Qt::gray);
    int x = rc.left() + (rc.width() - 48) / 2;
    int y = rc.top() + (rc.height() - 48) / 2;
    QPixmap pm(":ard/images/unix/pazzle.png");
    painter->drawPixmap(x, y, pm);
};

static void draw_wait(QPainter *painter, const QRect& rc)
{
    int x = rc.left() + (rc.width() - 48) / 2;
    int y = rc.top() + (rc.height() - 48) / 2;
    QPixmap pm(":ard/images/unix/wait-blue.png");
    painter->drawPixmap(x, y, pm);
};



void utils::drawImagePixmap(const QPixmap& pm, QPainter *painter, const QRect& rc, QPoint* ptLT)
{  
    QRect rcImage = pm.rect();
    if(rcImage.height() == 0 || rc.height() == 0)
        return;

    if(rcImage.width() > rc.width() ||
       rcImage.height() > rc.height())
        {
            int calc_width = 0;
            int calc_height = 0;
            double image_aspect_ratio = (double)rcImage.width() / rcImage.height();
            if(image_aspect_ratio > 1.0)
                {
                    calc_width = rc.width();
                    calc_height = (int)((double)calc_width / image_aspect_ratio);
                    if(calc_height > rc.height())
                        {
                            calc_height = rc.height();
                            calc_width = (int)(calc_height * image_aspect_ratio);
                        }
                }
            else
                {
                    calc_height = rc.height();
                    calc_width = (int)(calc_height * image_aspect_ratio);
                }

            QRect rc2 = rc;
            rc2.setHeight(calc_height);
            rc2.setWidth(calc_width);

            if(ptLT)
                {
                    rc2.translate(*ptLT);
                }

            painter->drawPixmap(rc2, pm.scaled(calc_width, calc_height, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        }
    else
        {
            QPoint ptLeftTop = QPoint(rc.left(), rc.top());
            if(ptLT)
                {
                    ptLeftTop = *ptLT;
                }
            painter->drawPixmap(ptLeftTop, pm);
        }
};

bool is_image_type_supported(QString ext)
{
    static std::set<QString> supported_images;
    static bool first_call = true;
    if(first_call)
        {
            supported_images.insert("png");
            supported_images.insert("jpg");
            supported_images.insert("jpeg");
            supported_images.insert("bmp");
            supported_images.insert("tiff");
            first_call = false;
        }

    bool rv = supported_images.find(ext) != supported_images.end();
    return rv;
};

int gui::lineHeight()
{
    return _outlineHeight;
};

int gui::shortLineHeight() 
{
    return _outlineSmallHeight;
};

int gui::headerHeight()
{
    return (int)utils::projectHeaderHeight();
};

bool ard::confirmBox(QWidget* parent, QString s)
{
    return (ConfirmBox::confirm(parent, s) == YesNoConfirm::yes);
};

YesNoConfirm ard::confirmBoxWithOption(QWidget* parent, QString s, QString optionStr)
{
	return ConfirmBox::confirmWithOption(parent, s, optionStr);
};


int ard::choice(std::vector<QString>& options_list, int selected, QString label)
{
    return utils::choice(options_list, selected, label);
};

int ard::choiceCombo(std::vector<QString>& options_list, int selected, QString label, std::vector<QString>* buttons_list)
{
    return utils::choiceCombo(options_list, selected, label, buttons_list);
};


int utils::choice(std::vector<QString>& options_list, int selected, QString label)
{
    return ChoiceBox::choice(options_list, selected, label);
};

int utils::choiceCombo(std::vector<QString>& options_list, int selected, QString label, std::vector<QString>* buttons_list)
{
    return ComboChoiceBox::choice(options_list, selected, label, buttons_list);
};

int gui::choice(std::vector<QString>& options_list, int selected, QString label)
{
    return utils::choice(options_list, selected, label);
};

int gui::choiceCombo(std::vector<QString>& options_list, int selected, QString label, std::vector<QString>* buttons_list)
{
    return utils::choiceCombo(options_list, selected, label, buttons_list);
};

std::pair<bool, QString> gui::edit(QString text, QString label, bool selected, QWidget* parentWidget)
{
    return EditBox::edit(text, label, selected, false, parentWidget);
};

std::vector<QString> gui::edit(std::vector<QString> labels, QString header, QString confirmStr)
{
    return MultiEditBox::edit(labels, header, confirmStr);
};

void utils::drawCompletedBox(topic_ptr it, QRectF& rc, QPainter *painter)
{
	int perc = it->getToDoDonePercent4GUI();
	bool bcompleted = (perc > 99);

	if (it->isToDo())
	{
		bool drawEmptyBox = false;//(perc == 0);
		utils::drawCheckBox(painter, rc, bcompleted, perc, drawEmptyBox);
	}
};


void utils::drawCheckBox(QPainter *painter, const QRectF& rc, bool checked, int CompletedPercentages /*= 0*/, bool drawEmptyBox /*= false*/)
{
    QRect rc2;
    rectf2rect(rc, rc2);
    rc2.setWidth((int)gui::lineHeight());
    rc2.setHeight((int)gui::lineHeight());

    if(drawEmptyBox)
        {
            QPixmap pm(":ard/images/unix/checkbox_unchecked.png");
            painter->drawPixmap(rc2, pm);      
        }

    if(checked)
        {
            QPixmap pm(":ard/images/unix/check.png");
            painter->drawPixmap(rc2, pm);
        }
    else
        {
            if(CompletedPercentages > 0)
                {
                    int pheight = progressHeight();
                    PGUARD(painter);
                    painter->setPen(Qt::NoPen);
                    rc2.setLeft(rc2.left() + 1);
                    rc2.setTop(rc2.top() + (gui::lineHeight() - pheight) / 2);
                    rc2.setWidth(rc2.width() - 2);
                    rc2.setHeight(pheight/* - 3*/);
                    QBrush b1(color::Navy);
                    painter->setBrush(b1);
                    painter->drawRect(rc2);
                    int w = (int)(rc2.width()*((double)CompletedPercentages / 100));
                    if(w < 2)w = 2;
                    rc2.setWidth(w);
                    QBrush b2(color::Green);
                    painter->setBrush(b2);
                    painter->drawRect(rc2);
                }
        }  
};

void utils::drawPriorityIcon(topic_ptr it, QRect& rc, QPainter *painter)
{
    QRect rc2 = rc;
    rc2.setWidth(gui::lineHeight());
    rc2.setHeight(gui::lineHeight());

    if(it->hasToDoPriority())
        {
            int prio = it->getToDoPriorityAsInt();
            switch(prio)
                {
                case 2:
                    {
                        QPixmap pm(":ard/images/unix/red-exclamation.png");
                        painter->drawPixmap(rc2, pm);
                    }break;
                case 3:
                    {
                        QPixmap pm(":ard/images/unix/exclamation-in-circle.png");
                        painter->drawPixmap(rc2, pm);
                    }break;
                }      
        }
};

QColor gui::darkSceneBk()
{
    QColor clBkDef = gui::colorTheme_BkColor();
    QColor cl = color::Black;
    if(clBkDef.black() == 255)
        {
            cl = color::Black;
        }
    else if(clBkDef.black() < 10)
        {
            cl = color::Gray_1;
        }
    else
        {
            cl = clBkDef.darker(300);
        }
    return cl;
};


void utils::drawQFolderStatus(ard::rule_runner* f, QPainter* p, const QRect& rct) 
{
    auto t3 = f->ternaryIconWidth();
    assert_return_void(t3.first == TernaryIconType::gmailFolderStatus, "Expected Q-topic for G-Folder");
    assert_return_void(t3.second > 0, "Invalid t-width of Q-topic for G-Folder");

    PGUARD(p);
    p->setRenderHint(QPainter::Antialiasing, true);
	auto s = f->outlineLabel();
    if (!s.isEmpty()) {
        QFont* fnt = ard::defaultSmallFont();
        p->setFont(*fnt);
        //QString s = QString("%1").arg(c);
        QFontMetrics fm(*fnt);
        QRect r2c = fm.boundingRect(s);
        r2c.setLeft(r2c.left() - 2 * ARD_MARGIN);
        r2c.setWidth(r2c.width() + 4 * ARD_MARGIN);

        QRect r2d = rct;
        r2d.setTop(r2d.top() + 2 * ARD_MARGIN);


        if (r2c.width() > t3.second) {
            f->setTernaryIconWidth(r2c.width());
        }
        
        r2d.setWidth(r2c.width());
        
        QBrush br(color::Gray_1);
        p->setBrush(br);
        QPen pn(color::Gray_1);
        p->setPen(pn);

        if (f->downloadProgressPercentage() == 0){
            int flags = Qt::AlignHCenter | Qt::AlignVCenter;
            //p->drawEllipse(r2d);
            int radius = 10;
            p->drawRoundedRect(r2d, radius, radius);

            QPen pn1(color::White);
            p->setPen(pn1);
            p->drawText(r2d, flags, s);
        }
        else {
            p->setBrush(Qt::NoBrush);
            QPen pn(color::LightNavy);
            pn.setWidth(2);
            p->setPen(pn);
            p->drawEllipse(rct);

            pn.setColor(color::True1Navy);
            pn.setWidth(2);
            p->setPen(pn);

            int grad = (int)(360.0 * f->downloadProgressPercentage() / 100);
            if (grad < 5) {
                p->drawArc(rct, 0, 600);
            }
            else {
                p->drawArc(rct, 0, grad * 16);
            }
        }       
    }
};


void gui::removeEmptySubfolders(QString parentPath)
{
    utils::removeEmptySubfolders(parentPath);
};

void utils::removeEmptySubfolders(QString parentPath)
{
	QDir dd(parentPath);
	QFileInfoList ilst = dd.entryInfoList(QDir::NoDotAndDotDot | QDir::Dirs);
	if (!ilst.isEmpty())
	{
		for (QFileInfoList::iterator i = ilst.begin(); i != ilst.end(); i++)
		{
			QFileInfo fi = *i;
			if (fi.isDir())
			{
				QString name = fi.fileName();
				QString sp = fi.canonicalFilePath();
				QFileInfoList l2 = QDir(sp).entryInfoList(QDir::NoDotAndDotDot | QDir::AllEntries);
				if (l2.isEmpty()){
					if (!dd.rmdir(name))
					{
						ASSERT(0, "failed-to-remove-empty-dir") << parentPath << name;
					}
				}
			}
		}
	}
};


void utils::setupCentralWidget(QWidget *parent, QWidget* child)
{
    QHBoxLayout *l = new QHBoxLayout(parent);
    utils::setupBoxLayout(l);
    l->addWidget(child);
};

void utils::setupBoxLayout(QLayout* l)
{
    l->setContentsMargins(0,0,0,0);
    l->setSpacing(0);
    l->setAlignment(Qt::AlignLeft);
};

void utils::prepareGuiAfterTopicsMoved(const TOPICS_LIST& )
{
    ///we moved topics away, have to relist Q-topics

    bool relist_q_topic = false;
    EOutlinePolicy main_pol = gui::currPolicy();
    switch (main_pol)
    {
    case outline_policy_PadEmail:
        relist_q_topic = true;
        break;
    default: break;
    }

    if (relist_q_topic) {
		auto db = ard::db();
		if (db && db->isOpen())
        {
            auto qt = dynamic_cast<ard::rule_runner*>(db->gmail_runner());
            if (qt) {
                qt->qrelist(true, "moved-topics");
            }
        }
    }
};

void gui::setupBoxLayout(QLayout* l) 
{
    utils::setupBoxLayout(l);
};

void utils::setupProgressBar(QProgressBar* b) 
{
    b->setMaximumHeight(10);
    b->setTextVisible(false);
};


void setupMainLayout(QLayout* l)
{
#ifndef ARD_BIG
    utils::setupBoxLayout(l);
#else
    Q_UNUSED(l);
#endif
};

void utils::expandWidget(QWidget* w)
{
    QSizePolicy sp4preview(QSizePolicy::Expanding, QSizePolicy::Expanding);
    sp4preview.setHorizontalStretch(0);
    sp4preview.setVerticalStretch(0);    
    w->setSizePolicy(sp4preview);
};

void utils::setupToolButton(QPushButton* b)
{
    QSizePolicy sizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    sizePolicy.setHorizontalStretch(0);
    sizePolicy.setVerticalStretch(0);
    sizePolicy.setHeightForWidth(b->sizePolicy().hasHeightForWidth());
    b->setSizePolicy(sizePolicy);
};

unsigned utils::node_width(QString s, unsigned minWidth)
{
    unsigned widthText = utils::calcWidth(s);// + gui::lineHeight();
    int nodeWidth = widthText > minWidth ? widthText : minWidth;
    int rv = IWIDTH(nodeWidth);
    return rv;
};

unsigned utils::node_width(topic_ptr p, unsigned minWidth)
{
    return node_width(p->title(), minWidth);
};

unsigned calc_node_width(QString s, unsigned minWidth)
{
    return utils::node_width(s, minWidth);
};

unsigned calc_node_width(topic_ptr t, unsigned minWidth)
{
    return calc_node_width(t->title(), minWidth);
};

void utils::drawDateLabel(topic_ptr it, QPainter *p, const QRectF& rc)
{
    //auto it = proto()->topic();
    QString date_label = it->dateColumnLabel();
    if (!date_label.isEmpty())
    {
        QSize lbl_sz = utils::calcSize(date_label, utils::defaultTinyFont());
        int lbl_w = (lbl_sz.width() + ARD_MARGIN);

        QRectF rc2 = rc;
        rc2.setLeft(rc.right() - lbl_w);
        rc2.setBottom(rc.top() + lbl_sz.height());

        PGUARD(p);
        p->setBackgroundMode(Qt::OpaqueMode);
        p->setFont(*utils::defaultTinyFont());
        p->drawText(rc2, Qt::AlignRight | Qt::AlignTop, date_label);

        rc2.setBottom(rc2.top() + gui::lineHeight());

        QRect rc_ico;
        rc2.setBottom(rc.bottom());
        rectf2rect(rc2, rc_ico);
    }
};

void utils::drawLabels(topic_ptr it, QPainter *painter, const QRectF& rc1)
{
    auto gm = ard::gmail_model();
    assert_return_void(gm, "expected g-model");

    PGUARD(painter);
    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setFont(*utils::defaultTinyFont());

    QRectF rc = rc1;
    int ico_w = gui::lineHeight();
    QRect rc_ico;
    rectf2rect(rc, rc_ico);

    if (it->hasAttachment()) {
        rc_ico.setLeft(rc.right() - ico_w);
        rc_ico.setRight(rc_ico.left() + ico_w);
        rc_ico.setBottom(rc_ico.top() + ico_w);
        QPixmap pm(QString(":ard/images/unix/attach.png"));
        painter->drawPixmap(rc_ico, pm);
        rc.setRight(rc.right() - rc_ico.width());
    }

    rc.setTop(rc.bottom() - utils::outlineTinyHeight());
    auto thr = dynamic_cast<ard::ethread*>(it);
    if (thr)
    {
        auto p = thr->parent();
        if (p)
        {
            auto s = p->title();
            unsigned w = utils::calcWidth(s, utils::defaultTinyFont());
            QRectF rc_hotspot = rc;
            rc_hotspot.setLeft(rc_hotspot.right() - (w + ARD_MARGIN));
            rc_hotspot.setTop(rc_hotspot.top() + 1);
            rc_hotspot.setBottom(rc_hotspot.bottom() - 1);

            if (rc_hotspot.left() < rc.left())
                rc_hotspot.setLeft(rc.left());
            QPainterPath r_path;
            r_path.addRoundedRect(rc_hotspot, 5, 5);
            painter->setPen(model()->penGray());
            auto bkclr = ard::cidx2color(p->colorIndex());

            QBrush brush(bkclr);

            painter->setBrush(brush);
            painter->drawPath(r_path);
            painter->setBrush(Qt::NoBrush);
            painter->setPen(Qt::NoPen);

            QPen penText(color::invert(bkclr));
            painter->setPen(penText);
            painter->drawText(rc_hotspot, Qt::AlignLeft, s);
            painter->setPen(Qt::NoPen);

            rc.setRight(rc.right() - (w + ARD_MARGIN));
        }
    }

    if (dbp::configFileGetDrawDebugHint())
    {
        drawGmailLabels(it, painter, rc);
    }
    
#undef DRAW_ALL_LABEL_NAMES
};

void utils::drawGmailLabels(topic_ptr it, QPainter *painter, const QRectF& rc1) 
{
    QRect rc_ico;
    QRectF rc = rc1;
    rectf2rect(rc, rc_ico);
    int ico_w = gui::lineHeight() / 2;
    rc_ico.setBottom(rc_ico.top() + ico_w);

    auto labels = it->getLabels();
    for (auto& lbl : labels)
    {
        if (lbl->isStarred() ||
            lbl->isUnread() ||
            lbl->isImportant() ||
            lbl->isPromotionCategory() ||
            lbl->isSocialCategory() ||
            lbl->isUpdatesCategory() ||
            lbl->isForumsCategory()
            )
        {
            if (!dbp::configFileGetDrawDebugHint()) {
                continue;
            }
        }

#define CONTINUE_AFTER_DRAW_LABEL_ICON(I) rc_ico.setLeft(rc.right() - ico_w); \
            rc_ico.setRight(rc_ico.left() + ico_w);                     \
            QPixmap pm(QString(":ard/images/unix/%1.png").arg(I));      \
            painter->drawPixmap(rc_ico, pm);                            \
            rc.setRight(rc.right() - rc_ico.width());                   \
            if (!dbp::configFileGetDrawDebugHint())continue;            \


        if (lbl->isSpam()) {
            CONTINUE_AFTER_DRAW_LABEL_ICON("spam");
        }
        else if (lbl->isTrash()) {
            CONTINUE_AFTER_DRAW_LABEL_ICON("trash-bin");
        }
        else if (lbl->isDraft()) {
            CONTINUE_AFTER_DRAW_LABEL_ICON("draft");
        }


        bool bDrawLabel = !lbl->isSystem();
        if (dbp::configFileGetDrawDebugHint()) {
            bDrawLabel = true;
        }

#undef CONTINUE_AFTER_DRAW_LABEL_ICON
        if (bDrawLabel) {
            QString s = lbl->labelName();
            unsigned w = utils::calcWidth(s, utils::defaultTinyFont());
            QRectF rc_hotspot = rc;
            rc_hotspot.setLeft(rc_hotspot.right() - (w + ARD_MARGIN));
            rc_hotspot.setTop(rc_hotspot.top() + 1);
            rc_hotspot.setBottom(rc_hotspot.bottom() - 1);

            if (rc_hotspot.left() < rc.left())
                rc_hotspot.setLeft(rc.left());
            QPainterPath r_path;
            r_path.addRoundedRect(rc_hotspot, 5, 5);
            painter->setPen(model()->penGray());
            auto bkclr = ard::cidx2color(it->colorIndex());

            QBrush brush(bkclr);

            painter->setBrush(brush);
            painter->drawPath(r_path);
            painter->setBrush(Qt::NoBrush);
            painter->setPen(Qt::NoPen);

            QPen penText(color::invert(bkclr));
            painter->setPen(penText);
            painter->drawText(rc_hotspot, Qt::AlignLeft, s);
            painter->setPen(Qt::NoPen);

            //...            
            rc.setRight(rc.right() - (w + ARD_MARGIN));
        }
        if (rc.left() > rc.right() - 10)
            break;
    }
};

QSize calc_text_svg_node_size(const QSizeF& szOriginalImg, QString s)
{
    int w = utils::node_width(s, (int)szOriginalImg.width());
    int h = (int)(w * szOriginalImg.height() / szOriginalImg.width());
    QSize szNode(w, h);
    return szNode;
};

void svg2pixmap(QPixmap* pm, QString svg_resource, const QSize& sz)
{
    Q_UNUSED(pm);
    Q_UNUSED(svg_resource);
    Q_UNUSED(sz);
    /*
    *pm = QPixmap(sz);
    pm->fill(Qt::transparent);
    QSvgRenderer renderer(QString(":ard/images/unix/%1.svg").arg(svg_resource));
    QPainter painter(pm);
    renderer.render(&painter, pm->rect());
    */
};

int guessOutlineHeight(ProtoGItem* git, int textlWidth, OutlineContext c)
{
    ProtoPanel* p = git->p();
    if(!p)
        {
            ASSERT(0, "expected valid panel");
            return 0;
        }

    if(!p->hasProp(ProtoPanel::PP_RTF))
        {
            int rv = (int)gui::lineHeight();
            return rv;
        }

    topic_ptr it = git->topic();

    int w = textlWidth;
    bool hasMainIcon = !it->getIcon(c).isNull();
    if(hasMainIcon)
        w -= (int)ICON_WIDTH;

    int oneLineH = (int)gui::lineHeight();
    
    if (it->hasColorByHashIndex() && hasMainIcon) {
        /// it's a hard case, not actually polished
        auto ch = it->getColorHashIndex();
        if (ch.second != 0) {
            oneLineH = oneLineH + oneLineH;
        }
    }

    int rv = oneLineH;

    if(it->thumbnail())
        {
            rv = MP1_HEIGHT;
        }
    else
        {
            if(p->hasProp(ProtoPanel::PP_MultiLine))
                {
                    static int MAX_SENTENCE_H = 2 * MP1_HEIGHT;

                    QFontMetrics fm(*_defaultFont);
                    QRect rc2(0,0,w, MP1_HEIGHT);
                    QRect rc3 = fm.boundingRect(rc2, Qt::TextWordWrap, it->title());
                    rv = rc3.height();
                    if(rv < oneLineH)
                        rv = oneLineH;
                    if(rv > gui::lineHeight())
                        rv += 2;//I don't know why..
                    if(rv > MAX_SENTENCE_H)
                        rv = MAX_SENTENCE_H;
                }
        }
    return rv;
};

int guessOutlineAnnotationHeight(QString annotation, int textWidth)
{
    if (annotation.isEmpty()) {
        return 0;
    }
    QRect rc2(0, 0, textWidth, MP1_HEIGHT);
    QFontMetrics fm(*_groupHeaderFont);
    QRect rc3 = fm.boundingRect(rc2, Qt::TextWordWrap, annotation);
    if (rc3.height() > 300) {
        rc3.setHeight(300);
    }
    return rc3.height();
};

int guessOutlineExpandedNoteHeight(QString annotation, int textWidth)
{
    if (annotation.isEmpty()) {
        return 0;
    }
    QRect rc2(0, 0, textWidth, MP1_HEIGHT);
    QFontMetrics fm(*_defaultSmallFont);
    QRect rc3 = fm.boundingRect(rc2, Qt::TextWordWrap, annotation);
    return rc3.height();
};


/**
   DnDHelper
*/
bool DnDHelper::canStartDnD(QGraphicsSceneMouseEvent * e)
{
    bool rv = e->buttons() & Qt::LeftButton &&
        QLineF(e->screenPos(), e->buttonDownScreenPos(Qt::LeftButton)).length() > QApplication::startDragDistance();
    return rv;
};

void DnDHelper::process_dragEnterEvent(QGraphicsSceneDragDropEvent *e)
{   
    m_dragOver = calcDragOverType(e);
    if (m_dragOver != dragUnknown)
    {
        e->setAccepted(true);
        updateGuiAfterDnd();
    }
    else
    {
        e->setAccepted(false);
    }
};

void DnDHelper::process_dragLeaveEvent(QGraphicsSceneDragDropEvent *)
{
    m_dragOver = dragUnknown;
    updateGuiAfterDnd();
};

void DnDHelper::drawDnDMark(QPainter * p)
{
    if (m_dragOver != dragUnknown)
    {
        PGUARD(p);
        qreal x1 = dndRect().left();
        qreal x2 = x1 + 2 * ICON_WIDTH;
        qreal y = 0;

        {
            switch (m_dragOver)
            {
            case dragBelow: y = dndRect().bottom() - 1; break;
            case dragAbove: y = dndRect().top(); break;
            case dragInside:y = (dndRect().top() + dndRect().bottom()) / 2.0; break;
            default:break;
            }
            QPen pen(QColor(color::invert(p->background().color().rgb())), 3);
            p->setPen(pen);
            p->drawLine((int)x1, (int)y, (int)x2, (int)y);
        }


        if (m_dragOver == dragInside)
        {
            qreal y1 = y - 5;
            qreal y2 = y + 5;
            p->drawLine((int)x1, (int)y1, (int)x1, (int)y2);
            p->drawLine((int)x2, (int)y1, (int)x2, (int)y2);
        }
    }
};

/**
   TextFilter
*/
static TextFilter __textFilter;

TextFilter& globalTextFilter()
{
    return __textFilter;
};

TextFilter::TextFilter()
{

};

bool TextFilter::isActive()const
{
    return !m_fc.key_str.isEmpty();
};

void TextFilter::prepareFormatRangeList(const QFont& pf, QString s, int idx, QList<QTextLayout::FormatRange>& fr)
{
    QFont fnt(pf);
    //fnt.setBold(true);

    QTextCharFormat f;
    f.setFont(fnt);
    f.setForeground(Qt::red);

    int len = m_fc.key_str.length();
    while(idx != -1)
        {
            QTextLayout::FormatRange fr_tracker;
            fr_tracker.start = idx;
            fr_tracker.length = len;
            fr_tracker.format = f;
            fr.push_back(fr_tracker);

            idx = s.indexOf(m_fc.key_str, idx + len, Qt::CaseInsensitive);
        } 
};

void TextFilter::prepareOutlineLabelsFormatRangeList(const QFont& font, int labelLen, QList<QTextLayout::FormatRange>& fr)
{
    QFont fnt(font);
    fnt.setBold(true);

    QTextCharFormat f;
    f.setFont(fnt);
    // f.setForeground(Qt::green);
  
    QTextLayout::FormatRange fr_tracker;
    fr_tracker.start = 0;
    fr_tracker.length = labelLen;
    fr_tracker.format = f;
    fr.push_back(fr_tracker);
};

QRectF TextFilter::drawTextLine(QPainter *p, const QRectF& rc, QString s)
{
    QRectF rv;
    s.replace(QLatin1Char('\n'), QChar::LineSeparator);
    if(isActive())
        {
            int idx = s.indexOf(m_fc.key_str, 0, Qt::CaseInsensitive);
            if(idx == -1)
                {
                    rv = drawDefaultTextLine(p, rc, s);
                }
            else
                {
                    QList<QTextLayout::FormatRange> fr;
                    prepareFormatRangeList(p->font(), s, idx, fr);
                    rv = drawDefaultTextLine(p, rc, s, &fr);
                }
        }
    else
        {
            rv = drawDefaultTextLine(p, rc, s);
        }

    return rv;
};

QRectF TextFilter::drawDefaultTextLine(QPainter *p, const QRectF& rc, QString s, QList<QTextLayout::FormatRange>* fl)
{
    QFontMetrics fm(p->font());

    QTextLayout l(s, p->font());
    QTextOption o = l.textOption();
    o.setAlignment(Qt::AlignLeft);
    l.setTextOption(o);
    if(fl)
        {
            l.setAdditionalFormats(*fl);
        }
    l.beginLayout();
    QTextLine line = l.createLine();
    line.setLineWidth( rc.width() );
    line.setPosition( QPointF( 0, 0 ) );
    l.endLayout();
    QRectF rv = l.boundingRect();
    QPainterPath pp;
    pp.addRect(rc);
    PGUARD(p);
    p->setClipPath(pp, Qt::IntersectClip);
    l.draw( p, QPointF(rc.topLeft()));
    return rv;
};

QSize TextFilter::drawTextMLine(QPainter *painter, 
    const QFont& font, 
    const QRectF& rc, 
    QString s1, 
    bool alignCenter, 
    const QRectF* rc_thumb /*= nullptr */, 
    QList<QTextLayout::FormatRange>* custom_format /*= nullptr*/)
{
/*    bool hasImage = (img && rc_thumb);

    if (painter && hasImage)
        {
            QRect rc2;
            rectf2rect(*rc_thumb, rc2);         
            utils::drawThumbnail(img, painter, rc2);
        }
        */
    QSize rv(0,0);    
    int keyStartSearch = 0;

    QString s = s1;
    s.replace(QLatin1Char('\n'), QChar::LineSeparator);

    if(isActive())
        {
            int idx = s.indexOf(m_fc.key_str, keyStartSearch, Qt::CaseInsensitive);
            if(idx == -1)
                {
                    rv = drawDefaultTextMLine(painter, font, rc, s, alignCenter, custom_format, rc_thumb);
                }
            else
                {
                    QList<QTextLayout::FormatRange> fr;
                    if (custom_format) {
                        fr = *custom_format;
                    }
                    prepareFormatRangeList(font, s, idx, fr);
                    rv = drawDefaultTextMLine(painter, font, rc, s, alignCenter, &fr, rc_thumb);
                }      
        }
    else
        {
            rv = drawDefaultTextMLine(painter, font, rc, s, alignCenter, custom_format, rc_thumb);
        }
/*
    if (hasImage && rv.height() < MP1_HEIGHT)
        {
            rv.setHeight(MP1_HEIGHT);
        }
        */

    return rv;
};

QSize TextFilter::drawDefaultTextMLine(QPainter *painter, const QFont& font, const QRectF& rc, QString s, bool alignCenter, QList<QTextLayout::FormatRange>* fl, const QRectF* rc_clip)
{
    if (rc.height() > 10000 || rc.width() > 10000) {
        ASSERT(0, "invalid size provided");
        return QSize(1,1);
    }

    QFontMetrics fm(font);
    QRect rc_calc = fm.boundingRect("H");
    int line_h = rc_calc.height();
    int total_h = (int)rc.height();
    int total_w = (int)rc.width();
    if (total_h < 1 || total_w < 1) {
        return QSize(1, 1);
    }


    QTextLayout l(s, font);
    if(fl && fl->size() > 0)
        {
            l.setAdditionalFormats(*fl);
        }
    QTextOption opt;
    if(alignCenter)
        {
            opt.setAlignment(Qt::AlignCenter);
        }
    else
        {
            opt.setAlignment(Qt::AlignLeft);
        }
    opt.setFlags(0);
    opt.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
    l.setTextOption(opt);
    l.beginLayout();
    int ypos = 0;
    bool useClip = (rc_clip);
    int thumb_width = 0;
    if(useClip)
        {
            thumb_width = (int)rc_clip->width();
        }

    int bottom = total_h - line_h;
    while(ypos <= bottom)
        {
            QTextLine line = l.createLine();
            if(line.isValid())
                {
                    int line_width = total_w;
                    if(useClip)line_width -= thumb_width;
                    line.setLineWidth( line_width );
                    line.setPosition( QPointF( useClip ? thumb_width : 0, ypos ) );
                }
            ypos += line_h;
            if(useClip && ypos > rc_clip->bottom())
                {
                    useClip = false;
                }
        }

    l.endLayout();
    int calc_h = (int)l.boundingRect().height();
    int calc_w = (int)l.boundingRect().width();
    qreal t = rc.topLeft().y();
    if(alignCenter)
        {
            if(calc_h < total_h)
                {
                    t = t + (total_h - calc_h) / 2;
                }
        }
    if(painter)
        {
            l.draw( painter, QPointF(rc.topLeft().x(), t));
        }

    return QSize(calc_w, calc_h);
};


bool TextFilter::filterIn(topic_cptr f)const 
{
    return f->hasText4SearchFilter(m_fc);
};

void TextFilter::setSearchContext(const TextFilterContext& fc)
{
    m_fc= fc;
    ard::setHoistedInFilter(nullptr);
    ard::ContactsRoot::setHoistedContactInFilter(nullptr);    
};

QSize ard::calcSize(QString s, QFont* font)
{
    return utils::calcSize(s, font);
};

QFont* ard::defaultFont()
{
    return _defaultFont;//ard::defaultFont();
};

QFont* ard::defaultOutlineLabelFont() 
{
    return _defaultLabelFont;// utils::defaultOutlineLabelFont();
};

QFont* ard::defaultNoteFont() 
{
    return utils::defaultNoteFont();
};

QFont* ard::getTextlikeFont(ard::BoardItemShape sh) 
{
    QFont* rv = ard::defaultFont();
    switch (sh) {
    case ard::BoardItemShape::text_normal:
        rv = _boardNormalTextFont;
        break;
    case ard::BoardItemShape::text_italic:
        rv = _boardNormalItalicFont;
        break;
    case ard::BoardItemShape::text_bold:
        rv = _boardNormalBoldFont;
        break;
    default:break;
    }
    return rv;
};

void ard::resetAllFonts(int defaultFontSize) 
{
    if (defaultFontSize >= MIN_CUST_FONT_SIZE && defaultFontSize <= MAX_CUST_FONT_SIZE)
    {
        dbp::configFileSetCustomFontSize(defaultFontSize);
        utils::resetFonts();
        main_wnd()->resetGui();
        gui::rebuildOutline(outline_policy_Uknown, true);
    }
};

void ard::resetNotesFonts()
{
    _defaultNoteFont = new QFont(ARD_FALLBACK_DEFAULT_NOTE_FONT_FAMILY, ARD_FALLBACK_DEFAULT_NOTE_FONT_SIZE);
    auto NoteFontFamily = dbp::configFileNoteFontFamily();
    if (!NoteFontFamily.isEmpty()) {
        _defaultNoteFont->setFamily(NoteFontFamily);
    }
    auto NoteFontSize = dbp::configFileNoteFontSize();
    if (NoteFontSize >= 8 && NoteFontSize < 48) {
        _defaultNoteFont->setPointSize(NoteFontSize);
    }
};

QFont* ard::defaultBoldFont()
{
    return utils::defaultBoldFont();
};

QFont* ard::defaultSmallFont() 
{
    return _defaultSmallFont;
};

QFont* ard::defaultSmall2Font()
{
    return utils::defaultSmall2Font();
};

QFont* ard::graphNodeHeaderFont()
{
    return utils::graphNodeHeaderFont();
};

bool gui::searchFilterIsActive()
{
    return globalTextFilter().isActive();
};

QString gui::searchFilterText()
{
    return globalTextFilter().fcontext().key_str;
};


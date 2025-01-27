#include <QCryptographicHash>
#include <QTime>
#include <QFile>
#include <QSettings>
#include <QSize>
#include <QApplication>
#include <QDesktopWidget>
#include <QScreen>
#include <math.h>
#include <QDir>

#include "a-db-utils.h"
#include "GoogleClient.h"
#include "Endpoint.h"
#include "dbp.h"
#include "ard-db.h"
#include "anfolder.h"
#include "ansyncdb.h"
#include "email.h"
#include "db-merge.h"
#include "rule.h"
#include "ard-db.h"
#include "board.h"
#include "locus_folder.h"

extern QString getUserPassword(QString hint, bool& cancelled, bool& RO_Request, QString title = "");

/**
   config variables
*/
static bool              _is_big_screen = true;

static bool              _configFileSuppliedDemoDBUsed = false;
static DB_ID_TYPE        _configLastDestHoistedOID = 0;
static DB_ID_TYPE        _configLastDestGenericTopicOID = 0;
static DB_ID_TYPE        _configLastHoistedOID = 0;
static DB_ID_TYPE        _configLastSelectedKRingKeyOID = 0;
static DB_ID_TYPE        _configSupportCmdLevel = 0;
static QString           _configLastDBName = "";
static bool             _configDrawDebugHint = false;
static bool             _configMailBoardUnreadFilterON = false;
static bool				_configFileFilterInbox = true;
static bool				_configPreFileFilterInbox = true;
static bool				_configMaiBoardSchedule = true;
static QString           _configLastSearchFilter = "";
static QString           _configLastSearchStr = "";
static int               _configCustomFontSize = 0;
static int               _configNoteFontSize = ARD_FALLBACK_DEFAULT_NOTE_FONT_SIZE;
static QString           _configNoteFontFamily = "Times";
static QString           _configEmailUserId = "";
static QString           _configFallbackEmailUserId = "";
static bool              _configFollowDestination = false;
static bool              _configRunInSysTray = false;
static bool              _configFileGoogleEmailListCheckSelectColumn = false;
static std::pair<QString, QString> _configFileCurrDbgMsgId = { "", "" };
static QString           _configFileContactIndexStr = "";
static QString           _configFileLastShellAccessDir = "";
static DB_ID_TYPE       _configFileContactHoistedGroupId = 0;
static DB_ID_TYPE       _configFileELabel = ard::static_rule::default_qtype();
static QString          _popup_ids;
#ifdef ARD_CHROME
static qreal                    _configFileChromeZoom = 1.0;
#else
int                     _configFileTexZoom = 1;
#endif
static EOutlinePolicy _configFileBornAgainPolicy = outline_policy_Pad;
static std::set<ard::EColor>  _configColorGrepInFilter;


using DB_NAMES = std::vector<QString>;
using ACCOUNT2NAMES = std::map<QString, DB_NAMES>;
ACCOUNT2NAMES _account2names;

#define MAX_GMAIL_ACCOUNTS 5

QString configFilePath()
{
  QString rv = defaultRepositoryPath();
  rv += "ardi.ini";
  return rv;
}

QString account2DB_FilePath()
{
    QString rv = defaultRepositoryPath();
    rv += "account2db.ini";
    return rv;
}

QString gmailCachePath()
{
    QString rv = defaultRepositoryPath();
    rv += "gmail-cache/";
    return rv;
}

QString gmailDBFilePath()
{
    QString rv = gmailCachePath();
    rv += "gmail.sqlite";
    return rv;
}

bool is_big_screen()
{
  return _is_big_screen;
};

//..
static void load_account2DB() 
{
    _account2names.clear();

    QFile file(account2DB_FilePath());
    if (file.exists()) {
        if (!file.open(QIODevice::ReadOnly)) {
            ASSERT(0, "failed to open account2DB file") << account2DB_FilePath() << file.errorString();
            return;
        }

        QTextStream in(&file);
        while (!in.atEnd()) {
            QString line = in.readLine().trimmed();
            auto fields = line.split(";");
            if (fields.size() == 3) {
                auto s_type = fields[0];
                auto account = fields[1].trimmed();
                auto db_name = fields[2].trimmed();

                auto i = _account2names.find(account);
                if (i == _account2names.end()) {
                    DB_NAMES names;
                    names.push_back(db_name);
                    _account2names[account] = names;
                }
                else {
                    i->second.push_back(db_name);
                }
            }
        }
        file.close();
    }
}

bool dbp::isDBSyncInCurrentAccountEnabled()
{
    QString account = dbp::configEmailUserId();
    assert_return_false(!account.isEmpty(), "Expected gmail account");
    assert_return_false(ard::isDbConnected(), "expected open DB");
    QString db_name = gui::currentDBName();
    qDebug() << "isDBSyncInCurrentAccountEnabled" << account << db_name;

    auto i = _account2names.find(account);
    if (i != _account2names.end()) {
        for (auto& j : i->second) {
            if (j.compare(db_name, Qt::CaseInsensitive) == 0) {
                return true;
            }
        }
    }

    return false;
};

void dbp::enableBSyncInCurrentAccount()
{
    QString account = dbp::configEmailUserId();
    assert_return_void(!account.isEmpty(), "Expected gmail account");
    assert_return_void(ard::isDbConnected(), "expected open DB");
    QString db_name = gui::currentDBName();
    qDebug() << "enableBSyncInCurrentAccount" << account << db_name;

    if (!dbp::isDBSyncInCurrentAccountEnabled()) {
        auto i = _account2names.find(account);
        if (i == _account2names.end()) {
            DB_NAMES names;
            names.push_back(db_name);
            _account2names[account] = names;
        }
        else {
            i->second.push_back(db_name);
        }

        QFile file(account2DB_FilePath());
        if (!file.open(QIODevice::WriteOnly|QIODevice::Truncate)) {
            ASSERT(0, "failed to open account2DB file") << account2DB_FilePath() << file.errorString();
            return;
        }

        QTextStream tout(&file);
        for (const auto& i : _account2names) {
            QString account = i.first;
            for (const auto& j : i.second) {
                QString db_name = j;
                QString storage = "gd";
                tout << storage << ";" << account << ";" << db_name << endl;
            }
        }

        file.close();
    }
};

QSize availableSize;
qreal ard_screen_inches = 1;
void dbp::loadFileSettings()
{
    load_account2DB();

    QScreen* srn = QGuiApplication::primaryScreen();
    qreal w = srn->physicalSize().width();
    qreal h = srn->physicalSize().height();
    ard_screen_inches = sqrt(w * w + h * h) / 25.4;
    availableSize = srn->availableSize();
    if (availableSize.width() == 0)availableSize.setWidth(100);
    if (availableSize.height() == 0)availableSize.setHeight(100);

#ifdef ARD_BIG
    _is_big_screen = true;
#else
    _is_big_screen = (ard_screen_inches > 6);
#endif


    int tmp = 0;
    QString sfile = configFilePath();
    QSettings settings(sfile, QSettings::IniFormat);

	std::function<bool(QString, bool)> readBool = [&](QString name, bool defaultVal) 
	{
		bool rv = false;
		auto tmp = settings.value(name, defaultVal ? "1" : "0").toInt();
		if (tmp > 0)rv = true;
		return rv;
	};

    tmp = settings.value("supplied-used", "0").toInt();
    if (tmp > 0) {
        _configFileSuppliedDemoDBUsed = true;
    }

    //_configLastPopup = settings.value("last-popup").toRect();

    //_configColorTheme = settings.value("color-theme", "color-theme-default").toString();

    _configLastDBName = settings.value("last-db-name", DEFAULT_DB_NAME).toString();
    _configSupportCmdLevel = settings.value("support-level", "0").toInt();

    _configCustomFontSize = settings.value("custom-font-size", "0").toInt();
    if (_configCustomFontSize < MIN_CUST_FONT_SIZE || _configCustomFontSize > MAX_CUST_FONT_SIZE)
    {
        _configCustomFontSize = 0;
    }

    _configNoteFontSize = settings.value("note-font-size", QString("%1").arg(ARD_FALLBACK_DEFAULT_NOTE_FONT_SIZE)).toInt();
    if (_configNoteFontSize < 8 || _configNoteFontSize > 48)
    {
        _configNoteFontSize = ARD_FALLBACK_DEFAULT_NOTE_FONT_SIZE;
    }

    _configNoteFontFamily = settings.value("note-font-family", ARD_FALLBACK_DEFAULT_NOTE_FONT_FAMILY).toString();

    _configEmailUserId = settings.value("email-userid", "").toString();
	_configFallbackEmailUserId = settings.value("fallback-email-userid", "").toString();
	if (_configEmailUserId.isEmpty() && !_configFallbackEmailUserId.isEmpty()) {
		_configEmailUserId = _configFallbackEmailUserId;
		ard::trail(QString("loadsettings-empty-userid-init-fallback [%1]").arg(_configFallbackEmailUserId));
	}
#ifdef API_QT_AUTOTEST
    if (_configEmailUserId.isEmpty()) {
        _configEmailUserId = "me@gmail.com";
    }
#endif //API_QT_AUTOTEST

#ifdef ARD_OPENSSL
    if (!_configEmailUserId.isEmpty()) {
        ard::CryptoConfig::cfg().reloadCryptoConfig();
    }
#endif

    _configFileLastShellAccessDir = settings.value("shell-dir", "").toString();

    _configFollowDestination = (settings.value("follow-dest", "0").toInt() > 0);
    _configFileGoogleEmailListCheckSelectColumn = (settings.value("email-list-check-sel", "0").toInt() > 0);

    _configRunInSysTray = (settings.value("systray", "0").toInt() > 0);

#ifdef ARD_CHROME
    _configFileChromeZoom = settings.value("chrome-browser-zoom", "1").toReal();
#else
    _configFileTexZoom = settings.value("text-browser-zoom", "1").toReal();
#endif

	_configDrawDebugHint		= readBool("debug-hint", false);
	_configFileFilterInbox		= readBool("filter-inbox", true);
	_configPreFileFilterInbox	= readBool("prefilter-inbox", true);
	_configMaiBoardSchedule		= readBool("mail-board-schedule", true);
	

    tmp = settings.value("main-policy", "1").toInt();
    switch (tmp) {
    case outline_policy_Pad:
    case outline_policy_PadEmail:
	case outline_policy_BoardSelector:
    {
        _configFileBornAgainPolicy = (EOutlinePolicy)tmp;
    }break;
    default:_configFileBornAgainPolicy = outline_policy_Pad;
    }

    _configFileCurrDbgMsgId.first = settings.value("dbg-gm-t-id", "").toString();
    _configFileCurrDbgMsgId.second = settings.value("dbg-gm-msg-id", "").toString();

    _configLastSearchFilter = settings.value("last-search-filter", "").toString();
    _configLastSearchStr = settings.value("last-search-str", "").toString();
	 
    /*int val = settings.value("group-type", "2").toInt();
    if (val < dbp::TypeNotes || val > dbp::TypeToDos)
    {
        val = dbp::TypeNotes;
    }*/
//    _configGroupType = (dbp::ETypeSummary)val;	
    _configColorGrepInFilter.insert({ ard::EColor::purple, ard::EColor::red, ard::EColor::blue, ard::EColor::green });
};

void dbp::loadDBSettings(ArdDB& db)
{
#define IF_INT(N, V) else if(n.compare(N, Qt::CaseInsensitive) == 0){V = q->value(1).toInt();}

    _popup_ids = "";
    _configLastDestHoistedOID = 0;
    _configLastHoistedOID = 0;
    _configLastDestGenericTopicOID = 0;
    _configLastSelectedKRingKeyOID = 0;

    QString sql = "SELECT name, value FROM ard_config";

    auto q = db.selectQuery(sql);
    assert_return_void(q, "expected query");
    QString s;
    while (q->next())
    {
        QString n = q->value(0).toString();

        if (n.compare("popup-ids", Qt::CaseInsensitive) == 0) {
            _popup_ids = q->value(1).toString();
        }
        //IF_INT("outline-root-label", _configRootOutlineLabel)
            IF_INT("dest-hoisted", _configLastDestHoistedOID)
            IF_INT("hoisted", _configLastHoistedOID)
            IF_INT("dest-topic", _configLastDestGenericTopicOID)
            //IF_INT("contact-id", _configLastSelectedContactOID)
            IF_INT("k-id", _configLastSelectedKRingKeyOID)
            //IF_INT("contact-g-id", _configLastSelectedContactGroupOID)
    }
#undef IF_INT
};

template <class T>
static void writeFileConfigRecord(QString name, T v)
{
  QString sfile = configFilePath();
  QSettings settings(sfile, QSettings::IniFormat);
  settings.setValue(name, v);
}

//@todo - there should be only one call for db for all those getConfigXX functions
int getConfigIntValue(ArdDB* db, QString configName, int defaultValue)
{
  int rv = defaultValue;
  int _cid = db->queryInt(QString("SELECT config_id from ard_config WHERE name = '%1'").arg(configName));
  if(_cid != 0)
    { 
      rv = db->queryInt(QString("SELECT value FROM ard_config WHERE name='%1'").arg(configName));
    }
  return rv;
}

QString getConfigStringValue(ArdDB* db, QString configName, QString defaultValue)
{
  QString rv = defaultValue;
  int _cid = db->queryInt(QString("SELECT config_id from ard_config WHERE name = '%1'").arg(configName));
  if(_cid != 0)
    { 
      rv = db->queryString(QString("SELECT value FROM ard_config WHERE name='%1'").arg(configName));
    }
  return rv;
}


template<class T>
void setConfigValue(ArdDB* db, QString configName, T Value)
{
  QString _sql;
  int _cid = db->queryInt(QString("SELECT config_id from ard_config WHERE name = '%1'").arg(configName));
  if(_cid != 0)
    { 
      _sql = QString("UPDATE ard_config SET value = '%1' WHERE config_id=%2").arg(Value).arg(_cid);
    }
  else
    {
      _sql = QString("INSERT INTO ard_config(name, value) VALUES('%1', '%2')").arg(configName).arg(Value);
    }
  db->execQuery(_sql);
}


bool dbp::configFileIsSyncEnabled() 
{
#ifdef ARD_OPENSSL
    auto& cfg = ard::CryptoConfig::cfg();
    bool rv = (cfg.hasPassword() || cfg.hasPasswordChangeRequest());
    return rv;
#else
    return false;
#endif
};


bool dbp::configFileSuppliedDemoDBUsed() 
{
    return _configFileSuppliedDemoDBUsed;
};

void dbp::configFileSetSuppliedDemoDBUsed(bool val) 
{
    _configFileSuppliedDemoDBUsed = val;
    writeFileConfigRecord("supplied-used", _configFileSuppliedDemoDBUsed ? 1 : 0);
};

static void importSupplied(QString supplied_path)
{
	if (QFile::exists(supplied_path))
	{
		auto r = ard::root();
		if (r)
		{
			qWarning() << "importing supplied.." << supplied_path;

			auto res = ArdiDbMerger::mergeJSONfile(supplied_path);
			auto str = QString("merge supplied DB +topics=%1 +contacts=%2 skip-topics=%3 skip-contacts=%4")
				.arg(res.merged_topics)
				.arg(res.merged_contacts)
				.arg(res.skipped_topics)
				.arg(res.skipped_contacts);

			qWarning() << str;
			r->ensurePersistant(-1);

			/// create demo board here ///
			auto broot = ard::db()->boards_model()->boards_root();
			auto b = broot->addBoard();
			if (b)
			{
				b->setTitle("Demo board");
				auto h = b->ensureOutlineTopicsHolder();
				if (h)
				{
					std::function<void(OutlineSample, ard::InsertBranchType it, int band_idx, ard::BoardItemShape)> add_outline =
						[&](OutlineSample smp, ard::InsertBranchType it, int band_idx, ard::BoardItemShape shp)
					{
						auto f = ard::buildOutlineSample(smp, h);
						f->ensurePersistant(-1);
						TOPICS_LIST lst;
						lst.push_back(f);
						b->insertTopicsWithBranches(it, lst, band_idx, -1, 300, shp);
					};


					add_outline(OutlineSample::GreekAlphabet, ard::InsertBranchType::branch_expanded_from_center, 1, ard::BoardItemShape::box);
				}
			}
			/// end demo board code ///			

			ard::messageBox(gui::mainWnd(), "Ardi imported demo outline and blackboard diagram. Feel free to delete provided topics any time.");
		}
	}
	else
	{
		ard::errorBox(ard::mainWnd(), QString("Supplied file not found %1").arg(supplied_path));
	}
};

void importSuppliedDemo() 
{
	auto db = ard::db();
	if (db && db->isOpen() && db->isMainDb())
	{
		auto r = ard::root();
		if (r)
		{
			auto supplied_path = get_tmp_import_dir_path() + "supplied.json";;
			QUrl url("https://www.prokarpaty.net/ard_download/supplied.json");
			std::map<QString, QString> rawHeaders;
			rawHeaders["Content-Type"] = "application/x-www-form-urlencoded";
			rawHeaders["User-Agent"] = "Mozilla/Firefox 3.6.12";

			ard::downloadHttpData(url, supplied_path, [supplied_path](QNetworkReply* r)
			{
				if (r->isFinished() && r->error() == QNetworkReply::NoError)
				{
					importSupplied(supplied_path);
					dbp::configFileSetSuppliedDemoDBUsed(true);
					gui::rebuildOutline(outline_policy_Pad, true);
				}
			}, &rawHeaders);
		}
	}
}

void dbp::checkOnSuppliedDemoImport()
{
    if (!dbp::configFileSuppliedDemoDBUsed()) 
    {
		importSuppliedDemo();
    }
};

QString dbp::configEmailUserId()
{
    return _configEmailUserId;
};

QString dbp::configFallbackEmailUserId() 
{
	return _configFallbackEmailUserId;
};

void dbp::configSetEmailUserId(QString email)
{
    if (_configEmailUserId != email) 
    {        
		if (!_configEmailUserId.isEmpty() && _configFallbackEmailUserId != _configEmailUserId)
		{
			_configFallbackEmailUserId = _configEmailUserId;
			writeFileConfigRecord("fallback-email-userid", _configFallbackEmailUserId);
			ard::trail(QString("save-fallback-userid [%1][%2]").arg(_configFallbackEmailUserId).arg(email));
		}

        _configEmailUserId = email;
        writeFileConfigRecord("email-userid", email);
#ifdef ARD_OPENSSL
        ard::CryptoConfig::cfg().clearConfig();
        if (!_configEmailUserId.isEmpty()) {
            ard::CryptoConfig::cfg().reloadCryptoConfig();
        }
#endif
        ard::asyncExec(AR_OnChangedGmailUser);
    }
};

bool dbp::configSwitchToFallbackEmailUserId() 
{
	auto old_userid = dbp::configFallbackEmailUserId();
	if (!old_userid.isEmpty() && old_userid != _configEmailUserId)
	{
		auto token_file = dbp::configEmailUserTokenFilePath(old_userid);
		if (QFile::exists(token_file))
		{
			ard::trail(QString("switched2fallback-userid [%1][%2]").arg(_configFallbackEmailUserId).arg(_configEmailUserId));
			_configEmailUserId = old_userid;
			writeFileConfigRecord("email-userid", _configEmailUserId);
			return true;
		}
	}
	return false;
};

QString aes_account_login() 
{
    return _configEmailUserId;
};

QString dbp::configEmailUserTokenFilePath(QString userId)
{
    QString s2 = QString(QCryptographicHash::hash((userId.toUtf8()), QCryptographicHash::Md5).toHex());
    QString tokenFileName = QString("token-") + gui::makeValidFileName(userId) + "-" + s2 + ".info";
    QString rv = gmailCachePath() + tokenFileName;
    return rv;
};

QString dbp::configTmpUserTokenFilePath()
{
    QString tokenFileName = QString("tmp-token.info");
    QString rv = defaultRepositoryPath() + tokenFileName;
    return rv;
};

bool dbp::guiCheckNetworkConnection()
{
    if (!gui::isConnectedToNetwork()) {
		ard::messageBox(gui::mainWnd(), "Network connection not detected.");
        return false;
    }

#ifndef API_QT_AUTOTEST
    if (!QSslSocket::supportsSsl()) {
		ard::messageBox(gui::mainWnd(), "SSL support required. GMail module will be disabled. Please reinstall OpenSSL and restart program.");
        return false;
    }
#endif

    return true;
};

EOutlinePolicy dbp::configFileBornAgainPolicy()
{
    return _configFileBornAgainPolicy;
};

void dbp::configFileSetBornAgainPolicy(EOutlinePolicy pol)
{
    _configFileBornAgainPolicy = pol;
    writeFileConfigRecord("main-policy", pol);
};

DB_ID_TYPE dbp::configFileContactHoistedGroupId()
{
    return _configFileContactHoistedGroupId;
};

void dbp::configFileSetContactHoistedGroupId(DB_ID_TYPE groupId)
{
    _configFileContactHoistedGroupId = groupId;
};

DB_ID_TYPE dbp::configFileELabelHoisted()
{
    return _configFileELabel;
};

void dbp::configFileSetELabelHoisted(DB_ID_TYPE lt)
{
    _configFileELabel = lt;
};

QString dbp::configFileLastShellAccessDir() 
{
    QString rv = _configFileLastShellAccessDir;
    if (rv.isEmpty()) {
        rv = QDir::homePath();
    }
    else {
        QDir d(rv);
        if (!d.exists()) {
            QFileInfo fi(rv);
            rv = fi.dir().absolutePath();
            QDir d2(rv);
            if (!d2.exists()) {
                rv = QDir::homePath();
            }
        }
    }

    return rv;
};

void dbp::configFileSetLastShellAccessDir(QString val, bool recoverDirFromFile)
{
    if (!val.isEmpty()) {
        if (recoverDirFromFile) {
            QFileInfo fi(val);
            val = fi.dir().absolutePath();
        }

        _configFileLastShellAccessDir = val;
        writeFileConfigRecord("shell-dir", val);
    }
};


QString dbp::configFileContactHoistedGroupSyId() 
{
    QString rv;
    auto id1 = dbp::configFileContactHoistedGroupId();
    auto g = ard::lookup(id1);
    if (g) {
        rv = g->syid();
    }
    return rv;
};

QString dbp::configFileContactIndexStr() 
{
    return _configFileContactIndexStr;
};

void dbp::configFileSetContactIndexStr(QString str) 
{
    _configFileContactIndexStr = str;
};


#ifdef ARD_CHROME
qreal dbp::zoomChromeBrowser() 
{
    return _configFileChromeZoom;
};
void dbp::setZoomChromeBrowser(qreal val) 
{
    _configFileChromeZoom = val;
    writeFileConfigRecord("chrome-browser-zoom", val);
};
#else
int dbp::zoomTextBrowser() 
{
    return _configFileTexZoom;
};
void dbp::setZoomTextBrowser(int val) 
{
    _configFileTexZoom = val;
    writeFileConfigRecord("text-browser-zoom", val);
};
#endif


const std::pair<QString, QString>& dbp::configFileCurrDbgMsgId()
{
    return _configFileCurrDbgMsgId;
};


void dbp::configFileSetCurrDbgMsgId(const std::pair<QString, QString>& val)
{
    _configFileCurrDbgMsgId = val;
    writeFileConfigRecord("dbg-gm-t-id", val.first);
    writeFileConfigRecord("dbg-gm-msg-id", val.second);
};

DB_ID_TYPE dbp::configLastDestHoistedOID()
{
  return _configLastDestHoistedOID;
};

void dbp::configSetLastDestHoistedOID(DB_ID_TYPE val)
{
  if(val != _configLastDestHoistedOID)
    {
      _configLastDestHoistedOID = val;
      setConfigValue(&dbp::defaultDB(), "dest-hoisted", val);
    }
};

DB_ID_TYPE dbp::configLastHoistedOID() 
{
    return _configLastHoistedOID;
};

void dbp::configSetLastHoistedOID(DB_ID_TYPE val) 
{
    if (val != _configLastHoistedOID) 
    {
        _configLastHoistedOID = val;
        setConfigValue(&dbp::defaultDB(), "hoisted", _configLastHoistedOID);
    }
};

/*
DB_ID_TYPE dbp::configLastSelectedContactOID()
{
    return _configLastSelectedContactOID;
};

void dbp::configSetLastSelectedContactOID(DB_ID_TYPE val)
{
    if (val != _configLastSelectedContactOID)
    {
        _configLastSelectedContactOID = val;
        setConfigValue(&dbp::defaultDB(), "contact-id", val);
    }
};*/

//..
DB_ID_TYPE dbp::configLastSelectedKRingKeyOID()
{
    return _configLastSelectedKRingKeyOID;
};

void dbp::configSetLastSelectedKRingKeyOID(DB_ID_TYPE val)
{
    if (val != _configLastSelectedKRingKeyOID)
    {
        _configLastSelectedKRingKeyOID = val;
        setConfigValue(&dbp::defaultDB(), "k-id", val);
    }
};
/*
DB_ID_TYPE dbp::configLastSelectedContactGroupOID()
{
    return _configLastSelectedContactGroupOID;
};

void dbp::configSetLastSelectedContactGroupOID(DB_ID_TYPE val)
{
    if (val != _configLastSelectedContactGroupOID)
    {
        _configLastSelectedContactGroupOID = val;
        setConfigValue(&dbp::defaultDB(), "contact-g-id", val);
    }
};
*/
//...

DB_ID_TYPE dbp::configLastDestGenericTopicOID()
{
  return _configLastDestGenericTopicOID;
};

void dbp::configSetLastDestGenericTopicOID(DB_ID_TYPE val)
{
  if(val != _configLastDestGenericTopicOID)
    {
      _configLastDestGenericTopicOID = val;
      setConfigValue(&dbp::defaultDB(), "dest-topic", val);
    }
};


#ifdef ARD_BIG

namespace dbp {
    struct PopupTopicConfigInfo
    {
        EOBJ        otype;
        QString     oid;
        QString     pid;
        bool        locked_selector{ false };
       // QRect       pprect;
    };

    using POPUP_CONFIG_LST = std::vector<dbp::PopupTopicConfigInfo>;

    struct PopupConfigInfo 
    {
		POPUP_CONFIG_LST    tab_topics;
        int                 curr_tab_index{0};
    };

    static PopupConfigInfo configLoadPopupIDs()
    {
        PopupConfigInfo rv;

        std::function<POPUP_CONFIG_LST(const QJsonArray&)> load_cfg_items = [](const QJsonArray& js_arr) 
        {
            POPUP_CONFIG_LST rv;
            int Max = js_arr.size();
            for (int i = 0; i < Max; ++i)
            {
                auto js             = js_arr[i].toObject();
                auto id_str         = js["id"].toString();
                auto pid_str        = js["pid"].toString();
                auto otype          = js["otype"].toString().toInt();
                auto locked_selector = (js["locked_selector"].toString().toInt() > 0);
                //auto ppl            = js["pp_l"].toString().toInt();
                //auto ppt            = js["pp_t"].toString().toInt();
                //auto ppw            = js["pp_w"].toString().toInt();
                //auto pph            = js["pp_h"].toString().toInt();
                if (otype < objBoardItem)
                {
                    EOBJ ot = (EOBJ)otype;
                    dbp::PopupTopicConfigInfo ci;
                    ci.otype = ot;
                    ci.oid = id_str;
                    ci.pid = pid_str;
                    ci.locked_selector = locked_selector;
                    //ci.pprect = QRect(ppl, ppt, ppw, pph);
                    rv.push_back(ci);
                }
            }
            return rv;
        };

        if (!_popup_ids.isEmpty())
        {
            QJsonObject js;
            QJsonParseError  err;
            QJsonDocument jd = QJsonDocument().fromJson(_popup_ids.toUtf8(), &err);
            if (err.error == QJsonParseError::NoError) {
                js = jd.object();
                rv.tab_topics = load_cfg_items(js["wspace"].toArray());
                rv.curr_tab_index = js["tab-index"].toString().toInt();
            }
            else {
                qWarning() << "json-reading error" << err.error << err.errorString();
            }
        }
        return rv;
    };
}

dbp::popup_state dbp::loadPopupTopics()
{
    std::function<popup_state::pstate_arr(const POPUP_CONFIG_LST&)> load_all_popup_except_emails = [](const POPUP_CONFIG_LST& cfg_lst)
    {
        popup_state::pstate_arr rv;
        int idx = 0;
        for (auto& i : cfg_lst)
        {
            switch (i.otype)
            {
            case objFolder:
            case objEmailDraft:
            case objBoard:
            {
                auto f = ard::lookup(i.oid.toInt());
                ASSERT(f, "failed to locate popup topic") << i.oid.toInt();
                if (f) {
                    popup_state::topic_pstate st;
                    st.topic = f;
                    st.index = idx;
                    st.locked_selector = i.locked_selector;
                    //st.pprect = i.pprect;
                    rv.push_back(st);
                }
            }break;
            default:ASSERT(0, "unsupported tab topic") << static_cast<int>(i.otype);
            }
            idx++;
        }
        return rv;
    };
    
    std::function<popup_state::pstate_arr(const POPUP_CONFIG_LST&, ard::email_model*)> load_all_popup = [](const POPUP_CONFIG_LST& cfg_lst, ard::email_model* gm)
    {
        popup_state::pstate_arr rv;
        int idx = 0;

        std::unordered_map<QString, std::vector<QString>> t2m;
        std::unordered_map<QString, int> oid2idx;
        for (auto& i : cfg_lst)
        {
            switch (i.otype)
            {
            case objEmail:
            {
                auto j = t2m.find(i.pid);
                if (j == t2m.end()) {
                    std::vector<QString> pv;
                    pv.push_back(i.oid);
                    t2m[i.pid] = pv;
                    oid2idx[i.oid] = idx;
                }
                else {
                    j->second.push_back(i.oid);
                }
            }break;
            case objFolder:
            case objEmailDraft:
            case objBoard:       
			case objPicture:
            {
                auto f = ard::lookup(i.oid.toInt());                
                if (f) {
                    popup_state::topic_pstate st;
                    st.topic = f;
                    st.index = idx;
                    st.locked_selector = i.locked_selector;
                    //st.pprect = i.pprect;
                    rv.push_back(st);
                }
                else{
                    qWarning() << "failed to locate popup type=" << i.otype << " oid=" << i.oid.toInt();
                }
            }break;
			case objFoldersBoard:
			{
				auto d = ard::db();
				if (d && d->isOpen()){
					auto mb = d->boards_model()->folders_board();
					if (mb) {
						popup_state::topic_pstate st;
						st.topic = mb;
						st.index = idx;
						st.locked_selector = i.locked_selector;
						//st.pprect = i.pprect;
						rv.push_back(st);
					}
				}					
			}break;
			case objMailBoard:
			case objFileRef:break;
            default:ASSERT(0, "unsupported tab topic") << static_cast<int>(i.otype);
            }
            idx++;
        }
        auto r = gm->lookupOrWrapGthreadMessages(t2m);
        for (auto i : r)
        {
            popup_state::topic_pstate st;
            st.topic = i;
            auto j = oid2idx.find(i->optr()->id());
            if (j != oid2idx.end()) {
                st.index = j->second;
                auto& st2 = cfg_lst[j->second];
                st.locked_selector = st2.locked_selector;
                //st.pprect = st2.pprect;
            }
            rv.push_back(st);
        }

        return rv;
    };


    auto l1 = dbp::configLoadPopupIDs();
	popup_state::pstate_arr lst_tabs;// , lst_popups;
    auto gm = ard::gmail_model();
    if (gm)
    {
        lst_tabs = load_all_popup(l1.tab_topics, gm);
        //lst_popups = load_all_popup(l1.popup_topics, gm);
    }
    else
    {
        /// no gmail attached, pick up only topics
        lst_tabs = load_all_popup_except_emails(l1.tab_topics);
        //lst_popups = load_all_popup_except_emails(l1.popup_topics);
    }

    std::sort(lst_tabs.begin(), lst_tabs.end(), [](const popup_state::topic_pstate& p1, const popup_state::topic_pstate& p2)
    {
        return p1.index < p2.index;
    });

   // std::sort(lst_popups.begin(), lst_popups.end(), [](const popup_state::topic_pstate& p1, const popup_state::topic_pstate& p2)
   // {
   //     return p1.index < p2.index;
   // });

    TOPICS_SET tset;
    dbp::popup_state rv;
    for (auto i : lst_tabs)
    {
        auto j = tset.find(i.topic);
        if (j == tset.end()) {
            rv.tab_topics.push_back(i);
            tset.insert(i.topic);
        }
        else {
            ASSERT(0, "duplicate topic detected in config file 1");
        }
    }
    rv.curr_tab_index = l1.curr_tab_index;
    return rv;
};

void dbp::configStorePopupIDs(QString s) 
{
    _popup_ids = s;
    setConfigValue(&dbp::defaultDB(), "popup-ids", s);
};

#endif

void dbp::configStoreCurrentTopic()
{
    ArdDB& db = dbp::defaultDB();
    if (db.isOpen()) {
        auto f = ard::currentTopic();
        if (f) {
			f = f->shortcutUnderlying();
            auto id = f->id();
            if (IS_VALID_DB_ID(id))
            {
                setConfigValue(&db, "current-topic", id);
            }
        }
    }
};

void dbp::configRestoreCurrentTopic()
{
    ArdDB& db = dbp::defaultDB();
    if (db.isOpen()) {
        int id = getConfigIntValue(&dbp::defaultDB(), "current-topic", 0);
        if (IS_VALID_DB_ID(id)){
            auto f = db.lookupLoadedItem(id);
            if (f && !dynamic_cast<ard::locus_folder*>(f))
			{
                gui::ensureVisibleInOutline(f);
            }
        }
    }
};


void dbp::configSaveMoveDestID(int idx, DB_ID_TYPE id)
{
  QString cname = QString("move-dest-%1").arg(idx);
  setConfigValue(&dbp::defaultDB(), cname, id);
};

/*bool dbp::configFileGoogleIsEnabled()
{
    return _configFileGoogleIsEnabled;
};

void dbp::configFileEnableGoogle(bool set_it)
{
    _configFileGoogleIsEnabled = set_it;
    writeFileConfigRecord("g-api-on", _configFileGoogleIsEnabled);
};*/


void dbp::configGDriveSyncLoadLastSyncTime(ArdDB* db, LastSyncInfo& si)
{
    si.invalidate();
    QString s = getConfigStringValue(db, "g-last-sync", "").trimmed();
    si.fromString(s);
};


void dbp::configGDriveSyncStoreLastSyncTime(ArdDB* db, const LastSyncInfo& si)
{
    QString s = si.toString();
    setConfigValue(db, "g-last-sync", s);
};

void dbp::configLocalSyncLoadLastSyncTime(ArdDB* db, LastSyncInfo& si) 
{
    si.invalidate();
    QString s = getConfigStringValue(db, "l-last-sync", "").trimmed();
    si.fromString(s);
};

void dbp::configLocalSyncStoreLastSyncTime(ArdDB* db, const LastSyncInfo& si) 
{
    QString s = si.toString();
    setConfigValue(db, "l-last-sync", s);
};

DB_ID_TYPE dbp::configLoadMoveDestID(int idx)
{
  QString cname = QString("move-dest-%1").arg(idx);
  return getConfigIntValue(&dbp::defaultDB(), cname, 0);
};

std::map<int, int> dbp::configLoadBoardRulesSettings()
{
	std::map<int, int> rv;
	auto d = &dbp::defaultDB();
	if (d && d->isOpen())
	{
		QString str = getConfigStringValue(d, "board-bands", "").trimmed();
		auto bval = str.split(";");
		for (auto& s : bval) 
		{
			auto s1lst = s.split(":");
			if (s1lst.size() == 2) 
			{
				auto r_id = s1lst[0].toInt();
				auto r_w = s1lst[1].toInt();
				rv[r_id] = r_w;
			}
		}
	}
	return rv;
};

void dbp::configStoreBoardRulesSettings() 
{
	auto d = &dbp::defaultDB();
	if (d && d->isOpen())
	{
		QString str;
		auto lst = d->rmodel()->boardRules();
		for (auto& r : lst) {
			if(r->boardBandWidth() != BBOARD_BAND_DEFAULT_WIDTH)
				str += QString("%1:%2;").arg(r->id()).arg(r->boardBandWidth());
		}
		setConfigValue(d, "board-bands", str);
	}
};

std::map<int, int> 	dbp::configLoadBoardFoldersSettings() 
{
	std::map<int, int> rv;
	return rv;
};

void dbp::configStoreBoardFolderBandWidth(int dbid, int w)
{
	auto d = &dbp::defaultDB();
	if (d && d->isOpen())
	{
		//auto str = QString("%1:%2;").arg(dbid).arg(r->boardBandWidth());
		setConfigValue(d, QString("folder-board-bands-%1").arg(dbid), w);
	}
};


bool dbp::configFileCheck4Trial()
{
#define TRIAL_PERIOD_DAYS 20

  QString sfile = configFilePath();
  QSettings settings(sfile, QSettings::IniFormat);

  int trialVer = settings.value("t-v", "0").toInt();
  if(trialVer < get_app_version_as_int())
    {
      writeFileConfigRecord("t-v", get_app_version_as_int());
      QDate dt = QDate::currentDate();
      writeFileConfigRecord("t-d", dt.toString("yyyyMMdd"));
      return true;
    }
  QString tdate = settings.value("t-d", "").toString();
  if(!tdate.isEmpty())
    {
      QDate dt = QDate::fromString(tdate, "yyyyMMdd");
      if(dt.isValid())
    {
      dt = dt.addDays(TRIAL_PERIOD_DAYS);
      QDate dtCurr = QDate::currentDate();
      int daysLeft = dtCurr.daysTo(dt);
      if(daysLeft > 0)
        {
          qDebug() << "trial-days-left" << daysLeft;
          return true;
        }
    }
    }

  return false;
};

bool dbp::configFileGetDrawDebugHint() 
{
    return _configDrawDebugHint;
};

void dbp::configFileSetDrawDebugHint(bool val) 
{
    _configDrawDebugHint = val;
    writeFileConfigRecord("debug-hint", val ? 1 : 0);
};

bool dbp::configIsPwdValid(ArdDB* db, QString pwd)
{
  bool rv = false;
  pwd = pwd.trimmed();
  QString dbpwd = getConfigStringValue(db, "db-pwd", "").trimmed();
  if(pwd.isEmpty() && dbpwd.isEmpty())
    {
      rv = true;
    }
  else
    {
      QString hash_s = QCryptographicHash::hash((pwd.toUtf8()), QCryptographicHash::Md5).toHex();
      rv = (dbpwd.compare(hash_s) == 0);
    }
  return rv;
};

bool dbp::configHasPwd(ArdDB* db)
{
  QString dbpwd = getConfigStringValue(db, "db-pwd", "").trimmed();
  bool rv = !dbpwd.isEmpty();
  return rv;
};

bool dbp::configChangePwd(ArdDB* db, QString oldPwd, QString newPwd, QString pwdHint, bool enableReadOnly)
{
  newPwd = newPwd.trimmed();

  if(!configIsPwdValid(db, oldPwd))
    {
      ASSERT(0, "invalid old password provided");
      return false;
    }
  QString hash_s = "";
  if(!newPwd.isEmpty())
    {
      hash_s = QCryptographicHash::hash((newPwd.toUtf8()), QCryptographicHash::Md5).toHex();
    }
  setConfigValue(db, "db-pwd", hash_s);
  setConfigValue(db, "db-pwd-hint", pwdHint);
  setConfigValue(db, "db-pwd-ROA", enableReadOnly ? 1 : 0);
  dbp::setLastUsedPWD(newPwd);
  return true;
};

QString dbp::configGetPwdHint(ArdDB* db)
{
  QString rv = getConfigStringValue(db, "db-pwd-hint", "").trimmed();
  return rv;
};

bool dbp::configCanROA_WithoutPassword(ArdDB* db)
{
  int r = getConfigIntValue(db, "db-pwd-ROA", 0);
  bool rv = (r == 1);
  return rv;
};

bool dbp::guiCheckPassword(bool& ROmodeRequest, ArdDB* db, QString hint /*= ""*/)
{
  bool pwdOK = true;
  ROmodeRequest = false;
  if(configHasPwd(db) && 
     !configIsPwdValid(db, dbp::lastUsedPWD()))
    {      
      pwdOK = false;
      bool ask4Pwd = true;
      while(ask4Pwd)
    {
      bool cancelled = false;
      QString pwd = getUserPassword(configGetPwdHint(db),cancelled, ROmodeRequest, hint);
      if(cancelled)return false;
      pwd = pwd.trimmed();
      ask4Pwd = !pwd.isEmpty();
      if(configIsPwdValid(db, pwd))
        {
          dbp::setLastUsedPWD(pwd);
          pwdOK = true;
          break;
        }
      else
        {
          if(ROmodeRequest)return false;
		  ard::messageBox(gui::mainWnd(), "Invalid Password");
        }
    }
    }
  return pwdOK;
};


int dbp::configFileCustomFontSize()
{
    return _configCustomFontSize;
};

void dbp::configFileSetCustomFontSize(int val)
{
    if (val >= MIN_CUST_FONT_SIZE && val <= MAX_CUST_FONT_SIZE)
    {
        _configCustomFontSize = val;
        writeFileConfigRecord("custom-font-size", val);
    }
};


int dbp::configFileNoteFontSize() 
{
    return _configNoteFontSize;
};

void dbp::configFileSetNoteFontSize(int val) 
{
    _configNoteFontSize = val;
    writeFileConfigRecord("note-font-size", val);
};


QString dbp::configFileNoteFontFamily() 
{
    return _configNoteFontFamily;
};

void dbp::configFileSetNoteFontFamily(QString val) 
{
    _configNoteFontFamily = val;
    writeFileConfigRecord("note-font-family", val);
};

extern void setupSystemTrayIcon();
void dbp::configFileSetRunInSysTray(bool v) 
{
    if (v != _configRunInSysTray) {
        _configRunInSysTray = v;
        writeFileConfigRecord("systray", v ? 1 : 0);
        setupSystemTrayIcon();
    }
};

bool dbp::configFileGetRunInSysTray() 
{
    return _configRunInSysTray;
};

DB_ID_TYPE dbp::configFileSupportCmdLevel()
{
  return _configSupportCmdLevel;
};

void dbp::configFileSetSupportCmdLevel(int val)
{
  _configSupportCmdLevel = val;
  writeFileConfigRecord("support-level", _configSupportCmdLevel);
};



QString dbp::configFileGetLastDB()
{
  return _configLastDBName;
};

void dbp::configFileSetLastDB(QString val)
{
  _configLastDBName = val;
  writeFileConfigRecord("last-db-name", _configLastDBName);
};

bool dbp::configFileFollowDestination() 
{
    return _configFollowDestination;
};

void dbp::configFileSetFollowDestination(bool val) 
{
    _configFollowDestination = val;
    writeFileConfigRecord("follow-dest", val ? 1 : 0);
};

bool dbp::configFileGoogleEmailListCheckSelectColumn()
{
    return _configFileGoogleEmailListCheckSelectColumn;
}

void dbp::configFileEnableGoogleEmailListCheckSelectColumn(bool val)
{
    _configFileGoogleEmailListCheckSelectColumn = val;
    writeFileConfigRecord("email-list-check-sel", val ? 1 : 0);
};

QString dbp::configFileLastSearchStr() 
{
    return _configLastSearchStr;
};

void dbp::configFileSetLastSearchStr(QString s) 
{
    _configLastSearchStr = s;
    writeFileConfigRecord("last-search-str", s);
};

bool dbp::configFileMailBoardUnreadFilterON() 
{
	return _configMailBoardUnreadFilterON;
};

void dbp::configFileSetMailBoardUnreadFilterON(bool set_it) 
{
	_configMailBoardUnreadFilterON = set_it;
};

bool dbp::configFileFilterInbox() 
{
	return _configFileFilterInbox;
};

void dbp::configFileSetFilterInbox(bool set_it) 
{
	_configFileFilterInbox = set_it;
	writeFileConfigRecord("filter-inbox", set_it ? "1" : "0");
};

bool dbp::configFilePreFilterInbox() 
{
	return _configPreFileFilterInbox;
};

void dbp::configFileSetPreFilterInbox(bool set_it) 
{
	_configPreFileFilterInbox = set_it;
	writeFileConfigRecord("prefilter-inbox", set_it ? "1" : "0");
};

bool dbp::configFileMaiBoardSchedule() 
{
	return _configMaiBoardSchedule;
};

void dbp::configFileSetMaiBoardSchedule(bool set_it) 
{
	_configMaiBoardSchedule = set_it;
	writeFileConfigRecord("mail-board-schedule", set_it ? "1" : "0");
};



QString dbp::configFileLastSearchFilter()
{
  return _configLastSearchStr;
};

void dbp::configFileSetLastSearchFilter(QString s)
{
  _configLastSearchFilter = s;
  writeFileConfigRecord("last-search-filter", s);
};


void dbp::getDBStatistics(DB_STATISTICS& db_stat)
{
  QString sql = "SELECT * FROM(SELECT COUNT(*) num, 1 ctype FROM ard_tree WHERE otype=1 AND subtype=0"
    " UNION "
    " SELECT COUNT(*) num, 2 ctype FROM ard_ext_note "
    " UNION "
    " SELECT MAX(mdc) num, 3 ctype FROM ard_tree"
    " UNION "
    " SELECT MAX(mvc) num, 4 ctype FROM ard_tree"
    ") ORDER BY ctype ";

  auto q = dbp::defaultDB().selectQuery(sql);
  assert_return_void(q, "expected query");
  QString name, val;

  QString db_path = get_db_file_path();
  QFile f(db_path);
  val = QString("%1").arg(f.size() / 1024);
  db_stat.push_back(std::make_pair("DB Size (KB)", val));

  while(q->next())
    {
      int num   = q->value(0).toInt();
      int ctype = q->value(1).toInt();
      switch(ctype)
    {
    case 1: name = QString("Topics");break;
    case 2: name = QString("Notes");break;
    case 3: name = QString("maxMOD");break;
    case 4: name = QString("maxMOV");break;
    }
      val = QString("%1").arg(num);
      db_stat.push_back(std::make_pair(name, val));
    }  
};


void dbp::findItemsByType(EOBJ otype, TOPICS_LIST& items_list, ArdDB* db, int subtype /*= -1*/)
{
  QString sql = QString("SELECT oid FROM ard_tree WHERE otype=%1").arg(otype);
  if(subtype != -1)
    {
      sql = QString("SELECT oid FROM ard_tree WHERE otype=%1 AND subtype=%2").arg(otype).arg(subtype);
      //      qDebug() << "<<<< sub-type selecting" << sql;
    }

  auto q = db->selectQuery(sql); 
  while(q->next())
    {
      DB_ID_TYPE oid = q->value(0).toInt();
      auto it = db->lookupLoadedItem(oid);
      if(it)
    {
      items_list.push_back(it);
    }
    }
};

//......
void dbp::findItemsWHERE(TOPICS_LIST& items_list, ArdDB* db, QString where)
{
  QString sql = QString("SELECT oid FROM ard_tree WHERE %1").arg(where);
  auto q = db->selectQuery(sql);
  while(q->next())
    {
      DB_ID_TYPE oid = q->value(0).toInt();
      auto it = db->lookupLoadedItem(oid);
      if(it)
    {
      items_list.push_back(it);
    }
    }
};

int dbp::getRetiredCount()
{
  QString sql = QString("SELECT count(*) FROM ard_tree WHERE retired>0");
  int rv = defaultDB().queryInt(sql);
  return rv;
};

TOPICS_LIST dbp::selectLockedList(QString sql, ArdDB* db)
{
    auto q = db->selectQuery(sql);
    if (!q) {
        ASSERT(0, "expected query");
		TOPICS_LIST rv;
        return rv;
    }
    IDS_LIST ids;
    while (q->next())
    {
        DB_ID_TYPE oid = q->value(0).toInt();
        ids.push_back(oid);
    }
    return db->lookupAndLockLoadedItemsList(ids);
};

TOPICS_LIST dbp::findByText(QString s, ArdDB* db)
{   
    QString sql = QString("SELECT oid FROM ard_tree WHERE subtype IN(0, 2) AND otype=1 AND (UPPER(title) LIKE UPPER('%%1%') OR UPPER(annotation) LIKE UPPER('%%1%') OR oid IN(SELECT oid FROM ard_ext_note WHERE UPPER(note_plain_text) LIKE UPPER('%%1%')))").arg(s);
    return selectLockedList(sql, db);
};

TOPICS_LIST dbp::findToDos(ArdDB* db)
{
    QString sql = QString("SELECT oid FROM ard_tree WHERE todo<>0 AND (retired IS NULL OR retired&1<>1)");
    return selectLockedList(sql, db);
};

TOPICS_LIST dbp::findAnnotated(ArdDB* db)
{
    QString sql = QString("SELECT oid FROM ard_tree WHERE (annotation is not null AND annotation <> '') AND (retired IS NULL OR retired&1<>1)");
    return selectLockedList(sql, db);
};

TOPICS_LIST dbp::findNotes(ArdDB* db)
{
    QString sql = QString("SELECT oid FROM ard_tree WHERE otype=1 and subtype=0 and oid IN(SELECT oid FROM ard_ext_note WHERE note_plain_text<>'')");
    return selectLockedList(sql, db);
};

TOPICS_LIST dbp::findBookmarks(ArdDB* db)
{
	QString sql = QString("SELECT oid FROM ard_tree WHERE otype=19");
	return selectLockedList(sql, db);
};

TOPICS_LIST dbp::findPictures(ArdDB* db)
{
	QString sql = QString("SELECT oid FROM ard_tree WHERE otype=5");
	return selectLockedList(sql, db);
};

TOPICS_LIST dbp::findColors(ArdDB* db)
{
	QString sql = QString("SELECT oid FROM ard_tree WHERE retired&1<>1 AND retired&26<>0 AND otype IN(1,2,3,4,5,19,31)");
	return selectLockedList(sql, db);
};

void dbp::findByDatesItems(EDatesSummary mods, TOPICS_LIST& items_list, ArdDB* db)
{
#define MAKE_DATE_SQL(T1, T2) QString("SELECT oid FROM ard_tree WHERE (mod_time>%1 and mod_time<%2) or oid IN(SELECT oid FROM ard_ext_note WHERE mod_time>%1 and mod_time<%2)").arg(T1.toTime_t()).arg(T2.toTime_t())

  QDateTime _now = QDateTime::currentDateTime();
  QDateTime _today = QDateTime(QDate::currentDate());
  QDateTime _yesterday = _today.addDays(-1);
  QDateTime _week = _today.addDays(-7);
  QDateTime _month = _today.addMonths(-1);
  QDateTime _year = _today.addYears(-1);

  QString sql = "";
  switch(mods)
    {
    case ModifiedToday: sql = MAKE_DATE_SQL(_today, _now);break;
    case ModifiedSinceYesterday: sql = MAKE_DATE_SQL(_yesterday, _today);break;
    case ModifiedSince1Week: sql = MAKE_DATE_SQL(_week, _yesterday);break;
    case Modified1SinceMonth: sql = MAKE_DATE_SQL(_month, _week);break;
    case ModifiedSince1Year: sql = MAKE_DATE_SQL(_year, _month);break;
    case ModifiedOlderThenYear:sql = QString("SELECT oid FROM ard_tree WHERE mod_time IS NULL or mod_time<%1 or oid IN(SELECT oid FROM ard_ext_note WHERE mod_time IS NULL or mod_time<%1)").arg(_today.addYears(-1).toTime_t());break;
    case ModifiedUknown:ASSERT(0, "NA");break;
    }
  db->pipeItems(sql, items_list);
#undef MAKE_DATE_SQL
};

void dbp::findByTypeItems(ETypeSummary type, TOPICS_LIST& items_list, ArdDB* db)
{
  QString sql = "";
  switch(type)
    {
    case TypeNotes:sql = QString("SELECT oid FROM ard_tree WHERE oid IN(SELECT oid FROM ard_ext_note WHERE length(note_plain_text) > 0)");break;
    case TypeRetired:sql = QString("SELECT oid FROM ard_tree WHERE (retired IS NULL OR retired&1=1)");break;
    case TypeToDos:sql = QString("SELECT oid FROM ard_tree WHERE (todo > 0)");break;
    case TypeResources:sql = QString("SELECT oid FROM ard_tree WHERE otype=%1").arg(objPrjResource);break;
    case TypeNonSummary:
    case TypeAnyItem:ASSERT(0, "NA");break;
    }
  db->pipeItems(sql, items_list);
};

void dbp::findBySyid(QString syid, TOPICS_LIST& items_list, ArdDB* db/*, EOBJ eo*/)
{
  QString sql = QString("SELECT oid FROM ard_tree WHERE syid='%1'").arg(syid);;
  db->pipeItems(sql, items_list);
};

QString dbp::currentTimeStamp()
{
    auto rv = QDateTime::currentDateTime().toString(Qt::ISODate);
    return rv;
};

QString dbp::currentDateStamp()
{
    auto rv = QDate::currentDate().toString(Qt::DefaultLocaleShortDate);
    return rv;
};

std::set<ard::EColor> dbp::configColorGrepInFilter()
{
    return _configColorGrepInFilter;
};

void dbp::configSetColorGrepInFilter(std::set<ard::EColor> f)
{
    _configColorGrepInFilter = f;
};

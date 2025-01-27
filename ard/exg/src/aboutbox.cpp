#include "aboutbox.h"
#include <QDebug>
#include <QTextBrowser>
#include <QHBoxLayout>
#include <QDesktopServices>
#include <iostream>
#include <QImageReader>
#include <QTextCodec>
#include <QSslSocket>
#include <QDate>
#include <QCoreApplication>
#include <QApplication>
#include <QDesktopWidget>
#include <QPushButton>
#include <QStandardItemModel>
#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QLineEdit>
#include <QNetworkInterface>

#include "ChoiceBox.h"
#include "a-db-utils.h"
#include "dbp.h"
#include "anfolder.h"
#include "email.h"
#include "snc-tree.h"
#include "ansyncdb.h"
#include "custom-widgets.h"

extern QString get_app_version_as_long_string();
extern bool is_instance_read_only_mode();
extern QString get_app_version_moto();
extern QString gmailDBFilePath();
extern QString gmailCachePath();

QDate projectCompilationDate(void)
{
    QString strDate = __DATE__;
    QStringList lstDate = strDate.split(QRegExp("\\s+"), QString::SkipEmptyParts);
    if(lstDate.size() < 3)
        return QDate();

    QLocale us = QLocale("en_US");
    int year = us.toDate(lstDate[2], "yyyy").year();
    int month = us.toDate(lstDate[0], "MMM").month();
    int day = us.toDate(lstDate[1], "d").day();
 
    return QDate(year, month, day);
}

#ifdef ARD_BETA

#define BETA_EXPIRE_AFTER_MONTHS 36

bool checkBetaExpire()
{
	auto start_date = projectCompilationDate();
	auto dtExpireDate = start_date.addMonths(BETA_EXPIRE_AFTER_MONTHS);
	int daysLeft = QDate::currentDate().daysTo(dtExpireDate);
	qWarning() << "betadaysleft" << daysLeft << "from" << start_date.toString();
	if (daysLeft <= 0)
	{
		ard::messageBox(gui::mainWnd(), QString("Thank you for evaluating '%1'. We are sorry for inconvenience but this BETA version of the program has expired. Please uninstall it and setup latest release version. See www.prokarpaty.net for more information.").arg(programName()));
		return false;
	}

	if (daysLeft <= 90)
	{
		ard::messageBox(gui::mainWnd(), QString("Thank you for evaluating %1. We are sorry for inconvenience but this BETA version of the program will expire in %2 day(s). Please uninstall it and setup latest release version. See www.prokarpaty.net for more information.").arg(programName()).arg(daysLeft));
	}

	return true;
};
#endif


static QString about_html()
{
    typedef std::vector<std::pair<QString, QString> > INFO_LIST;
    INFO_LIST info;
    info.push_back(std::make_pair("Version", QString("<b><font color=\"red\">%1<font></b> - '%2'").arg(get_app_version_as_string()).arg(get_app_version_moto())));

    QString stmp = "";
#ifdef ARD_BETA
    stmp = QString("%1/%2").arg(qVersion()).arg(QT_VERSION_STR);
    info.push_back(std::make_pair("Qt", stmp));
    extern qreal ard_screen_inches;
    info.push_back(std::make_pair("Screen/Icon", QString("Screen:%1 ICON_WIDTH:%2").arg(ard_screen_inches).arg(ICON_WIDTH)));
	auto dtExpireDate = projectCompilationDate().addMonths(BETA_EXPIRE_AFTER_MONTHS);
	info.push_back(std::make_pair("BETA expiration date", QString("<b><font color=\"red\">%1<font></b>").arg(dtExpireDate.toString())));
#endif //ARD_BETA

    QString sBuildInfo = "";


#ifdef ARD_BIG
    sBuildInfo += "BIG/";
#else
    sBuildInfo += "SMALL/";
#endif

#ifdef ARD_BETA
    sBuildInfo += "BETA/";
#endif

#ifdef _DEBUG
    sBuildInfo += "DEBUG/";
#else
    sBuildInfo += "RELEASE/";
#endif

#ifdef _DEBUG
    sBuildInfo += "DEBUG/";
#endif

#ifdef _CPPRTTI
    sBuildInfo += "RTTI/";
#endif

#ifdef __LP64__
    sBuildInfo += "x64/";
#else
    sBuildInfo += "x32/";
#endif

    int idx = stmp.indexOf("(");
    if(idx != -1)
        {
            qDebug() << "GV-version:" << stmp;
            stmp = stmp.remove(idx-1, stmp.length());
            stmp = stmp.trimmed();
        }

    sBuildInfo += QString("%1/").arg(stmp);
    sBuildInfo += QString("%1/").arg(qVersion());

    //sBuildInfo += projectCompilationDate().toString(Qt::SystemLocaleShortDate);
    sBuildInfo += projectCompilationDate().toString("dd-MM-yyyy");
    info.push_back(std::make_pair("Build", sBuildInfo));

    if(!gui::isDBAttached())
        {
            info.push_back(std::make_pair("Data", "DISCONNECTED"));
        }
    else
        {
            if(is_instance_read_only_mode())
                {  
                    info.push_back(std::make_pair("DB-mode", "Read only"));
                    info.push_back(std::make_pair("Data", ard::db()->databaseName()));
                    // info.push_back(std::make_pair("Data", get_db_tmp_file_path()));
                }
        }  

    info.push_back(std::make_pair("Log file", get_program_appdata_log_file_name()));
    info.push_back(std::make_pair("Current dir", QDir::currentPath()));
    info.push_back(std::make_pair("applicationDirPath", QCoreApplication::applicationDirPath()));
	info.push_back(std::make_pair("emailUserId", dbp::configEmailUserId()));
	info.push_back(std::make_pair("emailDB", gmailDBFilePath()));
	info.push_back(std::make_pair("emailCacheDir", gmailCachePath()));
	info.push_back(std::make_pair("emailAttachementDir", ard::getAttachmentDownloadDir()));



    
    QFileInfo log_fi(get_program_appdata_log_file_name());
    QString log_s = QString("%1/%2")
        .arg(googleQt::size_human(log_fi.size()))
        .arg(log_fi.lastModified().toString());
    info.push_back(std::make_pair("Log info", log_s));
    //  info.push_back(std::make_pair("appDirPath", QCoreApplication::applicationDirPath()));
    QString sdir = defaultRepositoryPath();
    QDir d1(sdir);
    bool defRepo_exists = d1.exists();  
    info.push_back(std::make_pair(QString("defaultRepositoryPath - %1").arg(defRepo_exists), sdir));
    info.push_back(std::make_pair("home", QDir::homePath()));
    QString netInfo = "";
    QList<QNetworkInterface> ifaces = QNetworkInterface::allInterfaces();
    for (int i = 0; i < ifaces.count(); i++)
    {
        QNetworkInterface iface = ifaces.at(i);
        if (iface.flags().testFlag(QNetworkInterface::IsUp) && !iface.flags().testFlag(QNetworkInterface::IsLoopBack))
        {
            if (iface.addressEntries().count() > 0)
            {
                netInfo += iface.name();
                netInfo += " ";
                break;
            }
        }
    }
    
    info.push_back(std::make_pair("Network", netInfo));
    info.push_back(std::make_pair("SSL", QSslSocket::supportsSsl() ? "YES" : "NO"));

    QString gscope = "---";
    auto cl = ard::google();
    if (cl) 
    {
        extern QString ScopeCode2ScopeLabels(int);
        int scode = cl->getAccessScope();
        gscope = QString("%1 - ").arg(scode);
        gscope += ScopeCode2ScopeLabels(scode);
    }
    info.push_back(std::make_pair("GoogleAccessScope", gscope));

    /*
      QString sdir = QStandardPaths::displayName(QStandardPaths::AppDataLocation);
      QDir d1(sdir);
      bool AppDataLocation_exists = d1.exists();
      info.push_back(std::make_pair(QString("AppDataLocation(%1)").arg(AppDataLocation_exists), sdir));
    */

    //#define PRINT_ENV(E)info.push_back(std::make_pair(E, QString::fromLocal8Bit( qgetenv(E).constData())));
    //  PRINT_ENV("ARD_ROOT_DIR");
    //#undef PRINT_ENV
#define PRINT_ENV(E) if(true){                                          \
        QString _senv = QString::fromLocal8Bit( qgetenv(E).constData()); \
        if(!_senv.isEmpty())info.push_back(std::make_pair(E, _senv));   \
    }                                                                   \
    
    PRINT_ENV("ARD_ROOT_DIR");
#undef PRINT_ENV
  
    stmp = "";
    if(gui::isDBAttached())
        {
            info.push_back(std::make_pair("Database", QString("<b><font color=\"red\">%1<font></b>").arg(dbp::currentDBName())));
            info.push_back(std::make_pair("DBID", QString("%1").arg(ard::db()->syncDb()->db_id())));
            info.push_back(std::make_pair("DB-Path", get_db_file_path()));

            dbp::DB_STATISTICS db_stat;
            dbp::getDBStatistics(db_stat);
            //for(dbp::DB_STATISTICS::iterator i = db_stat.begin(); i != db_stat.end(); i++)
            for(const auto& i : db_stat)    {
                    stmp += i.first;
                    stmp += ":";
                    stmp += i.second;
                    stmp += ";";
                }
            info.push_back(std::make_pair("Summary", stmp));
            //..
            LastSyncInfo si;
            dbp::configGDriveSyncLoadLastSyncTime(&dbp::defaultDB(), si);
            if (si.isValid()) {
                info.push_back(std::make_pair("LastG=Sync", si.toString4Human()));
            }

            dbp::configLocalSyncLoadLastSyncTime(&dbp::defaultDB(), si);
            if (si.isValid()) {
                info.push_back(std::make_pair("LastL-Sync", si.toString4Human()));
            }
            //..
        }

    info.push_back(std::make_pair("DB-drivers", QSqlDatabase::drivers().join(";") ));

    QString supported_images = "";
    QList<QByteArray> supported_list = QImageReader::supportedImageFormats();  
    for(const auto& ba : supported_list)    {
            supported_images += QString(ba) + " "; 
        }  
    info.push_back(std::make_pair(QString("Images(%1)").arg(supported_list.size()), supported_images));

    auto r = ard::gmail();
    if (r) {
        auto st = ard::gstorage();
        if (st) {
            info.push_back(std::make_pair("GMailHistory", QString("%1").arg(st->lastHistoryId())));
        }
    }

    //<basefont color=\"red\" size=\"5\">
    //<style> body {background-color:lightgray}h1{color:blue}p{color:green}</style>
    QString css = "";//"<style>body{font-size: 62.5%;}</style>";


    QString s = QString("<html>\n<head>%1</head>").arg(css);
    s+= "<body></br>";
    s += QString("<h2>%1 - The Organizer</h2>").arg(programName());
    s += "<a href=\"https://prokarpaty.net\">https://prokarpaty.net</a>";
    s += "<table></br>";

    s += "<tr>";
    s += QString("<td colspan=\"2\" align=\"right\"><i>%1</i></td>\n").arg("On mysterious \"Quest\" to obtain the fires of inspiration...");
    s += "</tr>";


    for(const auto& i : info)    {
            s += "<tr>";
            s += QString("<td>%1</td>\n<td>%2</td>").arg(i.first).arg(i.second);
            s += "</tr>";
        }

    s += "</table>";

    s+= "</body>\n</html>";
    return s;
}

static AboutBox* about_box = nullptr;

AboutBox::AboutBox():QDialog(gui::mainWnd())
{
    bview = new ArdTextBrowser();//new QTextBrowser(this); 
    bview->setHtml(about_html());
    bview->setOpenLinks(false);
    connect(bview, SIGNAL(anchorClicked(const QUrl &)), this, SLOT(navigate(const QUrl &)));

    QVBoxLayout *l = new QVBoxLayout;
    l->addWidget(bview);
  
    QHBoxLayout* h1 = new QHBoxLayout();
    QPushButton* b = nullptr;
    
    b = new QPushButton("Version", this);
    connect(b, SIGNAL(released()), this, SLOT(checkVersion()));
    h1->addWidget(b);  
    
    b = new QPushButton("Cancel", this);
    connect(b, SIGNAL(released()), this, SLOT(close()));
    b->setDefault(true);
    gui::setButtonMinHeight(b);
    h1->addWidget(b);  
    l->addLayout(h1);

    setWindowModality(Qt::ApplicationModal);
    setAttribute(Qt::WA_DeleteOnClose);

    MODAL_DIALOG_SIZE(l, QSize(800, 600));
  
	about_box = this;
    exec();
};

void AboutBox::navigate(const QUrl & url)
{
    QDesktopServices::openUrl(url);
};

/*void AboutBox::showEvent(QShowEvent * e)
{
    QDialog::showEvent(e);
};*/


void AboutBox::closeEvent(QCloseEvent *e) 
{
	about_box = nullptr;
	QDialog::closeEvent(e);
};

QString size_human(float num)
{
    //float num = this->size();
    QStringList list;
    list << "KB" << "MB" << "GB" << "TB";

    QStringListIterator i(list);
    QString unit("bytes");

    while(num >= 1024.0 && i.hasNext())
        {
            unit = i.next();
            num /= 1024.0;
        }
    return QString().setNum(num,'f',2)+" "+unit;
}

void AboutBox::checkVersion()
{
	if (!gui::isConnectedToNetwork())
	{
		ard::errorBox(this, "Network connection is not detected. Make sure you are connected to the Internet.");
		return;
	}

	QUrl url("https://www.prokarpaty.net/ard_download/version.info");
	std::map<QString, QString> rawHeaders;
	rawHeaders["Accept"] = "image/*";
	rawHeaders["User-Agent"] = "Mozilla/Firefox 3.6.12";


	ard::fetchHttpData(url, [](QNetworkReply* r)
	{
		if (r->isFinished())
		{
			QByteArray bytes = r->readAll();
			QString s = QString::fromUtf8(bytes.data(), bytes.size()).trimmed();
			if (s.isEmpty())
			{
				ard::messageBox(about_box ? about_box : gui::mainWnd(), "Failed to obtain version#, please try again later.");
			}
			else
			{
				int ver = s.toInt();
				if (ver > get_app_version_as_int())
				{
					ard::messageBox(about_box ? about_box : gui::mainWnd(), "The new version is available to download.");
				}
				else
				{
					ard::messageBox(about_box ? about_box : gui::mainWnd(), QString("Your binary (%1) is up to date. Check next month please.").arg(get_app_version_as_string()));
				}
			}
		}
	}, &rawHeaders);
};


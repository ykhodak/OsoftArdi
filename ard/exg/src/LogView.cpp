#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QApplication>
#include <QDesktopWidget>
#include <QScrollBar>
#include <QLineEdit>
#include <QLabel>
#include <QClipboard>
#include <QComboBox>
#include <QProgressBar>
#include <QHeaderView>
#include <QStandardItemModel>
#include <QDir>
#include <QDesktopServices>
#include "gdrive/GdriveRoutes.h"

#include "LogView.h"
#include "anfolder.h"
#include "ChoiceBox.h"


void LogView::runIt(int tabSel, QString logDirPrefix2Select)
{
    new LogView(tabSel, logDirPrefix2Select);
};

extern QString get_prog_os();

LogView::LogView(int tabSel, QString logDirPrefix2Select):
    QDialog(gui::mainWnd())
{
    m_logDirPrefix2Select = logDirPrefix2Select;    

    m_working_log = new QPlainTextEdit;
    m_sync_log = new QPlainTextEdit;
	QFont font("Courier", 9);
	font.setStyleHint(QFont::Monospace);
	m_working_log->setFont(font);
	m_sync_log->setFont(font);

	auto ss = "color: white; background-color: black;";
	m_working_log->setStyleSheet(ss);
	m_sync_log->setStyleSheet(ss);


    m_archive_view = new QTableView;
    m_archive_view->setEditTriggers(QAbstractItemView::NoEditTriggers);

    QWidget* wsync_log = new QWidget;
    m_log_dir_combo = new QComboBox;
    QVBoxLayout *sl = new QVBoxLayout;
    sl->addWidget(m_log_dir_combo);
    sl->addWidget(m_sync_log);
    wsync_log->setLayout(sl);

    connect(m_log_dir_combo, SIGNAL(currentIndexChanged (int)),
            this, SLOT(SyncDirCurrentIndexChanged(int)));

    /*connect(m_log_dir_combo, &QComboBox::currentIndexChanged, [=](int idx)
    {
        QString slog = m_log_dir_combo->itemData(idx).toString();
        loadSyncLogFile(slog);
    });*/



    m_tab = new QTabWidget;
    m_tab->addTab(m_working_log, "Working Log");
    m_tab->addTab(wsync_log, "Sync Log");
    m_tab->addTab(m_archive_view, "Log Archives");
    connect(m_tab, &QTabWidget::currentChanged, [=](int idx) 
    {
        if (idx == 2 && !m_logArchivesLoaded){
            loadSyncLogArchives();
            m_logArchivesLoaded = true;
        }
    });

    //connect(m_tab, SIGNAL(currentChanged(int)),
    //        this, SLOT(currentTabChanged(int)));

    QVBoxLayout* vl_main = new QVBoxLayout;
    vl_main->addWidget(m_tab);

    QHBoxLayout *h_buttons = new QHBoxLayout;

#ifdef DEPRECATED_ARDI
    ard::addBoxButton(h_buttons, "Delete", [&]() {
            QString logPath = get_program_appdata_log_file_name();
            if(ard::confirmBox(this, QString("Please confirm deleting Ardi log file '%1'").arg(logPath))){
                if(!QFile::remove(logPath)){
					ard::errorBox(this, QString("Failed to delete file '%1', please restart your device").arg(logPath));
                }
                close();
              
            }
        }); 
#endif

    ard::addBoxButton(h_buttons, "Share", [&]() {
        QString logPath = get_program_appdata_log_file_name();
        if (QFile::exists(logPath)) {
            if (!ard::google()) {
				ard::errorBox(this, "Google module not enabled.");
                return;
            }
            auto s = QString("Please confirm sharing Ardi log file '%1'. The file will be uploaded to your cloud folder for further processing by support team. Please make sure you have reliable network connection.").arg(logPath);
            if (ard::confirmBox(this, s))
            {
                m_uploadInProgress = true;
                qWarning() << s;
                QString os_info = get_prog_os();
                QString log_name = QString("ardi-on-%1-%2.log").arg(os_info).arg(QDate::currentDate().toString(Qt::ISODate));
                std::pair<QString, QString> r = ard::google()->gdrive()->shareFile(logPath, log_name, get_cloud_support_folder_name(), "text/plain");
                //m_progress_bar->setVisible(false);
                if (!r.first.isEmpty() && !r.second.isEmpty()) 
                {
                    m_uploadInProgress = false;
                    s = "Log file placed on cloud and shared for everyone. Please copy/paste link and provide it to Ardi support team. Press 'OK' to open link.";
                    qWarning() << s;
                    qWarning() << r.second;
                    auto r2 = EditBox::edit(r.second, s, true, true);
                    if (r2.first && !r2.second.isEmpty()) {
                        QDesktopServices::openUrl(QUrl(r2.second));
                    }
                }
            }
        }
    });

    ard::addBoxButton(h_buttons, "Cancel", [&]() {close(); });
    vl_main->addLayout(h_buttons);

    connect(m_archive_view, &QAbstractItemView::doubleClicked, [=](const QModelIndex &idx) 
    {
        auto dm = m_archive_view->model();
        if (dm) {
            auto arch_root = get_sync_log_archives_path();
            auto arch_file = dm->data(idx).toString();
            m_log_dir_combo->addItem(arch_file, arch_root+arch_file);
            m_log_dir_combo->setCurrentIndex(m_log_dir_combo->count() - 1);
            m_tab->setCurrentIndex(1);
        }
    });


    if(tabSel != -1)
        {
            m_tab->setCurrentIndex(tabSel);
        }

    setLayout(vl_main);

    gui::resizeWidget(this, QSize(900, 600));
    setAttribute(Qt::WA_DeleteOnClose);

    buildSyncDirListCombo();
    loadWorkingLogFile();

    exec();
};

void LogView::closeEvent(QCloseEvent *e) 
{
    if (m_uploadInProgress) {
		ard::messageBox(this, "Please wait to complete log upload.");
        return;
    }

    QDialog::closeEvent(e);
};

void LogView::add2log(QString s, QPlainTextEdit* e)
{
    e->appendPlainText(s.trimmed());
};

void LogView::loadFile(QString logPath, QPlainTextEdit* e)
{
    e->setPlainText("");

    QFile f(logPath);
    if (!f.open(QIODevice::ReadOnly)) {
        add2log("Log file not found" + logPath, e);
        return;
    }
    add2log(QString("%1").arg(logPath), e);
    if (f.size() > 1024)
    {
        add2log(QString("Log size %1 (KB)").arg(f.size() / 1024), e);

        if (f.size() > 1024 * 300)
        {
            int newPos = static_cast<int>(f.size()) - 100 * 1024;
            add2log(QString("Log file is too big for parsing. Reading at pos %1 bytes").arg(newPos),e );
            if (!f.seek(newPos))
            {
                add2log(QString("ERROR failed to seek to pos: %1").arg(newPos), e);
            };
        }
    }
    else
    {
        add2log(QString("Log size %1 (Bytes)").arg(f.size()), e);
    }

    QString s = f.readLine();
    while (!f.atEnd())
    {
        add2log(s, e);
        s = f.readLine();
    }
    f.close();
};

void LogView::loadWorkingLogFile()
{
    QString logPath = get_program_appdata_log_file_name();
    loadFile(logPath, m_working_log);
};

void LogView::loadSyncLogFile(QString logPath)
{
    loadFile(logPath, m_sync_log);
};

QStandardItemModel* LogView::generateArchLogModel()
{
    QStandardItemModel* sm{ nullptr };

    QStringList column_labels;
    column_labels.push_back("file name");
    sm = new QStandardItemModel();
    sm->setHorizontalHeaderLabels(column_labels);

    int row = 0;

    QDir log_dir(get_sync_log_archives_path());
    QFileInfoList flist = log_dir.entryInfoList(QDir::Files, QDir::Time);

#define MAX_FILES_NUM2LOAD 200
    for (int i = 0; i < flist.size(); i++)
    {
        QFileInfo fi = flist.at(i);
        QString log_name = fi.fileName();
        if (!fi.baseName().isEmpty())
        {
            auto it = new QStandardItem(log_name);
            auto path = fi.absoluteFilePath();
            it->setData(path);
            sm->setItem(row, 0, it);
            row++;

            if (row > MAX_FILES_NUM2LOAD)break;
        }
    }

#undef MAX_FILES_NUM2LOAD

    return sm;
};

void LogView::loadSyncLogArchives()
{
    auto m = generateArchLogModel();
    if (m) {
        m_archive_view->setModel(m);
        QHeaderView* h = m_archive_view->horizontalHeader();
        int columns = m->columnCount();
        for (int i = 0; i < columns; i++) {
            h->setSectionResizeMode(i, QHeaderView::Stretch);
        }
    }
};

void LogView::buildSyncDirListCombo()
{
    extern QString get_tmp_sync_dir_prefix();
    const QString logPrefix = get_tmp_sync_dir_prefix();//QString("tmp_sync_area4");

    QDir d(ard::logDir());
    QStringList filters;
    filters << logPrefix + "*";

    QFileInfoList el = d.entryInfoList(filters, QDir::Dirs);
    QString sdb_book_name = dbp::currentDBName();

    int comboIndex2Select = -1, currentDb2Select = -1;

    for (QFileInfoList::iterator i = el.begin(); i != el.end(); i++)
    {
        QFileInfo fi = *i;
        QString s = fi.fileName();
        QString p = fi.absoluteFilePath() + "/sync.log";
        int idx = s.indexOf(logPrefix, 0, Qt::CaseInsensitive);
        if (idx == 0)
        {
            s = s.mid(logPrefix.length());
        }

        m_log_dir_combo->addItem(s, p);

        idx = s.indexOf(sdb_book_name, 0, Qt::CaseInsensitive);
        if (idx != -1) {
            currentDb2Select = m_log_dir_combo->count() - 1;
        }       

        if (!m_logDirPrefix2Select.isEmpty())
        {
            QString lpath = get_sync_log_path(m_logDirPrefix2Select);
            QFileInfo fi1(p);
            QFileInfo fi2(lpath);
            if (fi1 == fi2)
            {
                comboIndex2Select = m_log_dir_combo->count() - 1;
            }
        }
    }

    if (comboIndex2Select != -1)
    {
        m_log_dir_combo->setCurrentIndex(comboIndex2Select);
    }
    else 
    {
        if (currentDb2Select != -1) 
        {
            m_log_dir_combo->setCurrentIndex(currentDb2Select);
        }
    }
};

void LogView::SyncDirCurrentIndexChanged(int idx)
{    
    QString slog = m_log_dir_combo->itemData(idx).toString();
    loadSyncLogFile(slog);
};



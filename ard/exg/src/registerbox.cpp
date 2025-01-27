#include <QLabel> 
#include <QDebug> 
#include <QTextEdit> 
#include <QVBoxLayout>
#include <QPushButton>
#include <QDesktopServices>
#include <QUrl>
#include <QCryptographicHash>
#include <QFile>
#include <QTime>
#include <QApplication>
#include <QDesktopWidget>
#include <QCheckBox>
#include <QLineEdit>
#include <QFileInfo>
#include <QStandardItemModel>
#include <time.h>
#include <QTableView>
#include <QHeaderView>
#include <QClipboard>
#include <QDialog>
#include <csignal>
#include <QFileDialog>
#include <QPlainTextEdit>

#include "snc-tree.h"
#include "registerbox.h"
#include "a-db-utils.h"
#include "syncpoint.h"
#include "ansyncdb.h"
#include "dbp.h"
#include "ChoiceBox.h"
#include "custom-widgets.h"
#include "custom-boxes.h"
#include "email_draft.h"
#include "email.h"
#include "ethread.h"
#include "GoogleClient.h"
#include "gmail/GmailRoutes.h"
#include "ios/test-view.h"
#include "db-merge.h"
#include "fileref.h"
#include "ansearch.h"
#include "csv-util.h"
#include "rule_runner.h"
#include "locus_folder.h"

extern QDate projectCompilationDate();
extern void exit_app();


static bool instance_read_only_mode = false;
bool is_instance_read_only_mode(){return instance_read_only_mode;}

QString get_app_version_as_string()
{
  QString rv = QString("%1").arg(get_app_version_as_int());
  rv.insert(1, ".");
  return rv;
}

QString get_app_version_moto()
{
  return "Quest for Fire";
}

QString get_app_version_as_long_string()
{
  QString rv = QString("%1 - '%2'").arg(get_app_version_as_string()).arg(get_app_version_moto());
  return rv;
}

QString get_prog_os()
{
#ifdef Q_OS_WIN
    return "win";  
#else    
#ifdef Q_OS_MAC
    #ifdef Q_OS_IOS
        return "ios";
    #else
        return "mac";
    #endif
#else
    return "linux";
#endif
#endif
    //return "undef";
}

/**   
   SupportWindow
*/
SupportWindow* supportWnd = nullptr;

void SupportWindow::runWindow()
{
    new SupportWindow;
};

SupportWindow::NAME_2_FUNCTION SupportWindow::m_commands;
SupportWindow::NAME_2_DESC SupportWindow::m_command2descr;

SupportWindow::SupportWindow()
{
    supportWnd = this;
    //m_list_model_initialized = false;

    QVBoxLayout *v = new QVBoxLayout;
    m_info_label = new QLabel(QString("This option is designed for processing of special support tickets and testing specific cases. Normally you shouldn't invoke the command and usage is limited to special beta test group. Press 'Cancel' unless you were specifically instructed.\nCurrent debug level: %1").arg(dbp::configFileSupportCmdLevel()));
    m_info_label->setWordWrap(true);
    v->addWidget(m_info_label);
    m_commands_table = new ArdTableView();
    m_commands_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    v->addWidget(m_commands_table);
    ENABLE_OBJ(m_commands_table, false);

    QHBoxLayout *h1 = new QHBoxLayout;
    h1->addWidget(new QLabel("Command:"));
    m_edit_cmd = new QLineEdit;
    h1->addWidget(m_edit_cmd);
    v->addLayout(h1);

    m_butons_layout = new QHBoxLayout;
    QPushButton* b = nullptr;

    m_copy_button = nullptr;

    b = new QPushButton(this);
    b->setText("Cancel");
    gui::setButtonMinHeight(b);
    connect(b, SIGNAL(released()), this, SLOT(cancelWindow()));
    m_butons_layout->addWidget(b);

    b = new QPushButton(this);
    b->setText("Run");
    gui::setButtonMinHeight(b);
    connect(b, SIGNAL(released()), this, SLOT(acceptCommand()));
    b->setDefault(true);
    b->setAutoDefault(true);
    m_butons_layout->addWidget(b);

    v->addLayout(m_butons_layout);
    setLayout(v);


#define ADDFUNC(N, F, D)m_commands[N] = [=](QString s){return F(s);};m_command2descr[N]=D;
    ADDFUNC("extra", run_commandEnableExtraFeatures, "Support commands level. Show extra(experimental/admin) commands");
    ADDFUNC("syid", run_commandFindBySyid, "find item by SYID");
    ADDFUNC("id", run_commandFindByID, "find item by ID");
    ADDFUNC("sql", run_commandExecSQL, "execute SQL");
    ADDFUNC("atest", run_commandAutoTest, "run autotest procedure");

    ADDFUNC("debug-draw", run_commandToggleDrawDebugHint, "draw debug hint symbols");
    ADDFUNC("drop-ini", run_commandDropIniFile, "reset program settings to default");
    ADDFUNC("hash", run_commandPrintHashTable, "print hash strings on items");
    ADDFUNC("print-current", run_commandPrintCurrent, "print current item info into log");
    //ADDFUNC("exit", run_commandExit, "exit application");
    //ADDFUNC("abort", run_commandAbort, "abort application - program may core, depending on evironment");
    ADDFUNC("throw", run_commandRaiseException, "throw exception - program may core, depending on evironment");
    ADDFUNC("signal", run_commandSEGSignal, "signal SIGSEGV - program may core, depending on evironment");


    ADDFUNC("regenerate-db-indexes", run_commandRecreateIndexes, "drop and recreate all DB indexes");
    ADDFUNC("clear-sync-token", run_commandClearSyncToken, "force to clear SYNC token. User will be forced to authorize program again.");

    ADDFUNC("gm-set-msg-id", run_commandGmSetCurrMsgId, "Set current email-dbg-msg ID");
    ADDFUNC("gm-find-msg-id", run_commandGmFindByMsgId, "Find email by current email-dbg-msg ID");
    ADDFUNC("gm-print-headers", run_commandGmPrintHeaders, "print headers for current email-dbg-msg");
    ADDFUNC("gm-hist", run_commandGmHistory, "check history");

    ADDFUNC("threads", run_commandShowThreads, "print adopted threads");
    ADDFUNC("gm-disable", run_commandDisableGoogle, "disable google account");
	ADDFUNC("gm-gdrive", run_commandGmRequestGDrive, "request gdrive scope access");

    ADDFUNC("diagnostics", run_commandShowDiagnostics, "show diagnostics");
    ADDFUNC("labels", run_commandViewLabels, "view label");
    ADDFUNC("contacts", run_commandViewContacts, "view contacts");
    ADDFUNC("export-contacts", run_commandExportContacts, "export contacts");
    ADDFUNC("export-uncrypted", run_commandExportUnEncrypted, "export non-encrypted DB (v.1)");
    ADDFUNC("demo-build-contacts", run_commandTestGenerateContacts, "Generate demo contacts");
    ADDFUNC("json-export", run_commandExportJSON, "export to JSON");
    ADDFUNC("json-import", run_commandImportJSON, "import(merge) from JSON");
    ADDFUNC("crypto-test", run_commandTestCrypto, "crypto test");
    ADDFUNC("recover-hint", run_commandRecoverPwdHint, "recover pwd hint");
    ADDFUNC("terminal-gdrive", run_commandGDriveTerminal, "run gdrive terminal");
	ADDFUNC("file", run_commandOpenFile, "open Ardi file");
	ADDFUNC("tda", run_commandInsertTDA, "read TDA statement file");
	ADDFUNC("supplied", run_commandImportSupplied, "import supplied demo data");
#undef ADDFUNC


    gui::resizeWidget(this, QSize(400, 600));
	m_edit_cmd->setFocus();
    exec();
};

SupportWindow::~SupportWindow() 
{
    supportWnd = nullptr;
};

void SupportWindow::cancelWindow()
{
  close();
};

void SupportWindow::acceptCommand()
{
    QString cmd = m_edit_cmd->text();

    if (cmd.indexOf("ls", 0, Qt::CaseInsensitive) == 0) {
        ListCommands();
        return;
    }

    bool command_recognized;
    if (run_command(cmd, command_recognized))
    {
        accept();
    }

    if (!command_recognized)
    {
		ard::messageBox(this, QString("Command not recognized: %1").arg(cmd));
    }
};

bool SupportWindow::run_command(QString cmd, bool& command_recognized)
{

  command_recognized = false;

  for (auto& k : m_commands) {
      if (cmd.indexOf(k.first, 0, Qt::CaseInsensitive) == 0)
      {
          command_recognized = true;
          QString param = "";
          if (cmd.indexOf(":") != -1)
          {
              QString s2 = k.first + ":";
              param = cmd.mid(s2.length());
          }

          if (!k.second(param))
              return false;

          return true;
      }
  }

  return true;
};


bool SupportWindow::run_commandEnableExtraFeatures(QString _param)
{
  int r = _param.toInt();
  if (r > 0)
    {
      dbp::configFileSetSupportCmdLevel(1);
	  ard::messageBox(this, QString("Command accepted. Set support level: %1. You need to restart application to apply changes.").arg(dbp::configFileSupportCmdLevel()));
    }
  else
    {
      dbp::configFileSetSupportCmdLevel(0);
	  ard::messageBox(this, QString("Command accepted. Set support level: %1. You need to restart application to apply changes.").arg(dbp::configFileSupportCmdLevel()));
    }
  return true;
};

bool SupportWindow::run_commandFindBySyid(QString syid)
{
	bool dbfound = false;
	bool ok = gui::locateBySYID(syid, dbfound); //model()->locateBySYID(syid, eo, dbfound);
	if (!ok)
	{
		if (dbfound)
		{
			ard::messageBox(this, QString("DB record found but can't find item in the outline by SYID. The DB object could be orphaned."));
			ASSERT(0, "failed to locate item by SYID") << syid;
			return false;
		}
		else
		{
			ard::messageBox(this, QString("Failed to locate item in database by SYID %1. Item was deleted or SYID was changed").arg(syid));
			return false;
		}
	}

	return true;
};

bool SupportWindow::run_commandFindByID(QString ids)
{
    DB_ID_TYPE id = ids.toInt();
    auto f = ard::lookup(id);
    if(f)
    {
        bool locate_ok = gui::ensureVisibleInOutline(f);
        if(!locate_ok)
        {
			ard::messageBox(this, QString("DB record found but can't find item in the outline. The DB object could be orphaned."));
            ASSERT(0, "failed to locate item by SYID") << ids;
            return false;
        }//locate_ok
    }
    else
    {
		ard::messageBox(this, QString("Failed to locate item in database by DBID %1").arg(ids));
      return false; 
    }
    return true;
};

bool SupportWindow::run_commandCompressDB(QString db_path)
{
  qDebug() << "compress:" << db_path;
  QFileInfo fi(db_path);
  if(!fi.exists())
    {
      ard::messageBox(this,QString("File not found '%1'").arg(db_path));
      return false;
    }
  QString tmp_db = fi.canonicalPath() + "/" + fi.baseName() + ".qpk";
  if(QFile::exists(tmp_db))
    {
      QString s = QString("File on the way %1").arg(tmp_db);
	  ard::messageBox(this, s);
      return false;
    };
  if(SyncPoint::compress(db_path, tmp_db))
    {
      QString s = QString("Finished compressing DB '%1' into '%2'").arg(db_path).arg(tmp_db);
	  ard::messageBox(this, s);
    }
  else
    {
      QString s = QString("Failed to compress DB '%1' into '%2'").arg(db_path).arg(tmp_db);
	  ard::messageBox(this, s);
      return false;
    }

  return true;
};

bool SupportWindow::run_commandExecSQL(QString )
{
  gui::runSQLRig();
  return true;
};

extern bool runSyncAutoTest(int test_level, QString current_sync_pwd, QString kring_pwd);

bool SupportWindow::run_commandAutoTest(QString _param)
{
    if (!ard::isDbConnected()) {
		ard::messageBox(this, "expected connected DB");
        return false;
    }

    if(_param.isEmpty()){
		ard::errorBox(this, "Specify test level (0,1,2,3..)");
        return false;
    }

  int test_level = _param.toInt();

  std::vector<QString> labels;
  labels.push_back("Data Password");
  labels.push_back("KeyRing Master Password");

  auto res = gui::edit(labels, 
                    "Please enter Main data password and KRing Master password. <b><font color=\"red\">Please write down both passwords</font></b> as autotest procedure will be adjusting them to emulate password change.",
                    "Please confirm AUTOTEST procedure. Cancel if not sure.");
  if (res.empty()) {
      return false;
  }
  QString pwd, kring_pwd;
  pwd = res[0];
  kring_pwd = res[1];

  if (pwd.isEmpty() || kring_pwd.isEmpty())
  {
      return false;
  }

  runSyncAutoTest(test_level, pwd, kring_pwd);
  return true;
};

bool SupportWindow::ListCommands()
{
    ENABLE_OBJ(m_info_label, false);
    ENABLE_OBJ(m_commands_table, true);

    if (!m_commands_table->model())
    {
        //   m_list_model_initialized = true;

        QStringList column_labels;
        column_labels.push_back("command");
        column_labels.push_back("desc");

        QStandardItemModel* sm = new QStandardItemModel();
        sm->setHorizontalHeaderLabels(column_labels);

        int row = 0;
        //for(NAME_2_FUNCTION::iterator k = m_commands.begin();k != m_commands.end(); k++)
        for (auto& k : m_commands) {
            QString s = k.first;
            NAME_2_DESC::iterator j = m_command2descr.find(s);
            sm->setItem(row, 0, new QStandardItem(s));
            sm->setItem(row, 1, new QStandardItem(j->second));
            row++;
        }

        m_commands_table->setModel(sm);
        m_commands_table->setWordWrap(true);
        m_commands_table->setTextElideMode(Qt::ElideMiddle);
        m_commands_table->resizeRowsToContents();

        QHeaderView* h = m_commands_table->horizontalHeader();
        h->setSectionResizeMode(1, QHeaderView::Stretch);

        //gui::resizeWidget(this, QSize(width(), height() + 400));
    }

    if (!m_copy_button)
    {
        m_copy_button = new QPushButton(this);
        m_copy_button->setText("Copy");
        gui::setButtonMinHeight(m_copy_button);
        connect(m_copy_button, SIGNAL(released()),
            this, SLOT(copyCommand()));
        m_butons_layout->insertWidget(0, m_copy_button);
    }

    m_edit_cmd->setText("");

    return true;
};

void SupportWindow::copyCommand()
{
  QItemSelectionModel *sm = m_commands_table->selectionModel();
  if(sm)
    {
      int rowidx = sm->currentIndex().row();
      QString scmd = m_commands_table->model()->index(rowidx , 0).data().toString() + ":";
      m_edit_cmd->setText(scmd);
    }
};



bool SupportWindow::run_commandForceModify(QString )
{
    
    ard::messageBox(this,"Command deleted.");
  return true;
};

QString syncCommand2String(SyncAuxCommand c)
{
  QString rv;
  switch(c)
    {
    case scmdPrintDataOnHashError:
      {
    rv = "print-data-4-hash";
      }break;
    default: rv = "UKNOWN";
    }
  return rv;
}

bool SupportWindow::run_commandClearSyncToken(QString)
{
#ifdef ARD_GD
    ard::revokeGoogleTokenWithConfirm();
#else
    ard::messageBox(this,"SYNC module is not defined");
#endif
    return true;
};

bool SupportWindow::run_commandGmSetCurrMsgId(QString) 
{
    auto f = ard::currentTopic();
    if (f) {
        email_ptr e = dynamic_cast<email_ptr>(f);
        ethread_ptr t = nullptr; //dynamic_cast<ethread_ptr>(f);
        if (e) {
            t = e->parent();
            if (!t) {
                QString s = QString("Selected email message '%1' has no parent thread. It's not allowed.").arg(e->optr()->id());
                return false;
            }
        }
        else {
            t = dynamic_cast<ethread_ptr>(f);
            if (!t) {
                ard::messageBox(this,"Select email or thread to proceed.");
                return false;
            }
            else {
                e = t->headMsg();
                if (!e) {
                    ard::messageBox(this,"Selected thread has no head message.");
                    return false;
                }
            }
        }

        if (e) {
            QString s = QString("Confirm selecting current email-dbg msg-id=[%1] thread-id=[%2] %3")
                .arg(e->optr()->id())
                .arg(t->optr()->id())
                .arg(e->title());
            if (ard::confirmBox(this, s)) {
                std::pair<QString, QString> tm = { t->optr()->id(), e->optr()->id() };
                dbp::configFileSetCurrDbgMsgId(tm);
                ard::messageBox(this,QString("Current email-dbg-msg: %1 %2").arg(e->optr()->id()).arg(e->title()));
            }
        }
        else {
			ard::messageBox(this, "Select email to proceed.");
        }
    }
    else {
		ard::messageBox(this, "Select email to proceed.");
    }


    return true;
};

email_ptr SupportWindow::GmFindCurrDbgMsg()
{
    auto& eid = dbp::configFileCurrDbgMsgId();
    if (eid.first.isEmpty() ||
        eid.second.isEmpty()) {
        ard::messageBox(this,"Curent email-dbg-msg is not defined.");
        return nullptr;
    }

    if (!ard::confirmBox(this, QString("Confirm searching for current thread-id=[%1] email-dbg-msg=[%2]").arg(eid.first).arg(eid.second))) {
        return nullptr;
    };

    auto r = ard::gmail();
    if (!r || !r->cacheRoutes()) {
        ard::messageBox(this,"Gmail model not detected");
        return nullptr;
    }
    auto storage = r->cacheRoutes()->storage();
    if (!storage) {
        ard::messageBox(this,"Gmail cache data storage is not defined");
        return nullptr;
    }

    QString userId = dbp::configEmailUserId();
    if (userId.isEmpty()) {
        ard::messageBox(this,"Gmail userId is not defined");
        return nullptr;
    }
    return nullptr;
};

bool SupportWindow::run_commandGmFindByMsgId(QString) 
{
    email_ptr e = GmFindCurrDbgMsg();
    if (e) {
        ard::messageBox(this,QString("Message located %1 %2")
            .arg(e->optr()->id())
            .arg(e->title()));
        gui::ensureVisibleInOutline(e);
    }

    return true;
};

bool SupportWindow::run_commandGmPrintHeaders(QString s_param) 
{
    bool print_all_headers = false;
    if (!s_param.isEmpty() && s_param.toInt() > 0) {
        print_all_headers = true;
    }

    auto&  eid = dbp::configFileCurrDbgMsgId();
    if (eid.first.isEmpty() ||
        eid.second.isEmpty()) {
        ard::messageBox(this,"Curent email-dbg-msg is not defined.");
        return false;
    }

    auto r = ard::gmail();
    if (!r) {
        ard::messageBox(this,"Gmail model not detected");
        return false;
    }

    QString userId = dbp::configEmailUserId();
    if (userId.isEmpty()) {
        ard::messageBox(this,"Gmail userId is not defined");
        return false;
    }

    try
    {
        googleQt::gmail::IdArg mid(userId, eid.second);
        mid.setFormat("minimal");
        auto m = r->getMessages()->get(mid);
        QString s;
        s += QString("id=%1\n").arg(m->id());
        s += QString("tid=%1\n").arg(m->threadid());
        s += QString("snippet=%1\n").arg(m->snippet()).left(128);

        auto p = m->payload();
        auto header_list = p.headers();
        s += QString("headers-count=%1\n").arg(header_list.size());
        if (print_all_headers) {
            for (auto h : header_list) {
                QString s2 = QString("%1=%2\n").arg(h.name()).arg(h.value());
                s += s2;
            }
        }
        else {
            static std::set<QString> headers_to_print = { "From", "To", "Subject", "CC", "BCC" };
            for (auto h : header_list){
                if (headers_to_print.find(h.name()) != headers_to_print.end()) {
                    QString s2 = QString("%1=%2\n").arg(h.name()).arg(h.value());
                    s += s2;
                }
            }
        }
        ard::messageBox(this,s);
    }
    catch (googleQt::GoogleException& e)
    {
        ard::messageBox(this,QString("Error: %1").arg(e.what()));
    }

    return true;
};

bool SupportWindow::run_commandGmHistory(QString ) 
{
    auto r = ard::gmail();
    if (!r) {
        ard::messageBox(this,"Gmail model not detected");
        return false;
    }

    auto st = ard::gstorage();
    if (!st) {
        ard::messageBox(this,"Gmail model storage not detected");
        return false;
    }

    QString userId = dbp::configEmailUserId();
    if (userId.isEmpty()) {
        ard::messageBox(this,"Gmail userId is not defined");
        return false;
    }

    try
    {
        auto lastHistId = st->lastHistoryId();

        QString s;
        googleQt::gmail::HistoryListArg histArg(userId, st->lastHistoryId());
        histArg.setMaxResults(10);
        auto hlst = r->getHistory()->list(histArg);
        s = QString("[last=%1] hist-size: %2\n").arg(lastHistId).arg(hlst->history().size());
        for (auto h : hlst->history())
        {
            QString s1 = QString("%1 msg=%2 added=%3\r")
                .arg(h.id())
                .arg(h.messages().size())
                .arg(h.messagesadded().size());
            s += s1;
        }
        ard::messageBox(this,s);
    }
    catch (googleQt::GoogleException& e)
    {
        ard::messageBox(this,QString("Error: %1").arg(e.what()));
    }

    return true;
};

bool SupportWindow::run_commandDisableGoogle(QString)
{
    auto m = ard::gmail_model();
    if (!m) {
        ard::messageBox(this,"Gmail model not detected");
        return false;
    }

    m->disableGoogle();

    return true;
}

bool SupportWindow::run_commandGmRequestGDrive(QString _param) 
{
	if (!gui::isConnectedToNetwork()) {
		ard::messageBox(this,"Network connection not detected.");
		return false;
	}
	extern bool g__request_gdrive_scope_access;
	if (!ard::confirmBox(this, "Please confirm authorization of GDrive access. You will be requested to confirm authorization on Google page. Please note that this is optional feature not required by normal execution of the program."))
	{
		return false;
	}

	g__request_gdrive_scope_access = true;
	ard::authAndConnectNewGoogleUser();
	return true;
};

bool SupportWindow::run_commandShowThreads(QString)
{
    AdoptedThreadsBox::showThreads();
    return true;
}

bool SupportWindow::run_commandShowDiagnostics(QString) 
{
    DiagnosticsBox::showDiagnostics();
    return true;
};

bool SupportWindow::run_commandViewLabels(QString)
{
    LabelsBox::showLabels();
    return true;
};

bool SupportWindow::run_commandViewContacts(QString ) 
{
    ContactsBox::showContacts();
    return true;
};

bool SupportWindow::run_commandExportContacts(QString _param) 
{
    ard::ArdiCsv::guiExportAllContactsGroups(supportWnd);
    return true;
};


bool SupportWindow::run_commandTestGenerateContacts(QString ) 
{
    if (ard::confirmBox(this, "Please confirm generating demo contacts groups. You can later delete contacts and groups."))
    {
        extern std::pair<int, int> generate_demo_contacts();
        generate_demo_contacts();
    }
    return true;
};

bool SupportWindow::run_commandExportUnEncrypted(QString )
{
    ArdDB* db = ard::db();
    assert_return_false(db, "expected DB");
    assert_return_false(db->isOpen(), "expected DB");

    //..
    QString dir = QFileDialog::getExistingDirectory(gui::mainWnd(),
        "Select Directory to export",
        dbp::configFileLastShellAccessDir(),
        QFileDialog::ShowDirsOnly
        | QFileDialog::DontResolveSymlinks);
    if (!dir.isEmpty()) {
        dbp::configFileSetLastShellAccessDir(dir, false);
        QString name = "ardi_plain_export.qpk";
        QString fullPath = dir + "/" + name;
        int idx = 1;
        while (QFile::exists(fullPath)) {
            name = QString("ardi_plain_export%1.qpk").arg(idx);
            fullPath = dir + "/" + name;
            idx++;
            if (idx > 200) {
				ard::errorBox(this, "Too many backup files in same folder. Aborted.");
                return false;
            }
        }

        if (db->backupDB(fullPath)) {
            ard::messageBox(this, QString("Export completed - '%1'").arg(fullPath));
        }
        else {
            ard::messageBox(this, "Export failed.");
        }
    }

    //..
    return true;
};

bool SupportWindow::run_commandOpenFile(QString)
{
	ard::selectArdiDbFile();
	return true;
};

bool SupportWindow::run_commandExportJSON(QString) 
{
    ArdiDbMerger::guiExportToJSON();
    return true;
};

bool SupportWindow::run_commandImportJSON(QString) 
{
    ArdiDbMerger::guiImportFromJSON();
    return true;
};

bool SupportWindow::run_commandTestCrypto(QString)
{
#ifdef ARD_OPENSSL
#define MAX_TEST_DATA 900000
    int idx_test_count = 0;
    bool test_passed = true;
    size_t total_data_size = 0;
    size_t total_enc_size = 0;
    QString str2encrypt;
    str2encrypt.reserve(MAX_TEST_DATA);
    for (int i = 0; i < 300; i++) {
        str2encrypt += QString("some long string..123-test-%1").arg(i);
        QByteArray data = str2encrypt.toStdString().c_str();
        QString pwd = QString("pwd123123-%1").arg(i);
        auto er = ard::aes_encrypt_archive(pwd, QString("hint-%1").arg(i), data);
        auto dr = ard::aes_decrypt_archive(pwd, er.second);
        if (data.size() != dr.second.size() || data != dr.second) 
        {
            ASSERT(0, "descrypt error");
            qDebug() << i << ". encr-err";
            test_passed = false;
        }
        else {
            total_data_size += data.size();
            total_enc_size += er.second.size();
        }
        idx_test_count++;

        if (str2encrypt.size() > MAX_TEST_DATA) {
            str2encrypt = QString::fromLatin1(er.second.toBase64()).left(1024);
        }
        else {
            str2encrypt += QString::fromLatin1(er.second.toBase64());
        }
    }
    if (test_passed) {
        extern QString size_human(float num);
        ard::messageBox(this,QString("Crypto test passed [%1 tests] [data: %2] [enc: %3]")
                            .arg(idx_test_count)
                            .arg(size_human(total_data_size))
                            .arg(size_human(total_enc_size)));
    }
    else {
        ard::messageBox(this,"Crypto test failed.");
    }
#else
    ard::messageBox(this,"OPENSSSL module not compiled in");
#endif //ARD_OPENSSL
    return true;
};

bool SupportWindow::run_commandRecoverPwdHint(QString)
{
#ifdef ARD_OPENSSL
    QString fileName = QFileDialog::getOpenFileName(gui::mainWnd(),
        "Select Ardi file",
        dbp::configFileLastShellAccessDir(),
        "Ardi Data Files  (*.qpk);; AES archives (*.aes)");

    if (!fileName.isEmpty()) {
        auto r = ard::aes_archive_recover_hint(fileName);
        if (r.first == ard::aes_status::ok) {
            ard::messageBox(this,r.second);
        }
        else if (r.first == ard::aes_status::corrupted) {
			ard::errorBox(this, "Selected is probably corrupted and can't be recovered. Please consider backup version.");
            ard::messageBox(this,r.second);
        }
    }
#else
    ard::messageBox(this,"OPENSSSL module not compiled in");
#endif //ARD_OPENSSL
    return true;
};

bool SupportWindow::run_commandDropIniFile(QString )
{
  extern QString configFilePath();

  if(!ard::confirmBox(this, "Please confirm reset setting to default values. You will have to manualy restart application to apply the changes."))
    {
      return false;
    }
  
  QString sfile = configFilePath();
  if(QFile::exists(sfile))
    {
      if(!QFile::remove(sfile))
    {
      QString s = QString("Failed to delete config file '%1'").arg(sfile);
      ASSERT(0, s);
      ard::messageBox(this,s);
      return false;
    }
    }
  
  ard::messageBox(this,"Please restart to apply the changes.");

  return true;
};


bool SupportWindow::run_commandToggleDrawDebugHint(QString ) 
{
    if (dbp::configFileGetDrawDebugHint()) {
        if (ard::confirmBox(this, "Debug Draw is ON. Please confirm disable debug draw.")) {
            dbp::configFileSetDrawDebugHint(false);
        }
        else {
            return false;
        }
    }
    else {
        if (ard::confirmBox(this, "Debug Draw is OFF. Please confirm enable debug draw.")) {
            dbp::configFileSetDrawDebugHint(true);
        }
        else {
            return false;
        }
    }

    gui::rebuildOutline(gui::currPolicy(), true);
    return true;
};

static void buildRootHashTable(topic_ptr root, QStandardItemModel* sm, int& row)
{
  snc::MemFindAllPipe mp;
  root->memFindItems(&mp);  
  for(auto& i : mp.items())  {
      topic_ptr it = dynamic_cast<topic_ptr>(i);
      if(it)
    {
      sm->setItem(row, 0, new QStandardItem(QString("%1").arg(it->id())));
      sm->setItem(row, 1, new QStandardItem(it->syid()));
      sm->setItem(row, 2, new QStandardItem(it->title()));
      sm->setItem(row, 3, new QStandardItem(it->calcContentHashString()));
      sm->setItem(row, 4, new QStandardItem(it->objName().left(1)));
      row++;
      qDebug() << "hash" << it->id() << it->syid() << it->title() << it->calcContentHashString() << it->objName().left(1);
    }      
    } 
};

bool SupportWindow::run_commandPrintHashTable(QString )
{
  if(!gui::isDBAttached())
    {
      ard::messageBox(this,"expected attached DB");
      return false;
    }

  QStringList column_labels;
  column_labels.push_back("ID");
  column_labels.push_back("SYID");
  column_labels.push_back("Title");
  column_labels.push_back("Hash");
  column_labels.push_back("Type");

  QStandardItemModel* sm = new QStandardItemModel();
  sm->setHorizontalHeaderLabels(column_labels);


  int row = 0;
  qDebug() << "======= begin hash table ============";
  buildRootHashTable(ard::root(), sm, row);
  qDebug() << "======= end hash table ============";
  TableBox::showModel(sm, "hash Table", 0);

  return true;
};



bool SupportWindow::run_commandPrintCurrent(QString _param)
{
  if(!gui::isDBAttached())
    {
      ard::messageBox(this,"expected attached DB");
      return false;
    }

  auto t = ard::currentTopic();
  if(!t)
    {
      ard::messageBox(this,"select topic to proceed");
      return false;
    }  

  QString topic_info = t->dbgHint();
  QString ver_info = get_app_version_as_string();
  QString date_info = QDate::currentDate().toString();
  
  QString s = QString("[%1] %2, %3, %4").arg(_param).arg(date_info).arg(ver_info).arg(topic_info);
  ard::messageBox(this,s);
  extern void writeToMainLog(const QString& in_msg, QtMsgType type);
  writeToMainLog(s, QtWarningMsg);
  //ASSERT(0, s);

  return true;
};

bool SupportWindow::run_commandInsertTDA(QString _param) 
{
	QString fileName = QFileDialog::getOpenFileName(gui::mainWnd(),
		"Select TDA Account statement file",
		dbp::configFileLastShellAccessDir(),
		"TDA Files  (*.csv);; ");

	if (!fileName.isEmpty())
	{
		dbp::configFileSetLastShellAccessDir(fileName, true);

		auto p = ard::hoisted();
		if (!p) {
			p = ard::Sortbox();
		}

		auto r = new ard::fileref("statement:" + fileName);
		if (p && p->canAcceptChild(r)) {
			p->addItem(r);
			p->ensurePersistant(1);
			gui::ensureVisibleInOutline(p);
		}
	}

	return true;
};

bool SupportWindow::run_commandImportSupplied(QString ) 
{
	extern void importSuppliedDemo();
	importSuppliedDemo();
	return true;
};

bool SupportWindow::run_commandRaiseException(QString) 
{
    if (ard::confirmBox(this, "Confirm raising std::invalid_argument exception."))
    {
        throw std::invalid_argument("Exception has arrived");
    }
    return true;
};

bool SupportWindow::run_commandSEGSignal(QString)
{
    if (ard::confirmBox(this, "Confirm sending signal SIGSEGV - invalid memory reference"))
    {
        ::raise(SIGSEGV);
    }
    return true;

};

bool SupportWindow::run_commandRecreateIndexes(QString )
{
  if(!gui::isDBAttached())
    {
      ard::messageBox(this,"expected attached DB");
      return false;
    }

  if(ard::confirmBox(this, "Confirm gereneration of database indexes"))
    {
      int indexes_created;
      ard::db()->recreateDBIndexes(indexes_created);
      ard::messageBox(this,QString("Created %1 indexes").arg(indexes_created));
    }

  return true;
};

bool SupportWindow::run_commandGDriveTerminal(QString) 
{
    extern void run_gdrive_terminal();
    run_gdrive_terminal();
    return true;
};

#ifdef Q_OS_IOS
class IOSTestViewRunner : public QDialog
{
public:
    static QString run(QString str) 
    {
        IOSTestViewRunner d(str);
        if (d.exec() == QDialog::Accepted)
        {
            str = d.m_str;
        }
        return str;
    };
protected:
    IOSTestViewRunner(QString str) 
    {
        QVBoxLayout *v_main = new QVBoxLayout;
        v_main->addWidget(new QLabel("test-view4ios"));
        m_v = new TestView;
        m_v->setStr(str);
        v_main->addWidget(m_v);

        QHBoxLayout *h_1 = new QHBoxLayout;
        ard::addBoxButton(h_1, "OK", [&]() 
        {
            m_str = m_v->getStr();
            accept(); 
        });
        ard::addBoxButton(h_1, "Cancel", [&]() {reject(); });
        v_main->addLayout(h_1);

        MODAL_DIALOG(v_main);
    };

protected:
    QLineEdit*  m_text_edit;
    QLabel*     m_label;
    QString     m_str;
    TestView*   m_v{nullptr};
};


bool SupportWindow::run_testView4IOS(QString )
{
    IOSTestViewRunner::run("testing text view ..");
    return true;
};
#endif//Q_OS_IOS

/**
   TerminalBox
*/
static ard::terminal* _tbox = nullptr;
ard::terminal* ard::terminal::tbox()
{
    return _tbox;
};

ard::terminal::terminal()
{
    QVBoxLayout *v_main = new QVBoxLayout;
    QVBoxLayout *v_top = new QVBoxLayout;
    QHBoxLayout *h_btns = new QHBoxLayout;
    QHBoxLayout *h_top = new QHBoxLayout;

    v_main->addLayout(h_top);
    h_top->addLayout(v_top);

    m_name = new QComboBox;
    v_top->addWidget(m_name);
    m_desc = new QLineEdit;
    m_desc->setReadOnly(true);
    v_top->addWidget(m_desc);


    m_arg = new QLineEdit;
    QHBoxLayout *h_arg = new QHBoxLayout;
    h_arg->addWidget(new QLabel("Argument:"));
    h_arg->addWidget(m_arg);
    v_main->addLayout(h_arg);

    m_edit = new QPlainTextEdit;
    m_edit->setReadOnly(true);
    v_main->addWidget(m_edit);

    QFont fnt(ARD_FALLBACK_DEFAULT_NOTE_FONT_FAMILY, ARD_FALLBACK_DEFAULT_NOTE_FONT_SIZE);
    auto NoteFontSize = dbp::configFileNoteFontSize();
    if (NoteFontSize >= 8 && NoteFontSize < 48) {
        fnt.setPointSize(NoteFontSize);
    }
    m_edit->setFont(fnt);

    connect(m_name, static_cast<void (QComboBox::*) (int)> (&QComboBox::currentIndexChanged), [&](int idx)
    {
        m_desc->setText("");
        auto s = m_name->currentText();
        if (!s.isEmpty()) {
            qDebug() << "ykh:command" << idx;
            auto i = m_sel_map.find(s);
            if (i != m_sel_map.end()) {
                m_desc->setText(i->second.description);
            }
        }
    });
    
    v_main->addLayout(h_btns);
    QPushButton* b = nullptr;
    b = new QPushButton("Run");
    connect(b, &QPushButton::released, this, &ard::terminal::runCommand);
    //..
    QSizePolicy sizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    sizePolicy.setHorizontalStretch(0);
    sizePolicy.setVerticalStretch(0);
    sizePolicy.setHeightForWidth(b->sizePolicy().hasHeightForWidth());
    b->setSizePolicy(sizePolicy);
    //..

    h_top->addWidget(b);
    //ADD_BUTTON2LAYOUT(h_top, "Run", &ard::terminal::runCommand);
    ADD_BUTTON2LAYOUT(h_btns, "Close", &QPushButton::close);

    _tbox = this;
    MODAL_DIALOG_SIZE(v_main, QSize(900, 900)); 
};

ard::terminal::~terminal()
{
    _tbox = nullptr;
};

void ard::terminal::addAction(QString name, QString description, std::function<void(QString)> action)
{
    Selection s;
    s.name = name;
    s.description = description;
    s.action = action;
    m_sel.push_back(s);
    m_sel_map[name.toLower()] = s;
};

void ard::terminal::buildMenu()
{
    for(const auto& i : m_sel){
        m_name->addItem(i.name);
    }
};

void ard::terminal::addSeparator()
{
    addAction("---------", "        ", [](QString){});
};

void ard::terminal::runCommand()
{
    auto s = m_name->currentText();
    if (!s.isEmpty()) {
        auto i = m_sel_map.find(s);
        if (i != m_sel_map.end()) {
            qDebug() << "running command" << s;
            if (_tbox) {
                //tbox->m_edit->appendPlainText("");
                //tbox->m_edit->appendHtml(QString("<b><font color=\"red\">%1</font></b>").arg(s));
                _tbox->m_edit->appendHtml(QString("<b><font color=\"#880015\">%1</font></b>").arg(s));
                _tbox->m_edit->appendPlainText("");
                //tbox->m_edit->appendPlainText(QString("<font color=\"black\">%1</font>").arg(""));
            }
            i->second.action(m_arg->text());
        }
    }
};

TerminalOut tout;

TerminalOut& ard::terminal::out()
{
    return tout;
};

TerminalOut& operator << (TerminalOut& to, QString str)
{
    if (_tbox) {
        _tbox->m_edit->moveCursor(QTextCursor::End);
        _tbox->m_edit->textCursor().insertText(str);
        _tbox->m_edit->moveCursor(QTextCursor::End);
    }
    return to;
};

TerminalOut& operator << (TerminalOut& to, quint64 v)
{
    if (_tbox) {
        _tbox->m_edit->moveCursor(QTextCursor::End);
        _tbox->m_edit->textCursor().insertText(QString("%1").arg(v));
        _tbox->m_edit->moveCursor(QTextCursor::End);
    }
    return to;
};

TerminalOut& operator << (TerminalOut& to, TerminalEndl) 
{
    if (_tbox) {
        _tbox->m_edit->appendPlainText("");
    }
    return to;
};


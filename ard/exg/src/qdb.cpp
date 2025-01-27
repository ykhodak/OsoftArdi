#include <QSplitter>
#include <QAction>
#include <QMenuBar>
#include <QTextEdit>
#include <QTableView>
#include <QToolBar>
#include <QTabWidget>
#include <QCloseEvent>
#include <QFileDialog>
#include <QApplication>
#include <QSqlQueryModel>
#include <QSqlQuery>
#include <QSqlError>
#include <QTextStream>
#include <QMainWindow>
#include <QDesktopWidget>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QHeaderView>
#include <iostream>

#include "qdb.h"
#include "a-db-utils.h"
#include "dbp.h"
#include "anfolder.h"
#include "custom-widgets.h"


extern QString asHtmlDocument(QString sPlainText);

void show_last_sql_err(QSqlQuery& q, QString sql)
{
  QString error = q.lastError().text();
  QString s = QString("%1\n%2").arg(error).arg(sql);
  std::cout << s.toStdString() << std::endl;
  ard::errorBox(ard::mainWnd(), s);
}


QString QueryDB::query_string(QString sql)
{
  QSqlDatabase* db = qdb();
  if(!db)return "";

  QString rv = 0;
  QSqlQuery q(*db);
  if(!q.prepare(sql))
    {
      show_last_sql_err(q, sql);
      return 0;
    };
  if(!q.exec())
    {
      show_last_sql_err(q, sql);
      return 0;
    }

  if(q.first())
    {
      rv = q.value(0).toString();
    }
  return rv;
}


QueryDB::QueryDB(QWidget *)
{
  m_tablesM = new QSqlQueryModel;
  m_selected_tableM = new QSqlQueryModel;
  queryM = new QSqlQueryModel;
    

  QMainWindow* query_main_wnd = new QMainWindow(this);
  QSplitter* sp_query = new QSplitter(Qt::Vertical);
  sql_edit = new QTextEdit(this);
  res_table = new ArdTableView;
  res_table->setModel(queryM);
  sp_query->addWidget(sql_edit);
  sp_query->addWidget(res_table);
  query_main_wnd->setCentralWidget(sp_query);
  buildQueryToolbar(query_main_wnd);


  QMainWindow* db_main_wnd = new QMainWindow(this);
  metadata_table = new ArdTableView;
  metadata_view = new QTextEdit(this);
  metadata_tables_list = new QListView;
  metadata_tables_list->setModel(m_tablesM);
  metadata_table->setModel(m_selected_tableM);

  connect(metadata_tables_list->selectionModel(),
      SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection &)),
      this,
      SLOT(slotTablesListCurrChanged(const QItemSelection &, const QItemSelection &))
      );


  QSplitter* sp2 = new QSplitter(Qt::Horizontal);
  QSplitter* sp3 = new QSplitter(Qt::Vertical);
  sp3->addWidget(metadata_view);
  sp3->addWidget(metadata_table);
  sp2->addWidget(metadata_tables_list);
  sp2->addWidget(sp3);
  db_main_wnd->setCentralWidget(sp2);
  buildDBToolbar(db_main_wnd);

  QList<int> lstSizes;
  lstSizes.push_back(80);
  lstSizes.push_back(400);
  sp2->setSizes(lstSizes);
  main_tab = new QTabWidget(this);
  main_tab->addTab(db_main_wnd, "DB");
  main_tab->addTab(query_main_wnd, "Query");

  auto ei = new QPlainTextEdit;

  QString s =   ("natural sort: select * from ard_tree order by otype, pindex \n"
                 "particular parent: select * from ard_tree where pid=1 order by otype, pindex \n"
                 "email attachments: select * from apigmail_attachments where local_file_name is not null\n"
                 "email loaded: select * from apigmail_msg where msg_state=3\n"
				 "links: select syid, count(syid) from ard_board_links group by syid\n"
                 "roots: select * from ard_tree where pid=-1\n");

  s += "\n===== object types ========\n";
  s += QString("SELECT * FROM ard_tree WHERE otype IN(,)\n");
#define ADD_O(T) s += QString("%1: %2\n").arg(#T).arg((int)T);
  ADD_O(objUnknown);
  ADD_O(objFolder);
  ADD_O(objEThread);
  ADD_O(objEmailDraft);
  ADD_O(objFileRef);
  ADD_O(objDataRoot);
  ADD_O(objToolItem);
  ADD_O(objUrl);
  ADD_O(objPicture);
  ADD_O(objEThreadsRoot);
  ADD_O(objContact);
  ADD_O(objContactGroup);
#undef ADD_O

  s += "\n===== folder types ========\n";
#define ADD_F(T) s += QString("%1: %2\n").arg(#T).arg((int)EFolderType::T);
  ADD_F(folderUnknown);
  ADD_F(folderGeneric);
  ADD_F(folderRecycle);
  ADD_F(folderMaybe);
  ADD_F(folderReference);
  ADD_F(folderDelegated);
  ADD_F(folderSortbox);
  ADD_F(folderUserSortersHolder);
  ADD_F(folderUserSorter);
  ADD_F(folderBacklog);
  ADD_F(folderBoardTopicsHolder);
#undef ADD_F

  s += "\n===== gmail cache ========\n";
  s += ("threads-in-filters: select * from apigmail_thread where filter_mask<>0\n");
  s += ("threads-per-Q: select * from apigmail_thread where thread_id IN (select thread_id from  apigmail_qres where q_id=2)");

  ei->setPlainText(s);
  main_tab->addTab(ei, "info");

  QWidget* w = buildMiscWidget();
  main_tab->addTab(w, "misc");

  QVBoxLayout* vl_main = new QVBoxLayout;
  vl_main->addWidget(main_tab);
  useDefautDB();
  reloadTablesList();

  MODAL_DIALOG_SIZE(vl_main, QSize(900, 600));
}

QueryDB::~QueryDB()
{
  if(m_opened_db.isOpen())
    m_opened_db.close();
    
}

void QueryDB::meta_tablerow_activated(const QModelIndex &idx)
{
    Q_UNUSED(idx);
    reloadSelectedTable();
}

void QueryDB::slotTablesListCurrChanged(const QItemSelection &, const QItemSelection &)
{
  reloadSelectedTable();
};


void QueryDB::buildQueryToolbar(QMainWindow* parent)
{
    QToolBar *tb = new QToolBar(parent);
    tb->setWindowTitle(tr("Query Actions"));
    parent->addToolBar(tb);

    QAction* a;
    a = new QAction(tr("Exec"), this);
    connect(a, SIGNAL(triggered()), this, SLOT(execSQL()));
    tb->addAction(a);

    action_prevSQL = new QAction(tr("<<"), this);
    connect(action_prevSQL, SIGNAL(triggered()), this, SLOT(prevSQL()));
    tb->addAction(action_prevSQL);
    action_nextSQL = new QAction(tr(">>"), this);
    connect(action_nextSQL, SIGNAL(triggered()), this, SLOT(nextSQL()));
    tb->addAction(action_nextSQL);

    a = new QAction(tr("Export"), this);
    connect(a, SIGNAL(triggered()), this, SLOT(exportData()));
    tb->addAction(a);

    updateShortHistActions();
}


void QueryDB::buildDBToolbar(QMainWindow* parent)
{
    QToolBar *tb = new QToolBar(parent);
    tb->setWindowTitle(tr("DB Actions"));
    parent->addToolBar(tb);

    QAction* a;
    a = new QAction(tr("Open"), this);
    connect(a, SIGNAL(triggered()), this, SLOT(openDB()));
    tb->addAction(a);

    a = new QAction(tr("Default"), this);
    connect(a, SIGNAL(triggered()), this, SLOT(useDefautDB()));
    tb->addAction(a);

    a = new QAction(tr("Gmail Cache"), this);
    connect(a, SIGNAL(triggered()), this, SLOT(openGmailCacheDB()));
    tb->addAction(a);

    a = new QAction(tr("Close"), this);
    connect(a, SIGNAL(triggered()), this, SLOT(close()));
    tb->addAction(a);
}

QWidget* QueryDB::buildMiscWidget()
{
    QWidget* wm = new QWidget();
    QVBoxLayout *l_main = new QVBoxLayout;
    QHBoxLayout *l_btn = new QHBoxLayout;
    m_misc_e1 = new QLineEdit();
    l_btn->addWidget(m_misc_e1);
    QPushButton* b;// = new QPushButton("chain-up");
    //connect(b, SIGNAL(released()), this, SLOT(processChainUp()));
    //l_btn->addWidget(b);
    ADD_BUTTON2LAYOUT(l_btn, "chain-up (id)", &QueryDB::processChainUp);
    //ADD_BUTTON2LAYOUT(l_btn, "load-I&N", &QueryDB::processLoadAllImgNotes);
    l_main->addLayout(l_btn);
    m_misc_res = new QTextEdit();
    l_main->addWidget(m_misc_res);
    wm->setLayout(l_main);
    return wm;
};

void QueryDB::processChainUp()
{
    QString s_oid = m_misc_e1->text();
    int id = s_oid.toInt();
    auto f = ard::lookup(id);
    if(!f)
    {
      QString s = asHtmlDocument(QString("NOT located: %1").arg(id));
      m_misc_res->setHtml(s);
        return;
    }
    
    QString s = asHtmlDocument(QString("located: %1 %2<br>").arg(id).arg(f->dbgHint()));
    auto p = f->parent();
    while(p)
    {
        auto f_loc = ard::lookup(p->id());
        QString s1;
        if(f_loc)
        {
            s1 = asHtmlDocument(QString("located: %1 %2<br>").arg(p->id()).arg(p->dbgHint()));
        }
        else
        {
            s1 = asHtmlDocument(QString("NOT located: %1 %2<br>").arg(p->id()).arg(p->dbgHint()));
        }
        p = p->parent();
        s += s1;
    }

    m_misc_res->setHtml(s);

};

void QueryDB::reloadTablesList()
{
  QSqlDatabase* db = qdb();
  if(!db)return;

  m_tablesM->setQuery("SELECT name FROM sqlite_master WHERE type = 'table'", *db);
};


QSqlDatabase* QueryDB::qdb()
{
  QSqlDatabase* rv = nullptr;
  if(m_opened_db.isOpen())
    {
      rv = &m_opened_db;
    }
  else
    {
      if(gui::isDBAttached())
    {
      rv = &(ard::db()->m_db);
    }      
    }

  return rv;
};

void QueryDB::reloadSelectedTable()
{
  QSqlDatabase* db = qdb();
  if(!db)return;

  QModelIndex idx = metadata_tables_list->currentIndex();
  if(idx.isValid())
    {
      QString table_name = metadata_tables_list->model()->data(idx, Qt::DisplayRole).toString();
      QString sql = QString("SELECT * FROM %1").arg(table_name);
      m_selected_tableM->setQuery(sql, *db);

      sql = QString("SELECT sql FROM sqlite_master WHERE name='%1'").arg(table_name);
      QString s = query_string(sql);
      metadata_view->setPlainText(s);
    }
};

void QueryDB::openDB()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open database file"),
        dbp::configFileLastShellAccessDir(),
        tr("All Files (*.*)"));

    if (!fileName.isEmpty())
    {
        dbp::configFileSetLastShellAccessDir(fileName, true);
        if (m_opened_db.isOpen())
            m_opened_db.close();


        m_opened_db = QSqlDatabase::addDatabase("QSQLITE");
        m_opened_db.setDatabaseName(fileName);
        if (m_opened_db.open())
        {
            reloadTablesList();
            setWindowTitle("DB: " + fileName);
            metadata_view->setPlainText("");
        }
    }
}

void QueryDB::useDefautDB()
{
    if (m_opened_db.isOpen())
        m_opened_db.close();

    if (!gui::isDBAttached())
    {
		ard::messageBox(this, "Database is not connected");
        return;
    }

    setWindowTitle("DB: " + ard::db()->databaseName());
    reloadTablesList();
}

void QueryDB::openGmailCacheDB() 
{
    if (m_opened_db.isOpen())
        m_opened_db.close();

    extern QString gmailDBFilePath();
    QString fileName = gmailDBFilePath();

    m_opened_db = QSqlDatabase::addDatabase("QSQLITE");
    m_opened_db.setDatabaseName(fileName);
    if (m_opened_db.open())
    {
        reloadTablesList();
        setWindowTitle("DB: " + fileName);
        metadata_view->setPlainText("");
    }
};

void QueryDB::execSQL()
{
  QSqlDatabase* db = qdb();
  if(!db)
    {
	  ard::messageBox(this, "Database is not connected");
      return;
    }

  QString sql = sql_edit->toPlainText();
  sql = sql.trimmed();
  queryM->setQuery(sql, *db);
  if (queryM->lastError().isValid())
    {
	  ard::errorBox(ard::mainWnd(), queryM->lastError().text());
      return;
    }

  if(m_short_hist.size() > 0)
    {
      SHORT_HIST::reverse_iterator i = m_short_hist.rbegin();
      if(sql.compare(*i, Qt::CaseInsensitive) == 0)
        {
      return;
        }
    }

  m_short_hist.push_back(sql);
  m_short_hist_idx = m_short_hist.size() - 1;
  updateShortHistActions();
}

void QueryDB::exportData()
{
  QString filename = QFileDialog::getSaveFileName( 
        this, 
        tr("Save Document"), 
        dbp::configFileLastShellAccessDir(),
        tr("Documents (*.txt)") );
    if( !filename.isNull() )
    {
        dbp::configFileSetLastShellAccessDir(filename, true);
        QFile file(filename);
        file.open(QIODevice::WriteOnly | QIODevice::Text);
        QTextStream exp_out(&file);

        int rows = queryM->rowCount();
        int columns = queryM->columnCount();
        for(int r = 0; r < rows; r++)
        {
            QString line = "";
            for(int c = 0; c < columns;c++)
            {
                QString s = QString("\"%1\"").arg(queryM->data(queryM->index(r, c)).toString());
                line += s;
                if(c != columns-1)
                    line += ",";
            }
            line += "\n";
            exp_out << line;
        }
        file.close();
      //qDebug( filename.toAscii() );
    }
};

void QueryDB::prevSQL()
{
    if(m_short_hist.empty() || m_short_hist_idx == 0)
        return;
    m_short_hist_idx--;
    QString sql = m_short_hist[m_short_hist_idx];
    sql_edit->setHtml(sql);
    updateShortHistActions();
}

void QueryDB::nextSQL()
{
    if(m_short_hist.empty() || m_short_hist_idx == m_short_hist.size() - 1)
        return;
    m_short_hist_idx++;
    if(m_short_hist_idx >= m_short_hist.size() - 1)
        m_short_hist_idx = m_short_hist.size() - 1;
    QString sql = m_short_hist[m_short_hist_idx];
    sql_edit->setHtml(sql);
    updateShortHistActions();
}

void QueryDB::updateShortHistActions()
{
    action_prevSQL->setEnabled(!m_short_hist.empty() && m_short_hist_idx > 0);
    action_nextSQL->setEnabled(!m_short_hist.empty() && m_short_hist_idx < m_short_hist.size() - 1);
}

void gui::runSQLRig()
{
  QueryDB::runIt();
};

void QueryDB::runIt()
{
  QueryDB w;  
  w.exec();
};

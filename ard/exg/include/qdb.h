#ifndef QUERYDB_H
#define QUERYDB_H

#include <QAbstractTableModel>
#include <QDialog>
#include <QListView>
#include <QSqlDatabase>

class QTextEdit;
class QTableView;
class MetaDataTablesListView;
class QGraphicsView;
class QSqlDatabase;
class QSqlQueryModel;
class QMainWindow;
class QLineEdit;
class QPushButton;

class QueryDB : public QDialog
{
  Q_OBJECT
  public:

  static void runIt();

  virtual ~QueryDB();

signals:
    
public slots:
  void    execSQL();
  void    prevSQL();
  void    nextSQL();
  void    openDB();
  void    useDefautDB();
  void    openGmailCacheDB();
  void    exportData();
  void    processChainUp();
  //void    processLoadAllImgNotes();

  void meta_tablerow_activated(const QModelIndex &idx);
  void slotTablesListCurrChanged(const QItemSelection &, const QItemSelection &);
    
protected:
  QueryDB(QWidget *parent = 0);

  void        buildDBToolbar(QMainWindow* parent);
  void        buildQueryToolbar(QMainWindow* parent);
  QWidget*    buildMiscWidget();

  void        updateShortHistActions();
  void    reloadTablesList();
  void    reloadSelectedTable();

protected:
  QSqlDatabase* qdb();
  QString query_string(QString sql);

private:
  typedef std::vector<QString>    SHORT_HIST;

  QTextEdit*  sql_edit;
  QTableView* res_table;
  QLineEdit*  m_misc_e1;
  QTextEdit*  m_misc_res;

  QTextEdit*  metadata_view;
  QTableView* metadata_table;
  QListView* metadata_tables_list;
  QGraphicsView*  history_view;
  QTabWidget*     main_tab;
  SHORT_HIST      m_short_hist;
  size_t          m_short_hist_idx;

  QSqlQueryModel *m_tablesM, *m_selected_tableM, *queryM;

  QSqlDatabase  m_opened_db;

  QAction     *action_prevSQL,
    *action_nextSQL;
};


#endif

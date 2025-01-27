#pragma once
#include <QDialog>
#include "a-db-utils.h"
#include "custom-widgets.h"

class QListWidget;
class QProgressBar;
class QLabel;
class QCheckBox;
class QComboBox;
class SyncMap;
class QTabWidget;
class QTableView;

class SyncStatusDlg :   public QDialog,
                        public googleQtProgressControl
{
    Q_OBJECT
public:
    SyncStatusDlg();
    virtual ~SyncStatusDlg();
    void addBuffered(QString s);
protected:
    void closeEvent ( QCloseEvent * event );

private slots:
    void closeMonitor();

public:
    QTabWidget* m_tabs;
    QListWidget*      m_lw;
    QTableView*       m_img_view{nullptr};
    QProgressBar*   m_progress;
    QLabel*         m_status;
    STRING_LIST     m_buffered_string_list;
    bool            m_buffer_timer_started{ false };

};

/*
class SyncLogViewConfirm: public QDialog
{
    Q_OBJECT
public:
    SyncLogViewConfirm(QString  logDirPrefix2Select = "");

public slots:
    void closeView();
    void openLog();

protected:
    QString           m_logDirPrefix2Select;
    QTabWidget*       m_tabs;
    QTableView*       m_img_view{ nullptr };
    SyncMap*          m_sync_map;
};
*/
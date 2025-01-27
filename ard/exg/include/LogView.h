#pragma once
#include <QDialog>
#include "custom-widgets.h"

class OutlineSceneBase;
class OutlineView;
class QTabWidget;
class QLineEdit;
class anLogLine;
class QComboBox;
class QProgressBar;

namespace ard 
{
	class topic;
};

/**
   LogView - display various logs
*/
class LogView:  public QDialog
{
    Q_OBJECT
public:
    static void runIt(int tabSel = -1, QString logDirPrefix2Select = "");

protected:
    LogView(int tabSel = -1, QString logDirPrefix2Select = "");
    void   loadWorkingLogFile();

    void    loadSyncLogFile(QString logPath);
    void    loadSyncLogArchives();
    void    add2log(QString s, QPlainTextEdit*);
    void    buildSyncDirListCombo();

private slots: 
    void   SyncDirCurrentIndexChanged(int idx);
protected:  
    void    closeEvent(QCloseEvent *e)override;

    void   loadFile(QString path, QPlainTextEdit* e);
    QStandardItemModel*     generateArchLogModel();

protected:
    QPlainTextEdit          *m_working_log{ nullptr }, *m_sync_log{nullptr};
    QTableView*             m_archive_view{ nullptr };

    QComboBox*              m_log_dir_combo {nullptr};
    
    topic_ptr               m_locate_sync_item;
    QTabWidget*             m_tab;
    QString                 m_logDirPrefix2Select;
    bool                    m_logArchivesLoaded{ false }, m_uploadInProgress{false};
};


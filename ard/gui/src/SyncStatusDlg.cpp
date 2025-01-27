#include <QDebug>
#include <QListWidget>
#include <QVBoxLayout>
#include <QPushButton>
#include <QScrollBar>
#include <QProgressBar>
#include <QCloseEvent>
#include <QApplication>
#include <QDesktopWidget>
#include <QLabel>
#include <QCheckBox>
#include <QComboBox>
#include <QHeaderView>

#include "SyncStatusDlg.h"
#include "a-db-utils.h"
#include "syncpoint.h"
#include "utils.h"
#include "LogView.h"
#include "snc-tree.h"
#include "custom-boxes.h"

static SyncStatusDlg* m_statusDlg = nullptr;

#define ERR_MSG(S)if(silence_mode)qDebug() << S;else ard::messageBox(gui::mainWnd(), S);

void startMonitorSyncActivity(bool silence_mode)
{  
    if(!silence_mode)
    {
        if(!m_statusDlg)
        {
            m_statusDlg = new SyncStatusDlg;
        }
    }
    //!m_statusDlg can diappear here is user canceled dialog
};

void monitorSyncActivity(QString s, bool silence_mode)
{  
    if(!silence_mode){
        if(m_statusDlg){
            m_statusDlg->addBuffered(s);
        }
    }
};

void monitorSyncActivity(const STRING_LIST& string_list, bool silence_mode)
{
#ifdef _DEBUG
    for(auto& s : string_list){
        qDebug() << "[s]" << s;
    }
#endif

    if (!silence_mode)
    {
        if (m_statusDlg)
        {
            for (auto& k : string_list)
                m_statusDlg->m_lw->addItem(k);
        }

        // m_statusDlg->m_lw->addItem(s);
        int h_scroll = m_statusDlg->m_lw->verticalScrollBar()->maximum();
        m_statusDlg->m_lw->verticalScrollBar()->setValue(h_scroll);
        QCoreApplication::processEvents();
    }           
};

void monitorSyncStatus(QString s, bool silence_mode)
{
	dbg_print(QString("[s] %1").arg(s));
    if(!silence_mode){
        if(m_statusDlg)m_statusDlg->m_status->setText(s);        
    }
}

void stopMonitorSyncActivity(bool closeMonitorWindow, QString rdb_string, bool silence_mode)
{  
    if(silence_mode)
        return;

    if(m_statusDlg){
        m_statusDlg->m_progress->setValue(100);
        if(closeMonitorWindow)
        {
            m_statusDlg->close();
            QCoreApplication::processEvents();
        }
    }

    if(SyncPoint::isSyncBroken())
    {
        ERR_MSG("Sync aborted.");
        return;
    }

    if(!silence_mode)
    {
        QString msg = "";
        auto res = SyncPoint::lastSyncResult();
        if(res.status == ard::aes_status::ok)
        {
           // new SyncLogViewConfirm(synced_images, rdb_string);
            return;
        }
        else
        {
            auto res = SyncPoint::lastSyncResult();
#ifdef ARD_OPENSSL
            if (res.status == ard::aes_status::old_archive_key_tag) {
                auto s = SyncOldPasswordBox::getOldPassword(res);
                if (!s.isEmpty()) {
                    auto& cfg = ard::CryptoConfig::cfg();
                    cfg.tryOldSyncPassword(s);
                    ard::asyncExec(AR_synchronize);
                    return;
                }
            }
            else {
                if (res.status == ard::aes_status::decr_error || 
                    res.status == ard::aes_status::old_provided_key_tag) 
                {
                    if (SyncPasswordBox::changePassword()) {
                        ard::asyncExec(AR_synchronize);
                        return;
                    };
                }
            }
#endif            

            msg = "Sync failed. Press 'Yes' to open log.";
            if (ard::confirmBox(ard::mainWnd(), msg))
            {
                LogView::runIt(1, rdb_string);
            }           
        }
    }
};

void stepMonitorSyncActivity(int percentages, bool silence_mode)
{
    if(!silence_mode)
    {
        if(m_statusDlg)
        {
            if (percentages != m_statusDlg->m_progress->value()) {
                m_statusDlg->m_progress->setValue(percentages);
                QCoreApplication::processEvents();
            }
        }
    }
};


SyncStatusDlg::SyncStatusDlg()//:QDialog(gui::mainWnd())
{
    QVBoxLayout *l_main = new QVBoxLayout;

    m_tabs = new QTabWidget();
    m_tabs->setTabPosition(QTabWidget::North);
    l_main->addWidget(m_tabs);

    m_lw = new QListWidget;
    m_tabs->addTab(m_lw, "Sync log");

    m_progress = new QProgressBar();
    l_main->addWidget(m_progress);
    //addProgressBar(l_main, true);
    addProgressBar(l_main, false);

    m_progress->setMaximum(100);
    m_progress->setValue(0);
    m_status = new QLabel();
    m_status->setMaximumWidth(500);
    m_status->setMaximumHeight(gui::lineHeight() * 3);
    m_status->setWordWrap(true);
    l_main->addWidget(m_status);

    QPushButton* b = new QPushButton(this);
    b->setText("Close");
    connect(b, SIGNAL(released()), this, SLOT(closeMonitor()));
    l_main->addWidget(b);

    setLayout(l_main);

    gui::resizeWidget(this, QSize(300, 600));
    setAttribute(Qt::WA_DeleteOnClose);
    show();
};

SyncStatusDlg::~SyncStatusDlg()
{
    m_statusDlg = nullptr;
};


void SyncStatusDlg::closeMonitor()
{
    if(SyncPoint::isSyncRunning())
    {
        if(SyncPoint::isSyncBroken())
        {
			ard::messageBox(this, "Cancel of Sync has been initiated. Please wait.");
            return;
        }
      
        if(ard::confirmBox(this, "Sync procedure in process. Do you want to cancel?"))
        {
            SyncPoint::breakSync();
            return;
        }

        //showMessage("Pease wait sync procedure to complete.");
        //      return;
    }
    close();
};

void SyncStatusDlg::closeEvent ( QCloseEvent * ev )
{
    if(SyncPoint::isSyncRunning() && !SyncPoint::isSyncBroken())
    {
        ev->ignore();
    }
    else
    {
        ev->accept();
    }
};

void SyncStatusDlg::addBuffered(QString s) 
{
    m_buffered_string_list.push_back(s);
    QCoreApplication::processEvents();
    if (!m_buffer_timer_started) {
        m_buffer_timer_started = true;
        QTimer::singleShot(700, [=]() {
            if (m_statusDlg) {
                if (!m_statusDlg->m_buffered_string_list.empty()) {
                    for (auto& s : m_buffered_string_list) {
                        m_statusDlg->m_lw->addItem(s);
                    }

                    // m_statusDlg->m_lw->addItem(s);
                    int h_scroll = m_statusDlg->m_lw->verticalScrollBar()->maximum();
                    m_statusDlg->m_lw->verticalScrollBar()->setValue(h_scroll);
                    m_buffered_string_list.clear();
                    m_buffer_timer_started = false;
                }
            }
            });
    }
};

/**
   SyncLogViewConfirm
SyncLogViewConfirm::SyncLogViewConfirm(QString logDirPrefix2Select)
     :m_logDirPrefix2Select(logDirPrefix2Select)
{
    QVBoxLayout* l_main = new QVBoxLayout();
    QString s = "Sync successfully completed.";
    if(!m_logDirPrefix2Select.isEmpty())
    {
        s = QString("'%1' sync successfully completed.").arg(m_logDirPrefix2Select);
    }
    QLabel* lb = new QLabel(s);
    l_main->addWidget(lb);
  
    m_tabs = new QTabWidget();
    m_tabs->setTabPosition(QTabWidget::North);
    l_main->addWidget(m_tabs);


    m_sync_map = new SyncMap();
    m_tabs->addTab(m_sync_map, "Sync map");
    m_sync_map->remap();
    //if (!m_synced_images.empty()) {
    //    buildImagesTable();
    //}

    QHBoxLayout* h_buttons = new QHBoxLayout(); 


    QPushButton* b = nullptr;

    ADD_BUTTON2LAYOUT(h_buttons, "View Log", &SyncLogViewConfirm::openLog);
    ADD_BUTTON2LAYOUT(h_buttons, "OK", &SyncLogViewConfirm::closeView);
    b->setDefault(true);

    l_main->addLayout(h_buttons);
    setLayout(l_main);

    exec();
};


void SyncLogViewConfirm::closeView()
{
    close();
};

void SyncLogViewConfirm::openLog()
{
    LogView::runIt(1, m_logDirPrefix2Select);
    close();
};
*/
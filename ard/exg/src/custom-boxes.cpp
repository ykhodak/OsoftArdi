#include <QPainter>
#include <QScrollBar>
#include <QScroller>
#include <QTimer>
#include <QDesktopWidget>
#include <QApplication>
#include <QFormLayout>
#include <QPushButton>
#include <QCalendarWidget>
#include <QProgressBar>
#include <QComboBox>
#include <QStylePainter>
#include <QLabel>
#include <csignal>
#include <QCheckBox>
#include <QFileInfo>
#include <QFileDialog>
#include <QListView>
#include <QStandardItemModel>
#include <QHeaderView>
#include <array>
#include <QFontComboBox>

#include "anfolder.h"
#include "custom-widgets.h"
#include "custom-boxes.h"
#include "registerbox.h"
#include "gmail/GmailRoutes.h"
#include "GoogleClient.h"
#include "Endpoint.h"
#include "email.h"
#include "ethread.h"
#include "db-merge.h"
#include "contact.h"
#include "board.h"
#include "board_links.h"
#include "rule_runner.h"
#include "locus_folder.h"
/**
* AsyncSingletonBox
*/
AsyncSingletonBox* AsyncSingletonBox::m_box = nullptr;
AsyncSingletonBox::AsyncSingletonBox()
{
    connect(this, &QObject::destroyed, []() {
        m_box = nullptr;
    });
    m_box = this;
};

void AsyncSingletonBox::callReloadBox()
{
    QMetaObject::invokeMethod(m_box, "reloadBox", Qt::QueuedConnection);
};

/**
* LabelsBox
*/
void LabelsBox::showLabels()
{
    LabelsBox d;
    d.exec();
};

LabelsBox::LabelsBox()
{
    QVBoxLayout *h_1 = new QVBoxLayout;
    QHBoxLayout *h_btns = new QHBoxLayout;

    auto sm = generateGlobalLabelCacheModel();
    if (sm) {
        m_label_view = gui::createTableView(sm);
        m_label_view->setSelectionBehavior(QAbstractItemView::SelectRows);
        h_1->addWidget(m_label_view);
    }
    h_1->addLayout(h_btns);

    ard::addBoxButton(h_btns, "Edit", [&]() {
        auto lb = selectedLabel();
        if (lb) {
            if (lb->isSystem()) {
				ard::messageBox(this, QString("Selected label '%1' is system reserved and can't be modified.").arg(lb->labelName()));
                return;
            }
            //QString name = 
            auto res = gui::edit(lb->labelName(), QString("Rename label '%1'").arg(lb->labelName()), true, this);
            if (res.first &&
                !res.second.isEmpty() &&
                res.second != lb->labelName())
            {
                googleQt::GmailRoutes* gm = ard::gmail();
                if (gm) {
                    auto cr = gm->cacheRoutes();
                    if (cr) {
                        auto t = cr->renameLabels_Async(lb->labelId(), res.second);
                        t->then([=]()
                        {
                            if (m_box) {
                                callReloadBox();
                            }
                        },
                            [=](std::unique_ptr<googleQt::GoogleException> ex)
                        {
                            ASSERT(0, "label edit error") << ex->statusCode() << ex->what();
                        });
                    }
                }
                else {
					ard::errorBox(this, "Gmail route connection is not available.");
                }
            }
        }
    });
    ard::addBoxButton(h_btns, "Add", [&]() {
        auto res = gui::edit("", QString("Create new label"), false, this);
        //QString name = gui::edit("", QString("Create new label"), false, this).trimmed();
        if (res.first && !res.second.isEmpty())
        {
            googleQt::GmailRoutes* gm = ard::gmail();
            if (gm) {
                auto cr = gm->cacheRoutes();
                if (cr) {
                    std::vector<QString> names;
                    names.push_back(res.second);
                    auto t = cr->createLabelList_Async(names);
                    t->then([=]()
                    {
                        if (m_box) {
                            callReloadBox();
                        }
                    },
                        [=](std::unique_ptr<googleQt::GoogleException> ex)
                    {
                        ASSERT(0, "label add error") << ex->statusCode() << ex->what();
                    });
                }
            }
            else {
				ard::errorBox(this, "Gmail route connection is not available.");
            }
        }
    });
    ard::addBoxButton(h_btns, "Remove", [&]() {
        auto lb = selectedLabel();
        if (lb) {
            if (lb->isSystem()) {
				ard::messageBox(this, QString("Selected label '%1' is system reserved and can't be modified.").arg(lb->labelName()));
                return;
            }

            if (ard::confirmBox(this, QString("Please confirm deleting '%1' label").arg(lb->labelName()))) {
                googleQt::GmailRoutes* gm = ard::gmail();
                if (gm) {
                    auto cr = gm->cacheRoutes();
                    if (cr) {
                        std::vector<QString> lids;
                        lids.push_back(lb->labelId());
                        auto t = cr->deleteLabelList_Async(lids);
                        t->then([=]()
                        {
                            if (m_box) {
                                callReloadBox();
                            }
                        },
                            [=](std::unique_ptr<googleQt::GoogleException> ex)
                        {
                            ASSERT(0, "label remove error") << ex->statusCode() << ex->what();
                        });
                    }
                }
                else {
					ard::errorBox(this, "Gmail route connection is not available.");
                }
            }
        }
    });
    ard::addBoxButton(h_btns, "Refresh", [&]() {
        googleQt::GmailRoutes* gm = ard::gmail();
        if (gm) {
            auto cr = gm->cacheRoutes();
            if (cr) {
                googleQt::GoogleVoidTask* t = ard::gmail()->cacheRoutes()->refreshLabels_Async();
                t->then([=]()
                {
                    if (m_box) {
                        callReloadBox();
                    }
                },
                    [=](std::unique_ptr<googleQt::GoogleException> ex)
                {
                    ASSERT(0, "label refresh error") << ex->statusCode() << ex->what();
                });
            }
        }
    });

    ard::addBoxButton(h_btns, "Close", [&]() { close(); });

    MODAL_DIALOG_SIZE(h_1, QSize(600, 900));
};

void LabelsBox::reloadBox()
{
    if (m_label_view) {
        auto m = generateGlobalLabelCacheModel();
        m_label_view->setModel(m);
    }
};


QStandardItemModel* LabelsBox::generateGlobalLabelCacheModel()
{
    assert_return_null(ard::gmail(), "expected gmail");
    assert_return_null(ard::gmail()->cacheRoutes(), "expected gmail cache");

    QStandardItemModel* sm{ nullptr };
    auto gm = ard::gmail_model();
    auto route = ard::gmail();
    if (gm && route) {
        auto storage = ard::gstorage();
        assert_return_null(storage, "expected g-storage");

        QStringList column_labels;
        column_labels.push_back("Name");
        column_labels.push_back("Labelid");
        column_labels.push_back("Mask");
        column_labels.push_back("UnreadMsg");
        column_labels.push_back("CachedMsg");
        sm = new QStandardItemModel();
        sm->setHorizontalHeaderLabels(column_labels);

        int row = 0;

        QBrush br_sys(color::Gray);

        auto labels = ard::gmail()->cacheRoutes()->getLoadedLabels();
        for (auto lb : labels)
        {
            if (lb->isSystem()) {
                auto it = new QStandardItem(lb->labelId());
                it->setBackground(br_sys);
                sm->setItem(row, 0, new QStandardItem(lb->labelName()));
                sm->setItem(row, 1, it);
                sm->setItem(row, 2, new QStandardItem(QString("%1^2=%2").arg(lb->labelMaskBase()).arg(lb->labelMask())));
                sm->setItem(row, 3, new QStandardItem(QString("%1").arg(lb->unreadMessages())));
                sm->setItem(row, 4, new QStandardItem(QString("%1").arg(storage->getCacheMessagesCount(lb))));
                //                sm->setItem(row, 4, new QStandardItem(QString("%1").arg(lb->unreadMessages())));
                row++;
            }
        }
        for (auto lb : labels)
        {
            if (!lb->isSystem()) {
                sm->setItem(row, 0, new QStandardItem(lb->labelName()));
                sm->setItem(row, 1, new QStandardItem(lb->labelId()));
                sm->setItem(row, 2, new QStandardItem(QString("%1^2=%2").arg(lb->labelMaskBase()).arg(lb->labelMask())));
                sm->setItem(row, 3, new QStandardItem(QString("%1").arg(lb->unreadMessages())));
                sm->setItem(row, 4, new QStandardItem(QString("%1").arg(storage->getCacheMessagesCount(lb))));
                row++;
            }
        }
    }
    return sm;
};

googleQt::mail_cache::label_ptr LabelsBox::selectedLabel()
{
    if (!m_label_view) {
        return nullptr;
    }

    googleQt::GmailRoutes* r = ard::gmail();
    if (!r)
        return nullptr;

    auto storage = ard::gstorage();
    assert_return_null(storage, "expected g-storage");

    QAbstractItemModel *dmodel = m_label_view->model();
    QItemSelectionModel *selm = m_label_view->selectionModel();
    auto sl = selm->selectedRows(1);
    for (auto& idx : sl) {
        if (idx.isValid()) {
            auto idx2 = idx.sibling(idx.row(), 1);
            if (idx2.isValid()) {
                auto labelId = dmodel->data(idx2).toString();
                auto lb = storage->findLabel(labelId);
                return lb;
            }
        }
    }

    return nullptr;
};

/**
* AdoptedThreadsBox
*/
void AdoptedThreadsBox::showThreads()
{
    AdoptedThreadsBox d;
    d.exec();
};

AdoptedThreadsBox::AdoptedThreadsBox()
{
    QVBoxLayout *h_1 = new QVBoxLayout;
    QHBoxLayout *h_btns = new QHBoxLayout;

    auto sm = generateThreadsModel();
    if (sm) {
        m_t_view = gui::createTableView(sm);
        m_t_view->setSelectionBehavior(QAbstractItemView::SelectRows);

        auto h = m_t_view->verticalHeader();
        h->setSectionResizeMode(QHeaderView::Stretch);
        h->setDefaultSectionSize(24);
        h_1->addWidget(m_t_view);
    }
    h_1->addLayout(h_btns);

    QPushButton* b = nullptr;
    ADD_BUTTON2LAYOUT(h_btns, "Find", &AdoptedThreadsBox::locateThread);
    ADD_BUTTON2LAYOUT(h_btns, "Properties", &AdoptedThreadsBox::viewThreadProperties);
    ADD_BUTTON2LAYOUT(h_btns, "Close", &QPushButton::close);

    MODAL_DIALOG_SIZE(h_1, QSize(600, 900));
};

QStandardItemModel* AdoptedThreadsBox::generateThreadsModel()
{
    QStandardItemModel* sm{ nullptr };

    QStringList column_labels;
    //column_labels.push_back("Snippet");
    column_labels.push_back("Title");
    column_labels.push_back("ToDo(%)");
    column_labels.push_back("Space");
    column_labels.push_back("ThreadID");
    column_labels.push_back("DBID");
    sm = new QStandardItemModel();
    sm->setHorizontalHeaderLabels(column_labels);

    QBrush br_root(color::Gray);
    QBrush br_default(color::White);
    QBrush br_err(color::Red);

    int row = 0;
    TOPICS_LIST items;
    auto dr = ard::root();
    if (dr) {
        items = dr->selectAllThreads();
        for (auto i : items) 
        {
            auto t = dynamic_cast<ard::ethread*>(i);
            auto p = t->parent();
            ASSERT(p, "expected parent for thread");
            if (t && p) {
                sm->setItem(row, 0, new QStandardItem(t->title()));
                if (t->isToDo()) {
                    QString s = QString("%1").arg(t->getToDoDonePercent4GUI());
                    sm->setItem(row, 1, new QStandardItem(s));
                }
                else {
                    sm->setItem(row, 1, new QStandardItem("-"));
                }
                auto it = new QStandardItem(p->title());
                it->setBackground(br_default);
                sm->setItem(row, 2, it);
                if (t->optr()) {
                    sm->setItem(row, 3, new QStandardItem(t->optr()->id()));
                }
                sm->setItem(row, 4, new QStandardItem(QString("%1").arg(t->id())));
                row++;
            }
        }
    }

    auto r = ard::db()->threads_root();
    if (r) {
        for (auto i : r->items()) {
            auto t = dynamic_cast<ard::ethread*>(i);
            if (t) {
                sm->setItem(row, 0, new QStandardItem(t->title()));
                if (t->isToDo()) {
                    QString s = QString("%1").arg(t->getToDoDonePercent4GUI());
                    sm->setItem(row, 1, new QStandardItem(s));
                }
                else {
                    sm->setItem(row, 1, new QStandardItem("-"));
                }
                auto it = new QStandardItem("--");              
                sm->setItem(row, 2, it);
                if (t->optr()) {
                    it->setBackground(br_root);
                    sm->setItem(row, 3, new QStandardItem(t->optr()->id()));
                }
                else {
                    it->setBackground(br_err);
                }
                sm->setItem(row, 4, new QStandardItem(QString("%1").arg(t->id())));
                row++;
            }
        }
    }//thread root

    return sm;
};

ethread_ptr AdoptedThreadsBox::selectedThread()
{
    QAbstractItemModel *dm = m_t_view->model();
    QItemSelectionModel *sm = m_t_view->selectionModel();
    QModelIndexList list = sm->selectedIndexes();
    if (list.size() > 0)
    {
        QModelIndex idx = list.at(0);
        auto idx2 = idx.sibling(idx.row(), 4);
        auto dbid = dm->data(idx2).toInt();

        auto f = ard::db()->lookupLoadedItem(dbid);
        ASSERT(f, "failed to locate thread object by dbid") << dbid;
        auto t = dynamic_cast<ard::ethread*>(f);
        return t;
    }
    return nullptr;
};

void AdoptedThreadsBox::locateThread()
{
    auto t_root = ard::db()->threads_root();
    auto t = selectedThread();
    if (t) {
        qDebug() << "ykh-2-lookup=" << t->dbgHint();
        if (t_root == t->parent()) {
            gui::setupMainOutlineSelectRequestOnRebuild(t);
            gui::outlineFolder(ard::Sortbox(), outline_policy_PadEmail);
        }
        else {
            ASSERT(0, "locateThread err1");
        }
    }
};

void AdoptedThreadsBox::viewThreadProperties()
{
    auto t = selectedThread();
    if (t) {
        ard::asyncExec(AR_ViewProperties, t);
    }
}

/**
* BackupsBox
*/
void BackupsBox::showBackups() 
{
    BackupsBox d;
    d.exec();
};

BackupsBox::BackupsBox() 
{
    QVBoxLayout *h_1 = new QVBoxLayout;
    QHBoxLayout *h_btns = new QHBoxLayout;

    auto sm = generateBackupsModel();
    if (sm) {
        m_t_view = gui::createTableView(sm);
        m_t_view->setSelectionBehavior(QAbstractItemView::SelectRows);
        h_1->addWidget(m_t_view);
    }
    h_1->addLayout(h_btns);

    ard::addBoxButton(h_btns, "Import", [&]() {
        QString backup_path = BackupsBox::selectedBackupFile();
        if (ard::confirmBox(this, QString("Please confirm restoring selected backup file '%1' ").arg(backup_path))) {
            qDebug() << backup_path << get_curr_db_weekly_backup_file_path();
            QFileInfo fi1(backup_path);
            QFileInfo fi2(get_curr_db_weekly_backup_file_path());
            QString b1 = fi1.baseName();
            QString b2 = fi2.baseName();
            bool proceed_with_merge = true;
            if (b1.compare(b2) != 0) {
                proceed_with_merge = ard::confirmBox(this, QString("Selected backup file was created in different Ardi space. Are you sure you want to imprort it into current '%1' space?").arg(dbp::currentDBName()));
            }
            if (proceed_with_merge) {
                qDebug() << "<<< merging" << b1 << b2;
                ArdiDbMerger::guiImportArdiFile(backup_path);
                close();
            }
        }
    });
    ard::addBoxButton(h_btns, "Import from External backup", [&]() {
        close();
        ArdiDbMerger::guiSelectAndImportArdiFile();
    });
    ard::addBoxButton(h_btns, "Close", [&]() { close(); });

    MODAL_DIALOG_SIZE(h_1, QSize(600, 900));
};

QStandardItemModel* BackupsBox::generateBackupsModel() 
{
    extern QString get_backup_dir_path();

    QStandardItemModel* sm{ nullptr };

    QStringList column_labels;
    column_labels.push_back("Space");
    column_labels.push_back("Time");
    column_labels.push_back("Name");
    column_labels.push_back("Size");

    sm = new QStandardItemModel();
    sm->setHorizontalHeaderLabels(column_labels);

    int row = 0;

    QFileInfo fi2(get_curr_db_weekly_backup_file_path());
    QString current_db_base = fi2.baseName();

    QDir d(get_backup_dir_path());
    auto lst = d.entryInfoList(QDir::Files, QDir::Time);
    for (auto& e : lst) {
        QString bname = e.baseName();
        QStringList lst = bname.split("--");
        if (lst.size() == 2) {
            auto space = lst[1];            
            auto mod_time = e.lastModified().toString();
            auto name = e.fileName();
            auto size_s = googleQt::size_human(e.size());

            auto it = new QStandardItem(space);
            it->setData(e.canonicalFilePath());
            if (bname.compare(current_db_base) != 0) {
                it->setBackground(Qt::gray);
            }
            sm->setItem(row, 0, it);
            sm->setItem(row, 1, new QStandardItem(mod_time));
            sm->setItem(row, 2, new QStandardItem(name));
            sm->setItem(row, 3, new QStandardItem(size_s));

            qDebug() << "[backup]" << bname << "space=" << space << "path=" << e.canonicalFilePath();
            row++;
        }
    }

    return sm;
};

QString BackupsBox::selectedBackupFile() 
{
    QString rv;

    QStandardItemModel* dm = dynamic_cast<QStandardItemModel*>(m_t_view->model());
    if (dm) {
        QItemSelectionModel *sm = m_t_view->selectionModel();
        QModelIndexList list = sm->selectedIndexes();
        if (list.size() > 0)
        {
            QModelIndex idx = list.at(0);
            auto it = dm->itemFromIndex(idx);
            if (it) {
                rv = it->data().toString();
            }
        }
    }

    return rv;
};

void ContactsBox::showContacts()
{
    ContactsBox d;
    d.exec();
};

ContactsBox::ContactsBox() 
{
    QVBoxLayout *h_1 = new QVBoxLayout;
    QHBoxLayout *h_btns = new QHBoxLayout;

    m_main_tab = new QTabWidget;
    m_main_tab->setTabPosition(QTabWidget::East);
    h_1->addWidget(m_main_tab);

    ///all contacts
    auto sm = generateContactsModel();
    if (sm) {
        m_t_view = gui::createTableView(sm);
        m_t_view->setSelectionBehavior(QAbstractItemView::SelectRows);

        auto h = m_t_view->verticalHeader();
        h->setSectionResizeMode(QHeaderView::Stretch);
        m_main_tab->addTab(m_t_view, "List");
    }

    ///one->fields
    m_one_view = gui::createTableView(sm);
    m_one_view->setSelectionBehavior(QAbstractItemView::SelectRows);
    auto h = m_one_view->verticalHeader();
    h->setSectionResizeMode(QHeaderView::Stretch);
    m_main_tab->addTab(m_one_view, "Contact");

    ///one->xml
    m_one_xml = new QPlainTextEdit;
    m_main_tab->addTab(m_one_xml, "Contact XML");


    ///groups
    m_groups_view = gui::createTableView(sm);
    m_groups_view->setSelectionBehavior(QAbstractItemView::SelectRows);
    h = m_groups_view->verticalHeader();
    h->setSectionResizeMode(QHeaderView::Stretch);
    m_main_tab->addTab(m_groups_view, "Groups");
    sm = generateGroupsModel();
    if (sm) {
        m_groups_view->setModel(sm);
    }   

    connect(m_main_tab, &QTabWidget::currentChanged, [&](int tab_index) {
        if (tab_index == 1) {
            auto c = selectedContact();
            if (c) {
                auto m2 = generateOneContactModel(c);
                if (m2) {
                    m_one_view->setModel(m2);
                }
            }
        }
        else if (tab_index == 2) {
            auto c = selectedContact();
            if (c) {
                auto e = c->ensureCExt();
                if (e) {
                    m_one_xml->setPlainText(e->toXml());
                }
            }
        }
    });


    h_1->addLayout(h_btns);

    QPushButton* b = nullptr;
    ADD_BUTTON2LAYOUT(h_btns, "Close", &QPushButton::close);
    MODAL_DIALOG_SIZE(h_1, QSize(600, 900));
};

contact_ptr ContactsBox::selectedContact()
{
    contact_ptr rv = nullptr;
    if (m_t_view && m_t_view->selectionModel()) {
        int rowidx = m_t_view->selectionModel()->currentIndex().row();
        if (rowidx == -1) {
            rowidx = 0;
        }
        auto m = m_t_view->model();
        if (m) {
            DB_ID_TYPE dbid = m->index(rowidx, 0).data().toInt();
            rv = ard::lookupAs<ard::contact>(dbid);
        }
    }
    return rv;
};

QStandardItemModel* ContactsBox::generateContactsModel() 
{
    assert_return_null(ard::isDbConnected(), "expected open DB");
    QStandardItemModel* sm{ nullptr };

    QStringList column_labels;
    column_labels.push_back("dbid");
    column_labels.push_back("cid");
    column_labels.push_back("G-ids");
    column_labels.push_back("G-names");
    column_labels.push_back("FName");
    column_labels.push_back("Email");
    column_labels.push_back("Phone");
    column_labels.push_back("Address");
    column_labels.push_back("Organization");
    column_labels.push_back("Notes");
    sm = new QStandardItemModel();
    sm->setHorizontalHeaderLabels(column_labels);

    auto c_list = ard::db()->cmodel()->croot()->contacts();
    auto s2t = toSyidMap(c_list);

    int row = 0;
    for (auto f : c_list) {
        auto c = dynamic_cast<ard::contact*>(f);
        if (c) {
            sm->setItem(row, 0, new QStandardItem(QString("%1").arg(c->id())));
            sm->setItem(row, 1, new QStandardItem(c->syid()));
            auto gm = c->groupsMember();
            QString str_names = "";
            QString str = "";
            for (auto& s : gm) {
                str += s;
                str += ";";

                auto it = s2t.find(s);
                if (it != s2t.end()) {
                    str_names += QString("'%1'").arg(it->second->title());
                    str_names += ";";
                }
            }
            if (str.size() > 0) {
                str.remove(str.size() - 1, 1);
            }
            if (str_names.size() > 0) {
                str_names.remove(str_names.size() - 1, 1);
            }

            sm->setItem(row, 2, new QStandardItem(str));
            sm->setItem(row, 3, new QStandardItem(str_names));
            sm->setItem(row, 4, new QStandardItem(c->contactFamilyName()));
            sm->setItem(row, 5, new QStandardItem(c->contactEmail()));
            sm->setItem(row, 6, new QStandardItem(c->contactPhone()));
            sm->setItem(row, 7, new QStandardItem(c->contactAddress()));
            sm->setItem(row, 8, new QStandardItem(c->contactOrganization()));
            sm->setItem(row, 9, new QStandardItem(c->contactNotes()));
            row++;
        }
    }

    return sm;
};

QStandardItemModel* ContactsBox::generateOneContactModel(ard::contact* c)
{
    assert_return_null(ard::isDbConnected(), "expected open DB");
    auto l2 = ard::db()->cmodel()->groot()->items();
    auto s2t = toSyidMap(l2);

    QStandardItemModel* sm{ nullptr };

    QStringList column_labels;
    column_labels.push_back("field");
    column_labels.push_back("value");
    sm = new QStandardItemModel();
    sm->setHorizontalHeaderLabels(column_labels);

    if (c) {
        int row = 0;
        sm->setItem(row, 0, new QStandardItem("dbid"));
        sm->setItem(row, 1, new QStandardItem(QString("%1").arg(c->id())));
        row++;
        sm->setItem(row, 0, new QStandardItem("clid"));
        sm->setItem(row, 1, new QStandardItem(QString("%1").arg(c->syid())));
        row++;

        //...
        auto gm = c->groupsMember();
        for (auto& s : gm) {
            sm->setItem(row, 0, new QStandardItem("groupid"));
            sm->setItem(row, 1, new QStandardItem(s));
            row++;

            auto it = s2t.find(s);
            if (it != s2t.end()) {
                sm->setItem(row, 0, new QStandardItem("group-name"));
                sm->setItem(row, 1, new QStandardItem(it->second->title()));
                row++;
                //str_names += QString("'%1'").arg(it->second->title());
                //str_names += ";";
            }
        }
        //...
        sm->setItem(row, 0, new QStandardItem("FamilyName"));
        sm->setItem(row, 1, new QStandardItem(c->contactFamilyName()));
        row++;

        sm->setItem(row, 0, new QStandardItem("Email"));
        sm->setItem(row, 1, new QStandardItem(c->contactEmail()));
        row++;

        sm->setItem(row, 0, new QStandardItem("Phone"));
        sm->setItem(row, 1, new QStandardItem(c->contactPhone()));
        row++;

        sm->setItem(row, 0, new QStandardItem("Address"));
        sm->setItem(row, 1, new QStandardItem(c->contactAddress()));
        row++;

        sm->setItem(row, 0, new QStandardItem("Organization"));
        sm->setItem(row, 1, new QStandardItem(c->contactOrganization()));
        row++;

        sm->setItem(row, 0, new QStandardItem("Notes"));
        sm->setItem(row, 1, new QStandardItem(c->contactNotes()));
        row++;
    }

    return sm;
};

QStandardItemModel* ContactsBox::generateGroupsModel() 
{
    assert_return_null(ard::isDbConnected(), "expected open DB");

    QStandardItemModel* sm{ nullptr };
    QStringList column_labels;
    column_labels.push_back("dbid");
    column_labels.push_back("gid");
    column_labels.push_back("Name");
    //column_labels.push_back("IsSystem");

    sm = new QStandardItemModel();
    sm->setHorizontalHeaderLabels(column_labels);

    int row = 0;
    auto gl = ard::db()->cmodel()->groot()->items();//cm->wrappedGroups();
    for (auto i : gl) {
        auto g = dynamic_cast<ard::contact_group*>(i);
        if (g) {
            /*auto o = g->optr();
            if (!o)
                continue;
                */
            sm->setItem(row, 0, new QStandardItem(QString("%1").arg(g->id())));
            sm->setItem(row, 1, new QStandardItem(g->syid()));
            sm->setItem(row, 2, new QStandardItem(g->title()));
            //sm->setItem(row, 3, new QStandardItem(g->isSystemGroup() ? "Y" : "N"));
            row++;
        }
    }

    return sm;
};

//...
ard::BoardDropOptions BoardTopicDropBox::showBoardTopicDropOptions(const TOPICS_LIST& dropped_topics, ard::BoardItemShape shp, int band_space)
{
    ard::BoardDropOptions rv;
    rv.insert_branch_type = ard::InsertBranchType::none;
    rv.item_shape = shp;
    rv.band_space = band_space;

    if (dropped_topics.empty())
        return rv;

    int max_sub = 0;
    for (auto i : dropped_topics) {
        if (i->isExpandableInBBoard()) {
            if (static_cast<int>(i->items().size()) > max_sub) {
                max_sub = i->items().size();
            }
        }
    }

    if (max_sub == 0) {
        rv.insert_branch_type = ard::InsertBranchType::single_topic;

        auto f = dropped_topics[0];
        auto t = dynamic_cast<ard::ethread*>(f);
        if (t) {
            rv.item_shape = ard::BoardItemShape::box;
        }
        return rv;
    }

    BoardTopicDropBox d(shp, band_space);
    d.exec();
    rv.insert_branch_type = d.m_drop_type;
    rv.item_shape = d.m_shape;
    rv.band_space = d.m_band_space;
    return rv;
};


BoardTopicDropBox::BoardTopicDropBox(ard::BoardItemShape shp, int band_space):m_shape(shp)
{
    QVBoxLayout *h_1 = new QVBoxLayout;
    QHBoxLayout *h_btns = new QHBoxLayout;
    QHBoxLayout *h_opt = new QHBoxLayout;

    m_main_tab = new QTabWidget;
    m_main_tab->setTabPosition(QTabWidget::East);
    h_1->addWidget(m_main_tab);

    auto dm = generateDropSelectionModel();
    if (dm) {
        m_t_view = gui::createTableView(dm);
        m_t_view->setSelectionBehavior(QAbstractItemView::SelectRows);
        m_t_view->setColumnWidth(0, 64);
        /*
        auto h = m_t_view->verticalHeader();
        h->setSectionResizeMode(QHeaderView::Stretch);
        */
        m_main_tab->addTab(m_t_view, "Expand Topic as..");

        QItemSelectionModel *selm = m_t_view->selectionModel();
        auto idx = dm->index(0, 0);
        selm->select(idx, QItemSelectionModel::Select);
    }

    h_1->addLayout(h_opt);
    h_1->addLayout(h_btns);

    m_box_items_chk = new QCheckBox("Box Topics");
    h_opt->addWidget(m_box_items_chk);
    h_opt->addWidget(new QLabel("Band group space"));
    m_cb_band_space = new QComboBox;
    h_opt->addWidget(m_cb_band_space);
    m_cb_band_space->addItem("0");
    m_cb_band_space->addItem("1");
    m_cb_band_space->addItem("2");

    auto idx = m_cb_band_space->findText(QString("%1").arg(band_space));
    ASSERT(idx != -1, "invalid band space value provided.") << band_space;
    if (idx != -1) 
    {
        m_cb_band_space->setCurrentIndex(idx);
    }

	auto spacer = ard::newSpacer(); //new QWidget();
    //spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    h_opt->addWidget(spacer);


    if (m_shape == ard::BoardItemShape::box) 
    {
        m_box_items_chk->setChecked(true);
    };

    QPushButton* b = nullptr;
    b = ard::addBoxButton(h_btns, "OK", [&](){
        QItemSelectionModel *sm = m_t_view->selectionModel();
        QModelIndexList list = sm->selectedIndexes();
        if (list.size() > 0)
        {
            QModelIndex idx = list.at(0);
            switch (idx.row()) 
            {
            case 0:m_drop_type = ard::InsertBranchType::single_topic; break;
            case 1:m_drop_type = ard::InsertBranchType::branch_expanded_to_right; break;
            case 2:m_drop_type = ard::InsertBranchType::branch_expanded_to_left; break;
            case 3:m_drop_type = ard::InsertBranchType::branch_expanded_from_center; break;
            case 4:m_drop_type = ard::InsertBranchType::branch_top_group_expanded_to_right; break;
            case 5:m_drop_type = ard::InsertBranchType::branch_top_group_expanded_to_down; break;
            default:
                m_drop_type = ard::InsertBranchType::single_topic; break;
            }
        }

        if (m_box_items_chk->isChecked()) {
            m_shape = ard::BoardItemShape::box;
        }
        else {
            m_shape = ard::BoardItemShape::text_normal;
        }
        //m_box_items = m_box_items_chk->isChecked();
        m_band_space = m_cb_band_space->currentText().toInt();

        close(); 
    });

    ADD_BUTTON2LAYOUT(h_btns, "Close", &QPushButton::close);

    MODAL_DIALOG_SIZE(h_1, QSize(600, 900));
};

QStandardItemModel* BoardTopicDropBox::generateDropSelectionModel() 
{
    assert_return_null(ard::isDbConnected(), "expected open DB");
    QStandardItemModel* sm{ nullptr };

    QStringList column_labels;
    column_labels.push_back("Board expand");
    sm = new QStandardItemModel();
    sm->setHorizontalHeaderLabels(column_labels);

    int row = 0;
    QStandardItem* si = nullptr;
    si = new QStandardItem();
    si->setData(QPixmap(":ard/images/unix/board-go-single.png"), Qt::DecorationRole);
    sm->setItem(row, 0, si);
    row++;

    si = new QStandardItem();
    si->setData(QPixmap(":ard/images/unix/board-go-right.png"), Qt::DecorationRole);
    sm->setItem(row, 0, si);
    row++;

    si = new QStandardItem();
    si->setData(QPixmap(":ard/images/unix/board-go-left.png"), Qt::DecorationRole);
    sm->setItem(row, 0, si);
    row++;

    si = new QStandardItem();
    si->setData(QPixmap(":ard/images/unix/board-go-center.png"), Qt::DecorationRole);
    sm->setItem(row, 0, si);
    row++;

    si = new QStandardItem();
    si->setData(QPixmap(":ard/images/unix/board-group-go-right.png"), Qt::DecorationRole);
    sm->setItem(row, 0, si);
    row++;

    si = new QStandardItem();
    si->setData(QPixmap(":ard/images/unix/board-group-go-down.png"), Qt::DecorationRole);
    sm->setItem(row, 0, si);
    row++;

    //sm->setHeaderData(0, Qt::Horizontal, QVariant(Qt::AlignCenter | Qt::AlignVCenter), Qt::TextAlignmentRole);
    //sm->setC

    //NameList->setColumnWidth(1, 500);
    /*
    int row = 0;
    for (int i = 0; i < 3; i++) {
        auto si = new QStandardItem();
        si->setData(QPixmap(":ard/images/unix/add-note.png"), Qt::DecorationRole);
        sm->setItem(row, 0, si);
        row++;
    }*/
    //new QStandardItem("one");

    return sm;
};

//....

/**
CalendarBox
*/
QDate CalendarBox::selectDate(const QDate& suggested_date)
{
    CalendarBox d(suggested_date);
    d.exec();
    return d.m_date_selected;
};

CalendarBox::CalendarBox(const QDate& suggested_date)
{
    m_date_selected = QDate();

    QVBoxLayout *vl = new QVBoxLayout;
    m_cal = new QCalendarWidget();
    m_cal->setStyleSheet(calendarStyleSheet());
    if (suggested_date.isValid()) {
        m_cal->setSelectedDate(suggested_date);
    }

    vl->addWidget(m_cal);

    QHBoxLayout* h1 = new QHBoxLayout();
    vl->addLayout(h1);

    ard::addBoxButton(h1, "Yes", [&]() {acceptWindow(); });
    ard::addBoxButton(h1, "Cancel", [&]() {close(); });

    MODAL_DIALOG(vl);
};

void CalendarBox::acceptWindow()
{
    m_date_selected = m_cal->selectedDate();
    close();
};


QString CalendarBox::calendarStyleSheet()
{
    QString ss = "QMenu { font-size:20px; width: 300px; left: 20px; }"
        "QToolButton {icon-size: 48px, 48px; font: 20px; height: 70px; width: 100px; }"
        "QAbstractItemView {selection-background-color: rgb(255, 174, 0);}"
        "QToolButton::menu-indicator{ width: 50px;}"
        "QToolButton::menu-indicator:pressed,"
        "QToolButton::menu-indicator:open{ top:10px; left: 10px;}"
        "QTableView { font: 30px; }"
        "QSpinBox { width: 100px; font: 20px;}"
        "QSpinBox::up-button { width:40px;}"
        "QSpinBox::down-button { width:40px;}";
    return ss;
};


/**
* LabelsCreatorBox
*/
bool LabelsCreatorBox::createLabels(const STRING_SET& lset)
{
    assert_return_false(ard::gmail(), "expected gmail");
    assert_return_false(ard::gmail()->cacheRoutes(), "expected gmail cache");

    STRING_SET existing_labels;
    STRING_SET labels2create;

    auto labels = ard::gmail()->cacheRoutes()->getLoadedLabels();
    for (auto lb : labels) {
        QString l_name = lb->labelName();
        existing_labels.insert(l_name);
    }

    for (auto s : lset) {
        if (existing_labels.find(s) == existing_labels.end()) {
            labels2create.insert(s);
        }
    }

    if (labels2create.empty()) {
        return true;
    }

    LabelsCreatorBox d(std::move(labels2create));
    d.exec();
    return true;
};


LabelsCreatorBox::LabelsCreatorBox(STRING_SET&& labels2create)
    :m_labels2create(labels2create)
{
    QVBoxLayout *vl = new QVBoxLayout;

    QLabel* lbl = new QLabel(QString("Please authorize creation of the following labels for gmail messages"));
    lbl->setWordWrap(true);
    lbl->setTextFormat(Qt::RichText);
    vl->addWidget(lbl);

    //...
    m_lview = new QListView();
    vl->addWidget(m_lview);
    QStandardItemModel* m = new QStandardItemModel();
    for (auto& s : m_labels2create) {
        auto si = new QStandardItem(s);
        m->appendRow(si);
    }
    m_lview->setModel(m);
    //...


    QHBoxLayout* h1 = new QHBoxLayout();
    vl->addLayout(h1);
    QPushButton* b;

    b = ard::addBoxButton(h1, "OK", [&]() {
        //      acceptWindow(); 
    });

    b->setDefault(true);
    b = ard::addBoxButton(h1, "Cancel", [&]() {close(); });


    MODAL_DIALOG(vl);
};

/**
GExceptionDialog
*/
void ard::resolveGoogleException(googleQt::GoogleException& e)
{
    GExceptionDialog::resolveException(e);
};

void GExceptionDialog::resolveException(googleQt::GoogleException& e)
{
    GExceptionDialog d(e);
    d.exec();
};

extern QString getEmailUserId4RecoveryMode();

static QTextEdit* produceUserIdEditor()
{
    QTextEdit* e = new QTextEdit;
    e->setAcceptRichText(false);
    e->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    e->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    QString userId = getEmailUserId4RecoveryMode();
    e->setPlainText(userId);
    QFontMetrics metrics(e->font());
    int lineHeight = metrics.height/*lineSpacing*/();
    e->setFixedHeight(lineHeight + 4);
    if (!userId.isEmpty()) {
        int idx = userId.indexOf("@");
        if (idx != -1) {
            QTextCursor c = e->textCursor();
            c.setPosition(0);
            c.setPosition(idx, QTextCursor::KeepAnchor);
            e->setTextCursor(c);
        }
    }
    return e;
}

GExceptionDialog::GExceptionDialog(googleQt::GoogleException& e)
{
    QVBoxLayout *vl = new QVBoxLayout;
    QString userId = dbp::configEmailUserId();
    if (!userId.isEmpty()) {
        QTextEdit* e = produceUserIdEditor();
        vl->addWidget(e);
        ard::addBoxButton(vl, "Change User", [=]()
        {
            ard::reconnectGoogle(e->toPlainText().trimmed());
            close();
        });
    }
    ard::addBoxButton(vl, "Reset Access Token", [&]()
    {
        ard::revokeGoogleTokenWithConfirm();
        close();
    });

    /*
    ard::addBoxButton(vl, "Disable Cloud Code", [&]()
    {
        dbp::configSetEnableCloudCode(false);
        close();
    });*/

    QLabel* lbl = new QLabel(e.what());
    lbl->setWordWrap(true);
    lbl->setTextFormat(Qt::RichText);
    vl->addWidget(lbl);

    ard::addBoxButton(vl, "Close", [&]() {close(); });

    MODAL_DIALOG(vl);
};

RecoveryBox::RecoveryBox(int signal_error)
    : QMainWindow()
{
    extern void clearRecoveryMode();

    QVBoxLayout *vl = new QVBoxLayout;

    QString err_info = "Undefined";
#define ADD_CASE(C, S) case C:err_info = QString("%1 %2").arg(#C).arg(S);break;
    switch (signal_error) {
        ADD_CASE(SIGTERM, "termination request, sent to the program");
        ADD_CASE(SIGSEGV, "invalid memory access (segmentation fault)");
        ADD_CASE(SIGINT, "external interrupt, usually initiated by the user");
        ADD_CASE(SIGILL, "invalid program image, such as invalid instruction");
        ADD_CASE(SIGABRT, "abnormal termination condition");
        ADD_CASE(SIGFPE, "erroneous arithmetic operation such as divide by zero");
    }
#undef ADD_CASE

    QLabel* l = new QLabel(QString("Unrecovered error[%1 %2] detected during last execution. Please press 'Continue' button and restart application or consult support team. Sorry for inconvenience.").arg(signal_error).arg(err_info));
    l->setWordWrap(true);
    vl->addWidget(l);

    QHBoxLayout* h1 = new QHBoxLayout();
    vl->addLayout(h1);

    ard::addBoxButton(h1, "Continue", [&]()
    {
        clearRecoveryMode();
		ard::messageBox(this, "Please start application again. The recovery mode was cleaned.");
        close();
        qWarning() << "Cleaned recovery mode";
    });
    ard::addBoxButton(h1, "Log", [&]()
    {
        SimpleLogView::runIt();
    });
    ard::addBoxButton(h1, "More..", [&]()
    {
        SupportWindow::runWindow();
    });

    ard::addBoxButton(vl, "Reset Access Token", [&]()
    {
        ard::revokeGoogleTokenWithConfirm();
        close();
        clearRecoveryMode();
		ard::messageBox(this, "Please start application again. The recovery mode was cleaned.");
        qWarning() << "Token deleted. Cleaned recovery mode";
    });

    QString userId = getEmailUserId4RecoveryMode();
    if (!userId.isEmpty()) {
        QTextEdit* e = produceUserIdEditor();
        if (e) {
            vl->addWidget(e);
            ard::addBoxButton(vl, "Change User", [=]()
            {
                dbp::configSetEmailUserId(e->toPlainText().trimmed());
                close();
                clearRecoveryMode();
				ard::messageBox(this, "Please start application again. The recovery mode was cleaned.");
                qWarning() << "User changed. Cleaned recovery mode";
            });
            e->setFocus();
        }
    }


    QWidget* ws_panel = new QWidget(this);
    ws_panel->setLayout(vl);
    setCentralWidget(ws_panel);
};


void SimpleLogView::runIt()
{
    SimpleLogView d;
    d.exec();
};

SimpleLogView::SimpleLogView()
{
    QVBoxLayout *vl = new QVBoxLayout;
    vl->setContentsMargins(0, 0, 0, 0);
    vl->setSpacing(0);
    vl->setAlignment(Qt::AlignLeft);


    QPlainTextEdit* e = new QPlainTextEdit();
    e->setReadOnly(true);
    vl->addWidget(e);
    ard::addBoxButton(vl, "Close", [&]() {close(); });

    QString  log_string = "";

    QString logPath = get_program_appdata_log_file_name();
    if (QFile::exists(logPath)) {
        QFile f(logPath);
        if (!f.open(QIODevice::ReadOnly)) {
            log_string = QString("Failed to open Log file:%1").arg(logPath);
        }
        else {
            extern QString size_human(float num);
            log_string += QString("Reading log file (%1)\n%2\n\n").arg(size_human(f.size())).arg(logPath);
#define READ_LIMIT 20000
            if (f.size() < READ_LIMIT) {
                log_string += f.readAll().data();
            }
            else {
                f.seek(f.size() - READ_LIMIT);
                log_string += f.read(READ_LIMIT).data();
            }
        }
        e->setPlainText(log_string);
        f.close();
    }
    else {
        e->setPlainText(QString("Log file not found:%1").arg(logPath));
    }

    QTimer::singleShot(10, this, [=]() {
        if (e->verticalScrollBar()) {
            e->verticalScrollBar()->setValue(e->verticalScrollBar()->maximum());
        }
    });

    MODAL_DIALOG(vl);
};



/**
*  DiagnosticsBox
*/
void DiagnosticsBox::showDiagnostics()
{
    DiagnosticsBox d;
    d.exec();
};

DiagnosticsBox::DiagnosticsBox() 
{
    QVBoxLayout *h_1 = new QVBoxLayout;
    QHBoxLayout *h_btns = new QHBoxLayout;

    auto sm = generateGoogleRequestsModel();
    if (sm) {
        m_dgn_view = gui::createTableView(sm);
        m_dgn_view->setSelectionBehavior(QAbstractItemView::SelectRows);
        h_1->addWidget(m_dgn_view);
    }
    h_1->addLayout(h_btns);

    //...
    ard::addBoxButton(h_btns, "Clear", [&]()
    {
        if (ard::confirmBox(this, "Please confirm clearing Google diagnistics stack")) {
            auto g = ard::google();
            if (g) {
                g->endpoint()->diagnosticClearRequestsList();
                close();
            }
        };
    });
    //...

    ard::addBoxButton(h_btns, "Close", [&]() { close(); });

    MODAL_DIALOG_SIZE(h_1, QSize(600, 900));
};

QStandardItemModel* DiagnosticsBox::generateGoogleRequestsModel()
{
    QStandardItemModel* sm{ nullptr };
    auto g = ard::google();
    if (g) {
        auto& dlst = g->endpoint()->diagnosticRequests();

        QStringList column_labels;
        column_labels.push_back(QString("request [%1]").arg(dlst.size()));
        sm = new QStandardItemModel();
        sm->setHorizontalHeaderLabels(column_labels);

        int row = 0;
        for (googleQt::DGN_LIST::const_reverse_iterator i = dlst.rbegin(); i != dlst.rend(); i++) {
            auto& r = *i;
            auto s = QString("%1\n%2\n%3").arg(r.context).arg(r.tag).arg(r.request);
            sm->setItem(row, 0, new QStandardItem(s));
            row++;
        };
    }

    return sm;
};

/**
ConfirmBox
*/
YesNoConfirm ard::YesNoCancel(QWidget* parent, QString msg, bool default2confirm /*= true*/)
{
    return ConfirmBox::confirm(parent, msg, default2confirm);
};

YesNoConfirm ConfirmBox::confirm(QWidget* parent, QString msg, bool default2confirm)
{
    ConfirmBox d(parent, msg, default2confirm);
    d.exec();
    return d.m_confirmed;
};

YesNoConfirm ConfirmBox::confirmWithOption(QWidget* parent, QString msg, QString optionStr) 
{
	ConfirmBox d(parent, msg, true);
	d.m_chk_option = new QCheckBox(optionStr);
	d.m_main_layout->insertWidget(1, d.m_chk_option);
	d.exec();
	return d.m_confirmed;
};

ConfirmBox::ConfirmBox(QWidget* parent, QString msg, bool default2confirm):QDialog(parent)
{
	m_main_layout = new QVBoxLayout;

    QLabel* lbl = new QLabel(msg);
    lbl->setWordWrap(true);
    lbl->setTextFormat(Qt::RichText);
	m_main_layout->addWidget(lbl);

    QHBoxLayout* h1 = new QHBoxLayout();
	m_main_layout->addLayout(h1);

    QPushButton* b;

    b = ard::addBoxButton(h1, "Yes", [&]() 
	{
		m_confirmed = YesNoConfirm::yes;
		if (m_chk_option && m_chk_option->isChecked()) {
			m_confirmed = YesNoConfirm::yes_with_option;
		}
		close(); 
	});
    if (default2confirm)b->setDefault(true);
    b = ard::addBoxButton(h1, "No", [&]() {m_confirmed = YesNoConfirm::no; close(); });
    b = ard::addBoxButton(h1, "Cancel", [&]() {m_confirmed = YesNoConfirm::cancel; close(); });
    if (!default2confirm)b->setDefault(true);

    /*
    switch (t)
    {
    case YesNo_confirm:
    case YesNoCancel_confirm:
    {
        b = ard::addBoxButton(h1, "Yes", [&]() {acceptWindow(); });
        if (default2confirm)b->setDefault(true);
        b = ard::addBoxButton(h1, "No", [&]() {close(); });
        if (t == YesNoCancel_confirm)
        {
            b = ard::addBoxButton(h1, "Cancel", [&]() {close(); });
            if (!default2confirm)b->setDefault(true);
        }
    }break;
    
    case OkCancel_confirm:
    {
        b = ard::addBoxButton(h1, "OK", [&]() {acceptWindow(); });
        if (default2confirm)b->setDefault(true);
        b = ard::addBoxButton(h1, "Cancel", [&]() {close(); });
        if (!default2confirm)b->setDefault(true);
    }break;
    case Ok_message:
    {
        b = ard::addBoxButton(h1, "OK", [&]() {close(); });
    }break; 
    }
    */

    MODAL_DIALOG(m_main_layout);
};

/**
AddPhoneOrEmailBox
*/
bool AddPhoneOrEmailBox::Result::ok()const
{
    bool r = accepted && !tlabel.isEmpty() && !data.isEmpty();
    return r;
};

AddPhoneOrEmailBox::Result AddPhoneOrEmailBox::add(EType ltype)
{
    AddPhoneOrEmailBox::Result r;

    AddPhoneOrEmailBox d(ltype, "", "");
    d.exec();
    r.accepted = d.m_accepted;
    if (d.m_accepted) {
        r.tlabel = googleQt::trim_alpha_label(d.m_cb_labels->currentText());
        r.data = d.m_edit->text().trimmed();
    }
    return r;
};

AddPhoneOrEmailBox::Result AddPhoneOrEmailBox::edit(EType ltype, QString tlabel, QString data)
{
    AddPhoneOrEmailBox::Result r;

    AddPhoneOrEmailBox d(ltype, tlabel, data);
    d.exec();
    r.accepted = d.m_accepted;
    if (d.m_accepted) {
        r.tlabel = googleQt::trim_alpha_label(d.m_cb_labels->currentText());
        r.data = d.m_edit->text().trimmed();
    }
    return r;
};

AddPhoneOrEmailBox::AddPhoneOrEmailBox(EType add_type, QString tlabel, QString data)
{
    std::vector<QString> opt_lbl_list;
    opt_lbl_list.push_back("home");
    opt_lbl_list.push_back("work");
    opt_lbl_list.push_back("other");

    m_cb_labels = new QComboBox;
    m_cb_labels->setEditable(true);

    for (const auto& s : opt_lbl_list) {
        m_cb_labels->addItem(s);
    }

    if (!tlabel.isEmpty()) {
        m_cb_labels->setCurrentText(tlabel);
    }

    auto ctrl_layout = new QVBoxLayout;
    ctrl_layout->addWidget(m_cb_labels);
    QString lbl = "";
    switch (add_type) {
    case EType::addPhone:
    {
        lbl = "Phone";
    }break;
    case EType::addEmail: {
        lbl = "Email";
    }break;
    default: {
        lbl = "---";
    }
    }

    ctrl_layout->addWidget(new QLabel(lbl));
    m_edit = new QLineEdit;
    if (!data.isEmpty()) {
        m_edit->setText(data);
    }
    ctrl_layout->addWidget(m_edit);

    QHBoxLayout *h_2 = new QHBoxLayout;
    ard::addBoxButton(h_2, "OK", [&]() {m_accepted = true; close(); });
    ard::addBoxButton(h_2, "Cancel", [&]() {m_accepted = false; close(); });

    QVBoxLayout *v_main = new QVBoxLayout;

    v_main->addLayout(ctrl_layout);
    v_main->addLayout(h_2);

    MODAL_DIALOG(v_main);
};

/**
find_box
*/
QString ard::find_box::findWhat(QString s)
{
	find_box d(s);
    d.exec();
    return d.m_what;
};

ard::find_box::find_box(QString s)
{
    qDebug() << "<<FindBox::create";
    QVBoxLayout *vl = new QVBoxLayout;

    QHBoxLayout* h0 = new QHBoxLayout();
    vl->addLayout(h0);

    QLabel* lbl = new QLabel("Find what:");
    h0->addWidget(lbl);
    m_edit = new QLineEdit;
    m_edit->setText(s);
    h0->addWidget(m_edit);

    QHBoxLayout* h1 = new QHBoxLayout();
    vl->addLayout(h1);

    ard::addBoxButton(h1, "Yes", [&]() {acceptWindow(); });
    ard::addBoxButton(h1, "Cancel", [&]() {close(); });

    MODAL_DIALOG(vl);
};

void ard::find_box::acceptWindow()
{
    m_what = m_edit->text();
    close();
};


/**
    OptionsBox
*/
void OptionsBox::showOptions() 
{
    OptionsBox d;
    d.exec();
};

#define FontSampleText  "The quick brown fox jumps over the lazy dog"

OptionsBox::OptionsBox() 
{
    m_tab = new QTabWidget();
    m_tab->setTabPosition(QTabWidget::East);
    auto vl = new QVBoxLayout();
    vl->addWidget(m_tab);

#ifdef ARD_OPENSSL
    /// sync
    addSyncTab();
#endif

    addFontsTab();
    addMiscTab();

    QHBoxLayout* h1 = new QHBoxLayout();
    vl->addLayout(h1);
    QPushButton* b;
    b = ard::addBoxButton(h1, "OK", [&]()
    {
        auto NotesFont = m_combo_font->currentText();
        dbp::configFileSetNoteFontFamily(NotesFont);
        auto NotesFontSize = m_combo_size->currentText().toInt();
        if (NotesFontSize > 6) {
            dbp::configFileSetNoteFontSize(NotesFontSize);
        }

        if (m_selected_font_size != 0) {
            ard::resetAllFonts(m_selected_font_size);
			ard::messageBox(this, QString("It is recommended to restart Ardi to apply changes."));
        }
        else {
            ard::resetNotesFonts();
        }

		dbp::configFileSetMaiBoardSchedule(m_schedule_board->isChecked());

		bool mail_filter_applied = false;
		bool set_it = false;
        if (m_sys_tray)dbp::configFileSetRunInSysTray(m_sys_tray->isChecked());
		if (m_filter_inbox) {
			set_it = m_filter_inbox->isChecked();
			if (set_it != dbp::configFileFilterInbox()) {
				dbp::configFileSetFilterInbox(set_it);
				mail_filter_applied = true;
			}
		}
		if (m_prefilter_inbox) {
			set_it = m_prefilter_inbox->isChecked();
			if (set_it != dbp::configFilePreFilterInbox()) {
				dbp::configFileSetPreFilterInbox(set_it);
				mail_filter_applied = true;
			}
		}

		if (mail_filter_applied) {
			auto h = ard::hoisted();
			if (h) 
			{
				auto rr = dynamic_cast<ard::rule_runner*>(h);
				if (rr) {
					rr->qrelist(true, QString("changed-filter-option [%1][%2]").arg(dbp::configFileFilterInbox()).arg(dbp::configFilePreFilterInbox()));
					gui::rebuildOutline();
					//rr->make_gui_query();
				}
			}
			//gui::rebuildOutline();
		}

        close();
    });
    b = ard::addBoxButton(h1, "Cancel", [&]() {close(); });
    b->setDefault(true);

    setLayout(vl);
};

void OptionsBox::addFontsTab() 
{
    auto main_fonts = new QVBoxLayout();
    auto vl_fonts = new QVBoxLayout();

    QWidget* wfont_space = new QWidget;
    wfont_space->setLayout(main_fonts);

    m_tab->addTab(wfont_space, "Fonts");

    auto currFnt = ard::defaultFont();
    int fnt_size = currFnt->pointSize();
    //qDebug() << "ykh-fnt-default-size" << fnt_size;

    auto lbl = new QLabel("Main Outline font.");
    lbl->setFont(*ard::defaultBoldFont());
    main_fonts->addWidget(lbl, 0, Qt::AlignTop);
    main_fonts->addLayout(vl_fonts);

    for (int i = ard::defaultFallbackFontSize() - 4; i <= ard::defaultFallbackFontSize() + 4; i += 2) {
        QFont fnt = *(ard::defaultFont());
        auto btn = new QToolButton;
        btn->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);

        auto bf = BtnFont{ i };
        m_b2bf[btn] = bf;

        fnt.setPixelSize(i);
        btn->setFont(fnt);
        btn->setText(FontSampleText);
        vl_fonts->addWidget(btn);

        connect(btn, &QPushButton::released, [=]()
        {
            auto i = m_b2bf.find(btn);
            if (i != m_b2bf.end()) {
                resetCurrentOutlineFontMark(i->second.fnt_size);
                m_selected_font_size = i->second.fnt_size;
            }
        });
    }

    //..
    auto line = new QWidget;
    line->setFixedHeight(2);
    line->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    line->setStyleSheet(QString("background-color: #c0c0c0;"));
    main_fonts->addWidget(line);
    //..

    lbl = new QLabel("Default Note font.");
    lbl->setFont(*ard::defaultBoldFont());
    main_fonts->addWidget(lbl, 0, Qt::AlignTop);

    QHBoxLayout* note_font_hlayout = new QHBoxLayout();
    m_combo_font = new QFontComboBox;
    m_combo_size = new QComboBox;

    note_font_hlayout->addWidget(m_combo_font, 1);
    note_font_hlayout->addWidget(m_combo_size);
    main_fonts->addLayout(note_font_hlayout);

    QFontDatabase db;
    foreach(int size, db.standardSizes())
        m_combo_size->addItem(QString::number(size));

    auto noteFamily = dbp::configFileNoteFontFamily();
    if (!noteFamily.isEmpty())
    {
       // qDebug() << "Fcombo-items" << m_combo_font->count();
        auto idx = m_combo_font->findText(noteFamily);
        if (idx == -1) {
            idx = m_combo_font->findText(noteFamily, Qt::MatchContains);
        }
        if (idx != -1) {
            m_combo_font->setCurrentIndex(idx);
        }
    }

    auto noteFontSize = dbp::configFileNoteFontSize();
    auto idx = m_combo_size->findText(QString("%1").arg(noteFontSize));
    if (idx != -1) {
        m_combo_size->setCurrentIndex(idx);
    }

    m_edit_font_sample_line = new QLabel("Note sample text");
    main_fonts->addWidget(m_edit_font_sample_line, 0, Qt::AlignHCenter);
    updateNoteFontSampleText();
    connect(m_combo_font, SIGNAL(currentIndexChanged(int)),
        this, SLOT(notesFontIndexChanged(int)));
    connect(m_combo_size, SIGNAL(currentIndexChanged(int)),
        this, SLOT(notesFontIndexChanged(int)));

    main_fonts->addWidget(new QWidget(), 1);//spacer

    resetCurrentOutlineFontMark(fnt_size);
};

void OptionsBox::addMiscTab() 
{
    auto misc_l = new QVBoxLayout();
    QWidget* misc_space = new QWidget;
    misc_space->setLayout(misc_l);

    m_tab->addTab(misc_space, "Misc");

	std::function<QCheckBox*(QString, bool)> addCheckbox = [&](QString title, bool is_checked) 
	{
		auto rv = new QCheckBox(title);
		if (is_checked)rv->setChecked(true);		
		misc_l->addWidget(rv);
		return rv;
	};

	m_sys_tray = addCheckbox("Run in system tray", dbp::configFileGetRunInSysTray());
	m_filter_inbox = addCheckbox("Enable Mail Inbox filter", dbp::configFileFilterInbox());
	m_prefilter_inbox = addCheckbox("Prefilter Mail Inbox", dbp::configFilePreFilterInbox());
	m_schedule_board = addCheckbox("Schedule Mail Board rules", dbp::configFileMaiBoardSchedule());
	misc_l->addWidget(ard::newSpacer(true));
    /*m_sys_tray = new QCheckBox("Run in system tray");
    if (dbp::configFileGetRunInSysTray()) {
        m_sys_tray->setChecked(true);
    }
	misc_l->addWidget(m_sys_tray);

	m_filter_inbox = new QCheckBox("Enable Mail Inbox filter");
	if (dbp::configFileFilterInbox()) {
		m_filter_inbox->setChecked(true);
	}
	m_prefilter_inbox = new QCheckBox("Prefilter Mail Inbox");
	if (dbp::configFilePreFilterInbox()) {
		m_prefilter_inbox->setChecked(true);
	}
	*/

    
};

#ifdef ARD_OPENSSL
void OptionsBox::addSyncTab()
{
    QString userid = dbp::configEmailUserId();
    QString label_str = "Ardi can use available cloud storage to maintain backups and synchronize data between "
        "instances of Ardi running on Desktop and smartphones. "
        "Ardi uses proven industry standard algorithm(AES 256) to encrypt data replicated to cloud.";
    if (userid.isEmpty()) {
        label_str = "Please authorize GDrive access for Ardi, so we can setup user name and can proceed with data synchronization.";
    }


    auto lb = new QLabel(label_str);

    lb->setWordWrap(true);

    auto main_sync = new QVBoxLayout();

    QWidget* wsync_space = new QWidget;
    wsync_space->setLayout(main_sync);

    main_sync->addWidget(lb);

    QHBoxLayout* hl1 = new QHBoxLayout();
    hl1->addWidget(new QLabel("Account:"));
    auto e = new QLineEdit(userid);
    e->setReadOnly(true);
    hl1->addWidget(e);
    main_sync->addLayout(hl1);

    if (userid.isEmpty()) {
        auto b = new QPushButton("Setup account");
        main_sync->addWidget(b);
        connect(b, &QPushButton::released, [=]()
        {
            if (!gui::isConnectedToNetwork()) {
				ard::messageBox(this, ("Network connection not detected.");
                return;
            }

            ard::authAndConnectNewGoogleUser();
            close();
            //processPasswordChange();
        });
    }

    m_sync_check = new QCheckBox("Enable Cloud Synchronization");
    main_sync->addWidget(m_sync_check);

    m_sync_change_pwd = new QPushButton("Change password");
    main_sync->addWidget(m_sync_change_pwd);

    if (userid.isEmpty()) {
        m_sync_check->setEnabled(false);
        m_sync_change_pwd->setEnabled(false);
    }

    if (dbp::configFileIsSyncEnabled()) {
        m_sync_check->setChecked(true);
    };

    connect(m_sync_change_pwd, &QPushButton::released, [=]()
    {
        processPasswordChange();
    });


    connect(m_sync_check, &QCheckBox::stateChanged, [&](int) {
        auto userid = dbp::configEmailUserId();
        if (userid.isEmpty()) {
			ard::messageBox(this, "Please setup account first.");
            return;
        }

        if (m_sync_check->isChecked()) {
            ard::CryptoConfig& cfg = ard::CryptoConfig::cfg();
            if (!cfg.hasPassword() && !cfg.hasPasswordChangeRequest()) {
                processPasswordChange();
            }
        }
        else {
            if (dbp::configFileIsSyncEnabled()) {
                if (ard::confirmBox(this, "Please confirm disabling cloud synchronisation. You can enable it later but will have to reenter password. Please make sure you stored password used to encrypt Ardi cloud storage.")) {
                    ard::CryptoConfig& cfg = ard::CryptoConfig::cfg();
                    cfg.purgeCryptoConfig();
                }
            }
        }
        //}
    });

    main_sync->addWidget(new QWidget(), 1);//spacer

    m_tab->addTab(wsync_space, "Synchronization");

};

void OptionsBox::processPasswordChange()
{
    if (SyncPasswordBox::changePassword()) {
        if (ard::confirmBox(this, "Cloud data will be encrypted with new password after synchronization. Would you like to start synchronization now (requires internet connection)?")) {
            ard::asyncExec(AR_synchronize);
            close();
        }
    };
};
#endif

void OptionsBox::resetCurrentOutlineFontMark(int fontSize)
{
    for (auto& bf : m_b2bf) {
        if (bf.second.fnt_size == fontSize) {
            bf.first->setIcon(ard::iconFromTheme("check"));
        }
        else{
            bf.first->setIcon(QIcon());
        }
        bf.first->setText(FontSampleText);
    }
};

void OptionsBox::notesFontIndexChanged(int) 
{
    updateNoteFontSampleText();
};

void OptionsBox::updateNoteFontSampleText()
{
    auto fnt = *ard::defaultNoteFont();
    fnt.setFamily(m_combo_font->currentText());
    fnt.setPointSize(m_combo_size->currentText().toInt());
    m_edit_font_sample_line->setFont(fnt);
    //m_edit_font_sample_line
    //m_edit_font_sample_line
};


#ifdef ARD_OPENSSL
/**
    SyncPasswordBox
*/
bool SyncPasswordBox::changePassword()
{
    bool rv = false;
    SyncPasswordBox d;
    d.exec();
    if (d.m_accepted) {
        rv = ard::CryptoConfig::cfg().hasPasswordChangeRequest();
    }

    return rv;
};

SyncPasswordBox::SyncPasswordBox() 
{
    QVBoxLayout *v_main = new QVBoxLayout;

    QLabel* lbl = new QLabel(QString("Please use good password that you can remmember. It will be used to protect data stored on cloud."));
    lbl->setWordWrap(true);
    lbl->setTextFormat(Qt::RichText);
    v_main->addWidget(lbl);

    //
    int gidx = 0;
    QGridLayout* pwdbox = new QGridLayout;
    auto& cfg = ard::CryptoConfig::cfg();
    if (cfg.hasPassword()) {
        pwdbox->addWidget(new QLabel("Old Password:"), gidx, 0);
        m_old_pwd = new QLineEdit(this);
        m_old_pwd->setEchoMode(QLineEdit::PasswordEchoOnEdit);
        pwdbox->addWidget(m_old_pwd, gidx, 1);
        gidx++;

        QLabel* lbl = new QLabel(QString("<b>%1</b>").arg(cfg.syncCrypto().second.hint));
        lbl->setWordWrap(true);
        lbl->setTextFormat(Qt::RichText);

        pwdbox->addWidget(new QLabel("Hint:"), gidx, 0);
        pwdbox->addWidget(lbl, gidx, 1);
        gidx++;
    };

    pwdbox->addWidget(new QLabel("Password:"), gidx, 0);
    m_pwd = new QLineEdit(this);
    m_pwd->setEchoMode(QLineEdit::PasswordEchoOnEdit);
    pwdbox->addWidget(m_pwd, gidx, 1);
    gidx++;

    pwdbox->addWidget(new QLabel("Confirm:"), gidx, 0);
    m_pwd2 = new QLineEdit(this);
    m_pwd2->setEchoMode(QLineEdit::PasswordEchoOnEdit);
    pwdbox->addWidget(m_pwd2, gidx, 1);
    gidx++;

    pwdbox->addWidget(new QLabel("Hint:"), gidx, 0);
    m_pwd_hint = new QLineEdit(this);
    pwdbox->addWidget(m_pwd_hint, gidx, 1);
    v_main->addLayout(pwdbox);
    gidx++;
//..

    QHBoxLayout* h1 = new QHBoxLayout();
    v_main->addLayout(h1);
    QPushButton* b;

    b = ard::addBoxButton(h1, "OK", [&]() {
        auto pwd = m_pwd->text().trimmed();
        auto pwd2 = m_pwd2->text().trimmed();
        auto pwd_hint = m_pwd_hint->text().trimmed();

        if (pwd.isEmpty()) {
			ard::messageBox(this, "Password can't be empty");
            return;
        }

        if (pwd != pwd2) {
			ard::messageBox(this, "Passwords don't match");
            return;
        }

        QString old_pwd = "";
        if (m_old_pwd) {
            old_pwd = m_old_pwd->text().trimmed();
        }

        if (old_pwd == pwd) {
			ard::messageBox(this, "Password should be different from old passwdord");
            return;
        }

        auto r = ard::CryptoConfig::cfg().request2ChangeSyncPassword(pwd, old_pwd, pwd_hint);
        if (r != ard::aes_status::ok) {
			ard::errorBox(this, QString("Change passsword failed with error '%1'").arg(ard::aes_status2str(r)));
            return;
        }
        m_accepted = true;
        close();
    });

    b->setDefault(true);
    b = ard::addBoxButton(h1, "Cancel", [&]() {close(); });

    MODAL_DIALOG(v_main);
};
#endif

/**
    SyncOldPasswordBox
*/
QString SyncOldPasswordBox::getOldPassword(ard::aes_result last_dec_res) 
{
    SyncOldPasswordBox d(last_dec_res);
    d.exec();
    return d.m_try_old_pwd;
};

SyncOldPasswordBox::SyncOldPasswordBox(ard::aes_result r) 
{
    QVBoxLayout *v_main = new QVBoxLayout;

    QLabel* lbl = new QLabel(QString(QString("Please enter password that was used to synchronize with the archive. The archive was entrypted on <b>%1</b> with following hint: <b><font color=red>%2</font></b>").arg(r.encr_date.toString()).arg(r.hint)));
    lbl->setWordWrap(true);
    lbl->setTextFormat(Qt::RichText);
    v_main->addWidget(lbl);

    m_old_pwd = new QLineEdit(this);
    v_main->addWidget(m_old_pwd);

    QHBoxLayout* h1 = new QHBoxLayout();
    v_main->addLayout(h1);
    QPushButton* b;

    b = ard::addBoxButton(h1, "OK", [&]() {
        m_try_old_pwd = m_old_pwd->text().trimmed();
        close();
    });

    b = ard::addBoxButton(h1, "Cancel", [&]() {close(); });

    MODAL_DIALOG(v_main);
};

/**
    GenericPasswordBox
*/
QString GenericPasswordEnterBox::enter_password(QString descr) 
{
    GenericPasswordEnterBox d(descr);
    d.exec();
    return d.m_pwd;
};

GenericPasswordEnterBox::GenericPasswordEnterBox(QString descr)
{
    QVBoxLayout *v_main = new QVBoxLayout;

    if (!descr.isEmpty()) {
        QLabel* lbl = new QLabel(descr);
        lbl->setWordWrap(true);
        lbl->setTextFormat(Qt::RichText);
        v_main->addWidget(lbl);
    }

    m_edit_pwd = new QLineEdit(this);
    v_main->addWidget(m_edit_pwd);

    QHBoxLayout* h1 = new QHBoxLayout();
    v_main->addLayout(h1);
    QPushButton* b;

    b = ard::addBoxButton(h1, "OK", [&]() {
        m_pwd = m_edit_pwd->text().trimmed();
        close();
    });

    b = ard::addBoxButton(h1, "Cancel", [&]() {close(); });

    MODAL_DIALOG(v_main);

};

/**
    GenericPasswordCreateBox
*/
ard::gui_pwd_info GenericPasswordCreateBox::create_password(QString descr, bool provide_old_pwd)
{
    GenericPasswordCreateBox d(descr, provide_old_pwd);
    d.exec();
    ard::gui_pwd_info rv;
    if (d.m_accepted) {
        rv.password = d.m_pwd->text().trimmed();
        if (d.m_old_pwd) {
            rv.old_password = d.m_old_pwd->text().trimmed();
        }
        rv.hint = d.m_pwd_hint->text().trimmed();
    }
    return rv;
};

GenericPasswordCreateBox::GenericPasswordCreateBox(QString descr, bool provide_old_pwd)
{
    QVBoxLayout *v_main = new QVBoxLayout;

    if (!descr.isEmpty()) {
        QLabel* lbl = new QLabel(descr);
        lbl->setWordWrap(true);
        lbl->setTextFormat(Qt::RichText);
        v_main->addWidget(lbl);
    }

    int gidx = 0;
    QGridLayout* pwdbox = new QGridLayout;

    if (provide_old_pwd) {
        pwdbox->addWidget(new QLabel("Old Password:"), gidx, 0);
        m_old_pwd = new QLineEdit(this);
        m_old_pwd->setEchoMode(QLineEdit::PasswordEchoOnEdit);
        pwdbox->addWidget(m_pwd, gidx, 1);
        gidx++;
    }

    pwdbox->addWidget(new QLabel("Password:"), gidx, 0);
    m_pwd = new QLineEdit(this);
    m_pwd->setEchoMode(QLineEdit::PasswordEchoOnEdit);
    pwdbox->addWidget(m_pwd, gidx, 1);
    gidx++;

    //m_pwd = new QLineEdit(this);
    //v_main->addWidget(m_pwd);

//  m_pwd2 = new QLineEdit(this);
//  v_main->addWidget(m_pwd2);
    pwdbox->addWidget(new QLabel("Repeat:"), gidx, 0);
    m_pwd2 = new QLineEdit(this);
    m_pwd2->setEchoMode(QLineEdit::PasswordEchoOnEdit);
    pwdbox->addWidget(m_pwd2, gidx, 1);
    gidx++;

    pwdbox->addWidget(new QLabel("Hint:"), gidx, 0);
    m_pwd_hint = new QLineEdit(this);
    m_pwd_hint->setEchoMode(QLineEdit::PasswordEchoOnEdit);
    pwdbox->addWidget(m_pwd_hint, gidx, 1);
    gidx++;

    v_main->addLayout(pwdbox);

    QHBoxLayout* h1 = new QHBoxLayout();
    v_main->addLayout(h1);
    QPushButton* b;

    b = ard::addBoxButton(h1, "OK", [&]() {
        auto pwd = m_pwd->text().trimmed();
        auto pwd2 = m_pwd2->text().trimmed();
        auto pwd_hint = m_pwd_hint->text().trimmed();

        if (pwd.isEmpty()) {
			ard::messageBox(this, "Password can't be empty");
            return;
        }

        if (pwd != pwd2) {
			ard::messageBox(this, "Passwords don't match");
            return;
        }

        QString old_pwd = "";
        if (m_old_pwd) {
            old_pwd = m_old_pwd->text().trimmed();
        }

        if (old_pwd == pwd) {
			ard::messageBox(this, "Password should be different from old passwdord");
            return;
        }

        m_accepted = true;
        close();
    });

    b = ard::addBoxButton(h1, "Cancel", [&]() {close(); });

    MODAL_DIALOG(v_main);
};


QString gui::enter_password(QString descr)
{
    return GenericPasswordEnterBox::enter_password(descr);
};

ard::gui_pwd_info gui::change_password(QString descr, bool provide_old_pwd)
{
    auto rv = GenericPasswordCreateBox::create_password(descr, provide_old_pwd);

    return rv;
};

/**
BoardItemEditArrow
*/
BoardItemEditArrowBox::BoardItemEditArrowBox(ard::selector_board* bb, ard::board_item* origin, ard::board_item* target, ard::board_link* lnk)
    :m_bb(bb), m_origin(origin), m_target(target), m_link(lnk)
{
    auto lbl_src = new QLabel(m_origin->refTopic()->impliedTitle());
    lbl_src->setWordWrap(true);
    lbl_src->setTextFormat(Qt::RichText);

    auto lbl_target = new QLabel(target->refTopic()->impliedTitle());
    lbl_target->setWordWrap(true);
    lbl_target->setTextFormat(Qt::RichText);


    m_edit = new QLineEdit;
    m_edit->setText(m_link->linkLabel());
    auto ctrl_layout = new QVBoxLayout;

    QHBoxLayout *h_1 = new QHBoxLayout;
    h_1->addWidget(lbl_src);
    h_1->addWidget(new QLabel(" -> "));
    h_1->addWidget(lbl_target);

    ctrl_layout->addLayout(h_1);
    ctrl_layout->addWidget(m_edit);

    QHBoxLayout *h_2 = new QHBoxLayout;
    ard::addBoxButton(h_2, "OK", [&]() 
    {
        m_modified = true;
        auto c = m_bb->syncDb();
        m_link->setLinkLabel(m_edit->text(), c);
        close(); 
    });
    ard::addBoxButton(h_2, "All Arrows from origin", [&]() {BoardItemArrowsBox::showArrows(m_bb, m_origin); close(); });
    ard::addBoxButton(h_2, "Cancel", [&]() {close(); });

    QVBoxLayout *v_main = new QVBoxLayout;

    v_main->addLayout(ctrl_layout);
    v_main->addLayout(h_2);

    MODAL_DIALOG(v_main);
};

bool BoardItemEditArrowBox::editArrow(ard::selector_board* b, ard::board_item* origin, ard::board_item* target, ard::board_link* lnk)
{
    BoardItemEditArrowBox d(b, origin, target, lnk);
    d.exec();
    return d.m_modified;
};

/*
class ArrowsTableView : public ArdTableView
{
public:
protected:
//  void     keyPressEvent(QKeyEvent * e)override;
    void     keyReleaseEvent(QKeyEvent * e)override;
};


void ArrowsTableView::keyPressEvent(QKeyEvent * e)
{
    if (e->key() == Qt::Key_Return)
    {
        // we captured the Enter key press, now we need to move to the next row
        auto currRow = currentIndex().row();
        auto currCol = currentIndex().column();
        qint32 nNextRow = currRow + 1;
        if (nNextRow + 1 > model()->rowCount(currentIndex()))
        {
            // we are all the way down, we can't go any further
            nNextRow = nNextRow - 1;
        }

        if (state() == QAbstractItemView::EditingState)
        {
            // if we are editing, confirm and move to the row below
            auto m = model();
            if (m) {
                auto s = m->index(currRow, currCol).data().toString();
                qDebug() << "edit: (" << currRow << currCol << ")" << s;

                QModelIndex oNextIndex = m->index(nNextRow, currCol);
                setCurrentIndex(oNextIndex);
                selectionModel()->select(oNextIndex, QItemSelectionModel::ClearAndSelect);
            }
        }
        else
        {
            // if we're not editing, start editing
            edit(currentIndex());
        }
    }
    else
    {
        // any other key was pressed, inform base class
        ArdTableView::keyPressEvent(e);
    }
};

void ArrowsTableView::keyReleaseEvent(QKeyEvent * e) 
{
    ArdTableView::keyReleaseEvent(e);

    //if (wasEdited) {
        auto currRow = currentIndex().row();
        auto currCol = currentIndex().column();

        auto m = model();
        if (m) {
            auto s = m->index(currRow, currCol).data().toString();
            qDebug() << "edit: (" << currRow << currCol << ")" << s;
        }
    //}
};*/


/**
BoardItemArrowsBox
*/
void BoardItemArrowsBox::showArrows(ard::selector_board* b, ard::board_item* bi)
{
    BoardItemArrowsBox d(b, bi);
    d.exec();
};

BoardItemArrowsBox::BoardItemArrowsBox(ard::selector_board* b, ard::board_item* bi):m_bb(b), m_bitem(bi)
{
    QVBoxLayout *h_1 = new QVBoxLayout;
    QHBoxLayout *h_btns = new QHBoxLayout;

    QLabel* lbl = new QLabel(bi->refTopic()->impliedTitle());
    lbl->setWordWrap(true);
    lbl->setTextFormat(Qt::RichText);
    h_1->addWidget(lbl);

    auto sm = generateLinksModel();
    if (sm) {
        m_links_view = new ArdTableView;
        m_links_view->setModel(sm);

        //gui::createTableView(sm, m_links_view);
        //m_links_view = gui::createTableView(sm);
        m_links_view->setSelectionBehavior(QAbstractItemView::SelectRows);
        h_1->addWidget(m_links_view);

        QHeaderView* h = m_links_view->horizontalHeader();
        h->setSectionResizeMode(1, QHeaderView::Stretch);
        //m_links_view->setColumnWidth(1, 100);
    }
    h_1->addLayout(h_btns);

    //...
    ard::addBoxButton(h_btns, "Clear", [&]()
    {
        if (ard::confirmBox(this, "Please confirm removing all links for item.")) {
            m_bb->removeBoardLinks4Origin(m_bitem);
            //auto i = m_bb->adjList().find(m_bitem);
            //if (i != m_bb->adjList().end())
            //{
            //}
            ard::asyncExec(AR_BoardRebuild, m_bb->id());
            close();
        };
    });
    //...
    ard::addBoxButton(h_btns, "OK", [&]() { if (storeLinks()) { ard::asyncExec(AR_BoardRebuild, m_bb->id()); } close(); });
    ard::addBoxButton(h_btns, "Cancel", [&]() { close(); });

    MODAL_DIALOG_SIZE(h_1, QSize(600, 900));
}

QStandardItemModel* BoardItemArrowsBox::generateLinksModel()
{
    QStandardItemModel* sm = new QStandardItemModel();

    QStringList column_labels;
    column_labels.push_back("<->");
    column_labels.push_back("Target");
    column_labels.push_back("Label");
    sm->setHorizontalHeaderLabels(column_labels);

    auto fnt = ard::defaultFont();

    int row = 0;
    bool add_origin = true;
    if (add_origin)
    {
        auto i = m_bb->adjList().find(m_bitem);
        if (i != m_bb->adjList().end())
        {
            auto& targets = i->second;
            for (auto j : targets) 
            {
                auto target = j.first;
                auto link_lst = j.second;
                if (link_lst->size() > 0)
                {
                    for (int i = 0; i < static_cast<int>(link_lst->size()); i++)
                    {
                        QVariant v_is_origin, v_title, v_link_label;
                        v_is_origin.setValue(true);

                        auto it_direction = new QStandardItem("->");
                        sm->setItem(row, 0, it_direction);
                        it_direction->setData(v_is_origin);
                        it_direction->setData(QVariant(QBrush(Qt::blue)), Qt::ForegroundRole);
                        it_direction->setData(QVariant(*fnt), Qt::FontRole);

                        auto ll = link_lst->getAt(i);
                        auto it_title = new QStandardItem(target->refTopic()->title());
                        v_title.setValue(static_cast<void*>(target));
                        it_title->setData(v_title);
                        //it_title->setData(QVariant(*fnt), Qt::FontRole);

                        sm->setItem(row, 1, it_title);
                        auto it_link_label = new QStandardItem(ll->linkLabel());
                        v_link_label.setValue(static_cast<void*>(ll));
                        it_link_label->setData(v_link_label);
                        //it_link_label->setData(QVariant(*fnt), Qt::FontRole);
                        sm->setItem(row, 2, it_link_label);
                        row++;
                    }
                }
            }
        }
    }///origin

    bool add_target = true;
    if (add_target)
    {
        auto i = m_bb->radjList().find(m_bitem);
        if (i != m_bb->radjList().end())
        {
            auto& origins = i->second;
            for (auto j : origins) 
            {
                auto origin = j.first;
                auto link_lst = j.second;
                if (link_lst->size() > 0)
                {
                    for (int i = 0; i < static_cast<int>(link_lst->size()); i++)
                    {
                        QVariant v_is_origin, v_title, v_link_label;
                        v_is_origin.setValue(false);

                        auto it_direction = new QStandardItem("<-");
                        sm->setItem(row, 0, it_direction);
                        it_direction->setData(v_is_origin);
                        it_direction->setData(QVariant(QBrush(Qt::red)), Qt::ForegroundRole);
                        it_direction->setData(QVariant(*fnt), Qt::FontRole);

                        auto ll = link_lst->getAt(i);
                        auto it_title = new QStandardItem(origin->refTopic()->title());
                        v_title.setValue(static_cast<void*>(origin));
                        it_title->setData(v_title);
                        //it_title->setData(QVariant(*fnt), Qt::FontRole);

                        sm->setItem(row, 1, it_title);
                        auto it_link_label = new QStandardItem(ll->linkLabel());
                        v_link_label.setValue(static_cast<void*>(ll));
                        it_link_label->setData(v_link_label);
                        //it_link_label->setData(QVariant(*fnt), Qt::FontRole);
                        sm->setItem(row, 2, it_link_label);
                        row++;
                    }
                }
            }
        }
    }

    return sm;
};


bool BoardItemArrowsBox::storeLinks()
{
    bool modified = false;
//  qDebug() << "======== begin/storeLinks ===========";
    auto m = dynamic_cast<QStandardItemModel*>(m_links_view->model());
    for (int r = 0; r < m->rowCount(); r++) 
    {
        auto it_title = m->item(r, 1);
        auto it_link_label = m->item(r, 2);
        QVariant v_title = it_title->data();
        QVariant v_link_label = it_link_label->data();
        auto target_or_origin = static_cast<ard::board_item*>(v_title.value<void*>());
        auto lnk = static_cast<ard::board_link*>(v_link_label.value<void*>());
        auto it = target_or_origin->refTopic();
        auto s = it_title->text();
        if (s != it->title()) {
        //  qDebug() << "modified-title" << r << s;// << "|" << lnk->linkLabel();
            it->setTitle(s);
            modified = true;
        }

        s = it_link_label->text();
        if (lnk->linkLabel() != s) {
        //  qDebug() << "modified-label" << r << s;// << "|" << lnk->linkLabel();
            lnk->setLinkLabel(s, m_bb->syncDb());
            modified = true;
        }
    }
    return modified;
//  qDebug() << "======== end/storeLinks ===========";
};

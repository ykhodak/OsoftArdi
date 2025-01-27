#pragma once

#include <QDialog>
#include <map>
#include <QModelIndex>
#include "anfolder.h"
#include "custom-widgets.h"
#include "custom-boxes.h"

class QButtonGroup;
class QLineEdit;
class QComboBox;
class QStandardItemModel;
class QTableView;
class QLabel;
class QTabWidget;
class QPlainTextEdit;
class QVBoxLayout;

/**
   topic details/properties
*/
class TopicBox : public AsyncSingletonBox
{
    Q_OBJECT
public:
    static void showTopic(topic_ptr f);
    void reloadBox()override {};

protected:
    TopicBox(topic_ptr f);
    virtual ~TopicBox();

    void         addTab(QTableView* v, QString tab_label, QWidget* wrapper_widget = nullptr);
    QStandardItemModel* generateCurrentTopicModel();
    QStandardItemModel* generateCurrentEmailModel();
    QStandardItemModel* generateCurrentEmailThreadModel();
    QStandardItemModel* generateCurrentContactModel();
    QStandardItemModel* generateNoteModel();
    QStandardItemModel* generateDraftModel();
    QString generateCurrentEmailSnapshotFromCloud();
    QTableView*         createTableView(QStandardItemModel* );

public slots:
    void locateEThread();

    void setEmailDetails(QString s);

protected:
    using TABLE_VIEW_INDEX = std::map<int, QTableView*>;
    QTabWidget*         m_tabs;
    TABLE_VIEW_INDEX    m_idx2tableview;
    QPlainTextEdit*     m_email_snapshot;
    QVBoxLayout*        m_prop_layout{nullptr};
    QPushButton*        m_imgButton{nullptr};
    QTableView*         m_img_view{ nullptr };
    QPlainTextEdit*     m_url{nullptr};
    topic_ptr           m_topic{nullptr};
};

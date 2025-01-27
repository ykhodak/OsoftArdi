#pragma once

#include <QDate>
#include <QToolButton>
#include <QGraphicsView>
#include <QLabel>
#include <QTableView>
#include <QTextBrowser>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDialog>
#include <QLineEdit>
#include <QMainWindow>
#include <QTimeLine>
#include <QGraphicsRectItem>
#include <QSplitter>
#include <QComboBox>
#include <QTabWidget>
#include "a-db-utils.h"
#include "board.h"

class QCalendarWidget;
class QListView;
class QFontComboBox;
class QCheckBox;

namespace ard 
{
    class selector_board;

    struct BoardDropOptions
    {
        ard::InsertBranchType   insert_branch_type{ ard::InsertBranchType::none };
        ard::BoardItemShape     item_shape{ ard::BoardItemShape::unknown };
        int                     band_space{ 1 };
    };
};

/**
* dialog to start long async operation and execute it
*/
class AsyncSingletonBox : public QDialog
{
    Q_OBJECT
public:
    AsyncSingletonBox();
    public slots :
    virtual void reloadBox() = 0;
protected:
    void callReloadBox();
protected:
    static AsyncSingletonBox* m_box;
};

/**
* show gmail labels in a list view
*/
class LabelsBox : public AsyncSingletonBox
{
    Q_OBJECT
public:
    static void showLabels();

    void reloadBox()override;

protected:
    LabelsBox();

protected:
    QStandardItemModel* generateGlobalLabelCacheModel();
    googleQt::mail_cache::label_ptr selectedLabel();

protected:
    QTableView*  m_label_view{ nullptr };
};

class AdoptedThreadsBox : public AsyncSingletonBox
{
    Q_OBJECT
public:
    static void showThreads();

    void reloadBox()override {};

protected:
    AdoptedThreadsBox();
    QStandardItemModel* generateThreadsModel();
    void locateThread();
    void viewThreadProperties();
    ethread_ptr selectedThread();

    QTableView*  m_t_view{ nullptr };
};

class BackupsBox : public AsyncSingletonBox
{
    Q_OBJECT
public:
    static void showBackups();
    void reloadBox()override {};
protected:
    BackupsBox();
    QStandardItemModel* generateBackupsModel();

    QString selectedBackupFile();

    QTableView*  m_t_view{ nullptr };
};

/**
    ContactsBox - table of contacts
*/
class ContactsBox : public QDialog
{
    Q_OBJECT
public:
    static void showContacts();
protected:
    ContactsBox();
    QStandardItemModel*     generateContactsModel();
    QStandardItemModel*     generateOneContactModel(contact_ptr c);
    QStandardItemModel*     generateGroupsModel();
    contact_ptr             selectedContact();


    QTabWidget*         m_main_tab{nullptr};
    QTableView*         m_t_view{ nullptr };
    QTableView*         m_one_view{ nullptr };
    QTableView*         m_groups_view{ nullptr };
    QPlainTextEdit*     m_one_xml{nullptr};
};

/**
    when we drop branch on board - how to expand it (to left/right/from-center etc.)
*/
class BoardTopicDropBox : public QDialog 
{
    Q_OBJECT
public:
    static ard::BoardDropOptions showBoardTopicDropOptions(const TOPICS_LIST& dropped_topics, ard::BoardItemShape shp = ard::BoardItemShape::text_normal, int band_space = 0);
protected:
    BoardTopicDropBox(ard::BoardItemShape shp, int band_space);

    QStandardItemModel*     generateDropSelectionModel();

    QTabWidget*             m_main_tab{ nullptr };
    QTableView*             m_t_view{ nullptr };
    QCheckBox*              m_box_items_chk{nullptr};
    QComboBox*              m_cb_band_space;
    ard::InsertBranchType   m_drop_type{ ard::InsertBranchType::none };
    ard::BoardItemShape     m_shape{ ard::BoardItemShape::text_normal };
    int                     m_band_space{0};
};

/**
CalendarBox - dialog window with calendar to select date
*/
class CalendarBox : public QDialog
{
    Q_OBJECT
public:
    static QDate selectDate(const QDate& suggested_date);

    static QString calendarStyleSheet();

    protected slots:
    void acceptWindow();

protected:
    CalendarBox(const QDate& suggested_date);

    QCalendarWidget* m_cal;
    bool m_selected;
    QDate m_date_selected;
};


/**
* give option to create list of labels, it checks against loaded
* cache labels, comparing names
*/
class LabelsCreatorBox : public QDialog
{
    Q_OBJECT
public:
    static bool createLabels(const STRING_SET& lset);

protected:
    LabelsCreatorBox(STRING_SET&& labels2create);

protected:
    STRING_SET m_labels2create;
    QListView* m_lview{ nullptr };
};

/**
GExceptionDialog - dialog window with option to resolve G-exception
*/
class GExceptionDialog : public QDialog
{
    Q_OBJECT
public:
    static void resolveException(googleQt::GoogleException& e);

protected:
    GExceptionDialog(googleQt::GoogleException& e);
};

/**
RecoveryBox - open instead of Ariadne main in case of exception/crash
*/
class RecoveryBox : public QMainWindow
{
    Q_OBJECT
public:
    RecoveryBox(int signal_error);
};

/**
SimpleLogView - dialog window to tail log file
*/
class SimpleLogView : public QDialog
{
    Q_OBJECT
public:
    static void runIt();

protected:
    SimpleLogView();
};

class DiagnosticsBox : public QDialog
{
    Q_OBJECT
public:
    static void showDiagnostics();

protected:
    DiagnosticsBox();
    QStandardItemModel* generateGoogleRequestsModel();

protected:
    QTableView*  m_dgn_view{ nullptr };
};

/**
ConfirmBox - yes/no ok/cancel
*/
class ConfirmBox : public QDialog
{
    Q_OBJECT
public:
    static YesNoConfirm confirm(QWidget* parent, QString msg, bool default2confirm = true);
	static YesNoConfirm confirmWithOption(QWidget* parent, QString msg, QString optionStr);

protected:
    ConfirmBox(QWidget* parent, QString msg, bool default2confirm);
    YesNoConfirm m_confirmed{ YesNoConfirm::cancel };
	QVBoxLayout*	m_main_layout{nullptr};
	QCheckBox*		m_chk_option{nullptr};
};

class AddPhoneOrEmailBox : public QDialog
{
    Q_OBJECT
public:
    enum class EType
    {
        addNone,
        addPhone,
        addEmail
    };

    struct Result
    {
        Result() {}

        bool ok()const;

        QString tlabel;
        QString data;
        bool    as_primary{ false };
        bool    accepted{ false };
    };

    static Result add(EType ltype);
    static Result edit(EType ltype, QString tlabel, QString data);
protected:
    AddPhoneOrEmailBox(EType add_type, QString tlabel, QString data);

    QComboBox*  m_cb_labels;
    QLineEdit*  m_edit{nullptr};
    bool        m_accepted{ false };
};


/**
    access global app options
*/
class OptionsBox : public QDialog
{
    Q_OBJECT
public:
    static void showOptions();
public slots:
    void notesFontIndexChanged(int);
protected:
    OptionsBox();
    void resetCurrentOutlineFontMark(int fontSize);
    void updateNoteFontSampleText();

    void addFontsTab();
    void addMiscTab();
#ifdef ARD_OPENSSL
    void addSyncTab();
    void processPasswordChange();
    QCheckBox       *m_sync_check{ nullptr };
    QPushButton     *m_sync_change_pwd{ nullptr };
#endif

    QTabWidget      *m_tab{nullptr};
    QFontComboBox   *m_combo_font{ nullptr };
    QComboBox       *m_combo_size{ nullptr };
    QLabel          *m_edit_font_sample_line{nullptr};
    QCheckBox       *m_sys_tray{ nullptr };
	QCheckBox       *m_filter_inbox{ nullptr };
	QCheckBox       *m_prefilter_inbox{ nullptr };
	QCheckBox       *m_schedule_board{ nullptr };

    struct BtnFont 
    {
        int  fnt_size;
    };
    using B2BF = std::map<QToolButton*, BtnFont>;
    B2BF m_b2bf;
    int m_selected_font_size{0};
};

#ifdef ARD_OPENSSL
/**
    change password for sync
*/
class SyncPasswordBox : public QDialog 
{
    Q_OBJECT
public:
    static bool changePassword();

protected:
    SyncPasswordBox();

    bool        m_accepted{false};
    QLineEdit*  m_old_pwd{ nullptr };
    QLineEdit*  m_pwd{ nullptr };
    QLineEdit*  m_pwd2{ nullptr };
    QLineEdit*  m_pwd_hint{ nullptr };
};
#endif

/**
    select old pwd, give hint
*/
class SyncOldPasswordBox : public QDialog
{
    Q_OBJECT
public:
    static QString getOldPassword(ard::aes_result last_dec_res);
protected:
    SyncOldPasswordBox(ard::aes_result last_dec_res);

    QLineEdit*  m_old_pwd{ nullptr };
    QString     m_try_old_pwd;
};

/**
    change some pwd utility box
*/
class GenericPasswordEnterBox : public QDialog
{
    Q_OBJECT
public:
    static QString enter_password(QString descr);
protected:
    GenericPasswordEnterBox(QString descr);
    QLineEdit*  m_edit_pwd{ nullptr };
    QString     m_pwd;
};

/**
create some pwd utility box
*/
class GenericPasswordCreateBox : public QDialog
{
    Q_OBJECT
public:
    static ard::gui_pwd_info create_password(QString descr, bool provide_old_pwd);
protected:
    GenericPasswordCreateBox(QString descr, bool provide_old_pwd);
    QLineEdit*  m_pwd{ nullptr };
    QLineEdit*  m_pwd2{ nullptr };
    QLineEdit*  m_pwd_hint{ nullptr };
    QLineEdit*  m_old_pwd{ nullptr };
    bool        m_accepted{false};
};

class BoardItemEditArrowBox : public QDialog 
{
    Q_OBJECT
public:
    static bool editArrow(ard::selector_board*, ard::board_item* origin, ard::board_item* target, ard::board_link*);

protected:
    BoardItemEditArrowBox(ard::selector_board*, ard::board_item* origin, ard::board_item* target, ard::board_link*);
    QLineEdit*          m_edit{ nullptr };
    ard::selector_board*         m_bb{ nullptr };
    ard::board_item*    m_origin{nullptr};
    ard::board_item*    m_target{ nullptr };
    ard::board_link*    m_link{nullptr};
    bool                m_modified{false};
};

class BoardItemArrowsBox : public QDialog
{
    Q_OBJECT
public:
    static void showArrows(ard::selector_board*, ard::board_item*);

protected:
    BoardItemArrowsBox(ard::selector_board*, ard::board_item*);
    QStandardItemModel* generateLinksModel();

protected:
    bool                storeLinks();

    QTableView*         m_links_view{ nullptr };
    ard::selector_board*         m_bb{nullptr};
    ard::board_item*    m_bitem{nullptr};
};

namespace ard
{
	/**
	FindBox find text in note
	*/
	class find_box : public QDialog
	{
		Q_OBJECT
	public:
		static QString findWhat(QString s);

		protected slots:
		void acceptWindow();

	protected:
		find_box(QString s);
	protected:
		QLineEdit*  m_edit{ nullptr };
		QString     m_what;
	};
};
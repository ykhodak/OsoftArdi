#pragma once

class ProtoGItem;
class ProtoScene;
class anUrl;
class QMimeData;
class QPushButton;
class QPlainTextEdit;
class QTextDocument;
class QTextCharFormat;
class QTextCursor;
class QMenu;
class QTableView;
class QStandardItemModel;

#include "snc.h"
/**
   gui - utility namespace for some global
   functions accessible by GUI level
*/
namespace gui
{
    bool                 isDBAttached();
    QString              currentDBName();
    void                 setupMainOutlineSelectRequestOnRebuild(topic_ptr it);
    bool                 wasSelectOnRebuildRequested();
    topic_ptr            selectNext(bool goUp);
    void                 renameSelected(EColumnType field2edit = EColumnType::Title, bool selectContent = false);
    //sleep - we process events and return after 'milliseconds'
    void                 sleep(int milliseconds = 2000);
    bool                 ensureVisibleInOutline(topic_ptr it/*, EOutlinePolicy enforcePolicy = outline_policy_Uknown*/);
    void                 outlineFolder(topic_ptr f = nullptr, EOutlinePolicy op = outline_policy_Uknown);
    EOutlinePolicy       currPolicy();
    void                 selectPrevPolicy();
    void                 rebuildOutline(EOutlinePolicy pol = outline_policy_Uknown, bool force = false);
    //bool                 confirmMessage(QString s, QWidget* parent = nullptr);
    //bool                 confirmDelete(QString s);
    void                 setupBoxLayout(QLayout* l);
    
    QTableView*          createTableView(QStandardItemModel* m, QTableView* tv = nullptr);
    void                 copyTableViewSelectionToClipbard(QTableView* v);

    int                 choice(std::vector<QString>& options_list, int selected = -1, QString label = "");
    int                 choiceCombo(std::vector<QString>& options_list, int selected = -1, QString label = "", std::vector<QString>* buttons_list=nullptr);
    std::pair<bool, QString> edit(QString text, QString label, bool selected = false, QWidget* parentWidget = nullptr);
    std::vector<QString>  edit(std::vector<QString> labels, QString header, QString confirmStr = "");
    /// open gui box that would return pwd
    QString              enter_password(QString descr);
    /// open gui box that returns pwd and old pwd
    ard::gui_pwd_info   change_password(QString descr, bool provide_old_pwd);
    QString              oauth2Code(QString labelText);
    int                  lineHeight();
    int                  shortLineHeight();
    int                  headerHeight();
    QPoint               lastMouseClick();
    //void                 rebuildAfterResize();    
    void                 runCommand(EAsyncCallRequest cmd,
                                    QObject* obj,
                                    int param);
    QColor               invertColor(QColor clr);
    void                 startSync();
    void                 emptyRecycle(topic_ptr);
    void                 open(topic_ptr);

    //bool                 isColorThemeDark();
    //QColor               colorTheme_FgColor();
    QColor               colorTheme_BkColor();
    //QColor               colorTheme_CardFgColor();
    QColor               colorTheme_CardBkColor();
    QColor               darkSceneBk();
    QString              policy2name(EOutlinePolicy);
    void                 invoke1(QString methodName);
    void                 setButtonMinHeight(QPushButton*);
    void                 resizeWidget(QWidget*, QSize);
    bool                 isConnectedToNetwork();

    bool                 locateBySYID(QString syid, bool& dbfound/*, EOBJ eo = objFolder*/);
    QString              imagesFilter();
    QWidget*             mainWnd();
    void                 updateMainWndTitle(QString s);
    void                 drawArdPixmap(QPainter * p, QString resStr, const QRect& rc);
    void                 drawArdPixmap(QPainter * p, QPixmap pm, const QRect& rc);

    bool                 searchFilterIsActive();
    QString              searchFilterText();
    void                 searchLocalText(QString s);
    void                 replaceGlobalText(QString sFrom, QString sTo, ESearchScope, DB_ID_TYPE id = 0);

    topic_ptr            insertTopic(QString stitle);

    QTextDocument*       helperTextDocument();
    QString              recoverUrl(QString s);
    void                 openUrl(QString url);
    void                 runSQLRig();
    void removeEmptySubfolders(QString parentPath);
    ///called by DB-level to invalidate all data-drived GUI objects
    void            detachGui();
    ///called by DB-level as indication that GUI can project data
    void            attachGui();
    ///GUI level should finalize all unsaved data becase DB is about to close
    void            finalSaveCall();

    void            showInGraphicalShell(QWidget *parent, const QString &pathIn);

    bool            isValidEmailAddress(QString email);

    QString         makeValidFileName(QString fileName);
    QString         makeUniqueFileName(QString path, QString name, QString ext);
};

#pragma once

#include "small_dialogs.h"

class QButtonGroup;
class QComboBox;


/**
   MoreCommands - contains main menu commands
*/
class MoreCommands : public ard::scene_view_dlg
{
    Q_OBJECT
public:

    static void runIt(ECommandTab);
protected:
    MoreCommands(ECommandTab t);
    virtual ~MoreCommands();

    void constructMultitabDlg(ECommandTab t);
    void constructSingletabDlg(ECommandTab t);

    public slots:
    void rebuild();
    void currentMarkPressed(int c, ProtoGItem*);
    void toolButtonPressed(int, int, QVariant);
    void outlineTmpSelectionBoxPressed(int, int) {};
    void cmdFileNew();
    void cmdFileMore();
    void processMoreEx(QAction* a);

protected:
    void addFolderTab();
    void addFilesTab();

    ECommandTab       currentTabType();
    void              register_tab(ECommandTab t, QWidget* w, /*OutlineView* v*/scene_view::ptr&&);
    void closeEvent(QCloseEvent *e)override;

protected:
    //scene_view::command2view    m_vtabs;

    OutlineSceneBase*   m_view_scene{ nullptr };
    OutlineView*        m_view_view{ nullptr };


    QPushButton*        m_moreBtn{ nullptr };

    int m_req_cmd{ AR_none };
    int m_req_cmd_param{ 0 };
    QVariant m_req_cmd_param2;


    typedef std::map<ECommandTab, int> TAB_2_INDEX;
    typedef std::map<int, ECommandTab> INDEX_2_TAB;
    TAB_2_INDEX         m_tab2index;
    INDEX_2_TAB         m_index2tab;
};

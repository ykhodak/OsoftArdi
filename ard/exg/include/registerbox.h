#pragma once

#include <QDialog>
#include <QComboBox>
#include <map>
#include <functional>

class QTextEdit;
class QCheckBox;
class QLineEdit;
class QTableView;
class QLabel;
class QHBoxLayout;
class QPlainTextEdit;

namespace ard {
    class email;
};

class SupportWindow : public QDialog
{
    Q_OBJECT
public:
    static void runWindow();

protected:
    SupportWindow();
    ~SupportWindow();

    private slots:
    void    cancelWindow();
    void    acceptCommand();
    void    copyCommand();

private:

   // typedef bool(*CMD_FUNCTION)(QString sparam);
	using CMD_FUNCTION = std::function<bool(QString)>;
    typedef std::map<QString, CMD_FUNCTION> NAME_2_FUNCTION;
    typedef std::map<QString, QString> NAME_2_DESC;
    static NAME_2_FUNCTION m_commands;
    static NAME_2_DESC m_command2descr;


     bool    run_command(QString cmd, bool& command_recognized);
     bool    run_commandFindBySyid(QString syid);
     bool    run_commandFindByID(QString id);
     bool    run_commandEnableExtraFeatures(QString);
     bool    run_commandCompressDB(QString db_file_path);
     bool    run_commandExecSQL(QString _sql);
     bool    run_commandAutoTest(QString _param);
     bool    run_commandForceModify(QString _param);
     bool    run_commandClearSyncToken(QString _param);
     bool    run_commandDropIniFile(QString _param);
     bool    run_commandPrintHashTable(QString _param);
     bool    run_commandPrintCurrent(QString _param);
     bool    run_commandRaiseException(QString _param);
     bool    run_commandSEGSignal(QString _param);
     bool    run_commandRecreateIndexes(QString _param);
     bool    run_commandGmSetCurrMsgId(QString _param);
     bool    run_commandGmFindByMsgId(QString _param);
     bool    run_commandGmPrintHeaders(QString _param);
     bool    run_commandGmHistory(QString _param);
	 bool    run_commandGmRequestGDrive(QString _param);
     bool    run_commandShowThreads(QString _param);
     bool    run_commandDisableGoogle(QString _param);
     bool    run_commandShowDiagnostics(QString _param);
     bool    run_commandViewLabels(QString _param);
     bool    run_commandViewContacts(QString _param);
     bool    run_commandExportContacts(QString _param);
     bool    run_commandExportUnEncrypted(QString _param);
	 bool    run_commandOpenFile(QString _param);

     bool    run_commandToggleDrawDebugHint(QString _param);

     bool    run_commandExportJSON(QString _param);
     bool    run_commandImportJSON(QString _param);
     bool    run_commandTestCrypto(QString _param);
     bool    run_commandRecoverPwdHint(QString _param);

     bool    run_commandGDriveTerminal(QString _param);
     bool    run_commandTestGenerateContacts(QString _param);
	 bool	 run_commandInsertTDA(QString _param);
	 bool	 run_commandImportSupplied(QString _param);
#ifdef Q_OS_IOS
     bool    run_testView4IOS(QString _param);
#endif//Q_OS_IOS

    bool    ListCommands();
    ard::email*  GmFindCurrDbgMsg();

protected:
    QLineEdit*   m_edit_cmd;
    QLabel*      m_info_label;
    QTableView*  m_commands_table;
    QPushButton* m_copy_button;
    QHBoxLayout* m_butons_layout;
};

class TerminalOut {};
class TerminalEndl {};
TerminalOut& operator << (TerminalOut&, QString);
TerminalOut& operator << (TerminalOut&, quint64);
TerminalOut& operator << (TerminalOut&, TerminalEndl);

namespace ard {
    class terminal : public QDialog
    {
    public:
        terminal();
        virtual ~terminal();

        static TerminalOut& out();
        static terminal* tbox();

        void addAction(QString name, QString description, std::function<void(QString)> action);
        void addSeparator();
        void buildMenu();

        friend TerminalOut& ::operator << (TerminalOut&, QString);
        friend TerminalOut& ::operator << (TerminalOut&, quint64);
        friend TerminalOut& ::operator << (TerminalOut&, TerminalEndl);

    protected:
        void runCommand();

        struct Selection
        {
            QString                         name;
            QString                         description;
            std::function<void(QString)>    action;
        };
        using SELECTION_LIST = std::list<Selection>;
        using SELECTION_MAP = std::map<QString, Selection>;


        QComboBox*      m_name;
        QLineEdit*      m_arg;
        QLineEdit*      m_desc;
        QPlainTextEdit* m_edit;
        SELECTION_LIST  m_sel;
        SELECTION_MAP   m_sel_map;

        friend class TerminalOut;
    };
}

#define tmout ard::terminal::out()
#define tendl TerminalEndl()

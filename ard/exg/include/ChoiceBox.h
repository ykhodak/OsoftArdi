#pragma once

#include <QDialog>
#include <map>
#include <QModelIndex>

class QButtonGroup;
class QLineEdit;
class QComboBox;
class QStandardItemModel;
class QTableView;
class QLabel;
class QTabWidget;
class QPlainTextEdit;
class QVBoxLayout;
class QCheckBox;

/**
   ChoiceBox - dialog with set of radio boxes
*/
class ChoiceBox: public QDialog
{
  Q_OBJECT
public:
  static int choice(std::vector<QString>& options_list, int selected = -1, QString label = "");
protected:
  ChoiceBox(std::vector<QString>& options_list, int selected = -1, QString label = "");

protected:
  int m_choice;
  QButtonGroup *m_bg;
};

typedef std::map<QString, int> STRING_2_INT;

/**
   ComboChoiceBox - dialog with combo box
*/
class ComboChoiceBox: public QDialog
{
  Q_OBJECT
public:
  static int choice(std::vector<QString>& options_list, int selected = -1, QString label = "", std::vector<QString>* buttons_list=nullptr);
  static QString editableChoice(std::vector<QString>& options_list, int selected = -1, QString label = "");
protected:
  ComboChoiceBox(std::vector<QString>& options_list, int selected = -1, QString label = "", std::vector<QString>* buttons_list= nullptr);

protected:
    int m_choice{-1};
    QComboBox  *m_cb;
    STRING_2_INT m_str2int;
};


/**
   EditBox - dialog with line edit
*/
class EditBox: public QDialog
{
    Q_OBJECT
public:
    static std::pair<bool, QString> edit(QString text, QString label, bool selected = false, bool readonly = false, QWidget* parentWidget = nullptr);
protected:
    EditBox(QString text, QString label, bool selected = false, bool readonly = false, QWidget* parentWidget = nullptr);

protected:
    QLineEdit*  m_text_edit;
    QString     m_text_res;
    bool        m_accepted{ false };
};

/**
MultiEditBox - dialog with multi line edit
*/
class MultiEditBox : public QDialog
{
    Q_OBJECT
public:
    static std::vector<QString> edit(std::vector<QString> labels, QString header, QString confirmStr);
protected:
    MultiEditBox(std::vector<QString> labels, QString header, QString confirmStr);

protected:
    std::vector<QLineEdit*> m_edit_arr;
    QCheckBox*              m_chk_box{nullptr};
    QString                 m_text_res;
    bool                    m_accepted{false};
};


/**
   TableBox - dialog with table
*/
class TableBox: public QDialog
{
  Q_OBJECT
public:
  static void showModel(QStandardItemModel*, QString slabel, int topicIDField = -1);
protected:
  TableBox(QStandardItemModel* sm, QString slabel, int topicIDField);

public slots:
  void  currentRowChanged(const QModelIndex & current, const QModelIndex & previous) ;


protected:
  QTableView* m_tv;
  QLabel*     m_label;
  int         m_topicIDField;
};


class TokenBox: public QDialog
{
  Q_OBJECT
public:
    static QString code(QString labelText = "");
protected:
  TokenBox(QString labelText);
  
protected:
  QLineEdit* m_text_edit;
  //QLineEdit* m_email_edit;
  QLabel*    m_label;
};

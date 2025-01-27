#pragma once

#include <QDialog>
#include "a-db-utils.h"

class QLineEdit;
class QCheckBox;
class QComboBox;
class QBoxLayout;
class QLabel;
class QFontComboBox;
class SearchMap;

/**
   SearchCommon - comment class for search/replace/format with scope combo & OK/Cancel buttons
*/
class SearchCommon: public QDialog
{
  Q_OBJECT
//public:

  //SearchCommon(QWidget * parent):QDialog(parent){};
protected:
  void addButtons(QBoxLayout* main_layout);

public slots:
  virtual void acceptWindow() = 0;

protected:
  SearchMap* m_search_map;
  DB_ID_TYPE m_project_id;
  DB_ID_TYPE m_gtd_folder_id;
};

/**
   SearchBox - search for text
*/
class SearchBox:public SearchCommon
{
  Q_OBJECT
public:
  static void search(QString search_local, QString search_email);
protected:
  SearchBox(QString search_local, QString search_email);
public slots:
    void acceptWindow()override;
    void  searchTextChanged(const QString & text);
protected:
    void openEmailSearchDlg();
protected:
  QLineEdit*    m_edit;
  /*
  QCheckBox*    m_chkSearchEmail{nullptr};
  QPushButton*  m_EmailSearchBtn{nullptr};
  QCheckBox*    m_chkEmailSearchOnlyPrimary{ nullptr };
  QString       m_email_search_str;
  */
};

/**
   ReplaceBox - replace text
*/
class ReplaceBox: public SearchCommon
{
  Q_OBJECT
public:
  static void replace(QString sFrom, QString sTo, ESearchScope scope);
protected:
  ReplaceBox(QString sFrom, QString sTo, ESearchScope scope);
public slots:
  virtual void acceptWindow();
protected:
  QLineEdit* m_from;
  QLineEdit* m_to;
};

class FormatFontDlg: public SearchCommon
{
  Q_OBJECT
public:

  static void format(TOPICS_LIST& lst);

protected:
  FormatFontDlg();

public slots:
  void acceptWindow();
  void toggledFont(bool);
  void toggledSize(bool);

protected:
  bool m_bAccepted;
  QCheckBox *m_chk_font;
  QCheckBox *m_chk_size;

  QFontComboBox  *m_combo_font;
  QComboBox      *m_combo_size;
};

#include <QLineEdit>
#include <QCheckBox>
#include <QVBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QDebug>
#include <QEventLoop>
#include <QComboBox>
#include <QFontComboBox>

#include "SearchBox.h"
#include "SearchMap.h"
#include "anfolder.h"
#include "ansyncdb.h"
#include "EmailSearchBox.h"

void SearchCommon::addButtons(QBoxLayout* main_layout)
{
  QHBoxLayout *h = nullptr;

  h = new QHBoxLayout;
  QPushButton* b;
  ADD_BUTTON2LAYOUT(h, "OK", &SearchCommon::acceptWindow);
  ADD_BUTTON2LAYOUT(h, "Cancel", &SearchCommon::reject);
  main_layout->addLayout(h);
};


/**
   SearchBox
*/

void SearchBox::search(QString search_local, QString search_email)
{
  SearchBox d(search_local, search_email);
  d.exec();
};

SearchBox::SearchBox(QString search_local, QString)
// :SearchCommon(gui::mainWnd())
{
  QVBoxLayout *v_main = new QVBoxLayout;
  m_search_map = new SearchMap();
  v_main->addWidget(m_search_map);
  
  m_edit = new QLineEdit;
  m_edit->setText(search_local);
  QHBoxLayout *h_0 = new QHBoxLayout;
  h_0->addWidget(new QLabel("Search for:"));
  h_0->addWidget(m_edit);
  v_main->addLayout(h_0);

  QHBoxLayout *h_1 = new QHBoxLayout;
  QVBoxLayout *v_1 = new QVBoxLayout;
  v_main->addLayout(h_1);
  h_1->addLayout(v_1);
  /*
  m_chkSearchEmail = new QCheckBox("Search Email");
  m_email_search_str = search_email;

  connect(m_chkSearchEmail, &QCheckBox::stateChanged, [&](int) 
  {
      if (m_chkSearchEmail->isChecked()) {
          m_EmailSearchBtn->setEnabled(true);
          if (m_email_search_str.isEmpty()) {
              m_email_search_str = m_edit->text();
          }
          m_chkEmailSearchOnlyPrimary->setEnabled(true);
      }
      else {
          m_EmailSearchBtn->setEnabled(false);
          m_chkEmailSearchOnlyPrimary->setEnabled(false);
      }
     // V = true; 
  });
  v_1->addWidget(m_chkSearchEmail);
  m_EmailSearchBtn = new QPushButton("...");
  m_EmailSearchBtn->setEnabled(false);

  m_chkEmailSearchOnlyPrimary = new QCheckBox("Primary emails only");
  m_chkEmailSearchOnlyPrimary->setEnabled(false);
  m_chkEmailSearchOnlyPrimary->setChecked(true);
  v_1->addWidget(m_chkEmailSearchOnlyPrimary);
  
  if (!m_email_search_str.isEmpty()) {
      m_chkSearchEmail->setChecked(true);
  }  
  gui::setButtonMinHeight(m_EmailSearchBtn);
  h_1->addWidget(m_EmailSearchBtn);
  connect(m_EmailSearchBtn, &QPushButton::released,
      this, &SearchBox::openEmailSearchDlg);
      */
  addButtons(v_main);

  m_edit->setFocus(Qt::OtherFocusReason);
  connect(m_edit, SIGNAL(textChanged(const QString&)), this, SLOT(searchTextChanged(const QString&)));
  m_search_map->mapRoot(ard::root());
  if(!search_local.isEmpty())
    m_search_map->filter(search_local);
  MODAL_DIALOG(v_main);
};

void SearchBox::acceptWindow()
{
  QString s = m_edit->text().trimmed();
  if(!s.isEmpty())
    {
      gui::searchLocalText(s);
    }
  accept();
};

void SearchBox::openEmailSearchDlg()
{
    /*
    if (m_email_search_str.isEmpty()) {
        m_email_search_str = m_edit->text();
    }
    auto p = EmailSearchBox::edit_qstr(m_email_search_str);
    if (p.first && !p.second.isEmpty()) {
        m_email_search_str = p.second;
    }
    */
};

void SearchBox::searchTextChanged(const QString & text)
{
  //  qDebug() << "<<<SearchBox::searchTextChanged" << text;
  m_search_map->filter(text);
};

/**
   ReplaceBox
*/
void ReplaceBox::replace(QString sFrom, QString sTo, ESearchScope scope)
{
  ReplaceBox d(sFrom, sTo, scope);
  d.exec();
};

ReplaceBox::ReplaceBox(QString sFrom, QString sTo, ESearchScope )
// :SearchCommon(gui::mainWnd())
{
  QVBoxLayout *v_main = new QVBoxLayout;
  QLabel* l = new QLabel("Enter the string to replace.");
  v_main->addWidget(l);
  
  m_from = new QLineEdit;
  m_from->setText(sFrom);
  QHBoxLayout *h_0 = new QHBoxLayout;
  h_0->addWidget(new QLabel("Replace from:"));
  h_0->addWidget(m_from);

  m_to = new QLineEdit;
  m_to->setText(sTo);
  QHBoxLayout *h_0_1 = new QHBoxLayout;
  h_0_1->addWidget(new QLabel("To:"));
  h_0_1->addWidget(m_to);

  v_main->addLayout(h_0);
  v_main->addLayout(h_0_1);
  addButtons(v_main/*, scope*/);
  m_from->setFocus(Qt::OtherFocusReason);

  MODAL_DIALOG(v_main);
};

void ReplaceBox::acceptWindow()
{
    /*
  QString sFrom = m_from->text().trimmed();
  if(!sFrom.isEmpty())
    {
      QString sTo = m_to->text().trimmed();
      ESearchScope scope = search_scopeTopic;
      DB_ID_TYPE pid = 0;
      getSearchScope(scope, pid);
      gui::replaceGlobalText(sFrom, sTo, scope, pid);
    }
    */
  accept();
};

/**
   FormatFontDlg
*/
void FormatFontDlg::format(TOPICS_LIST& lst)
{
  FormatFontDlg d;
  d.exec();
  if (d.m_bAccepted)
  {
      bool proceed = false;
      QTextCharFormat fmt;

      if (d.m_chk_font->isChecked())
      {
          QString fs = d.m_combo_font->currentFont().family();
          fmt.setFontFamily(fs);
          dbp::configFileSetNoteFontFamily(fs);
          proceed = true;
      }

      if (d.m_chk_size->isChecked())
      {
          qreal pointSize = d.m_combo_size->currentText().toFloat();
          if (pointSize > 0)
          {
              fmt.setFontPointSize(pointSize);
              dbp::configFileSetNoteFontSize(pointSize);
          }
          proceed = true;
      }

      if (proceed)
      {
          ard::formatNotes(lst, &fmt);
          //ESearchScope scope = search_scopeTopic;
          ///DB_ID_TYPE pid = 0;
    //      d.getSearchScope(scope, pid);
          //gui::formatGlobalText(&fmt, scope, pid);
      }
  }
};

FormatFontDlg::FormatFontDlg()// :SearchCommon(gui::mainWnd())
{
    m_bAccepted = false;

    QHBoxLayout *l1 = new QHBoxLayout;
    m_chk_font = new QCheckBox("Font");
    m_combo_font = new QFontComboBox();
    l1->addWidget(m_chk_font);
    l1->addWidget(m_combo_font);

    connect(m_chk_font, SIGNAL(toggled(bool)),
        this, SLOT(toggledFont(bool)));

    QHBoxLayout *l2 = new QHBoxLayout;
    m_chk_size = new QCheckBox("Size");
    m_combo_size = new QComboBox();
    l2->addWidget(m_chk_size);
    l2->addWidget(m_combo_size);

    connect(m_chk_size, SIGNAL(toggled(bool)),
        this, SLOT(toggledSize(bool)));

    QFontDatabase db;
    foreach(int size, db.standardSizes())
        m_combo_size->addItem(QString::number(size));

    QVBoxLayout *m_l = new QVBoxLayout;
    m_l->addLayout(l1);
    m_l->addLayout(l2);
    addButtons(m_l);

    QFont* defFont = ard::defaultFont();

    QString s = dbp::configFileNoteFontFamily().trimmed();
    if (!s.isEmpty())
    {
        m_combo_font->setCurrentIndex(m_combo_font->findText(s));
    }
    else {
        if (defFont) {
            auto idx = m_combo_font->findText(defFont->family());
            if (idx == -1) {
                idx = 0;
            }
            m_combo_font->setCurrentIndex(idx);
        }
    }

    int v = static_cast<int>(dbp::configFileNoteFontSize());
    if (v > 0)
    {
        s = QString("%1").arg(v);
        m_combo_size->setCurrentIndex(m_combo_size->findText(s));
    }
    else {
        if (defFont) {
            s = QString("%1").arg(defFont->pointSize());
            auto idx = m_combo_size->findText(s);
            if (idx == -1) {
                idx = m_combo_size->findText("10");
                if (idx == -1) {
                    idx = 0;
                }
            }
            m_combo_size->setCurrentIndex(idx);
        }
    }

    //m_combo_font->setEnabled(false);
    //m_combo_size->setEnabled(false);

    m_chk_font->setChecked(true);
    m_chk_size->setChecked(true);

    MODAL_DIALOG(m_l);
};


void FormatFontDlg::acceptWindow()
{
  if(!m_chk_font->isChecked() && !m_chk_size->isChecked())
    {
	  ard::messageBox(this, "Check box on the left to apply changes.");
      return;
    }

  m_bAccepted = true;
  close();
};

void FormatFontDlg::toggledFont(bool chk)
{
  if(chk)
    {
      m_combo_font->setEnabled(true);
    }
  else
    {
      m_combo_font->setEnabled(false);
    }
};

void FormatFontDlg::toggledSize(bool chk)
{
  if(chk)
    {
      m_combo_size->setEnabled(true);
    }
  else
    {
      m_combo_size->setEnabled(false);
    }
};

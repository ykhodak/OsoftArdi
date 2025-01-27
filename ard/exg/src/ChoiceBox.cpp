#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QRadioButton>
#include <QDebug>
#include <QButtonGroup>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QStandardItemModel>
#include <QTableView>
#include <QHeaderView>
#include <QApplication>
#include <QClipboard>
#include <QTabWidget>
#include <QFileInfo>
#include <QPlainTextEdit>
#include <QTimer>
#include <QDesktopServices>
#include <QCheckBox>

#include "ChoiceBox.h"
#include "anfolder.h"
#include "email.h"
#include "custom-widgets.h"
#include "snc-tree.h"
#include <sstream>
#include "google/demo/ApiTerminal.h"
#include "gdrive/GdriveRoutes.h"
#include "Endpoint.h"
#include "contact.h"

/**
   ChoiceBox
*/
int ChoiceBox::choice(std::vector<QString>& options_list, int selected, QString label)
{
  ChoiceBox cb(options_list, selected, label);
  cb.exec();
  return cb.m_choice;
};

ChoiceBox::ChoiceBox(std::vector<QString>& options_list, int selected, QString label)
  :QDialog(gui::mainWnd()),
   m_choice(-1)
{
  QVBoxLayout *h_1 = new QVBoxLayout;

  m_bg = new QButtonGroup(this);
  int idx = 0;
  //for(std::vector<QString>::iterator i = options_list.begin(); i != options_list.end(); i++)
    for(const auto& s : options_list){
      ///QString s = *i;
      QRadioButton *rb = new QRadioButton(s);
      m_bg->addButton(rb, idx);
      h_1->addWidget(rb);
      if(idx == selected)
    rb->setChecked(true);

      idx++;
    }

  QHBoxLayout *h_2 = new QHBoxLayout;
  ard::addBoxButton(h_2, "OK", [&]()
            {
              m_choice = m_bg->checkedId();
              close(); 
            });
  ard::addBoxButton(h_2, "Cancel", [&](){close();});

  QVBoxLayout *v_main = new QVBoxLayout;
  if(!label.isEmpty())
    {
      QLabel* l = new QLabel(label);
      l->setWordWrap(true);
      v_main->addWidget(l);
    }
  v_main->addLayout(h_1);
  v_main->addLayout(h_2);

  MODAL_DIALOG(v_main);
};

/*
void ChoiceBox::processChoice()
{
  m_choice = m_bg->checkedId();
  close();
};*/


/**
   ComboChoiceBox
*/
int ComboChoiceBox::choice(std::vector<QString>& options_list, int selected, QString label, std::vector<QString>* buttons_list)
{
  ComboChoiceBox cb(options_list, selected, label, buttons_list);
  cb.exec();
  return cb.m_choice;
};

QString ComboChoiceBox::editableChoice(std::vector<QString>& options_list, int selected, QString label)
{
    ComboChoiceBox cb(options_list, selected, label);
    cb.m_cb->setEditable(true);
    cb.exec();
    if (cb.m_choice == -1) {
        return "";
    }
    return cb.m_cb->currentText();
};

ComboChoiceBox::ComboChoiceBox(std::vector<QString>& options_list, int selected, QString label, std::vector<QString>* buttons_list)
  :QDialog(gui::mainWnd()),
   m_choice(-1)
{
  m_cb = new QComboBox;
  int idx = 0;
   for(const auto& s : options_list) {
      m_cb->addItem(s);
      idx++;
    }

  if(selected != -1)
    {
      m_cb->setCurrentIndex(selected);
    }

  QHBoxLayout *h_2 = new QHBoxLayout;
  ard::addBoxButton(h_2, "OK", [&](){m_choice = m_cb->currentIndex();close();});
  ard::addBoxButton(h_2, "Cancel", [&]() {m_choice = -1; close(); });


  QVBoxLayout *v_main = new QVBoxLayout;
  if(!label.isEmpty())
    {
      QLabel* l = new QLabel(label);
      v_main->addWidget(l);
    }
  v_main->addWidget(m_cb);
  v_main->addLayout(h_2);

  if(buttons_list)
    {
      int idx = options_list.size();
    for(auto& i : *buttons_list){
      m_str2int[i] = idx;
      idx++;

      ard::addBoxButton(v_main, 
                i, 
                [&]()
                {
                  m_choice = -1;
                  QAbstractButton* b = qobject_cast<QAbstractButton *>(QObject::sender());
                  if(b)
                {
                  STRING_2_INT::iterator i = m_str2int.find(b->text());
                  if(i != m_str2int.end())
                    {
                      m_choice = i->second;
                    }
                }
  
                  close();                
                });
    }
    }


  MODAL_DIALOG(v_main);
};


/**
   EditBox
*/
std::pair<bool, QString> EditBox::edit(QString text, QString label, bool selected /*= false*/, bool readonly /*= false*/, QWidget* parentWidget /*= nullptr*/)
{
    EditBox b(text, label, selected, readonly, parentWidget);
    b.exec();
    std::pair<bool, QString> rv;
    rv.first = b.m_accepted;
    rv.second = b.m_text_res.trimmed();
    return rv;
};

EditBox::EditBox(QString text, QString label, bool selected /*= false*/, bool readonly /*= false*/, QWidget* parentWidget /*= nullptr*/)
  :QDialog(parentWidget ? parentWidget : gui::mainWnd()),
   m_text_res("")
{
  QVBoxLayout *h_1 = new QVBoxLayout;
  QLabel* lb = new QLabel(label);
  lb->setWordWrap(true);
  h_1->addWidget(lb);
  m_text_edit = new QLineEdit(text);
  h_1->addWidget(m_text_edit);
  if(selected){
      m_text_edit->selectAll();
    }

  if(readonly){
      m_text_edit->setReadOnly(true);
  }
  
  QHBoxLayout *h_2 = new QHBoxLayout;
  ard::addBoxButton(h_2, "OK", [&]() {m_accepted = true; m_text_res = m_text_edit->text(); close(); });
  ard::addBoxButton(h_2, "Cancel", [&](){ close(); });

  QVBoxLayout *v_main = new QVBoxLayout;
  v_main->addLayout(h_1);
  v_main->addLayout(h_2);

  MODAL_DIALOG(v_main);
};

/**
MultiEditBox
*/
std::vector<QString> MultiEditBox::edit(std::vector<QString> labels, QString header, QString confirmStr)
{
    std::vector<QString> rv;
    MultiEditBox d(labels, header, confirmStr);
    d.exec();
    if (d.m_accepted) {
        if (!confirmStr.isEmpty()) {
            if (d.m_chk_box) {
                if (!d.m_chk_box->isChecked()) {
                    return rv;
                }
            }
        }

        for (auto& e : d.m_edit_arr) {
            rv.push_back(e->text().trimmed());
        }
    }
    return rv;
};

MultiEditBox::MultiEditBox(std::vector<QString> labels, QString header, QString confirmStr)
{
    QVBoxLayout *v_main = new QVBoxLayout;
    QLabel* lb = new QLabel(header);
    lb->setWordWrap(true);
    lb->setTextFormat(Qt::RichText);
    v_main->addWidget(lb);

    //..
    QGridLayout* pwdbox = new QGridLayout;
    int row = 0;
    for (auto& slb : labels) {
        auto e = new QLineEdit;
        pwdbox->addWidget(new QLabel(slb), row, 0);
        pwdbox->addWidget(e, row, 1);
        m_edit_arr.push_back(e);
        row++;
    }
    v_main->addLayout(pwdbox);
    //..

    if (!confirmStr.isEmpty()) {
        m_chk_box = new QCheckBox(confirmStr);
        v_main->addWidget(m_chk_box);
    }

    QHBoxLayout *h_btns = new QHBoxLayout;
    ard::addBoxButton(h_btns, "OK", [&]() { m_accepted = true; close(); });
    ard::addBoxButton(h_btns, "Cancel", [&]() { close(); });
    v_main->addLayout(h_btns);

    MODAL_DIALOG(v_main);
};

/**
   TableBox
*/
void TableBox::showModel(QStandardItemModel* sm, QString slabel, int topicIDField)
{
  TableBox d(sm, slabel, topicIDField);
  d.exec();
};

TableBox::TableBox(QStandardItemModel* sm, QString slabel, int topicIDField)
{
  m_topicIDField = topicIDField;

  QVBoxLayout *h_1 = new QVBoxLayout;
  QHBoxLayout *h_btns = new QHBoxLayout;
  m_label = new QLabel(slabel);
  m_label->setWordWrap(true);
  h_1->addWidget(m_label);

  m_tv = new ArdTableView();
  m_tv->setModel(sm);
  h_1->addWidget(m_tv);
  QItemSelectionModel *selm = m_tv->selectionModel();
  connect(selm, SIGNAL(currentRowChanged(const QModelIndex&, const QModelIndex&)), 
      this, SLOT(currentRowChanged(const QModelIndex&, const QModelIndex&)));

  QHeaderView* h = m_tv->horizontalHeader();
  h->setSectionResizeMode(1, QHeaderView::Stretch);

  m_tv->setWordWrap(true);
  m_tv->setTextElideMode(Qt::ElideMiddle);
  m_tv->resizeRowsToContents();

  h_1->addLayout(h_btns);
  ard::addBoxButton(h_btns, "Copy", [&]() 
  {
      m_tv->selectAll();
      QAbstractItemModel *abmodel = m_tv->model();
      QItemSelectionModel *m = m_tv->selectionModel();
      QModelIndexList list = m->selectedIndexes();
      QString copy_table;
      QModelIndex previous;
      for (int i = 0; i < list.size(); i++)
      {
          QModelIndex index = list.at(i);
          if (i == 0) {
              previous = index;
          }
          else {
              if (index.row() != previous.row()) {
                  copy_table.append('\n');
              }
              else {
                  copy_table.append('\t');
              }
          }
          QString text = abmodel->data(index).toString();
          copy_table.append(text);

          previous = index;
      }
      QClipboard *clipboard = QApplication::clipboard();
      if (clipboard) {
          clipboard->setText(copy_table);
      }
      //close(); 
  });
  ard::addBoxButton(h_btns, "Cancel", [&]() { close(); });
  //ard::addBoxButton(h_1, "Cancel", [&](){ close(); });

  h_1->setContentsMargins(0,0,0,0);
  h_1->setSpacing(0);
  MODAL_DIALOG(h_1);
};


void TableBox::currentRowChanged(const QModelIndex & current, const QModelIndex &)
{
  if(m_topicIDField != -1)
    {
      QAbstractItemModel* am = m_tv->model();
      if(am != NULL)
    {
      QStandardItemModel* m = dynamic_cast<QStandardItemModel*>(am);
      if(m != NULL)
        {
          QStandardItem* si = m->item(current.row(), m_topicIDField);
          if(si != NULL)
        {
          DB_ID_TYPE topicID = si->text().toInt();
          auto t = ard::lookup(topicID);
          assert_return_void(t != NULL, "failed to lookup loaded topic");
          m_label->setText(t->title());
        }
        }
    }
    }
};


/**
   TokenBox
*/
QString gui::oauth2Code(QString labelText)
{
    return TokenBox::code(labelText);
};

QString TokenBox::code(QString labelText)
{
    if(labelText.isEmpty()){
       labelText = "Please follow browser instructions and authorize access to GMail for organizing/sending emails and GDrive to synchronize local data, "
           "enter authorization code in this box and press ok to proceed.";
    }
    
    QString rv;
    TokenBox d(labelText);
    if(d.exec() == QDialog::Accepted)
        {
            rv = d.m_text_edit->text();
        };

    return rv;
};

TokenBox::TokenBox(QString labelText)
{
  QLabel* lb = new QLabel(labelText);
  lb->setWordWrap(true);
  m_text_edit = new QLineEdit();

  QHBoxLayout *h_1 = new QHBoxLayout;
  QHBoxLayout *h_2 = new QHBoxLayout;
  ard::addBoxButton(h_1, "OK", [&](){accept();});
  ard::addBoxButton(h_1, "Cancel", [&](){/*dbp::configFileSetSyncEnabled(false);*/ reject();});

  QVBoxLayout *v_main = new QVBoxLayout;
  v_main->addWidget(lb);
  v_main->addWidget(m_text_edit);
  ard::addBoxButton(h_2, "Paste Code", [&](){m_text_edit->paste();});
  ard::addBoxButton(h_2, "Clear", [&](){m_text_edit->clear();});
  
  v_main->addLayout(h_2);
  v_main->addLayout(h_1);
  
  MODAL_DIALOG_SIZE(v_main, QSize(700, 400));
};



#include <QListWidget>
#include <QVBoxLayout>
#include <QPushButton>
#include <QStringListModel>
#include <QModelIndexList>
#include <QLabel>
#include <QLineEdit>
#include <QCheckBox>
#include <QFileDialog>
#include <QTextEdit>
#include <QMenu>
#include <QAction>
#include "OpenDatabase.h"
#include "a-db-utils.h"
#include "dbp.h"
#include "syncpoint.h"

/**
   CreateDatabase
*/
bool CreateDatabase::run()
{
  CreateDatabase d;
  d.exec();
  return d.m_newDBwas_created;
};

CreateDatabase::CreateDatabase():m_newDBwas_created(false)
{
  QVBoxLayout *v_main = new QVBoxLayout;

  QHBoxLayout *h = new QHBoxLayout;
  h->addWidget(new QLabel("Name:"));
  m_name = new QLineEdit(this);
  h->addWidget(m_name);
  v_main->addLayout(h);

  QPushButton* b = nullptr;
  h = new QHBoxLayout;
  ADD_BUTTON2LAYOUT(h, "Cancel", &CreateDatabase::processCancel);
  ADD_BUTTON2LAYOUT(h, "OK", &CreateDatabase::processOK);
  b->setDefault(true);
  v_main->addLayout(h);

  MODAL_DIALOG(v_main);
};

void CreateDatabase::processOK()
{
  QString name = m_name->text().trimmed();
  if(name.compare(DEFAULT_DB_NAME, Qt::CaseInsensitive) == 0)
    {
	  ard::messageBox(this, QString("Name '%1' is reserved name for the default database").arg(name));
      return;
    }

  QStringList lst;
  loadDBsList(lst);
  for(int i = 0; i < lst.size(); i++)
    {
      QString s = lst.at(i);
      if(name.compare(s, Qt::CaseInsensitive) == 0)
    {
		  ard::messageBox(this, QString("Database '%1' already exists. Please enter different name").arg(s));
      return;
    }      
    }

  if (!dbp::create(name, "", ""))
  {
	  ard::messageBox(this, QString("Failed to create database file '%1'. Possible  reason - local security policy.").arg(name));
      return;
  }

  m_newDBwas_created = true;
  close();
};

void CreateDatabase::processCancel()
{
  close();
};

QString getUserPassword(QString, bool&, bool&, QString /*= ""*/)
{
    ASSERT(0, "NA");
    return "";
}

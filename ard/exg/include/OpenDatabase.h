#pragma once


#include <QDialog>

class QListWidget;
class QLineEdit;
class QCheckBox;
class QVBoxLayout;

class CreateDatabase: public QDialog
{
  Q_OBJECT
public:
  static bool run();

protected:
  CreateDatabase();

public slots:
  void processOK();
  void processCancel();

protected:
  QLineEdit*    m_name;
  bool          m_newDBwas_created;
};


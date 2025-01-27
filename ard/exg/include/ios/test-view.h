// source https://github.com/leopatras/UITextView-on-Qt

#pragma once

//#ifdef Q_OS_IOS

#include <QWidget>

Q_FORWARD_DECLARE_OBJC_CLASS(UITextView);

class TestView : public QWidget
{
  Q_OBJECT
  
public:
  
  TestView(QWidget* parent=0);
  ~TestView();
  void setStr(QString str);
  QString getStr()const;
protected:
  virtual bool event(QEvent*);
private:
  UITextView* m_TextView;
  bool m_scheduled;
private slots:
  void updateGeo();
};

//#endif //#ifdef Q_OS_IOS

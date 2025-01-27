#include "ios/test-view.h"
#include <QEvent>
#include <QTimer>
#include <QRect>
#import <CoreFoundation/CoreFoundation.h>
#import <UIKit/UIKit.h>

TestView::TestView(QWidget* parent):QWidget(parent),m_scheduled(false)
{
  m_TextView=[[UITextView alloc] init];
  UIView *parentView = reinterpret_cast<UIView *>(window()->winId());
  [parentView addSubview:m_TextView];

  [m_TextView setTextColor:[UIColor redColor]];
  [m_TextView setBackgroundColor:[UIColor blueColor]];
}

TestView::~TestView()
{
  [m_TextView removeFromSuperview];
  m_TextView=nil;
}

bool TestView::event(QEvent* e)
{
  if (e->type()==QEvent::Move || e->type()==QEvent::Resize) {
    if (!m_scheduled) {
      m_scheduled=true;
      QTimer::singleShot( 0, this, SLOT( updateGeo() ) );
    }
  }
  bool result=QWidget::event(e);
  return result;
}

static inline QRect globalRect(QWidget* widget)
{
  return QRect((widget)->mapToGlobal(QPoint(0,0)), (widget)->size());
}

void TestView::updateGeo()
{
  m_scheduled=false;
  QRect rg=globalRect(this);
  QRect rw=globalRect(window());
  CGRect cg=CGRectMake(rg.x(), rg.y()-rw.y(), rg.width(), rg.height());
  [m_TextView setFrame:cg];
}

void TestView::setStr(QString str)
{
    NSString* nstr = str.toNSString();
    [m_TextView setText:nstr];
};

QString TestView::getStr()const
{
   extern QString qt_mac_NSStringToQString(const NSString *nsstr);
   NSString *nstr = [NSString stringWithFormat:@"%@", m_TextView.text];
   QString rv = qt_mac_NSStringToQString(nstr);
   return rv;
};

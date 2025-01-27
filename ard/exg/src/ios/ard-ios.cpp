//#ifdef Q_OS_IOS

#include "a-db-utils.h"
#include "ard-iphone.h"
#include "anfolder.h"

int gadget::test1()
{
  return mac_testInt();
};

QString gadget::test2()
{
  return mac_testString();
};

void gadget::showDeveloperSettings()
{
  mac_showSettings();
}

extern void doAddResource(int reqParam, QString sname, QString email);
/*
void gadget::selectContact(int nparam)
{
  doAddResource(nparam, "", "");
  }*/

static QTextCursor* g__lastTextCursor = NULL;

extern void doTakePhotoWithCamera(anTopic* f, QTextCursor* cr);
void gadget::takePhoto(anTopic* f, QTextCursor* cr)
{
  g__lastTextCursor = cr;
  RETURN_VOID_ON_ASSERT(f != NULL, "expected topic");
  doTakePhotoWithCamera(f, cr);
}

QTextCursor* takePhotoInsertTextCursor()
{
  return g__lastTextCursor;
}


//#endif //Q_OS_IOS

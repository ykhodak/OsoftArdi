#include "a-db-utils.h"
#include "anfolder.h"

#ifdef Q_OS_ANDROID
#include <QAndroidJniObject>
#include <QAndroidJniEnvironment>
#include <QWidget>

int gadget::test1()
{
  jint value = QAndroidJniObject::callStaticMethod<jint>("com/ykhodak/ariadneorganizer/AriadneTest", "testInt");
  int rv = value;
  return rv;
};

QString gadget::test2()
{
  QAndroidJniObject value = QAndroidJniObject::callStaticObjectMethod("com/ykhodak/ariadneorganizer/AriadneTest", "testString", "()Ljava/lang/String;");
  QString rv = value.toString();
  return rv;
  
  return "";
};

void gadget::showDeveloperSettings()
{
  QAndroidJniEnvironment env;
  
  QAndroidJniObject activity = QAndroidJniObject::callStaticObjectMethod("org/qtproject/qt5/android/QtNative", "activity", "()Landroid/app/Activity;");
  if ( activity.isValid() )
    {
      
      // Equivalent to Jave code: 'Intent intent = new Intent();'
      QAndroidJniObject intent("android/content/Intent","()V");
      if ( intent.isValid() )
    {
      QAndroidJniObject param1 = QAndroidJniObject::fromString("com.android.settings");
      QAndroidJniObject param2 = QAndroidJniObject::fromString("com.android.settings.DevelopmentSettings");

      if ( param1.isValid() && param2.isValid() )
        {
          // Equivalent to Jave code: 'intent.setClassName("com.android.settings", "com.android.settings.DevelopmentSettings");'
          intent.callObjectMethod("setClassName","(Ljava/lang/String;Ljava/lang/String;)Landroid/content/Intent;",param1.object<jobject>(),param2.object<jobject>());

          // Equivalent to Jave code: 'startActivity(intent);'
          activity.callMethod<void>("startActivity","(Landroid/content/Intent;)V",intent.object<jobject>());
        }
    }
      
    }
  
  if (env->ExceptionOccurred()) {
    env->ExceptionDescribe();
    env->ExceptionClear();
  }  
};

#define ARD_INVOKE_JAVA(F, P) QAndroidJniObject::callStaticMethod<void>("com/ykhodak/ariadneorganizer/AriadneActivity", F, "(I)V", P);

void gadget::selectContact(int nparam)
{
  ARD_INVOKE_JAVA("selectContact", nparam);
};

static QTextCursor* g__lastTextCursor = NULL;
void gadget::takePhoto(anTopic* f, QTextCursor* cr)
{
  g__lastTextCursor = cr;
  RETURN_VOID_ON_ASSERT(f != NULL, "expected topic");
  ARD_INVOKE_JAVA("takePhoto", f->id());
}

QTextCursor* takePhotoInsertTextCursor()
{
  return g__lastTextCursor;
}

#undef ARD_INVOKE_JAVA

static void java2ard_notification(JNIEnv* env, jclass, jint reqParam, jint param1, jstring param2)
{
  int n_value = param1;
  QString s_value(env->GetStringUTFChars(param2, 0));
  QMetaObject::invokeMethod(gui::mainWnd(), "gadgetNotification",
                Qt::QueuedConnection,
                Q_ARG(int, reqParam),
                Q_ARG(int, n_value),
                Q_ARG(QString, s_value));
};

static JNINativeMethod methods[] = {
  { "java2ard_notification", // const char* function name;
    "(IILjava/lang/String;)V", // const char* function signature
    (void *)java2ard_notification // function pointer
  }
};

// step 1, per https://www.kdab.com/qt-android-episode-5/
// this method is called automatically by Java VM
// after the .so file is loaded
JNIEXPORT jint JNI_OnLoad(JavaVM* vm, void* /*reserved*/)
{
  JNIEnv* env;
  // get the JNIEnv pointer.
  if (vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6)
      != JNI_OK) {
    return JNI_ERR;
  }
 
  // step 3
  // search for Java class which declares the native methods com/ykhodak/ariadneorganizer/AriadneNotifier
  jclass javaClass = env->FindClass("com/ykhodak/ariadneorganizer/AriadneNotifier");
  if (!javaClass)
    return JNI_ERR;
 
  // step 4
  // register our native methods
  if (env->RegisterNatives(javaClass, methods,
               sizeof(methods) / sizeof(methods[0])) < 0) {
    return JNI_ERR;
  }
  return JNI_VERSION_1_6;
}


#endif //Q_OS_ANDROID

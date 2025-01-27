#include <QApplication>
#include <QDebug>
#include <QFile>
#include <QDateTime>
#include <math.h>
#include <QtPlugin>
#include <QDesktopWidget>
#include <QMessageBox>
#include <iostream>
#include <csignal>
#include <qapplication.h>

#include "MainWindow.h"
#include "OutlineMain.h"
#include "utils.h"
#include "dbp.h"
#include "ardmodel.h"
#include "custom-widgets.h"
#include "custom-boxes.h"

#ifdef Q_OS_WIN
#include "StackWalker.h"
#endif

#ifdef Q_OS_ANDROID
#include <android/log.h>
#endif

extern QString programName();
extern int get_app_version_as_int();

static QPoint _lastAppMouseClick;
QList<int> main_splitter_sizes_before_drag;

QSharedMemory ardi_instance_shared_memory;

QPoint gui::lastMouseClick()
{
    return _lastAppMouseClick;
};

#ifdef WIN32
#ifndef _DEBUG
#pragma comment(linker, "/SUBSYSTEM:windows /ENTRY:mainCRTStartup")
#endif
#endif

bool unlimitedLogFile = false;

void writeToMainLog(const QString& in_msg, QtMsgType type)
{
    static QMutex mtx;
    QMutexLocker locker(&mtx);
    if (in_msg.indexOf("is still in use, all queries will cease to work") != -1)
    {
        /// .. skip warnings ..
        return;
    }

#ifdef _DEBUG
    static char msg[16768] = "";
#else
    static char msg[2096] = "";
#endif
    std::string s_tmp = in_msg.toStdString();
    strncpy(msg, s_tmp.c_str(), sizeof(msg));

#ifdef ARD_BETA
#define COMPACT_START 2097152  
#else
#define COMPACT_START 1048576  
#endif

#define COMPACT_TO 10240

    static bool print_debug_msg = false;

#ifdef _DEBUG
    print_debug_msg = true;
    unlimitedLogFile = false;
#endif

    //#ifdef ARD_BETA
    //   print_debug_msg = true;
    //#endif


    static bool first_call = true;
    if (first_call)
    {
        const char* p = getenv("ARD_PRINT_DEBUG_MSG");
        if (p && strcmp(p, "YES") == 0)
            print_debug_msg = true;

        QFile f(get_program_appdata_log_file_name());
        qint64 fileSize = f.size();
        if (fileSize > COMPACT_START && !unlimitedLogFile)
        {
            if (QFile::exists(get_program_appdata_bak_log_file_name()))
            {
                if (QFile::exists(get_program_appdata_bak2_log_file_name())) {
                    QFile::remove(get_program_appdata_bak2_log_file_name());
                }

                QFile::rename(get_program_appdata_bak_log_file_name(), get_program_appdata_bak2_log_file_name());
            }
            QFile::copy(get_program_appdata_log_file_name(), get_program_appdata_bak_log_file_name());

            if (f.open(QIODevice::ReadOnly))
            {
                char* buff = new char[fileSize];
                qint64 bytesRead = f.read(buff, fileSize);
                f.close();
                if (bytesRead > 0)
                {
                    char* pCompact = buff + (bytesRead - COMPACT_TO);
                    pCompact = (char*)memchr(pCompact, '\n', COMPACT_TO);
                    if (pCompact)
                    {
                        f.remove();

                        qint64 compacted = bytesRead - (pCompact - buff);

                        QDateTime dt_curr = QDateTime::currentDateTime();
                        QString s = dt_curr.toString(Qt::ISODate);
                        s_tmp = get_program_appdata_log_file_name().toStdString();
                        FILE* f = fopen(s_tmp.c_str(), "w");
                        if (!f)
                        {
                            ard::sleep(1000);
                            f = fopen(s_tmp.c_str(), "w");
                        }

                        if (f)
                        {
                            int ver = get_app_version_as_int();
                            std::string s_tmp2 = s.toStdString();
                            fprintf(f, "------------compacted: %s (%d) ------------\n", s_tmp.c_str(), ver);
                            fwrite(pCompact, 1, compacted, f);
                            fclose(f);
                        }
                    }
                }
                delete[] buff;
            }
        }
    }

    if (type == QtDebugMsg && !print_debug_msg)
    {
        return;
    }

    s_tmp = get_program_appdata_log_file_name().toStdString();
    FILE* f = fopen(s_tmp.c_str(), "a+");
    if (f)
    {
        static bool first_call = true;
        if (first_call)
        {
            QString s = QDateTime::currentDateTime().toString();
            char m2[256] = "";
            std::string s_tmp = s.toStdString();
            strncpy(m2, s_tmp.c_str(), sizeof(m2));
            first_call = false;
            fprintf(f, "s: -------- %s  ---------\n", m2);
        }

        switch (type) {
        case QtDebugMsg:
            fprintf(f, "d: %s\n", msg);
            break;
        case QtInfoMsg:
            fprintf(f, "i: %s\n", msg);
            break;
        case QtWarningMsg:
            fprintf(f, "w: %s\n", msg);
            break;
        case QtCriticalMsg:
            fprintf(f, "c: %s\n", msg);
            break;
        case QtFatalMsg:
            fprintf(f, "f: %s\n", msg);
            fclose(f);
            abort();
        }
        fclose(f);
    }
};


void releaseLogMessageOutput(QtMsgType type, const QMessageLogContext &, const QString &in_msg)
{
    writeToMainLog(in_msg, type);
}


#ifdef Q_OS_ANDROID
//library: LOCAL_LDLIBS := -llog
//void releaseMessageOutput(QtMsgType type, const char *msg)
void releaseMessageOutput(QtMsgType type, const QMessageLogContext &mc, const QString &in_msg)
{
    releaseLogMessageOutput(type, mc, in_msg);

    static char APP_NAME[32];
    std::string s_tmp = programName().toStdString();
    strncpy(APP_NAME, s_tmp.c_str(), sizeof(APP_NAME));
    static char msg[256] = "";
    s_tmp = in_msg.toStdString();
    strncpy(msg, s_tmp.c_str(), sizeof(msg));

    switch (type) 
        {
        case QtDebugMsg:
            __android_log_print(ANDROID_LOG_VERBOSE, APP_NAME, "d: %s\n", msg);
            break;
        case QtWarningMsg:
            __android_log_print(ANDROID_LOG_VERBOSE, APP_NAME, "w: %s\n", msg);
            break;
        case QtCriticalMsg:
            __android_log_print(ANDROID_LOG_VERBOSE, APP_NAME, "c: %s\n", msg);
            break;
        case QtFatalMsg:
            __android_log_print(ANDROID_LOG_VERBOSE, APP_NAME, "f: %s\n", msg);
            //fclose(f);
            abort();      
        }
}
#endif //Q_OS_ANDROID

//ykh+ #ifndef _DEBUG
#ifndef Q_OS_ANDROID

void releaseMessageOutput(QtMsgType type, const QMessageLogContext &mc, const QString &in_msg)
{
    releaseLogMessageOutput(type, mc, in_msg);
}
#endif //Q_OS_ANDROID
//#endif//_DEBUG

#define IDLE_PERIOD_MSEC 30000
#define SHORT_IDLE_PERIOD_MSEC 4000

QPalette default_palette;

static void printVersionInfo() 
{
    extern QDate projectCompilationDate();
    extern QString get_app_version_as_string();
    QString aver = get_app_version_as_string();
    QString qver = QT_VERSION_STR;
    QString sbuild = "small";
    QString srel = "release";
    QString sbdate = projectCompilationDate().toString("dd-MM-yyyy");
#ifdef ARD_BIG
    sbuild = "big";
#endif

#ifdef _DEBUG
    srel = "debug";
#endif

    QString s = "";
#define PRINT_LINE(N, V)s = QString("%1:%2").arg(N).arg(V);\
                            std::cout << "" << s.toStdString() << std::endl;\

    PRINT_LINE("ver", aver);
    PRINT_LINE("qt", qver);
    PRINT_LINE("build", sbuild);
    PRINT_LINE("config", srel);
    PRINT_LINE("date", sbdate);

#undef PRINT_LINE
}

class ArdApplication: public QApplication
{
public:
    ArdApplication(int& argc, char** argv) : QApplication(argc, argv) 
    {
        //ykh!1
        installEventFilter(this);
        startTimer(SHORT_IDLE_PERIOD_MSEC);
        setWindowIcon(QIcon(":ard/images/unix/ard-icon-48x48"));

        QString ss_toolbar_buttons = "QToolButton {border: none;}";
        setStyleSheet(ss_toolbar_buttons);
    }

    bool notify(QObject* receiver, QEvent* event) {        
        bool done = true;
        try {
            done = QApplication::notify(receiver, event);
        }
        catch (googleQt::GoogleException& e)
        {
			ard::error(QString("google exception-in-notify [%1][%2]").arg(e.what()).arg(e.statusCode()));
            //qWarning() << "google exception-in-notify" << e.what() << "status-code=" << e.statusCode();
#ifdef Q_OS_WIN
            ard::ArdStackWalker sw;
            sw.ShowCallstack(GetCurrentThread(), sw.GetCurrentExceptionContext());
#endif
            if (e.statusCode() == 400) {
                ard::asyncExec(AR_GoogleRevokeToken);
            }
        }
        catch (std::bad_alloc& ba)
        {
            qWarning() << "bad_alloc-exception-in-notify" << ba.what();
#ifdef Q_OS_WIN
            ard::ArdStackWalker sw;
            sw.ShowCallstack(GetCurrentThread(), sw.GetCurrentExceptionContext());
#endif
        }
        catch (std::exception& e) {            
            qWarning() << "generic exception-in-notify" << e.what();
            qWarning() << "exiting..";
#ifdef Q_OS_WIN
            ard::ArdStackWalker sw;
            sw.ShowCallstack(GetCurrentThread(), sw.GetCurrentExceptionContext());
#endif
            extern void exit_app();
            exit_app();
        } 
        catch (...) 
        {
            qWarning() << "Undefined Exception in notify";
#ifdef Q_OS_WIN            
            ard::ArdStackWalker sw(StackWalker::AfterCatch);
            sw.ShowCallstack();
#endif
        }
        return done;
    }  

protected:

    /* ykh-block */
    bool eventFilter(QObject *obj, QEvent *ev)
    {
        if (ev->type() == QEvent::MouseButtonPress)
        {
            QMouseEvent* me = dynamic_cast<QMouseEvent*>(ev);
            if (me)
            {
                _lastAppMouseClick = me->globalPos();
                //qDebug() << "<<<m-click" << _lastAppMouseClick;
            }
        }

        if (ev->type() == QEvent::KeyPress || ev->type() == QEvent::MouseMove)
        {
            m_last_activity_time = QTime::currentTime();
        }
        return QApplication::eventFilter(obj, ev);
    }//eventFilter


    void timerEvent(QTimerEvent *)
    {
        if (m_last_activity_time.isValid())
        {
            QTime tm = QTime::currentTime();
            int inactivity_msec = m_last_activity_time.msecsTo(tm);
            if (inactivity_msec > IDLE_PERIOD_MSEC)
            {
                m_last_activity_time = QTime::currentTime();
                model()->onIdle();
            }
        }

    }

protected:
    QTime m_last_activity_time;
};

static ArdApplication* TheAPP = nullptr;
void exit_app()
{
    if(TheAPP){
        TheAPP->exit();
    }
}

#ifdef _DEBUG
void wait4ENTER()
{
    std::cout << "press ENTER to proceed" << std::endl;  
    std::cin.ignore();
}
#endif//_DEBUG

static void runMain(void* )
{
    /*qWarning() << "point1";
    ::raise(SIGSEGV);
    qWarning() << "point2";
    */
    TheAPP->exec();
};

extern QString configFilePath();
int getRecoveryModeRequest() 
{    
    QString sfile = configFilePath();
    QSettings settings(sfile, QSettings::IniFormat);
    int rv = settings.value("recovery-mode", "0").toInt();
    return rv;
}

void requestRecoveryMode(int signal_value) 
{
    if (signal_value == 0) {
        signal_value = 1;
    }

    QString sfile = configFilePath();
    QSettings settings(sfile, QSettings::IniFormat);
    settings.setValue("recovery-mode", signal_value);
}

void clearRecoveryMode() 
{
    QString sfile = configFilePath();
    QSettings settings(sfile, QSettings::IniFormat);
    settings.setValue("recovery-mode", 0);
    settings.setValue("main-policy", outline_policy_Pad);
}

QString getEmailUserId4RecoveryMode() 
{
    QString sfile = configFilePath();
    QSettings settings(sfile, QSettings::IniFormat);
    QString rv = settings.value("email-userid", "").toString();
    return rv;
}

static bool checkSingleInstance()
{   
    ardi_instance_shared_memory.setKey("Ardi.working.instance");
    if (!ardi_instance_shared_memory.create(1)){
        return false;
    }
    return true;
};

int main(int argc, char *argv[])
{
    if (argc > 1) {
        QString argt = argv[1];
        if (argt == "-v") {
            printVersionInfo();
            return 0;
        }
    }

#ifdef _DEBUG
    //    wait4ENTER();
#endif//_DEBUG

    qInstallMessageHandler(releaseMessageOutput);

    ArdApplication a(argc, argv);
    TheAPP = &a;

    if (!checkSingleInstance()) {
        QMessageBox::information(0, "Info", "Active instance of Ardi detected on your computer.", QMessageBox::Ok);
        return 0;
    }

    default_palette = a.palette();

    int recoveryMode = getRecoveryModeRequest();
    if (recoveryMode > 0) {
        RecoveryBox w(recoveryMode);
        w.show();
        runMain(nullptr);
    }
    else {        
        extern void initArdiInstance();
        initArdiInstance();
        qInfo() << "starting instance";
        MainWindow w;
        w.show();
        if (!tryCatchAbort(&runMain, nullptr, "running-main")) {
            qWarning() << "exception-in-main";
        };
    }
  
#ifdef API_QT_AUTOTEST
    extern QString getMPoolStatisticsAsString(QString sprefix);
    qDebug() << getMPoolStatisticsAsString("exiting..");
#endif
    return 0;
}

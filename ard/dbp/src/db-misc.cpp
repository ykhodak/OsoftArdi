#include <QtCore>
#include <QFile>
#include <QSqlError>
#include <QUrl>
#include <QTime>
#include <QFileInfo>
#include <QDir>
#include <QProgressDialog>
#include <QFutureWatcher>
#include <QPlainTextEdit>
#include <QStandardItemModel>
#include <QtConcurrent/QtConcurrent>


#include <iostream>
#include <time.h>
#include "a-db-utils.h"
#include "dbp.h"
#include "anfolder.h"
#include "ansyncdb.h"
#include "a-db-utils.h"
//#include "meta-utils.h"
#include "ard-db.h"

static ArdDB            g_sync_resolve_db;

COUNTER_TYPE getLargestDBModCounter(snc::cit* t)
{
    topic_ptr it = dynamic_cast<topic_ptr>(t);
    if(!it)
        {
            ASSERT(0, "expected basic item object");
            return INVALID_COUNTER;
        }

    ArdDB* adb = it->dataDb();
    if(adb == NULL)
        {
            ASSERT(0, "expected attached DB object");
            return INVALID_COUNTER;
        }

    if(!adb->isOpen())
        {
            ASSERT(0, "expected connected DB");
            return INVALID_COUNTER;
        }

    
    QString sql = QString("SELECT MAX(md) md, MAX(mc) mc FROM( \
SELECT MAX(mdc) md, MAX (mvc) mc from ard_tree         \
)");

    
    COUNTER_TYPE cm = INVALID_COUNTER;
    COUNTER_TYPE cv = INVALID_COUNTER;

    auto q = adb->selectQuery(sql);
    assert_return_0(q, "expected query");
    if(q->next())
        {
            cm = q->value(0).toInt();
            cv = q->value(1).toInt();
        }
        

    COUNTER_TYPE res = 0;
#define CHECK_COUNTER(C)if(C != INVALID_COUNTER && C > res)res = C;
    CHECK_COUNTER(cm);
    CHECK_COUNTER(cv);
#undef CHECK_COUNTER

    return res;
};


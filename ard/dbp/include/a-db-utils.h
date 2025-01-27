#pragma once

#include <QRgb>
#include <QFont>
#include <QMimeData>
#include <QSqlQuery>
#include <QMutex>

#include <list>
#include "snc.h"
//#include "global-enums.h"
#include "global-containers.h"
#include "global-functions.h"
#include "global-struct.h"
#include "global-media.h"
#include "global-color.h"
#include "ard-gui.h"
#include "ard-mime.h"
#include "db-stat.h"
#include "ard-db.h"
#include "anfolder.h"
#include "ard.h"
#include "snc-tree.h"

namespace ard 
{
	class topic;
};

struct LastSyncInfo
{
    LastSyncInfo();

    QDateTime   syncTime;
    quint64     syncDbSize;

    QString     toString()const;
    bool        fromString(QString val);
    QString     toString4Human()const;

    void        invalidate();
    bool        isValid()const;
};

class TopicsByType : public snc::MemFindPipe
{
public:

    using FType2Topic = std::unordered_map<int, ard::topic*>;

    ard::topic*    findGtdSortingFolder(EFolderType ft);

    void pipe(snc::cit*)override;
    TOPICS_LIST&    topics() { return m_topics; }
    FType2Topic     byType() { return   m_by_type; }
protected:
    TOPICS_LIST     m_topics;
    FType2Topic     m_by_type;
};


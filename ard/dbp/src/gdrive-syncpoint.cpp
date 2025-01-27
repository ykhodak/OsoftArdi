#include <QFileInfo>
#include <QUuid>
#include <QDesktopServices>
#include "gdrive-syncpoint.h"
#include "ansyncdb.h"
#include "gcontact/GcontactRoutes.h"
#include "gcontact/GcontactCache.h"
#include "gmail/GmailRoutes.h"
#include "GoogleClient.h"
#include "Endpoint.h"


#ifdef ARD_GD


using namespace googleQt;
using namespace gdrive;

GDSyncPoint::GDSyncPoint(bool silence_mode) :SyncPoint(silence_mode)
{
};

void GDSyncPoint::finalizeSync()
{
 //   if(!m_account_name.isEmpty()){
 //           ard::asyncExec(AR_GoogleReconnectedAfterSync);
 //       }
};

bool GDSyncPoint::prepare(QString hint)
{
    m_hint = hint;

#ifdef API_QT_AUTOTEST
    assert_return_false(false, "NA in case of API_QT_AUTOTEST");
#endif

    if (!ard::google())
        {
            log("Failed to connect to GDrive. Aborted.");
            return false;
        }

    if (!ard::google()) 
        {
            assert_return_false(false, "Expected GD client");
        }

    bool ok = false;

    try
        {
            auto gd = ard::google()->gdrive();

            AboutArg arg;
            arg.setFields("user(displayName,emailAddress,permissionId), storageQuota(limit,usage)");
            auto a = gd->getAbout()->get(arg);
            const about::UserInfo& u = a->user();
            const about::StorageQuota& q = a->storagequota();

            m_account_permission_id = u.permissionid();
            m_account_name = u.displayname();
            m_account_email = u.emailaddress();

            log("GoogleDrive sync");
            log(QString("GD-permissionId:%1")
                .arg(m_account_permission_id) 
                );            
            log(QString("GD-account:%1 %2")
                .arg(m_account_name) 
                .arg(m_account_email)
                );
            log(QString("GD-quota:%1 %2")
                .arg(size_human(q.usage()))
                .arg(size_human(q.limit()))
                );

            //.. get file modified time
            QString sRemoteDBFileZip = get_compressed_remote_db_file_name(m_composite_remote_db_prefix);
            QString fileId = gd->appDataFileExists(sRemoteDBFileZip);
            if (!fileId.isEmpty()){
                GetFileArg file_arg(fileId);
                arg.setFields("size,modifiedTime");
                auto f = gd->getFiles()->get(file_arg);
                QDateTime mod_time = f->modifiedtime();
                quint64 fsize = f->size();

                log(QString("GDB-file:%1 %2")                    
                    .arg(size_human(fsize))
                    .arg(mod_time.toString())
                );
            }

            ok = true;
        }
    catch (GoogleException& e)
        {
            log_error(QString("Get account info %1").arg(e.what()));
        }
    return ok;
}


bool GDSyncPoint::copyRemoteDB() 
{
    assert_return_false(ard::google(), "Expected GD client");
    QString localDestFile = compressed_remote_sync_area_db_clone();

    QString sRemoteDBFileZip = get_compressed_remote_db_file_name(m_composite_remote_db_prefix);
    QString fID = ard::google()->gdrive()->appDataFileExists(sRemoteDBFileZip);
    if (fID.isEmpty())
        {
            log(QString("Not found remote DB file: %1").arg(sRemoteDBFileZip));
            //touch the file
            QFile out(localDestFile);
            if (!out.open(QFile::WriteOnly | QIODevice::Truncate)) {
                qWarning() << "Error opening file: " << localDestFile;
                return false;
            }
            out.close();
            return true;
        }
    else
        {
            log(QString("load remote DB file: %1 %2").arg(sRemoteDBFileZip).arg(fID));
            gui::sleep(700);/// sometime google throws back 400 when requests come in too frequent
            if (!ard::google()->gdrive()->downloadFileByID(fID, localDestFile))
                {
                    log(QString("failed to download remote DB file: %1 %2").arg(sRemoteDBFileZip).arg(fID));
                    return false;
                };
        }
    log(QString("r-DB: %1 -> %2").arg(sRemoteDBFileZip).arg(localDestFile));
    return true;
};

bool GDSyncPoint::upgradeRemoteDB() 
{
    assert_return_false(ard::google(), "Expected GD client");

    QString sRemoteDBFileZip = get_compressed_remote_db_file_name(m_composite_remote_db_prefix);

    QFileInfo remote_compressed_fi(compressed_remote_sync_area_db_clone());

    if (!remote_compressed_fi.exists())
        {
            log("remote compressed sync DB clone not found:" + compressed_remote_sync_area_db_clone());
            return false;
        }

    QString fileId = ard::google()->gdrive()->upgradeAppDataFile(compressed_remote_sync_area_db_clone(), sRemoteDBFileZip);
    if (fileId.isEmpty())
        {
            log(QString("failed to upload DB file:").arg(sRemoteDBFileZip));
            return false;
        }
    else {
        //.. update last sync info
        //m_last_sync_info.invalidate();
        LastSyncInfo si;
        si.syncDbSize = remote_compressed_fi.size();
        si.syncTime = QDateTime::currentDateTime();
        dbp::configGDriveSyncStoreLastSyncTime(&m_local_clone_db, si);
    }


    log(QString("remote db updated: %1 %2").arg(sRemoteDBFileZip).arg(fileId));
    return true;
};

bool GDSyncPoint::syncExtraModules()
{
    bool rv = false;
    try
    {
        auto g = ard::google();
        auto gm = ard::gmail();
        if (g && gm && gm->cacheRoutes()) {
            g->endpoint()->diagnosticSetRequestContext("syncExtraModules/refreshLabels");
            gm->cacheRoutes()->refreshLabels();
            rv = true;
        }
    }
    catch (GoogleException& e)
    {
        log_error(QString("Get account info %1").arg(e.what()));
    }
    
    return rv;
};



#endif //ARD_GD

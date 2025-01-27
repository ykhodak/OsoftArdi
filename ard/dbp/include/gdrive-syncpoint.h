#pragma once

#ifdef ARD_GD

#include "syncpoint.h"
#include "GoogleClient.h"
#include "gdrive/GdriveRoutes.h"

class GDSyncPoint : public SyncPoint
{
    Q_OBJECT
public:

    GDSyncPoint(bool silence_mode);
    bool prepare               (QString hint)override;
    void finalizeSync          ()override;
    bool syncExtraModules() override;

    virtual QString     shortName()const override{return "gdrive";};
    CloudIdType         cloudIdType()const override { return CloudIdType::GDriveId; }
protected:
    /// copy remote DB archive, it doesn't fail if file doesn't exists
    bool        copyRemoteDB()override;
    /// upgrade cloud DB with new data
    bool        upgradeRemoteDB()override;
    /// enum of sync point
    snc::SyncPointID getPointID()const override{ return snc::syncpGDrive; }   
protected:
    QString     m_account_name, m_account_email, m_account_permission_id;    
};


#endif //ARD_GD


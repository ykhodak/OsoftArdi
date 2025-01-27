#pragma once

#include <QRect>
#include "gmail/GmailRoutes.h"

class ArdDB;
struct LastSyncInfo;

#define MAX_STORED_TSPACE_TABS      10
#define MAX_STORED_POPUP_PANELS     8

#define MIN_CUST_FONT_SIZE 10
#define MAX_CUST_FONT_SIZE 26

#define ARD_FALLBACK_DEFAULT_NOTE_FONT_SIZE 18
#define ARD_FALLBACK_DEFAULT_NOTE_FONT_FAMILY "Times"

namespace dbp
{
    struct popup_state 
    {
        struct topic_pstate 
        {
            topic_ptr   topic{nullptr};
            int         index;
            bool        locked_selector{false};
        };
        using pstate_arr = std::vector<topic_pstate>;

		pstate_arr      tab_topics;
        int             curr_tab_index;
    };

    typedef std::vector< std::pair<QString, QString> > DB_STATISTICS;
    enum EDatesSummary
        {
            ModifiedUknown = 0, ModifiedToday, ModifiedSinceYesterday, ModifiedSince1Week, Modified1SinceMonth, ModifiedSince1Year, ModifiedOlderThenYear
        };
    enum ETypeSummary
        {
            TypeNotes=1, TypeRetired, TypeToDos, TypeAnyItem,
            TypeNonSummary, 
            TypeResources 
        };

    void                findItemsByType           (EOBJ otype, TOPICS_LIST& items_list, ArdDB* db, int subtype = -1);
    void                findItemsWHERE           (TOPICS_LIST& items_list, ArdDB* db, QString where);
	TOPICS_LIST         findByText              (QString s, ArdDB* db);
	TOPICS_LIST         findToDos               (ArdDB* db);
	TOPICS_LIST         findNotes               (ArdDB* db);
	TOPICS_LIST         findBookmarks			(ArdDB* db);
	TOPICS_LIST         findPictures			(ArdDB* db);
	TOPICS_LIST         findAnnotated           (ArdDB* db);
	TOPICS_LIST         findColors              (ArdDB* db);
    void                findByDatesItems        (EDatesSummary mods, TOPICS_LIST& items_list, ArdDB* db);
    void                findByTypeItems          (ETypeSummary type, TOPICS_LIST& items_list, ArdDB* db);
    int                 getRetiredCount();

    void                findBySyid          (QString syid, TOPICS_LIST& items_list, ArdDB* db/*, EOBJ eo*/);

    void         getDBStatistics(DB_STATISTICS& db_stat);

    void            loadFileSettings();
    void            loadDBSettings(ArdDB& db);
    
    //some supplied demo database with basic topics
    bool            configFileSuppliedDemoDBUsed();
    void            configFileSetSuppliedDemoDBUsed(bool val = true);
    void            checkOnSuppliedDemoImport();

    //QRect           configFileLastPopup();
    //void            configFileSetLastPopup(QRect val);

    ///SupportCmdLevel - some protected support/debug function gets enabled
    DB_ID_TYPE      configFileSupportCmdLevel();
    void            configFileSetSupportCmdLevel(int val);
    ////GetLastDB - last DB opened
    QString         configFileGetLastDB();
    void            configFileSetLastDB(QString val);

    bool            configFileFollowDestination();
    void            configFileSetFollowDestination(bool val);

    bool            configFileIsSyncEnabled();

    int             configFileCustomFontSize();
    void            configFileSetCustomFontSize(int val);
    int             configFileNoteFontSize();
    void            configFileSetNoteFontSize(int val);
    QString         configFileNoteFontFamily();
    void            configFileSetNoteFontFamily(QString val);

    void            configFileSetRunInSysTray(bool v);
    bool            configFileGetRunInSysTray();


    bool            guiCheckNetworkConnection();
    ///default email ID
    QString         configEmailUserId();
	QString         configFallbackEmailUserId();
    void            configSetEmailUserId(QString);
    QString         configEmailUserTokenFilePath(QString userId);
	bool			configSwitchToFallbackEmailUserId();
    /// used during authorization process, get's renamed to actual token file after
    QString         configTmpUserTokenFilePath();


    DB_ID_TYPE      configLastDestGenericTopicOID();
    void            configSetLastDestGenericTopicOID(DB_ID_TYPE val);
    DB_ID_TYPE      configLastDestHoistedOID();
    void            configSetLastDestHoistedOID(DB_ID_TYPE val);

    DB_ID_TYPE      configLastHoistedOID();
    void            configSetLastHoistedOID(DB_ID_TYPE val);

	std::map<int, int>	configLoadBoardRulesSettings();
	void				configStoreBoardRulesSettings();
	std::map<int, int>	configLoadBoardFoldersSettings();
	void				configStoreBoardFolderBandWidth(int dbid, int w);

    DB_ID_TYPE      configLastSelectedKRingKeyOID();
    void            configSetLastSelectedKRingKeyOID(DB_ID_TYPE val);

    void            configSaveMoveDestID(int idx, DB_ID_TYPE id);
    DB_ID_TYPE      configLoadMoveDestID(int idx);

    void            configGDriveSyncLoadLastSyncTime(ArdDB* db, LastSyncInfo& si);
    void            configGDriveSyncStoreLastSyncTime(ArdDB* db, const LastSyncInfo& si);

    void            configLocalSyncLoadLastSyncTime(ArdDB* db, LastSyncInfo& si);
    void            configLocalSyncStoreLastSyncTime(ArdDB* db, const LastSyncInfo& si);

    //bool            configFileGoogleIsEnabled();
    //void            configFileEnableGoogle(bool set_it);

    bool            configFileGoogleEmailListCheckSelectColumn();
    void            configFileEnableGoogleEmailListCheckSelectColumn(bool set_it);
   
    QString          configFileContactIndexStr();
    void             configFileSetContactIndexStr(QString str);

	bool            configFileMailBoardUnreadFilterON();
	void            configFileSetMailBoardUnreadFilterON(bool set_it);
	bool            configFileFilterInbox();
	void            configFileSetFilterInbox(bool set_it);
	bool            configFilePreFilterInbox();
	void            configFileSetPreFilterInbox(bool set_it);
	bool            configFileMaiBoardSchedule();
	void            configFileSetMaiBoardSchedule(bool set_it);


    DB_ID_TYPE       configFileContactHoistedGroupId();
    QString          configFileContactHoistedGroupSyId();
    void             configFileSetContactHoistedGroupId(DB_ID_TYPE groupId);

    DB_ID_TYPE       configFileELabelHoisted();
    void             configFileSetELabelHoisted(DB_ID_TYPE lb_id);

    QString          configFileLastShellAccessDir();
    void             configFileSetLastShellAccessDir(QString val, bool recoverDirFromFile);

    bool            isDBSyncInCurrentAccountEnabled();
    void            enableBSyncInCurrentAccount();

	EOutlinePolicy	configFileBornAgainPolicy();
	void			configFileSetBornAgainPolicy(EOutlinePolicy pol);

    QString         currentTimeStamp();
    QString         currentDateStamp();

#ifdef ARD_BIG

    popup_state     loadPopupTopics();
    void            configStorePopupIDs(QString);
#endif
    void            configStoreCurrentTopic();
    void            configRestoreCurrentTopic();

    bool            configFileGetDrawDebugHint();
    void            configFileSetDrawDebugHint(bool val);

    bool            configFileCheck4Trial();

    ///pair thread-id,message-id
    const std::pair<QString, QString>&         configFileCurrDbgMsgId();
    void            configFileSetCurrDbgMsgId(const std::pair<QString, QString>& val);

#ifdef ARD_CHROME
    qreal            zoomChromeBrowser();
    void             setZoomChromeBrowser(qreal val);
#else
    int              zoomTextBrowser();
    void             setZoomTextBrowser(int val);
#endif  

    //dbp::ETypeSummary configFileGetGroupType();
    //void              configFileSetGroupType(dbp::ETypeSummary t);

    QString         configFileLastSearchStr();
    void            configFileSetLastSearchStr(QString s);
    QString         configFileLastSearchFilter();
    void            configFileSetLastSearchFilter(QString s);

	//bool			configFileRulesFilterON();
	//void            configFileSetRulesFilterON(bool val);


    bool            configIsPwdValid(ArdDB* db, QString pwd);
    bool            configChangePwd(ArdDB* db, QString oldPwd, QString newPwd, QString pwdHint, bool enableReadOnly);
    bool            configHasPwd(ArdDB* db);
    bool            guiCheckPassword(bool& ROmodeRequest, ArdDB* db, QString hint = "");
    QString         configGetPwdHint(ArdDB* db);
    bool            configCanROA_WithoutPassword(ArdDB* db);

	TOPICS_LIST     selectLockedList(QString sql, ArdDB* db);

    std::set<ard::EColor> configColorGrepInFilter();
    void            configSetColorGrepInFilter(std::set<ard::EColor>);
};

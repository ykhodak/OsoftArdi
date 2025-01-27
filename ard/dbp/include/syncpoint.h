#pragma once

#include <QString>
#include <QThread>
#include "snc.h"
#include "a-db-utils.h"
#include "ard-db.h"
#include "snc-enum.h"

class QFile;

class SyncPoint : public QObject 
{
  friend class SyncController;
  friend void sync_log(QString s);
  friend void sync_log_error(QString s);
  friend void sync_log(const STRING_LIST& string_list);
  friend QString get_SqlDatabase_info(ArdDB* db);
public:

  SyncPoint(bool silence_mode);
  virtual ~SyncPoint();

  static bool isSyncRunning();
  static bool isSyncBroken();
  static ard::aes_result lastSyncResult();
  static bool isLastSyncDBIdenticalAfterSync();
  static void breakSync();
  static void runGdriveSync(bool silence_mode, SYNC_AUX_COMMANDS sync_commands, QString hint);
  static void runLocalSync(bool silence_mode, SYNC_AUX_COMMANDS sync_commands, QString hint);
  
  bool initSyncPoint();
  virtual bool syncExtraModules() { return true; };
  bool checkSyncAuxCommands();

  virtual CloudIdType         cloudIdType()const = 0;
  virtual bool        prepare               (QString hint) = 0;
  virtual void        finalizeSync          () = 0;
  virtual QString     shortName()const = 0;
  
  static bool compress(QString sourceFile, QString destFile, int forceVersion = -1);
  static ard::aes_result uncompress(QString sourceFile, QString destFile, bool verify_mode);

protected:
  static void tryDboxSync(void* );
  static void tryGDSync(void*);
  
  bool sync();
  bool copy_local_db();
  bool upgrade_local_db();
  bool open_clones_db();
  //bool generate_resolution_db();
  void finalize();
  void log(QString s);
  void log(const STRING_LIST& string_list);
  void stat(QString s);
  void log_error(QString s);
  void incProgress();
  void archiveSyncLog();
  bool ensureSyncDir();
  ard::aes_result uncompressRemoteDB();
  bool compressRemoteDB();
  bool checkBreak()const;
  void closeLog();
  void checkLogFile();
  void setupSyncAuxCommands(const SYNC_AUX_COMMANDS& sc);  

  virtual void        initSyncBreak();  
  virtual bool        copyRemoteDB        () = 0;
  virtual bool        upgradeRemoteDB     () = 0;
  virtual snc::SyncPointID getPointID          ()const = 0;
  virtual void        aux_PrintDataOnHashError();
  virtual void        aux_Test(){};
  
protected:  
  QString     tmp_sync_dir_path();
  QString     local_sync_area_db_clone();
  QString     remote_sync_area_db_clone();

  QString     compressed_remote_sync_area_db_clone();

  bool        silenceMode()const{return m_silence_mode;};
  void        errorMessage(QString s);
  bool        confirmMessage(QString s);
protected:
  ArdDB              m_local_clone_db;
  ArdDB              m_remote_clone_db;
  bool               m_changes_detected,
                     m_hash_OK,
                     m_first_time_remote_db_cloned;
  int                m_sync_progress;
        //m_media_resolve_total, m_media_resolve_curr;
  STRING_MAP         m_hashCompareResult;
  QString               m_composite_remote_db_prefix,
                        m_composite_local_db_prefix;///they should be equal in normal sync, only in case of autotest we want to point 
                                                    ///two different local autotest DB to same sync master DB
  QString            m_hint;
  bool               m_silence_mode;
  SYNC_AUX_COMMANDS  m_sync_aux_commands;
  QFile              *m_logFile;
  QTextStream        *m_logStream;
  LastSyncInfo       m_last_sync_info;
};

class LocalSyncPoint : public SyncPoint
{
public:

  LocalSyncPoint(bool silence_mode); 
  bool                  prepare               (QString hint)override;
  void                  finalizeSync          ()override{};
  virtual QString     shortName()const override{return "local";};
  CloudIdType         cloudIdType()const override { return CloudIdType::LocalDbId; }

protected:
  bool copyRemoteDB          ()override;
  bool upgradeRemoteDB       ()override;

  snc::SyncPointID getPointID       ()const override{return snc::syncpLocal;}
protected:
  QString m_remote_db_path;
};

class SyncController : public QObject
{
  Q_OBJECT
public:
  SyncController(bool silence_mode, SYNC_AUX_COMMANDS sync_commands);
  ~SyncController();
  void initSync(snc::SyncPointID syncType, QString hint);
  SyncPoint* sp(){return m_sp.get();}
  static std::unique_ptr<SyncPoint> produceSyncPoint(snc::SyncPointID syncType, bool silence_mode);

public slots:
  void afterSync(QString composite_rdb_string);
protected:
    //SyncPoint*            m_sp{nullptr};
    std::unique_ptr<SyncPoint>  m_sp;
    bool                        m_silence_mode;
    SYNC_AUX_COMMANDS           m_controller_sync_aux_commands;
};

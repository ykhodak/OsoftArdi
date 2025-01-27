#pragma once

#define DEFAULT_DB_NAME     "MyData"
#define DB_FILE_NAME        "ardi.sqlite"
#define ROOT_DB_FILE_NAME   "ardi-root.sqlite"

namespace dbp
{
  class Resource;
};

namespace ard {
    class anKRingKeyExt;
    class contact_ext;
};

template <class T>std::unordered_map<QString, T*> toSyidMap(std::vector<T*>& lst);
template <class T> TOPICS_LIST wrapTopicsList(TOPICS_LIST& lst);


extern bool    is_big_screen();
extern bool    is_instance_read_only_mode();
extern bool    is_DB_read_only_mode();
extern int     get_app_version_as_int();
extern QString get_app_version_as_string();
extern QString get_app_version_as_long_string();
extern void    register_db_modification(topic_ptr it, EDB_MOD mod, bool reg_sync_mod = true);

extern void    enable_register_db_modification(bool enable);
extern QString get_db_file_path();
extern QString get_root_db_file_path();
extern QString get_curr_db_weekly_backup_file_path();
extern QString get_tmp_sync_dir_path(QString db_prefix);
extern QString get_tmp_import_dir_path();
extern QString get_compressed_remote_db_file_name(QString db_prefix);
extern QString get_temp_uncompressed_file_suffix();
extern QString get_sync_log_path(QString db_prefix);
extern QString get_tmp_code_dir_path();
extern QString get_sync_log_archives_path();
extern QString get_program_appdata_log_file_name();
extern QString get_program_appdata_bak_log_file_name();
extern QString get_program_appdata_bak2_log_file_name();
extern QString get_program_appdata_sync_autotest_log_file_name();
extern QString defaultRepositoryPath();
extern QString get_cloud_support_folder_name();
extern void    loadDBsList(QStringList& lst);
extern void    monitorSyncActivity(QString s, bool silence_mode);
extern void    monitorSyncActivity(const STRING_LIST& string_list, bool silence_mode);
extern void    monitorSyncStatus(QString s, bool silence_mode);
extern void    startMonitorSyncActivity(bool silence_mode);
extern void    stopMonitorSyncActivity(bool closeMonitorWindow, QString composite_rdb_string, bool silence_mode);
extern void    stepMonitorSyncActivity(int percentages, bool silence_mode);
void rectf2rect(const QRectF& rc_in, QRect& rc_out);
extern QFont* defaultFont();
extern void    detachOwnerGui(topic_ptr);
extern topic_ptr lookupLoadedTopic(DB_ID_TYPE id);
extern QString remoteDBPath4LocalSync(/*int masterDBindex = 2*/);
class QLayout;
extern void setupMainLayout(QLayout* l);

unsigned calc_node_width(QString s, unsigned minWidth);
unsigned calc_node_width(topic_ptr t, unsigned minWidth);
void svg2pixmap(QPixmap* pm, QString svg_resource, const QSize& sz);
QSize calc_text_svg_node_size(const QSizeF& szOriginalImg, QString s);


//copy file, replace old one if exists
extern bool    copyOverFile(QString sourceFile, QString destFile);
//move file, replace old one if exists
extern bool    renameOverFile(QString sourceFile, QString destFile);
//delete a directory along with all of its contents
extern bool    removeDir(const QString &dirName);

extern QString programName();
///returns true if view was built with policy that depends of cloud gmail/gcontact/gdrive data
///it might have to be rebuilt (when user was, switched for example)
class PolicyCategory {
public:
    static bool isGmailDepending(EOutlinePolicy p);    
    static bool isHoistedBased(EOutlinePolicy p);
    static EOutlinePolicy parentPolicy(EOutlinePolicy sub_policy);
    static bool isKRingView(EOutlinePolicy p);
	static bool is_observable_policy(EOutlinePolicy p);
	static bool is_shortcut_wrapping_policy(EOutlinePolicy p);
};

template <class T>
TOPICS_LIST wrapTopicsList(TOPICS_LIST& lst)
{
    TOPICS_LIST rv;
    for (auto& i : lst) {
        auto f = new T(i);
        rv.push_back(f);
    }
    return rv;
};

template <class T>
std::unordered_map<QString, T*> toSyidMap(std::vector<T*>& lst) 
{
    std::unordered_map<QString, T*> rv;
    for (auto& i : lst) {
        rv[i->syid()] = i;
    }
    return rv;
};
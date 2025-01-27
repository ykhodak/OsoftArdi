#pragma once

#include "anfolder.h"
#include "ard-db.h"

class ArdiDbMerger
{
public:
    ArdiDbMerger();

    struct MergeResult 
    {
        int merged_topics{ 0 };
        int merged_contacts{ 0 };
        int skipped_topics{ 0 };
        int skipped_contacts{ 0 };
        bool merge_error{false};
        QString error_str;
    };

    static void guiSelectAndImportArdiFile();
    static void guiImportArdiFile(QString fileName);
    static MergeResult mergeDB(QString dbCompressedFile, bool merge_contacts);
    static MergeResult mergeJSONfile(QString jsonFile);

    static void guiExportToJSON();
    static void guiImportFromJSON();
protected:

    class MergeContext
    {
        friend class ArdiDbMerger;
    public:
        void createMergeContext(ArdDB* db);
        void createMergeContext(ard::topic* root);

    protected:
        void calcHashRecusively(topic_ptr f);
        ard::contacts_model* m_cmodel_in_context{ nullptr };
        FTYPE2TOPIC     m_gtd_branches;///branches have one2one mapping: merge-from -> merge-into       
        TOPICS_LIST     m_all_branches;
        TOPICS_LIST     m_opt_branches; //ufolders + projects       
        TOPICS_LIST     m_projects;
        TOPICS_MAP      m_hash2topic;
    };


    ///returns -1 is failed, number of merged topics otherwise
    MergeResult mergeCompressedDB(QString dbCompressedFile, bool merge_contacts);
    MergeResult do_mergeJSONfile(QString jsonFile);
    ///returns number of merged in topics
    MergeResult     mergeImportedDB(bool merge_contacts);
    int             mergeBranches();
    static void     guiInterpretMergeResult(const MergeResult& r);
    /// returns (imported, skipped) if imported == -1 means error
    std::pair<int, int> mergeDataSpace();
    /// returns (imported, skipped) if imported == -1 means error
    std::pair<int, int> mergeContactsSpace();
    topic_ptr       locate_or_ensure_parent(topic_ptr imported_parent);///we have to go up in branches to get all parent

    ArdDB           m_db_imported;
    QString         m_err_str;

    MergeContext    m_this_mcontext;
    MergeContext    m_import_mcontext;
    TOPIC2TOPIC     m_import_branch2this_branch;
};
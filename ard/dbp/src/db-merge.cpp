#include <QFileDialog>
#include "a-db-utils.h"
#include "db-merge.h"
#include "ard-gui.h"
#include "syncpoint.h"
#include "contact.h"
#include "anfolder.h"
#include "ansyncdb.h"
#include "locus_folder.h"

#define DB_TMP_IMPORT_FILE_NAME "tmp_import_copy.qpk"

ArdiDbMerger::ArdiDbMerger() 
{

};

void ArdiDbMerger::guiSelectAndImportArdiFile() 
{
    QString fileName = QFileDialog::getOpenFileName(gui::mainWnd(),
        "Select Ardi file to import",
        dbp::configFileLastShellAccessDir(),
        "Ardi Data Files  (*.qpk)");

    if (!fileName.isEmpty()) {
        guiImportArdiFile(fileName);
    }
};

void ArdiDbMerger::guiImportArdiFile(QString fileName)
{
    auto res = mergeDB(fileName, true);
    guiInterpretMergeResult(res);
};

void ArdiDbMerger::guiInterpretMergeResult(const ArdiDbMerger::MergeResult& res)
{
    if (res.merge_error) {
        if (!res.error_str.isEmpty()) {
			ard::errorBox(ard::mainWnd(), res.error_str);
        }
    }
    else {
        if (res.merged_topics == 0 &&
            res.merged_contacts == 0)
        {
			ard::messageBox(gui::mainWnd(), QString("Import completed but no new topics found in incomming DB, no new topics were created."));
        }
        else {
			ard::messageBox(gui::mainWnd(), QString("Import completed. Merged '%1' topics and '%2' contacts. Skipped duplicate '%3' topics and '%4' contacts.")
                .arg(res.merged_topics)
                .arg(res.merged_contacts)
                .arg(res.skipped_topics)
                .arg(res.skipped_contacts));
        }
        gui::rebuildOutline();
    }
};

ArdiDbMerger::MergeResult ArdiDbMerger::mergeDB(QString dbCompressedFile, bool merge_contacts) 
{
    ArdiDbMerger m;
    auto res = m.mergeCompressedDB(dbCompressedFile, merge_contacts);
    res.error_str = m.m_err_str;
    return res;
};

ArdiDbMerger::MergeResult ArdiDbMerger::mergeJSONfile(QString jsonFile) 
{
    ArdiDbMerger m;
    auto res = m.do_mergeJSONfile(jsonFile);
    res.error_str = m.m_err_str;
    return res;
};

ArdiDbMerger::MergeResult ArdiDbMerger::mergeCompressedDB(QString dbCompressedFile, bool merge_contacts)
{
    MergeResult res;
    res.merge_error = true;

    if (!ard::isDbConnected()) {
        m_err_str = QString("Expected Open current database.");
        ASSERT(0, m_err_str);
        return res;
    }

    QString db_tmp_import_path = get_tmp_import_dir_path() + DB_TMP_IMPORT_FILE_NAME;

    if (QFile::exists(db_tmp_import_path)) {
        if (!QFile::remove(db_tmp_import_path)) {
            m_err_str = QString("Failed to remove temporary DB import file '%1'. Please reboot and try again.").arg(db_tmp_import_path);
            ASSERT(0, m_err_str);
            return res;
        }
    }

    if (!QFile::copy(dbCompressedFile, db_tmp_import_path)) {
        m_err_str = QString("Failed to copy files '%1' -> '%2'").arg(dbCompressedFile).arg(db_tmp_import_path);
        ASSERT(0, m_err_str);
        return res;
    }

    QFileInfo fi(db_tmp_import_path);
    QString dpath = fi.canonicalFilePath();
    QString db_tmp_path = dpath + get_temp_uncompressed_file_suffix();
    if (QFile::exists(db_tmp_path))
    {
        if (!QFile::remove(db_tmp_path)) {
            m_err_str = QString("Failed to remove temporary DB uncompressed import file '%1'. Please reboot and try again.").arg(db_tmp_path);
            ASSERT(0, m_err_str);
            return res;
        }
    }

    auto r2 = SyncPoint::uncompress(db_tmp_import_path, db_tmp_path, false);
    if (r2.status != ard::aes_status::ok) {
        m_err_str = QString("Failed to uncompress import file '%1'. Aborted.").arg(db_tmp_import_path);
        ASSERT(0, m_err_str);
        return res;
    }

    auto ok = m_db_imported.openDb("import2merge", db_tmp_path);
    if (!ok || !m_db_imported.isOpen())
    {
        m_err_str = QString("Failed to open merge DB '%1'").arg(db_tmp_path);
        ASSERT(0, m_err_str);
        return res;
    }

    if (!m_db_imported.verifyMetaData())
    {
        m_err_str = QString("Failed to verify merge DB meta data '%1'").arg(db_tmp_path);
        ASSERT(0, m_err_str);
        return res;
    }

    if (!m_db_imported.loadTree())
    {
        m_err_str = QString("Failed to load merge DB data tree '%1'").arg(db_tmp_path);
        ASSERT(0, m_err_str);
        return res;
    }

    m_this_mcontext.createMergeContext(ard::db());
    m_import_mcontext.createMergeContext(&m_db_imported);
    res = mergeImportedDB(merge_contacts);  
    if (!res.merge_error) {
        auto r = ard::db()->root();
        if (r) {
            auto ok = r->ensurePersistant(-1);
            if (!ok) {
                ASSERT(0, "failed to ensure root persistance.");
            }
        }
    }
    m_db_imported.close();
    //res.merge_error = false;
    if (!QFile::remove(db_tmp_path)) {
        m_err_str = QString("Failed to clean temporary DB import file '%1' after merge.").arg(db_tmp_import_path);
        ASSERT(0, m_err_str);
        return res;
    }

    if (!QFile::remove(db_tmp_import_path)) {
        m_err_str = QString("Failed to clean temporary compressed DB import file '%1' after merge.").arg(db_tmp_import_path);
        ASSERT(0, m_err_str);
        return res;
    }

    return res;
};

ArdiDbMerger::MergeResult ArdiDbMerger::do_mergeJSONfile(QString jsonFile)
{
    QFile file(jsonFile);
    file.open(QIODevice::ReadOnly | QIODevice::Text);
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();
    QJsonObject js = doc.object();

    auto r = new ard::topic("json-imported");
    r->fromJson(js);
    m_this_mcontext.createMergeContext(ard::db());
    m_import_mcontext.createMergeContext(r);
    ArdiDbMerger::MergeResult res = mergeImportedDB(false);
    if (!res.merge_error) {
        auto r = ard::db()->root();
        if (r) {
            auto ok = r->ensurePersistant(-1);
            if (!ok) {
                ASSERT(0, "failed to ensure root persistance.");
            }
        }
    }
    return res;
};

int ArdiDbMerger::mergeBranches() 
{
    int merged_num = 0;

    for (auto i : m_import_mcontext.m_gtd_branches) {
        auto ft = i.first;
        auto f_import = i.second;

        auto j = m_this_mcontext.m_gtd_branches.find(ft);
        ASSERT(j != m_this_mcontext.m_gtd_branches.end(), "failed to locate gtd topic by folder type") << static_cast<int>(ft);
        if (j != m_this_mcontext.m_gtd_branches.end()) {
            m_import_branch2this_branch[f_import] = j->second;
        }
    }

    /// first ensure branches from import
    /// opt branches are ufolders and projects, they
    /// can be different, we have to locate them by title
    auto u_holder = ard::db()->findLocusFolder(EFolderType::folderUserSortersHolder);
    if (!u_holder) {
        m_err_str = QString("Failed to locate locus folders container in local DB. Aborted.");
        ASSERT(0, m_err_str);
        return 0;
    }

/*    if (!p_holder) {
        m_err_str = QString("Failed to locate projects container in local DB. Aborted.");
        ASSERT(0, m_err_str);
        return 0;
    }*/


    for (auto& f : m_import_mcontext.m_opt_branches) {
        auto str = f->title();
        auto it = std::find_if(m_this_mcontext.m_opt_branches.begin(),
            m_this_mcontext.m_opt_branches.end(), [str](topic_ptr t)
        {
            bool rv = (t->title().compare(str) == 0);
            return rv;
        });

        if (it == m_this_mcontext.m_opt_branches.end()) {
            auto f2 = f->cloneInMerge();
            if (f2) {
                topic_ptr parent4clone = u_holder;
                //if (f2->isProject()) {
                //    parent4clone = p_holder;
                //}
                bool ok = parent4clone->addItem(f2);
                ASSERT(ok, "failed to add branch during merge") << f2->dbgHint() << parent4clone->dbgHint();
                if (ok) {
                    merged_num++;
                }
                /*
                else {

#ifdef _DEBUG
                    auto f2_debug = f->cloneInMerge();
                    qDebug() << f2_debug->dbgHint();
#endif //_DEBUG
                }
                */
                m_import_branch2this_branch[f] = f2;
            }
        }
        else {
            m_import_branch2this_branch[f] = *it;
            //f_import
        }
    }

    //for (auto it : m_import_branch2this_branch) {
    //  qDebug() << "<<merge b2b" << it.first->title() << "->" << it.second->title();
    //}

    return merged_num;
};

std::pair<int, int> ArdiDbMerger::mergeDataSpace()
{
    std::pair<int, int> rv{0,0};
    rv.first = mergeBranches();

    for (auto& i : m_import_mcontext.m_hash2topic) {
        auto str = i.first;
        auto j = m_this_mcontext.m_hash2topic.find(str);
        if (j == m_this_mcontext.m_hash2topic.end()) {
            /// if hash is not in our DB we can't ignore it
            topic_ptr f_imported = i.second;
            f_imported->setSkipMergeImport(false);
            auto p = f_imported->parent();
            while (p) {
                p->setSkipMergeImport(false);
                p = p->parent();
            }
        }
    }

    ///at this point we should know topics we want to accept
    TOPICS_LIST imported_topics2accept;
    for (auto i : m_import_mcontext.m_hash2topic) {
        auto f = i.second;
        if (!f->skipMergeImport()) {
            imported_topics2accept.push_back(f);
        }
        else {
            rv.second++;
        }
    }

    for (auto& f : imported_topics2accept) {
        auto pp_imported = f->parent();
        if (pp_imported) {
            auto pp_our = locate_or_ensure_parent(pp_imported);
            if (pp_our) {
                //if (m_import_branch2this_branch.find(f) != m_import_branch2this_branch.end()) {
                if(f->skipMergeImport()){
                    //ASSERT(0, "already imported as parent?");
                    continue;
                }
                auto f_local = f->cloneInMerge();
                bool ok = pp_our->addItem(f_local);
                if (!ok) {
                    ASSERT(0, "failed in insert subitem, merge skipped") << pp_our->dbgHint() << f_local->dbgHint();
                    f_local->release();
                    f_local = nullptr;
                    rv.second++;
                }
                else {
                    m_import_branch2this_branch[f] = f_local;
                    rv.first++;
                }
            }
        }
    }

    return rv;
};

ArdiDbMerger::MergeResult ArdiDbMerger::mergeImportedDB(bool merge_contacts)
{
    MergeResult res;
    res.merge_error = false;

    auto mg_topics = mergeDataSpace();
    res.merged_topics = mg_topics.first;
    res.skipped_topics = mg_topics.second;
    if (merge_contacts) {
        auto cmerge_res = mergeContactsSpace();
        res.merged_contacts = cmerge_res.first;
        res.skipped_contacts = cmerge_res.second;
    }

    return res;
};

std::pair<int, int> ArdiDbMerger::mergeContactsSpace()
{
    std::pair<int, int> res;
    auto this_cmodel = m_this_mcontext.m_cmodel_in_context;
    auto imported_cmodel = m_import_mcontext.m_cmodel_in_context;
    if (this_cmodel && imported_cmodel) {
        res = this_cmodel->mergeContacts(*imported_cmodel);
    }
    return res;
};

topic_ptr ArdiDbMerger::locate_or_ensure_parent(topic_ptr imported_parent) 
{
    /*
#ifdef _DEBUG
    if (imported_parent->title() == "Gmail") {
        int tmp = 0;
        tmp++;
        qDebug() << tmp;
    }
#endif
*/

    auto i = m_import_branch2this_branch.find(imported_parent);
    if (i != m_import_branch2this_branch.end()) {
        return i->second;
    }

    auto pp_imported = imported_parent->parent();
    if (!pp_imported) {
        ASSERT(0, "expected parent") << imported_parent->dbgHint();
        return nullptr;
    }

    auto pp_in_this = locate_or_ensure_parent(pp_imported);
    if (pp_in_this) {
        auto f_local = imported_parent->cloneInMerge();
        bool ok = pp_in_this->addItem(f_local);
        if (!ok) {
            ASSERT(0, "failed in insert subitem") << pp_in_this->dbgHint() << f_local->dbgHint();
            f_local->release();
            return nullptr;
        }
        m_import_branch2this_branch[imported_parent] = f_local;
        imported_parent->setSkipMergeImport(true);
        return f_local;
    }

    ASSERT(0, "failed to ensure parent in destination DB") << pp_imported->dbgHint();
    return nullptr;
};

void ArdiDbMerger::guiExportToJSON() 
{
    if (ard::isDbConnected()) {
        auto r = ard::root();
        if (r) {
            QString filename = QFileDialog::getSaveFileName(
                nullptr,
                "Save data to JSON",
                dbp::configFileLastShellAccessDir(),
                "JSON Documents (*.json)");
            if (!filename.isNull())
            {
                dbp::configFileSetLastShellAccessDir(filename, true);
                QJsonObject js;
                r->toJson(js);
                QJsonDocument doc(js);
                auto bd = doc.toJson(QJsonDocument::Indented);
                QFile file(filename);
                file.open(QIODevice::WriteOnly);
                file.write(bd);
                file.close();
            }
        }
    }
};

void ArdiDbMerger::guiImportFromJSON() 
{
    QString fileName = QFileDialog::getOpenFileName(gui::mainWnd(),
        "Select JSON file to import",
        dbp::configFileLastShellAccessDir(),
        "JSON Documents (*.json)");

    if (!fileName.isEmpty()) {
        dbp::configFileSetLastShellAccessDir(fileName, false);

        auto res = mergeJSONfile(fileName);
        guiInterpretMergeResult(res);
    }
};


/**
* MergeContext
*/
void ArdiDbMerger::MergeContext::createMergeContext(ArdDB* db)
{
    assert_return_void(db, "expected DB");
    assert_return_void(db->isOpen(), "expected open DB");

    m_cmodel_in_context = db->cmodel();
    createMergeContext(db->root());
};

void ArdiDbMerger::MergeContext::createMergeContext(ard::topic* root) 
{
    assert_return_void(root, "expected root topic");

    TopicsByType mf;
    root->memFindItems(&mf);

    topic_ptr f = nullptr;
#define ADD_F(T) f = mf.findGtdSortingFolder(T);if(f){m_gtd_branches[T] = f;m_all_branches.push_back(f);}

    ADD_F(EFolderType::folderSortbox);
    ADD_F(EFolderType::folderMaybe);
    ADD_F(EFolderType::folderReference);
    ADD_F(EFolderType::folderRecycle);
    ADD_F(EFolderType::folderDelegated);
    ADD_F(EFolderType::folderBoardTopicsHolder);

    f = mf.findGtdSortingFolder(EFolderType::folderUserSortersHolder);
    if (f) {
        for (auto& i : f->items()) {
            auto t = dynamic_cast<ard::topic*>(i);
            if (t) {
                m_all_branches.push_back(t);
                m_opt_branches.push_back(t);
                m_projects.push_back(t);
            }
        }
    }
#undef ADD_F

    for (auto t : m_all_branches) {
        calcHashRecusively(t);
    }

    //qDebug() << "<<merge create-context/branches=" << m_all_branches.size() << "h-map" << m_hash2topic.size();
};

void ArdiDbMerger::MergeContext::calcHashRecusively(topic_ptr f) 
{
    for (auto i : f->items()) {
        auto t = dynamic_cast<ard::topic*>(i);
        if (t) {
            auto s = t->calcMergeAtomicHash();
            t->setSkipMergeImport(true);
            if (!s.isEmpty()) {
                m_hash2topic[s] = t;
                //qDebug() << "<< merge hash=" << s << t->title();
            }
        }
        calcHashRecusively(t);
    }
};


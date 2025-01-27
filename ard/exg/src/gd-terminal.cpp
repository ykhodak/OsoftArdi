#include <QBuffer>
#include <QFileDialog>
#include "a-db-utils.h"
#include "gd-terminal.h"
#include "registerbox.h"
#include "gdrive/GdriveRoutes.h"
#include "gdrive/files/FilesCreateFileDetails.h"
#include "gdrive/files/FilesUpdateFileDetails.h"

namespace googleQt
{
    namespace files
    {
        class FileResource;
    };
}

using namespace googleQt;
using namespace gdrive;


class GdriveCommands
{
public:
    GdriveCommands(GoogleClient& c);
    ///get account information
    void about(QString);
    ///list root folder content
    void ls(QString);
    ///list folders in a directory content
    void ls_folders(QString);
    ///list folder content
    void ls_dir_content(QString);
    ///list space content
    void ls_space_content(QString);
    ///get file or folder meta information
    void get(QString fileId);
    ///move file or folder
    void move_file(QString fileId);
    ///download file media data to local folder,
    ///required fileID and local file name separated
    ///by space
    void download(QString fileId);
    ///download and print file content on the screen, good for text files
    void cat(QString fileId);
    ///delete files in appDataFolder by name except ID
    void rm_appdata_files(QString name_exceptId);
protected:
    GoogleClient& m_c;
    GdriveRoutes*  m_gd;
};


void run_gdrive_terminal() 
{
    auto gc = ard::google();
    if (!gc) {
        return;
    }

    GdriveCommands cmd(*gc);

    ard::terminal t;
    t.addAction("about", "About (Information about account - limits etc.)", [&](QString arg) {cmd.about(arg); });   
    t.addAction("ls", "List", [&](QString arg) {cmd.ls(arg); });
    t.addAction("ls_folders", "Show folders", [&](QString arg) {cmd.ls_folders(arg); });
    t.addAction("ls_dir_content", "Show directory content", [&](QString arg) {cmd.ls_dir_content(arg); });
    t.addAction("get", "Get File Info", [&](QString arg) {cmd.get(arg); });
    t.addAction("download", "Download file", [&](QString arg) {cmd.download(arg); });
    t.addAction("cat", "Print file content on screen", [&](QString arg) {cmd.cat(arg); });
    t.addSeparator();
    t.addAction("ls_space_content", "Show space content (appDataFolder, drive, photos)", [&](QString arg) {cmd.ls_space_content(arg); });
    t.addAction("rm_appdata_files", "Remove files in appDataFolder", [&](QString arg) {cmd.rm_appdata_files(arg); });
    t.addSeparator();

    t.buildMenu();
    t.exec();
};

GdriveCommands::GdriveCommands(GoogleClient& c) :m_c(c)
{
    m_gd = m_c.gdrive();
};

void GdriveCommands::about(QString)
{
    AboutArg arg;
    arg.setFields("user(displayName,emailAddress,permissionId), storageQuota(limit,usage)");
    m_gd->getAbout()->get_Async(arg)->then([&](std::unique_ptr<about::AboutResource> a)
    {
        const about::UserInfo& u = a->user();
        const about::StorageQuota& q = a->storagequota();
        tmout << "permissionID=" << u.permissionid() << tendl;
        tmout << "name=" << u.displayname()
            << " email=" << u.emailaddress() << tendl;

        tmout << "used=" << size_human(q.usage())
            << " limit=" << size_human(q.limit()) << tendl;
    }, 
        [](std::unique_ptr<GoogleException> e) 
    {
        tmout << "Exception: " << e->what() << tendl;
    });
};

/*
m_gd->getFiles()->list_Async(arg)->then([&](std::unique_ptr<googleQt::files::FileResourcesCollection> lst) {},
[](std::unique_ptr<GoogleException> e)
{
tmout << "Exception: " << e->what() << tendl;
});

*/

void GdriveCommands::ls(QString nextToken)
{
    FileListArg arg(nextToken);
    m_gd->getFiles()->list_Async(arg)->then([&](std::unique_ptr<googleQt::files::FileResourcesCollection> lst)
    {
        auto& files = lst->files();
        int idx = 1;
        for (const files::FileResource& f : files) {
            QString mimeType = f.mimetype();
            QString ftype = "[f]";
            if (mimeType == "application/vnd.google-apps.folder") {
                ftype = "[d]";
            }
            else if (mimeType == "image/jpeg") {
                ftype = "[i]";
            }
            tmout << QString("%1").arg(idx++).leftJustified(3, ' ')
                << ftype << " "
                << f.id() << " "
                << f.name() << " "
                << mimeType << tendl;
        }
        if (!lst->nextpagetoken().isEmpty())
        {
            tmout << "next token: " << lst->nextpagetoken() << tendl;
        }
    },
        [](std::unique_ptr<GoogleException> e) 
    {
        tmout << "Exception: " << e->what() << tendl;
    });
};

void GdriveCommands::ls_folders(QString nextToken)
{
    //auto name_filter = gui::edit("", "filter name? [string]");
    auto res = gui::edit("", "filter name? [string]");
    FileListArg arg(nextToken);
    arg.setQ("mimeType = 'application/vnd.google-apps.folder'");
    m_gd->getFiles()->list_Async(arg)->then([&, res](std::unique_ptr<googleQt::files::FileResourcesCollection> lst)
    {
        auto& files = lst->files();
        int idx = 1;
        for (const files::FileResource& f : files) {
            if (!res.second.isEmpty()) {
                auto p = f.name().indexOf(res.second);
                if (p == -1)
                    continue;
            }
            QString mimeType = f.mimetype();
            QString ftype = "[f]";
            if (mimeType == "application/vnd.google-apps.folder") {
                ftype = "[d]";
            }
            else if (mimeType == "image/jpeg") {
                ftype = "[i]";
            }
            tmout << QString("%1").arg(idx++).leftJustified(3, ' ')
                << ftype << " "
                << f.id() << " "
                << f.name();
            tmout << tendl;
        }
        tmout << "next token: " << lst->nextpagetoken() << tendl;
    },
        [](std::unique_ptr<GoogleException> e)
    {
        tmout << "Exception: " << e->what() << tendl;
    });
};

void GdriveCommands::ls_dir_content(QString folderId)
{
    if (folderId.isEmpty())
    {
		ard::messageBox(gui::mainWnd(), "folderId required");
        return;
    }

    FileListArg arg;
    arg.setQ(QString("'%1' in parents").arg(folderId));

    m_gd->getFiles()->list_Async(arg)->then([&](std::unique_ptr<googleQt::files::FileResourcesCollection> lst) 
    {
        auto& files = lst->files();
        int idx = 1;
        for (const files::FileResource& f : files) {
            QString mimeType = f.mimetype();
            QString ftype = "[f]";
            if (mimeType == "application/vnd.google-apps.folder") {
                ftype = "[d]";
            }
            else if (mimeType == "image/jpeg") {
                ftype = "[i]";
            }
            tmout << QString("%1").arg(idx++).leftJustified(3, ' ')
                << ftype << " "
                << f.id() << " "
                << f.name() << " "
                << mimeType << tendl;
        }
        if (!lst->nextpagetoken().isEmpty())
        {
            tmout << "next token: " << lst->nextpagetoken() << tendl;
        }
    },
        [](std::unique_ptr<GoogleException> e)
    {
        tmout << "Exception: " << e->what() << tendl;
    });
};

void GdriveCommands::get(QString fileId)
{
    if (fileId.isEmpty())
    {
		ard::messageBox(gui::mainWnd(), "fileId required");
        return;
    }

    GetFileArg arg(fileId);
    arg.setFields("id,name,size,mimeType,webContentLink,webViewLink,parents,spaces,modifiedByMeTime");
    m_gd->getFiles()->get_Async(arg)->then([&](std::unique_ptr<googleQt::files::FileResource> f) 
    {
        tmout << "id= " << f->id() << tendl
            << "name= " << f->name() << tendl
            << "type= " << f->mimetype() << tendl
            << "size= " << f->size() << tendl
            << "modified= " << f->modifiedbymetime().toString() << tendl;
        if (!f->webcontentlink().isEmpty())
        {
            tmout << "webContentLink= " << f->webcontentlink() << tendl;
        }
        if (!f->webviewlink().isEmpty())
        {
            tmout << "webViewLink= " << f->webviewlink() << tendl;
        }
        if (f->parents().size() != 0)
        {
            tmout << "=== parents ===" << tendl;
            for (auto j = f->parents().begin(); j != f->parents().end(); j++)
            {
                tmout << *j << tendl;
            }
        }
        if (f->spaces().size() != 0)
        {
            tmout << "=== spaces ===" << tendl;
            for (auto j = f->spaces().begin(); j != f->spaces().end(); j++)
            {
                tmout << *j << tendl;
            }
        }

    },
        [](std::unique_ptr<GoogleException> e)
    {
        tmout << "Exception: " << e->what() << tendl;
    });
};

void GdriveCommands::ls_space_content(QString spaceName)
{
    if (spaceName.isEmpty())
    {
        spaceName = "appDataFolder";
    }

    FileListArg arg;
    arg.setSpaces(spaceName);
    m_gd->getFiles()->list_Async(arg)->then([&](std::unique_ptr<googleQt::files::FileResourcesCollection> lst)
    {
        auto& files = lst->files();
        if (files.size() == 0)
        {
            tmout << "Empty space " << spaceName << tendl;
            return;
        }
        int idx = 1;
        for (const files::FileResource& f : files) {
            QString mimeType = f.mimetype();
            QString ftype = "[f]";
            if (mimeType == "application/vnd.google-apps.folder") {
                ftype = "[d]";
            }
            else if (mimeType == "image/jpeg") {
                ftype = "[i]";
            }
            tmout << QString("%1").arg(idx++).leftJustified(3, ' ')
                << ftype << " "
                << f.id() << " "
                << f.name() << " "
                << mimeType << tendl;
        }
        if (!lst->nextpagetoken().isEmpty())
        {
            tmout << "next token: " << lst->nextpagetoken() << tendl;
        }
    },
        [](std::unique_ptr<GoogleException> e)
    {
        tmout << "Exception: " << e->what() << tendl;
    });
};

void GdriveCommands::cat(QString fileId)
{
    if (fileId.isEmpty())
    {
		ard::messageBox(gui::mainWnd(), "fileId required");
        return;
    }

    QByteArray* byteArray = new QByteArray;
    QBuffer* buffer = new QBuffer(byteArray);
    buffer->open(QIODevice::WriteOnly);

    DownloadFileArg arg(fileId);
    m_gd->getFiles()->downloadFile_Async(arg, buffer)->then([&, byteArray, buffer]()
    {
        tmout << "=== file content " << size_human(buffer->size()) << " ===" << tendl;
        tmout << byteArray->constData() << tendl;
        QString hash_s = QCryptographicHash::hash((byteArray->constData()),
            QCryptographicHash::Md5).toHex();
        tmout << "hash: " << hash_s << tendl;
        buffer->close();
        delete buffer;
        delete byteArray;
    },
        [byteArray, buffer](std::unique_ptr<GoogleException> e)
    {
        tmout << "Exception: " << e->what() << tendl;
        delete buffer;
        delete byteArray;
    });
};

void GdriveCommands::rm_appdata_files(QString fileId)
{
    if (fileId.isEmpty())
    {
		ard::messageBox(gui::mainWnd(), "fileId required");
        return;
    }

    if (!ard::confirmBox(ard::mainWnd(), "Please confirm deleting file on cloud in app-folder. This operation can't be undone.")) {
        return;
    }

    gdrive::DeleteFileArg arg(fileId);
    m_gd->getFiles()->deleteOperation_Async(arg)->then([&, fileId]()
    {
        tmout << "deleted: " << fileId << tendl;
    },
        [](std::unique_ptr<GoogleException> e)
    {
        tmout << "Exception: " << e->what() << tendl;
    });
};

void GdriveCommands::download(QString fileId)
{
    if (fileId.isEmpty())
    {
		ard::messageBox(gui::mainWnd(), "fileId required");
        return;
    }

    QString dir_path = QFileDialog::getExistingDirectory(ard::terminal::tbox(), "Open Directory",
        dbp::configFileLastShellAccessDir(),
        QFileDialog::ShowDirsOnly
        | QFileDialog::DontResolveSymlinks);
    if (dir_path.isEmpty())
    {
        return;
    }

    GetFileArg arg1(fileId);
    arg1.setFields("name");

    m_gd->getFiles()->get_Async(arg1)->then([&, fileId, dir_path](std::unique_ptr<googleQt::files::FileResource> f)
    {
        tmout << "id= " << fileId << tendl
            << "name= " << f->name() << tendl;

        QString fileName = f->name();
        tmout << "loading file: " << f->name() << tendl;

        QFile* out = new QFile;
        out->setFileName(dir_path + "/" + f->name());
        if (!out->open(QFile::WriteOnly | QIODevice::Truncate)) {
            tmout << "Error opening file: " << out->fileName() << tendl;
            delete out;
            return;
        }
        
        DownloadFileArg arg(fileId);
        m_gd->getFiles()->downloadFile_Async(arg, out)->then([out]()
        {
            out->flush();
            tmout << "=== file downloaded ===" << tendl;
            tmout << size_human(out->size()) << tendl;
            out->close();
            delete out;
        }, 
            [out](std::unique_ptr<GoogleException> e1)
        {
            tmout << "Exception: " << e1->what() << tendl;
            delete out;
        });

    },
        [](std::unique_ptr<GoogleException> e)
    {
        tmout << "Exception: " << e->what() << tendl;
    });
};

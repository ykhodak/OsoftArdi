#include <QTextEdit>
#include <QLabel>
#include <QHBoxLayout>
#include <QPushButton>
#include <QFileDialog>
#include <QLineEdit>
#include <QFileInfo>
#include <iostream>
#include "importdir.h"
#include "anfolder.h"
#include "ardmodel.h"
#include "anfolder.h"
#include "OutlineMain.h"
#include "locus_folder.h"

static int folders_imported_count = 0;
static int objects_imported_count = 0;

void ImportDir::guiSelectAndImportTextFiles() 
{
    QStringList selectedFiles = QFileDialog::getOpenFileNames(nullptr,
        "Select one or more CSV file",
        dbp::configFileLastShellAccessDir(),
        "Text Files  (*.txt *.text *.log *.h *.cpp *.java);;Source Files  (*.h *.cpp *.java *.go); All files(*.*)");
    if (selectedFiles.size() <= 0) {
        return;
    }

    if (selectedFiles.size() > 0) {
        dbp::configFileSetLastShellAccessDir(selectedFiles[0], true);
        auto first_file = selectedFiles.at(0);
        QFileInfo fi(first_file);
        auto dir_path = fi.absolutePath();

        int total_imported = 0;
        QTextEdit work_edit;
        work_edit.setFontPointSize(model()->fontSize2Points(0));

        auto sb = ard::Sortbox();
        assert_return_void(sb, "expected inbox");
        auto parent4new_obj = new ard::topic(QString("Text imported '%1'").arg(dir_path));
        sb->addItem(parent4new_obj);

        for (auto& s : selectedFiles) {
            qDebug() << "<<<selected-file:" << s;
            QFile file(s);
            if ((file.open(QIODevice::ReadOnly | QIODevice::Text)))
            {
                QFileInfo fi(s);

                QString s_plain = QString(file.readAll()).trimmed();
                work_edit.setPlainText(s_plain);
                QString s_html = work_edit.toHtml().trimmed();
                QString s_plain2 = work_edit.toPlainText().trimmed();
                topic_ptr n = ard::topic::createNewNote(fi.fileName(), s_html, s_plain2);
                parent4new_obj->addItem(n);
                total_imported++;
            }
        }//for

        sb->ensurePersistant(-1);
        gui::rebuildOutline(outline_policy_Pad);
        gui::ensureVisibleInOutline(parent4new_obj);

        QString msg(QString("Imported %1 files(s).").arg(total_imported));
        ard::messageBox(gui::mainWnd(), msg);
    }
};

void ImportDir::importTextFiles()
{
  new ImportDir(false);
};

void ImportDir::importCProject()
{
  new ImportDir(true);
};

ImportDir::ImportDir(bool asCimport) :
    break_pressed(false)
{
    m_C_import = asCimport;

    QPushButton* b = nullptr;

    QLabel* lbl = new QLabel("Please specify root directory path for data import. Press '...' to oped directory selection windows.");

    QLabel* lbl2 = new QLabel("Please specify type of files you want to import. You can use regular expressions to setup files filter.");

    status_info = new QLabel("Press 'Process' to begin..");

    QHBoxLayout *hl_1 = new QHBoxLayout;

    m_dir_path_edit = new QLineEdit(this);
    m_dir_path_edit->setText(QDir::homePath());

    hl_1->addWidget(m_dir_path_edit);
    b = new QPushButton(this);
    b->setText("...");
    connect(b, &QPushButton::released, [=]() 
    {
        selectDir();
    });
    //connect(b, SIGNAL(released()), this, SLOT(selectDir()));
    hl_1->addWidget(b);


    QHBoxLayout *hl2 = new QHBoxLayout;
    b = new QPushButton(this);
    b->setText("Begin import");
    connect(b, &QPushButton::released, [=]() 
    {
        auto import_dir_path = m_dir_path_edit->text();
        auto import_start_topic = new ard::topic(QString("imported '%1'").arg(import_dir_path));
        auto r = ard::Sortbox();
        if (r)
        {
            r->addItem(import_start_topic);
            auto lst = m_filter_edit->toPlainText().split(QRegExp("\\s+"));
            do_import_dir(lst, import_dir_path, import_start_topic);
            r->ensurePersistant(-1);
            gui::rebuildOutline();
            gui::ensureVisibleInOutline(import_start_topic);
            ard::messageBox(this,"Import completed.");
            close();
        }
        //do_import_dir(filter_edit->toPlainText(), dir_path_edit->text(), );
    });
    //connect(b, SIGNAL(released()), this, SLOT(processImport()));
    hl2->addWidget(b);

    b = new QPushButton(this);
    b->setText("Cancel");
    connect(b, SIGNAL(released()), this, SLOT(calcelImport()));
    hl2->addWidget(b);

    m_filter_edit = new QTextEdit;
    if (m_C_import)
    {
        m_filter_edit->setPlainText("*.h *.hpp *cpp *.c Makefile*");
    }
    else
    {
        m_filter_edit->setPlainText("*.txt *.log *.html *.h *cpp");
    }

    textEditFileContent = new QTextEdit(this);
    textEditFileContent->hide();
    textEditFileContent->setFontPointSize(model()->fontSize2Points(0));

    QVBoxLayout *l = new QVBoxLayout;
    l->addWidget(lbl);
    l->addLayout(hl_1);
    l->addWidget(lbl2);
    l->addWidget(m_filter_edit);
    l->addLayout(hl2);
    l->addWidget(status_info);
    //l->addWidget(textEditFileContent);
    setLayout(l);

    //connect(main_wnd(), SIGNAL(progressChanged(int, int)), this, SLOT(progressChanged(int, int))); 

    setWindowModality(Qt::ApplicationModal);
    setAttribute(Qt::WA_DeleteOnClose);
    resize(600, 300);
    m_filter_edit->setFocus();
    show();
};

void ImportDir::do_import_dir(QStringList& type_filter, const QString& dir_path, topic_ptr parent_folder)
{
    if (break_pressed) {
        status_info->setText("canceled " + dir_path);
        status_info->repaint();
        return;
    }

    if (objects_imported_count > 500)
    {
        ard::messageBox(this,QString("Reached limit on files import %1").arg(objects_imported_count));
        break_pressed = true;
        return;
    }

    folders_imported_count++;
    status_info->setText(QString("[%1] scanning " + dir_path).arg(folders_imported_count));
    status_info->repaint();

    QDir d(dir_path);
    QFileInfoList l = d.entryInfoList(type_filter);
    foreach(QFileInfo f, l)
    {
        if (f.isFile())
        {
            QString sname = f.fileName();
            bool bskip = (sname.indexOf("moc_") == 0);
            if (!bskip)
            {
                qDebug() << "f:" << f.absoluteFilePath();

                QFile file(f.absoluteFilePath());
                if ((file.open(QIODevice::ReadOnly | QIODevice::Text)))
                {
                    QString s_plain = QString(file.readAll()).trimmed();
                    textEditFileContent->setPlainText(s_plain);

                    QString s_html = textEditFileContent->toHtml().trimmed();
                    QString s_plain2 = textEditFileContent->toPlainText().trimmed();
                    topic_ptr n = ard::topic::createNewNote(f.fileName(), s_html, s_plain2);
                    parent_folder->addItem(n);
                };
                objects_imported_count++;
            }
        }
    }

    QStringList type_filter_subfolders;
    QFileInfoList sub_folders = d.entryInfoList(type_filter_subfolders, QDir::Dirs | QDir::NoDotAndDotDot);
    foreach(QFileInfo f, sub_folders)
    {
        if (break_pressed)
            return;

        if (f.isDir())
        {

            topic_ptr new_folder = new ard::topic(f.fileName() + "/");

            QString dir_path_2 = f.absoluteFilePath();
            do_import_dir(type_filter, dir_path_2, new_folder);
            parent_folder->addItem(new_folder);
            objects_imported_count++;
        }
    }

}

void ImportDir::calcelImport()
{
  std::cout << "on cancel " << std::endl;
  break_pressed = true;
  close();
};

void ImportDir::selectDir()
{
  QString dir_path = QFileDialog::getExistingDirectory(this, tr("Open Directory"),
      m_dir_path_edit->text(),
      QFileDialog::ShowDirsOnly
      | QFileDialog::DontResolveSymlinks); 
  if(!dir_path.isEmpty())
    {
      dbp::configFileSetLastShellAccessDir(dir_path, false);
      m_dir_path_edit->setText(dir_path);
    }
};

void ImportDir::progressChanged(int total, int processed)
{
  status_info->setText(QString("processing %1 of %2 ").arg(processed).arg(total));
  status_info->repaint();
};

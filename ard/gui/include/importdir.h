#pragma once

#include <QWidget>
#include "anfolder.h"

class QTextEdit;
class QLineEdit;
class QLabel;

class ImportDir : public QWidget
{
    Q_OBJECT
public:
    /// select list of files to import
    static void guiSelectAndImportTextFiles();

    /// select directory for import
    static void importTextFiles();
    /// select directory for import with filter for source files
    static void importCProject();

    public slots:
    void calcelImport();
    void selectDir();
    void progressChanged(int total, int processed);

protected:
    ImportDir(bool asCimport);

    void do_import_dir(QStringList& type_filter, const QString& dir_path, topic_ptr parent_folder);

protected:
    QTextEdit*  m_filter_edit;
    QLineEdit*  m_dir_path_edit;
    QLabel* status_info;
    QTextEdit *textEditFileContent;
    bool   break_pressed;
    bool   m_C_import;
};

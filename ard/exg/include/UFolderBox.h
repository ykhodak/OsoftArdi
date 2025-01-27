#pragma once

#include <QDialog>
#include "locus_folder.h"
#include "custom-widgets.h"
#include "custom-boxes.h"

class QLineEdit;
class QComboBox;
class QLabel;
class ColorComboBox;
class QCheckBox;
class QRadioButton;

/**
    edit locused folder properties 
*/
class LocusBox : public QDialog
{
    friend bool ard::guiEditUFolder(topic_ptr f, QString boxHeader);

    Q_OBJECT
public:
    static bool addFolder();
    
protected:
    static bool editFolder(topic_ptr f, QString boxHeader = "");

    LocusBox(topic_ptr f, QString boxHeader);

public slots:
    void processOK();
    void processCancel();
    void defIndexChanged(int);
    

protected:
    bool    createNewFolder();

protected:
    topic_ptr       m_folder{ nullptr };
    QCheckBox*      m_chkShowInTab{ nullptr };
    QLineEdit*      m_name;
    QComboBox*      m_combo_def{ nullptr };
   // ColorButtonPanel* m_cbox{ nullptr };
    bool            m_accepted{ false };
};

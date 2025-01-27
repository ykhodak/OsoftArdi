#pragma once

#include <QDialog>
#include "anfolder.h"

class FlowLayout;
class QPlainTextEdit;
class QCheckBox;
class QButtonGroup;
class QComboBox;
//class anLocusFolder;

/**
   search in gmail messages
*/
class EmailSearchBox : public QDialog
{
    Q_OBJECT
public:
    static std::pair<bool, QString> edit_qstr(QString qstr);

protected:
    EmailSearchBox(QString search4);
    void    addSearchOpt(QString s);
    void    resizeEvent(QResizeEvent *e);
protected:
    QString             m_result_qstr;
    bool                m_accepted{false};
    FlowLayout*         m_layout{nullptr};
    QPlainTextEdit*     m_edit;
    QCheckBox*          m_or_check;
    QWidget*            m_customf_space{nullptr};
};

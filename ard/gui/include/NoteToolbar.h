#pragma once

#include "utils.h"

class QAction;
class QComboBox;
class QFontComboBox;
class NoteFrameWidget;
class QTextCharFormat;
class QMenu;
class NoteBarColorButtons;

#ifdef ARD_BIG

namespace ard 
{
    class TopicWidget;
};

class NoteTextToolbar: public QObject
{
    friend class NoteFrameWidget;
    Q_OBJECT;
public:
    NoteTextToolbar(QWidget * parent, ard::TopicWidget* tp);

    void showToolbar(bool val);

    QToolBar* editToolbar(){return m_EditToolbar;}
    //QToolBar* formatToolbar(){return m_FormatToolbar;}

    void attachEditor(NoteFrameWidget* e);

protected:
    NoteFrameWidget* m_editor;
    QToolBar*        m_EditToolbar;
    QComboBox      *comboStyle;
    QFontComboBox  *comboFont;
    QComboBox      *comboSize;

    QAction
    *actionUndo,
        *actionRedo,
        *actionCut,
        *actionCopy,
        *actionPaste,
        *actionPlainTextPaste,
        *actionTextBold,
        *actionTextUnderline,
        *actionTextItalic,
        *actionAlignLeft,
        *actionAlignCenter,
        *actionAlignRight,
        *actionAlignJustify,
        *actionTextColor,
        *actionTable,
        *actionSpeak,
        *actionFind,
        *actionSelectAll;
    ACTION_LIST      m_actions;
    QActionGroup   *grpAlignGroup;
    NoteBarColorButtons* m_bkgrColorButton{nullptr};

    ard::TopicWidget*   m_twidget{nullptr};
};

#endif //ARD_BIG

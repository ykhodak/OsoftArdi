#include <QToolBar>
#include <QApplication>
#include <QComboBox>
#include <QFontComboBox>
#include <QColorDialog>
#include <QMenu>
#include <QFileDialog>

#include "ardmodel.h"
#include "NoteToolbar.h"
#include "NoteFrameWidget.h"
#include "NoteEdit.h"
#include "PopupCard.h"
#include "extnote.h"

#ifdef ARD_BIG

#define CHECK_TBAR if(!m_tb){return;}

/**
   NoteTextToolbar
*/
NoteTextToolbar::NoteTextToolbar(QWidget * parent, ard::TopicWidget* tp)
    :QObject(parent), m_twidget(tp)
{
    actionUndo=actionRedo=actionCut=actionCopy=actionPaste = actionPlainTextPaste = nullptr;
    actionTextBold=actionTextUnderline=actionTextItalic=actionAlignLeft = nullptr;
    actionAlignCenter=actionAlignRight=actionAlignJustify=actionTextColor = nullptr;
    actionTable=actionFind = actionSelectAll = nullptr;
    grpAlignGroup = nullptr;

    QToolBar* tb = nullptr;
    QAction* a = nullptr;

    m_EditToolbar = new QToolBar(parent);
    m_EditToolbar->setWindowTitle("Edit Actions");
    tb = m_EditToolbar;

    a = actionPlainTextPaste = utils::actionFromTheme("edit-paste-plain", tr("&Paste as Plain Text"), this);
    //a->setShortcut(QKeySequence::Paste);
    ADD_TAB_CTRL_ACTION;
    tb->addSeparator();

    //cut copy paste
    if (!actionCopy)
    {
        if (is_big_screen())
        {
            a = actionCut = utils::actionFromTheme("edit-cut", tr("Cu&t"), this);
            a->setShortcut(QKeySequence::Cut);
            ADD_TAB_CTRL_ACTION;
        }

        a = actionCopy = utils::actionFromTheme("edit-copy", tr("&Copy"), this);
        a->setShortcut(QKeySequence::Copy);
        ADD_TAB_CTRL_ACTION;

        a = actionPaste = utils::actionFromTheme("edit-paste", tr("&Paste"), this);
        a->setShortcut(QKeySequence::Paste);
        ADD_TAB_CTRL_ACTION;
        tb->addSeparator();
    }

    if (!actionTextBold)
    {
        a = actionTextBold = utils::actionFromTheme("format-text-bold", tr("&Bold"), this);
        actionTextBold->setCheckable(true);
        actionTextBold->setShortcut(Qt::CTRL + Qt::Key_B);
        QFont bold;
        bold.setBold(true);
        actionTextBold->setFont(bold);
        ADD_TAB_CTRL_ACTION;
    }

    if (is_big_screen())
    {
        if (!actionTextItalic)
        {
            a = actionTextItalic = utils::actionFromTheme("format-text-italic", tr("&Italic"), this);
            actionTextItalic->setCheckable(true);
            actionTextItalic->setShortcut(Qt::CTRL + Qt::Key_I);
            QFont italic;
            italic.setItalic(true);
            actionTextItalic->setFont(italic);
            ADD_TAB_CTRL_ACTION;
        }

        //underline
        if (!actionTextUnderline)
        {
            a = actionTextUnderline = utils::actionFromTheme("format-text-underline", tr("&Underline"), this);
            actionTextUnderline->setCheckable(true);
            actionTextUnderline->setShortcut(Qt::CTRL + Qt::Key_U);
            QFont underline;
            underline.setUnderline(true);
            actionTextUnderline->setFont(underline);
            ADD_TAB_CTRL_ACTION;
        }
    }

    //align
    if (is_big_screen())
    {
        if (!grpAlignGroup)
        {
            QActionGroup *grp = grpAlignGroup = new QActionGroup(this);
            if (QApplication::isLeftToRight()) {
                actionAlignLeft = utils::actionFromTheme("format-justify-left", tr("&Left"), this);
                actionAlignCenter = utils::actionFromTheme("format-justify-center", tr("C&enter"), this);
                actionAlignRight = utils::actionFromTheme("format-justify-right", tr("&Right"), this);
            }
            else {

                actionAlignRight = utils::actionFromTheme("format-justify-right", tr("&Right"), this);
                actionAlignCenter = utils::actionFromTheme("format-justify-center", tr("C&enter"), this);
                actionAlignLeft = utils::actionFromTheme("format-justify-left", tr("&Left"), this);
            }
            actionAlignJustify = utils::actionFromTheme("format-justify-fill", tr("&Justify"), this);

            actionAlignLeft->setShortcut(Qt::CTRL + Qt::Key_L);
            actionAlignLeft->setCheckable(true);
            actionAlignCenter->setShortcut(Qt::CTRL + Qt::Key_E);
            actionAlignCenter->setCheckable(true);
            actionAlignRight->setShortcut(Qt::CTRL + Qt::Key_R);
            actionAlignRight->setCheckable(true);
            actionAlignJustify->setShortcut(Qt::CTRL + Qt::Key_J);
            actionAlignJustify->setCheckable(true);

            ADD_TAB_CTRL_GROUP_ACTION(actionAlignLeft);
            ADD_TAB_CTRL_GROUP_ACTION(actionAlignRight);
            ADD_TAB_CTRL_GROUP_ACTION(actionAlignCenter);
            ADD_TAB_CTRL_GROUP_ACTION(actionAlignJustify);
        }

        if (!actionTextColor)
        {
            QPixmap pix(16, 16);
            pix.fill(Qt::black);
            a = actionTextColor = new QAction(pix, tr("&Color..."), this);
            ADD_TAB_CTRL_ACTION;
        }

        auto bh = tb->iconSize().height();
        m_bkgrColorButton = new NoteBarColorButtons(bh);
        tb->addWidget(m_bkgrColorButton);
    }

    comboStyle = new QComboBox(tb);
    tb->addWidget(comboStyle);
    comboStyle->addItem("Standard");
    comboStyle->addItem("Bullet List (Disc)");
    comboStyle->addItem("Bullet List (Circle)");
    comboStyle->addItem("Bullet List (Square)");
    comboStyle->addItem("Ordered List (Decimal)");
    comboStyle->addItem("Ordered List (Alpha lower)");
    comboStyle->addItem("Ordered List (Alpha upper)");
    comboStyle->addItem("Ordered List (Roman lower)");
    comboStyle->addItem("Ordered List (Roman upper)");

    comboFont = new QFontComboBox(tb);
    tb->addWidget(comboFont);


    comboSize = new QComboBox(tb);
    comboSize->setObjectName("comboSize");
    tb->addWidget(comboSize);
    comboSize->setEditable(true);
    QFontDatabase db;
    foreach(int size, db.standardSizes())
        comboSize->addItem(QString::number(size));
    //comboSize->setCurrentIndex(comboSize->findText(QString::number(QApplication::font().pointSize()))); 



    if(!actionUndo)
        {
            a = actionUndo = utils::actionFromTheme("edit-undo", tr("&Undo"), this);
            actionUndo->setShortcut(QKeySequence::Undo);
            ADD_TAB_CTRL_ACTION;
        }

    if(!actionRedo)
        {  
            a = actionRedo = utils::actionFromTheme("edit-redo", tr("&Redo"), this);
            a->setShortcut(QKeySequence::Redo);
            ADD_TAB_CTRL_ACTION;
        }

    if(is_big_screen())
        {  
            if(!actionTable)
                {
                    a = actionTable = utils::actionFromTheme("add-table", tr("&Table"), this);
                    ADD_TAB_CTRL_ACTION;
                }

            if (!actionFind) {
                //a = actionFind = utils::actionFromTheme("edit-find", tr("&Find"), this);
				a = actionFind = utils::actionFromTheme("find", tr("&Find"), this);
                a->setShortcut(Qt::ALT + Qt::Key_F);
                ADD_TAB_CTRL_ACTION;
            }
        }
  
    if (!actionSelectAll) {
        a = actionSelectAll = utils::actionFromTheme("hud-br-down", tr("Select &All"), this);
        a->setShortcut(QKeySequence::SelectAll);
        ADD_TAB_CTRL_ACTION;
    }
};

void NoteTextToolbar::showToolbar(bool val) 
{
    if (m_EditToolbar) {
        m_EditToolbar->setVisible(val);
    }
    /*if (m_FormatToolbar) {
        m_FormatToolbar->setVisible(val);
    }*/
};

void NoteTextToolbar::attachEditor(NoteFrameWidget* ac)
{
    m_editor = ac;

    ASSERT(ac, "expected NoteFrameWidget");
  

    auto ne = m_editor->editor();
  
    if(actionUndo)
        {  
            connect(ne->document(), SIGNAL(undoAvailable(bool)),
                    actionUndo, SLOT(setEnabled(bool)));  
            actionUndo->setEnabled(ne->document()->isUndoAvailable());
            connect(actionUndo, SIGNAL(triggered()), ne, SLOT(undo()));
        }

    if(actionRedo)
        {    
            connect(ne->document(), SIGNAL(redoAvailable(bool)),
                    actionRedo, SLOT(setEnabled(bool)));
            actionRedo->setEnabled(ne->document()->isRedoAvailable());
            connect(actionRedo, SIGNAL(triggered()), ne, SLOT(redo()));
        }

    if(actionCut)
        {
            actionCut->setEnabled(false);
            connect(actionCut, SIGNAL(triggered()), ne, SLOT(cut()));
            connect(ne, SIGNAL(copyAvailable(bool)), 
                    actionCut, SLOT(setEnabled(bool)));      
        }
  
    if(actionCopy)
        {  
            actionCopy->setEnabled(false);
            connect(actionCopy, SIGNAL(triggered()), ne, SLOT(copy()));
            connect(actionPaste, SIGNAL(triggered()), ne, SLOT(paste()));
            connect(ne, SIGNAL(copyAvailable(bool)), 
                    actionCopy, SLOT(setEnabled(bool)));

            connect(actionPlainTextPaste, &QAction::triggered, ne, &WorkingNoteEdit::pastePlainText, Qt::UniqueConnection);

            /*
#ifndef QT_NO_CLIPBOARD
            if (const QMimeData *md = QApplication::clipboard()->mimeData())
                actionPaste->setEnabled(md->hasText());
#endif
*/
        }
  
    //  if(actionTitle)connect(actionTitle, SIGNAL(triggered()), editor(), SLOT(titleEdit()));

    //bold italic
    if(actionTextBold)connect(actionTextBold, SIGNAL(triggered()), m_editor, SLOT(textBold()));
    if(actionTextItalic)connect(actionTextItalic, SIGNAL(triggered()), m_editor, SLOT(textItalic()));
    if(actionTextUnderline)connect(actionTextUnderline, SIGNAL(triggered()), m_editor, SLOT(textUnderline()));

    //align
    if(grpAlignGroup)connect(grpAlignGroup, SIGNAL(triggered(QAction*)), m_editor, SLOT(textAlign(QAction*)));

    //color
    if(actionTextColor)connect(actionTextColor, &QAction::triggered, m_editor, &NoteFrameWidget::textColor, Qt::UniqueConnection);
    //if(actionTextColor)connect(actionTextColor, SIGNAL(triggered()), m_editor, SLOT(textColor()));

    if(actionTable)connect(actionTable, SIGNAL(triggered()), m_editor, SLOT(toggleTable()));

    if (actionFind)connect(actionFind, &QAction::triggered, m_editor, &NoteFrameWidget::openSearchWindow, Qt::UniqueConnection);
    if(actionSelectAll)connect(actionSelectAll, &QAction::triggered, m_editor, &NoteFrameWidget::selectAll, Qt::UniqueConnection);


    //format
    connect(comboStyle, SIGNAL(activated(int)),
        m_editor, SLOT(textStyle(int)));
    connect(comboFont, SIGNAL(activated(QString)),
        m_editor, SLOT(textFamily(QString)));
    connect(comboSize, SIGNAL(activated(QString)),
        m_editor, SLOT(textSize(QString)));

    if (m_bkgrColorButton) {
        connect(m_bkgrColorButton, &NoteBarColorButtons::onColorSelected, m_editor, &NoteFrameWidget::bkgColor, Qt::UniqueConnection);
        connect(m_bkgrColorButton, &NoteBarColorButtons::onColorSelectRequested, m_editor, &NoteFrameWidget::bkgColorSelect, Qt::UniqueConnection);
    }

    comboFont->setCurrentIndex(comboFont->findText(dbp::configFileNoteFontFamily()));
    comboSize->setCurrentIndex(comboSize->findText(QString::number(dbp::configFileNoteFontSize())));
};

void NoteFrameWidget::showToolbar(bool val)
{
#ifdef ARD_BIG
    if (m_tb) {
        m_tb->showToolbar(val);
    }
#else
    Q_UNUSED(val);
#endif
};

void NoteFrameWidget::textBold()
{
    CHECK_TBAR;
    if (m_tb->actionTextBold)
    {
        QTextCharFormat fmt;
        fmt.setFontWeight(m_tb->actionTextBold->isChecked() ? QFont::Bold : QFont::Normal);
        mergeFormatOnWordOrSelection(fmt);
    }
}


void NoteFrameWidget::textUnderline()
{
    CHECK_TBAR;
    if (m_tb->actionTextUnderline)
    {
        QTextCharFormat fmt;
        fmt.setFontUnderline(m_tb->actionTextUnderline->isChecked());
        mergeFormatOnWordOrSelection(fmt);
    }
}

void NoteFrameWidget::textItalic()
{
    CHECK_TBAR;
    if (m_tb->actionTextItalic)
    {
        QTextCharFormat fmt;
        fmt.setFontItalic(m_tb->actionTextItalic->isChecked());
        mergeFormatOnWordOrSelection(fmt);
    }
}

void NoteFrameWidget::textFamily(const QString &f)
{
#ifdef ARD_BIG
    QTextCharFormat fmt;
    fmt.setFontFamily(f);
    mergeFormatOnWordOrSelection(fmt);
#else
    Q_UNUSED(f);
#endif
}

void NoteFrameWidget::textSize(const QString &p)
{
    qreal pointSize = p.toFloat();
    if (p.toFloat() > 0) {
        QTextCharFormat fmt;
        fmt.setFontPointSize(pointSize);
        mergeFormatOnWordOrSelection(fmt);
    }
}

void NoteFrameWidget::textStyle(int styleIndex)
{
    //auto ne = m_editor->editor();
    QTextCursor cursor = m_text_edit1->textCursor();

    if (styleIndex != 0) {
        QTextListFormat::Style style = QTextListFormat::ListDisc;

        switch (styleIndex) {
        default:
        case 1:
            style = QTextListFormat::ListDisc;
            break;
        case 2:
            style = QTextListFormat::ListCircle;
            break;
        case 3:
            style = QTextListFormat::ListSquare;
            break;
        case 4:
            style = QTextListFormat::ListDecimal;
            break;
        case 5:
            style = QTextListFormat::ListLowerAlpha;
            break;
        case 6:
            style = QTextListFormat::ListUpperAlpha;
            break;
        case 7:
            style = QTextListFormat::ListLowerRoman;
            break;
        case 8:
            style = QTextListFormat::ListUpperRoman;
            break;
        }

        cursor.beginEditBlock();

        QTextBlockFormat blockFmt = cursor.blockFormat();

        QTextListFormat listFmt;

        if (cursor.currentList()) {
            listFmt = cursor.currentList()->format();
        }
        else {
            listFmt.setIndent(blockFmt.indent() + 1);
            blockFmt.setIndent(0);
            cursor.setBlockFormat(blockFmt);
        }

        listFmt.setStyle(style);

        cursor.createList(listFmt);

        cursor.endEditBlock();
    }
    else {
        // ####
        QTextBlockFormat bfmt;
        bfmt.setObjectIndex(-1);
        cursor.mergeBlockFormat(bfmt);
    }
}

void NoteFrameWidget::textColor()
{
    QColor col = QColorDialog::getColor(m_text_edit1->textColor(), this);
    if (!col.isValid())
        return;
    QTextCharFormat fmt;
    fmt.setForeground(col);
    mergeFormatOnWordOrSelection(fmt);
}

void NoteFrameWidget::bkgColor(const QColor& cl) 
{
    if (cl.isValid()) {
        QTextCharFormat fmt;
        fmt.setBackground(cl);
        mergeFormatOnWordOrSelection(fmt);
    }
};

void NoteFrameWidget::bkgColorSelect() 
{
    QColor col = QColorDialog::getColor();// (m_text_edit1->textColor(), this);
    if (!col.isValid())
        return;
    QTextCharFormat fmt;
    fmt.setBackground(col);
    mergeFormatOnWordOrSelection(fmt);
};

void NoteFrameWidget::toggleTable()
{
    QMenu m(this);
    QAction *a = nullptr;
    QPoint ptMenu = gui::lastMouseClick();

    QTextCursor tc = m_text_edit1->textCursor();
    QTextTable* tbl = tc.currentTable();
    if (!tbl)
    {
        a = new QAction("Insert Table", this);
        m.addAction(a);
        connect(a, SIGNAL(triggered()), this,
            SLOT(toggleInsertTable()));
    }
    else
    {
        a = new QAction("Insert Row", this);
        m.addAction(a);
        connect(a, SIGNAL(triggered()), this,
            SLOT(toggleInsertTableRow()));

        a = new QAction("Insert Column", this);
        m.addAction(a);
        connect(a, SIGNAL(triggered()), this,
            SLOT(toggleInsertTableColumn()));

        m.addSeparator();

        if (tbl->rows() > 1)
        {
            a = new QAction("Remove Row", this);
            m.addAction(a);
            connect(a, SIGNAL(triggered()), this,
                SLOT(toggleRemoveTableRow()));
        }

        if (tbl->columns() > 1)
        {
            a = new QAction("Remove Column", this);
            m.addAction(a);
            connect(a, SIGNAL(triggered()), this,
                SLOT(toggleRemoveTableColumn()));
        }

        m.addSeparator();

        a = new QAction("Insert Sub Table", this);
        m.addAction(a);
        connect(a, SIGNAL(triggered()), this,
            SLOT(toggleInsertTable()));
    }
    m.exec(ptMenu);
};

void NoteFrameWidget::toggleInsertTable()
{
    QTextTableFormat tf;
    tf.setBorderStyle(QTextFrameFormat::BorderStyle_Solid);
    tf.setCellPadding(0);
    tf.setCellSpacing(0);
    tf.setWidth(QTextLength(QTextLength::PercentageLength, 100));
    tf.setAlignment(Qt::AlignLeft);

    QTextCursor tc = m_text_edit1->textCursor();
    tc.insertTable(4, 2, tf);
};

void NoteFrameWidget::toggleInsertTableRow()
{
    QTextCursor tc = m_text_edit1->textCursor();
    QTextTable* tbl = tc.currentTable();
    assert_return_void(tbl, "expected current table");

    QTextTableCell tcell = tbl->cellAt(tc);
    if (tcell.isValid())
    {
        int rindex = tcell.row();
        tbl->insertRows(rindex + 1, 1);
    }
};

void NoteFrameWidget::toggleInsertTableColumn()
{
    QTextCursor tc = m_text_edit1->textCursor();
    QTextTable* tbl = tc.currentTable();
    assert_return_void(tbl, "expected current table");
    QTextTableCell tcell = tbl->cellAt(tc);
    if (tcell.isValid())
    {
        int cindex = tcell.column();
        tbl->insertColumns(cindex + 1, 1);
    }
};

void NoteFrameWidget::toggleRemoveTableRow()
{
    QTextCursor tc = m_text_edit1->textCursor();
    QTextTable* tbl = tc.currentTable();
    assert_return_void(tbl, "expected current table");
    QTextTableCell tcell = tbl->cellAt(tc);
    if (tcell.isValid())
    {
        int rindex = tcell.row();
        tbl->removeRows(rindex, 1);
    }
};

void NoteFrameWidget::toggleRemoveTableColumn()
{
    QTextCursor tc = m_text_edit1->textCursor();
    QTextTable* tbl = tc.currentTable();
    assert_return_void(tbl, "expected current table");
    QTextTableCell tcell = tbl->cellAt(tc);
    if (tcell.isValid())
    {
        int cindex = tcell.column();
        tbl->removeColumns(cindex, 1);
    }
};


void NoteFrameWidget::textAlign(QAction * a)
{
    CHECK_TBAR;
    if (m_tb->grpAlignGroup)
    {
        if (a == m_tb->actionAlignLeft)
            m_text_edit1->setAlignment(Qt::AlignLeft | Qt::AlignAbsolute);
        else if (a == m_tb->actionAlignCenter)
            m_text_edit1->setAlignment(Qt::AlignHCenter);
        else if (a == m_tb->actionAlignRight)
            m_text_edit1->setAlignment(Qt::AlignRight | Qt::AlignAbsolute);
        else if (a == m_tb->actionAlignJustify)
            m_text_edit1->setAlignment(Qt::AlignJustify);
    }
}

void NoteFrameWidget::fontChanged(const QFont &f)
{
    CHECK_TBAR;
    if (m_tb->comboFont)
    {
        m_tb->comboFont->setCurrentIndex(m_tb->comboFont->findText(QFontInfo(f).family()));
        m_tb->comboSize->setCurrentIndex(m_tb->comboSize->findText(QString::number(f.pointSize())));
    }

#define CHECK_ACT(A, V)if(A)A->setChecked(V);
    CHECK_ACT(m_tb->actionTextBold, f.bold());
    CHECK_ACT(m_tb->actionTextItalic, f.italic());
    CHECK_ACT(m_tb->actionTextUnderline, f.underline());
#undef CHECK_ACT
}

void NoteFrameWidget::colorChanged(const QColor &fgColor, const QColor &bgColor)
{
    CHECK_TBAR;
    if (m_tb->actionTextColor)
    {
        QPixmap pix(16, 16);
        pix.fill(fgColor);
        m_tb->actionTextColor->setIcon(pix);
    }

    if (m_tb->m_bkgrColorButton) 
    {
        m_tb->m_bkgrColorButton->selectColor(bgColor);
    }
}

void NoteFrameWidget::alignmentChanged(Qt::Alignment a)
{
    CHECK_TBAR;
    if (m_tb->grpAlignGroup)
    {
        if (a & Qt::AlignLeft) {
            m_tb->actionAlignLeft->setChecked(true);
        }
        else if (a & Qt::AlignHCenter) {
            m_tb->actionAlignCenter->setChecked(true);
        }
        else if (a & Qt::AlignRight) {
            m_tb->actionAlignRight->setChecked(true);
        }
        else if (a & Qt::AlignJustify) {
            m_tb->actionAlignJustify->setChecked(true);
        }
    }
}

void NoteFrameWidget::currentCharFormatChanged(const QTextCharFormat &format)
{
#ifdef ARD_BIG
    fontChanged(format.font());
    colorChanged(format.foreground().color(), format.background().color());
#else
    Q_UNUSED(format);
#endif
}

void NoteFrameWidget::mergeFormatOnWordOrSelection(const QTextCharFormat &format)
{
    m_text_edit1->mergeFormatOnWordOrSelection(format);
}

void NoteFrameWidget::splitIntoTopics()
{
    m_text_edit1->saveIfModified();
    auto c = m_text_edit1->comment();
    assert_return_void(c, "expected comment");
    topic_ptr it = c->owner();
    assert_return_void(it, "expected owner");
    auto p = it->parent();
    assert_return_void(p, "expected parent");
    int idx = p->indexOf(it);
    assert_return_void(idx != -1, "expected index");

    topic_ptr sp = new ard::topic("SPLIT:" + it->title());

    QString s = c->plain_text();
    QStringList l = s.split("\n", QString::SkipEmptyParts);
    for (QStringList::iterator i = l.begin(); i != l.end(); i++)
    {
        QString s2 = *i;
        topic_ptr t = new ard::topic(s2);
        sp->addItem(t);
    }
    sp->setExpanded(true);

    p->insertItem(sp, idx + 1);
    p->ensurePersistant(-1);
    gui::rebuildOutline(outline_policy_Pad);
};

#endif //ARD_BIG

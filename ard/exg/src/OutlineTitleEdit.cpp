#include <QKeyEvent>

#include <QScroller>
#include <QScrollBar>
#include <QFormLayout>
#include <QLabel>
#include <QApplication>

#include "anfolder.h"
#include "OutlineTitleEdit.h"
#include "custom-widgets.h"

/**
    FormLineEdit - edit with some hint drawing
*/
class FormLineEdit : public QTextEdit 
{
    friend class OutlineTitleEdit;
public:
    FormLineEdit(OutlineTitleEdit* e) :QTextEdit(e), m_e(e)
    {
        setStyleSheet("QTextEdit {border: 1px solid rgb(255,255,192);}");
    }

    void paintEvent(QPaintEvent * event)override
    {
        QTextEdit::paintEvent(event);
        auto doc = document();
        QWidget* v = viewport();
        if (doc && v) 
        {
            if (m_field_part.first == FieldParts::EType::Annotation)
            {
                QPainter p(v);
                QRect rc_ico = v->rect();
                int ico_w = gui::lineHeight();
                rc_ico.setLeft(rc_ico.right() - ico_w);
                rc_ico.setBottom(rc_ico.top() + ico_w);
                p.setOpacity(0.7);
                p.drawPixmap(rc_ico, getIcon_PinBtn());
            }
            else if (doc->isEmpty())
            {
                QPainter p(v);
                QPen pn(Qt::gray);
                p.setPen(pn);
                p.setFont(*ard::defaultFont());
                QString s = QString("<%1>").arg(FieldParts::type2label(m_field_part.first));
                p.drawText(v->geometry(), Qt::AlignLeft | Qt::AlignVCenter, s);
            }
        }
    }

    void setField(const std::pair<FieldParts::EType, QString>& f)
    {
        m_field_part = f;
        setPlainText(f.second);
        document()->setModified(false);
        if (m_field_part.first == FieldParts::EType::Annotation) {
            setStyleSheet("QTextEdit {border: 1px solid rgb(255,255,192);}");
        }
        else
        {
            setStyleSheet("QTextEdit {border: 1px solid rgb(49,106,197);}");
        }
    }

    void mousePressEvent(QMouseEvent *e)override
    {
        if (m_field_part.first == FieldParts::EType::Annotation)
        {
            auto pt = e->pos();
            QRect rc_ico = rect();
            int ico_w = gui::lineHeight();
            rc_ico.setLeft(rc_ico.right() - ico_w);
            rc_ico.setBottom(rc_ico.top() + ico_w);
            if (rc_ico.contains(pt)) 
            {
                auto f = m_e->topic();
                if (f) {
                    ard::popup_annotation(f);
                    m_e->detachEditorTopic();
                    m_e->hide();
                    m_e->parent_view()->resetGItemsGeometry();                  
                    return;
                }
            }
        }

        QTextEdit::mousePressEvent(e);
    }

    void keyPressEvent(QKeyEvent *e)
    {
        switch (e->key())
        {
        case Qt::Key_D: {
            if (QApplication::keyboardModifiers() & Qt::AltModifier) {
                auto str = dbp::currentDateStamp() + " ";
                insertPlainText(str);
                return;
            }
        }break;
        case Qt::Key_T: {
            if (QApplication::keyboardModifiers() & Qt::AltModifier) {
                auto str = dbp::currentTimeStamp() + " ";
                insertPlainText(str);
                return;
            }
        }break;
        }

        QTextEdit::keyPressEvent(e);
    };

    std::pair<FieldParts::EType, QString> m_field_part;
    OutlineTitleEdit* m_e{nullptr};
};

/**
    OutlineTitleEdit
*/
OutlineTitleEdit::OutlineTitleEdit(ArdGraphicsView *parent) :
    QWidget(parent), m_topic(nullptr)
{
    m_parent_view = parent;

    m_main_layout = new QFormLayout();
    setLayout(m_main_layout);
    gui::setupBoxLayout(m_main_layout);

    m_main_layout->setRowWrapPolicy(QFormLayout::DontWrapRows);
    m_main_layout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    m_main_layout->setFormAlignment(Qt::AlignLeft | Qt::AlignTop);
    m_main_layout->setLabelAlignment(Qt::AlignLeft);
    
    std::function<void(void)> addFildEditor = [&]() {
        QLabel* lbl = new QLabel(this);
        lbl->setFont(*ard::defaultFont());
        FormLineEdit* e = new FormLineEdit(this);

        e->setAcceptRichText(false);
        e->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        e->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        e->setFont(*ard::defaultFont());
        e->installEventFilter(this);
        
        ///setVerticalStretch is a must here, otherwise edit won't grow tall enough
        QSizePolicy szp = e->sizePolicy();
        szp.setVerticalPolicy(QSizePolicy::MinimumExpanding);
        szp.setVerticalStretch(1);
        e->setSizePolicy(szp);
        
        m_labels.push_back(lbl);
        m_editors.push_back(e);
    };

    for (int i = 0; i < 5; i++) {
        addFildEditor();
    }

//    QPalette pal = palette();
    setAutoFillBackground(true);
}

OutlineTitleEdit::~OutlineTitleEdit()
{
    detachEditorTopic(false);
}

bool OutlineTitleEdit::processKeyPressEvent(QKeyEvent * ev, QTextEdit* e)
{
    switch (ev->key()) 
    {
        case Qt::Key_Return:
        {
            detachEditorTopic(true);
            hide();
            m_parent_view->resetGItemsGeometry();
            parentWidget()->setFocus();
        }return true;

        case Qt::Key_Escape:
        {
            detachEditorTopic(false);
            hide();
            parentWidget()->setFocus();
        }return true;
        case Qt::Key_Tab: 
        {
            auto fnum = countVisibleFieldsNumber();
            if (fnum > 1) {
                size_t idx = 0;
                auto e = editorInFocus(&idx);
                if (e) {
                    if (idx + 1 >= fnum) {
                        idx = 0;
                    }
                    else {
                        idx++;
                    }
                    assert_return_false(idx < m_editors.size(), "invalid editor index");
                    e = m_editors[idx];
                    if (e->isEnabled()) {
                        e->setFocus();
                    }
                }
            }
        }return true;
    case Qt::Key_Down:
        {
            auto fnum = countVisibleFieldsNumber();
            if (fnum == 1) {
                //..
                FormLineEdit* fe = dynamic_cast<FormLineEdit*>(e);
                if (fe && m_topic) {
                    if (fe->m_field_part.first == FieldParts::EType::Annotation) {
                        detachEditorTopic(true);
                        hide();
                        gui::renameSelected();
                        return true;
                    }
                    else if (fe->m_field_part.first == FieldParts::EType::Title) {
                        detachEditorTopic(true);
                        hide();
                        auto f = gui::selectNext(false);
                        if (f) {
                            auto s_ann = f->annotation();
                            if (!s_ann.isEmpty()) {
                                gui::renameSelected(EColumnType::Annotation);
                            }
                            else {
                                gui::renameSelected();
                            }
                            return true;
                        }
                    }
                }
                //..

                detachEditorTopic(true);
                hide();
                gui::selectNext(false);
                gui::renameSelected();
                return true;
            }
            else{
                return false;
            }
        }break;
    case Qt::Key_Up:
        {
            auto fnum = countVisibleFieldsNumber();
            if (fnum == 1) {
                FormLineEdit* fe = dynamic_cast<FormLineEdit*>(e);
                if (fe && m_topic) {
                    if (fe->m_field_part.first == FieldParts::EType::Title) {
                        auto s_ann = m_topic->annotation();
                        if (!s_ann.isEmpty()) {
                            detachEditorTopic(true);
                            hide();
                            gui::renameSelected(EColumnType::Annotation);
                            return true;
                        }
                    }
                }
                detachEditorTopic(true);
                hide();
                gui::selectNext(true);
                gui::renameSelected();
                return true;
            }
            else{
                return false;
            }            
        }break;
    }

    /// autosize editor if text becomes bigger
    if (!m_has_many_editors) {
        QSizeF sz = e->document()->size();
        QRect our_geometry = geometry();

        if (sz.height() > our_geometry.height()) {
            QRect parent_geometry = m_parent_view->geometry();

            int h = our_geometry.height();
            h += gui::lineHeight();
            our_geometry.setHeight(h);
            if (our_geometry.height() < parent_geometry.height()) {
                setGeometry(our_geometry);
                m_parent_view->updateTitleEditFramePos();
            }
        }
    }

    return false;
};

void OutlineTitleEdit::processFocusOutEvent() 
{
    bool hasChildHasFocus = (editorInFocus() != nullptr);
    if (!hasChildHasFocus) {
        if (m_topic) {
            detachEditorTopic(true);
            m_parent_view->resetGItemsGeometry();
            hide();
        }
    }
};

bool OutlineTitleEdit::eventFilter(QObject *obj, QEvent *ev) 
{
    if (ev->type() == QEvent::FocusOut) {
        bool rv = QWidget::eventFilter(obj, ev);
        processFocusOutEvent();
        return rv;
    }
    else if (ev->type() == QEvent::KeyPress) {
        //qDebug() << rect() << "eventFilter" << obj;

        QTextEdit* te = dynamic_cast<QTextEdit*>(obj);
        if (!te) {
            ASSERT(0, "expected QTextEdit obj");
            return QWidget::eventFilter(obj, ev);
        }

        //bool rv = QWidget::eventFilter(obj, ev);
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(ev);
        if (m_own_even_filter_for_editors && processKeyPressEvent(keyEvent, te)) {
            return true;
        }
        else {
            return QWidget::eventFilter(obj, ev);
        }
        //return rv;
    }

    return QWidget::eventFilter(obj, ev);
};

void OutlineTitleEdit::hideFormControls() 
{
    hideWidgets<QLabel>(m_labels);
    hideWidgets<FormLineEdit>(m_editors);
};

QTextEdit* OutlineTitleEdit::editorInFocus(size_t* idxOfEditor /*= nullptr*/)
{
    int idx = 0;
    for (auto w : m_editors) {
        if (w->hasFocus()) {
            if (idxOfEditor) {
                *idxOfEditor = idx;
            }
            return w;
        }
        idx++;
    }
    return nullptr;
};

size_t OutlineTitleEdit::countVisibleFieldsNumber()const 
{
    size_t rv = 0;
    for (auto w : m_editors) {
        if (w->isVisible()) {
            rv++;
        }
    }
    return rv;
};

bool OutlineTitleEdit::hasFieldEditor()const 
{
    return (m_field_part.parts().size() > 0);
};

void OutlineTitleEdit::attachEditorTopic(topic_ptr f, EColumnType coltype, QString type_label)
{
    ASSERT(coltype != EColumnType::Uknown, "Expected valid column type");
    ASSERT(m_editors.size() > 0, "Expected editors array");

    m_topic = f;
    LOCK(m_topic);

    m_column = coltype;
    m_type_label = type_label;
    m_field_part = f->fieldValues(coltype, type_label);
    hideFormControls();
    
    ASSERT(m_editors.size() >= m_field_part.parts().size(), "Too small reserved editors array");

    m_has_many_editors = (m_field_part.parts().size() > 1);

    int idx = 0;
    for (auto f : m_field_part.parts()) {
        QLabel* lbl2add = nullptr;
        /* NO LABELS
        if (m_has_many_editors) {
            lbl2add = m_labels[idx];
            lbl2add->setText(FieldParts::type2label(f.first));
            lbl2add->setEnabled(true);
            lbl2add->show();
        }*/

        auto e = m_editors[idx];
        e->setEnabled(true);
        e->show();
        e->setField(f);
        //e->setPlainText(f.second);
        //e->document()->setModified(false);
        
        if (lbl2add) {
            m_main_layout->addRow(lbl2add, e);
        }
        else {
            m_main_layout->addRow(e);
        }

        idx++;
    }    
}

void OutlineTitleEdit::detachEditorTopic(bool save_data)
{
    if (!m_topic)return;
    if (!gui::isDBAttached()) {
        if (m_topic){
            m_topic->release();
            m_topic = nullptr;
        }
        emit onEditDetached();
        return;
    }

    bool anyModified = false;
    int idx = 0;
    FieldParts::FILD_PARTS& parts = m_field_part.parts();
    for (auto& f : parts) {
        auto e = m_editors[idx];
        ASSERT(e->isEnabled(), QString("Expected enabled editor at field part pos '%1'").arg(idx));
        if (e->isEnabled()) {
            f.second = e->toPlainText().trimmed();
            if(e->document()->isModified()){
                anyModified = true;
            }
        }
        idx++;
    }

    bool emptyTopic = m_topic->isEmptyTopic();
    if (save_data){
        if (emptyTopic && m_field_part.isEmptyData()) 
        {
            //if (ard::is_popup(m_topic)){
            //    ard::close_popup(m_topic);
            //}
            auto p = m_topic->parent();
            if(p){
                auto idx = p->indexOf(m_topic);
                if(idx != -1)
                    {
                    m_topic->killSilently(true);
                    gui::rebuildOutline();
                    auto sub_items = p->items().size();
                    if(sub_items == 0)
                        {
                            gui::ensureVisibleInOutline(p);
                        }
                    else
                        {
                            if(idx >= static_cast<int>(sub_items-1))
                                {
                                    idx--;
                                }
                            if(idx > 0)
                                {
                                    auto f = dynamic_cast<ard::topic*>(p->items()[idx]);
                                    gui::ensureVisibleInOutline(f);
                                    ard::focusOnOutline();
                                }
                        }
                    //ard::focusOnOutline();
                }
            }
        }
        else {
            if(anyModified){
                m_topic->setFieldValues(m_column, m_type_label, m_field_part);
            }
        }
    }
    else{
        if (emptyTopic) {
            m_topic->killSilently(true);
            gui::rebuildOutline();
        }
    }

    m_topic->release();
    m_topic = nullptr;
    emit onEditDetached();
}

QTextEdit* OutlineTitleEdit::mainEdit() 
{
    return m_editors[0];
};

void OutlineTitleEdit::showEditWithSuggestedGeometry(const QRect& rc, const QPoint& topLeftOfitemInViewCoord)
{
    if (!hasFieldEditor()) {
        return;
    }

    setEnabled(true);
    show();

    QTextEdit* e = mainEdit();
    if (e) {
        e->setFocus();
        e->show();
    }

    QRect rc2 = rc;
    assert_return_void(m_topic, "expected topic to edit");
    InPlaceEditorStyle edit_style = m_topic->inplaceEditorStyle(m_column);

    if (edit_style == InPlaceEditorStyle::maximized) {
        int edit_h = m_parent_view->viewport()->height() - topLeftOfitemInViewCoord.y();

        QSizeF sz(m_parent_view->viewport()->width(), m_parent_view->viewport()->height());
        rc2.setHeight(edit_h);
        if (e) {
            e->setMinimumHeight(edit_h);
            m_own_even_filter_for_editors = false;
        }
    }
    else {
        m_own_even_filter_for_editors = true;
        auto fnum = countVisibleFieldsNumber();
        int h = fnum * gui::lineHeight();
        if (rc2.height() < h) {
            rc2.setHeight(h);
        }
    }
    setGeometry(rc2);
};

void OutlineTitleEdit::selectAllContent() 
{
    QTextEdit* e = mainEdit();
    if (e) {
        e->selectAll();
    }
};

void OutlineTitleEdit::setupEditTextCursor(const QPoint& ptInGlobal) 
{
    QTextEdit* e = mainEdit();
    if (e) {
        QPoint pt2 = e->mapFromGlobal(ptInGlobal);
        QTextCursor cpos = e->cursorForPosition(pt2);
        e->setTextCursor(cpos);
    }
};

/**
   FormTitleEdit
*/
FormTitleEdit::FormTitleEdit()
{
    //  setDragMode(ScrollHandDrag);
    //  setNoScrollBars();
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_bSkipLostFocusSignal = false;

    QScroller::grabGesture(this, QScroller::LeftMouseButtonGesture);
};

void FormTitleEdit::focusOutEvent( QFocusEvent * e )
{
    QPlainTextEdit::focusOutEvent(e);
    if(m_bSkipLostFocusSignal)
        {
            m_bSkipLostFocusSignal = false;
            return;
        }

    emit lostFocus();
};

void FormTitleEdit::contextMenuEvent(QContextMenuEvent * e)
{
    m_bSkipLostFocusSignal = true;
    QPlainTextEdit::contextMenuEvent(e);
};

bool FormTitleEdit::event(QEvent* e)
{
    switch (e->type()) {
    case QEvent::ScrollPrepare:
        {

            QScrollPrepareEvent *se = static_cast<QScrollPrepareEvent *>(e);
            se->setViewportSize(QSizeF(size()));

            se->setContentPosRange(QRectF(0.0, 0.0, horizontalScrollBar()->maximum(), verticalScrollBar()->maximum()));
            se->setContentPos(QPointF(horizontalScrollBar()->value(), verticalScrollBar()->value()));

            se->accept();                     
        }break;
    default:
        return QPlainTextEdit::event(e);
    }

    return true;
};

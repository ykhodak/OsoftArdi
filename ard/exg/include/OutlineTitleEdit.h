#ifndef OUTLINETITLEEDIT_H
#define OUTLINETITLEEDIT_H

#include <QTextEdit>
#include <QPlainTextEdit>
#include <QFormLayout>
#include "a-db-utils.h"

class QHBoxLayout;
class QVBoxLayout;
class QLabel;
class ArdGraphicsView;
class FormLineEdit;

namespace ard 
{
	class topic;
};

class OutlineTitleEdit : public QWidget
{
    Q_OBJECT
public:
    explicit OutlineTitleEdit(ArdGraphicsView *parent = 0);
    virtual ~OutlineTitleEdit();
    
    void attachEditorTopic(topic_ptr, EColumnType coltype, QString type_label);
    void detachEditorTopic(bool save_data = false);

    void showEditWithSuggestedGeometry(const QRect& rc, const QPoint& topLeftOfitemInViewCoord);
    void selectAllContent();
    void setupEditTextCursor(const QPoint& ptInGlobal);
    bool hasFieldEditor()const;

    EColumnType columnType()const { return m_column; }
    topic_ptr   topic() { return m_topic; }
    ArdGraphicsView* parent_view() { return m_parent_view; }

protected:
    topic_ptr m_topic;
    EColumnType m_column;
    QString m_type_label;

    using LABELS_ARR = std::vector<QLabel*>;
    using EDIT_ARR = std::vector<FormLineEdit*>;


    QFormLayout*    m_main_layout;
    LABELS_ARR      m_labels;///@todo: maybe no labels any more
    EDIT_ARR        m_editors;
    FieldParts      m_field_part;
    ArdGraphicsView* m_parent_view;
    bool             m_own_even_filter_for_editors{true};
    bool            m_has_many_editors{false};
protected:
    QTextEdit*   mainEdit();

signals:
  void onEditDetached();

public slots:

protected:

    template<class T> void hideWidgets(std::vector<T*>& wlst) 
    {
        for (auto w : wlst) {
                w->hide();
                w->setDisabled(true);
                int idx = m_main_layout->indexOf(w);
                if (idx != -1) {
                    m_main_layout->removeWidget(w);
                }
            }
    };

    size_t countVisibleFieldsNumber()const;
    void hideFormControls();
    bool eventFilter(QObject *obj, QEvent *event)override;
    void processFocusOutEvent();

    ///if returns true, further processing is stopped
    bool processKeyPressEvent(QKeyEvent * ev, QTextEdit* e);

    QTextEdit* editorInFocus(size_t* idxOfEditor = nullptr);
    //ykh-need-it void keyPressEvent( QKeyEvent * event );
    ///ykh void focusOutEvent( QFocusEvent * e );

};

/**
 * FormTitleEdit - editor on form, we need it just for focusOutEvent
*/
class FormTitleEdit: public QPlainTextEdit
{
  Q_OBJECT
signals:
  void lostFocus();  

public:
  FormTitleEdit();

protected:
  bool event(QEvent* e);
  void focusOutEvent( QFocusEvent * e );
  void contextMenuEvent(QContextMenuEvent * e);

protected:
  bool m_bSkipLostFocusSignal;
};

#endif // OUTLINETITLEEDIT_H

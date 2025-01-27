#pragma once

#include <QTextEdit>
#include <QTextLine>
#include <QTimeLine>
#include "custom-widgets.h"

class QVBoxLayout;
class QGridLayout;
class QBoxLayout;
class Highlighter;
class ImageMimeData;
class TabControl;
class PopupCard;

namespace ard 
{
    class TopicWidget;
};

class WorkingNoteEdit : public NoteEditBase
{    
public:
    WorkingNoteEdit();
    ~WorkingNoteEdit();

    ard::note_ext* comment();
    const ard::note_ext* comment()const;

    void attachTopic(topic_ptr);
    void detachTopic(bool saveChanges = true);
    bool saveIfModified();
    void mergeFormatOnWordOrSelection(const QTextCharFormat &format);
    int  vscrollWidth()const {return m_vscroll_width;}
    void pastePlainText();

protected:
    void paintEvent(QPaintEvent *e)override;
    void keyPressEvent(QKeyEvent *e)override;
	void VscrolTimerTimeout();


    topic_ptr           m_topic{ nullptr };
    bool                m_hasEmptyNoteImage{ false };
    QTimer*             m_VScrolTimer;
    TabControl*         m_tiny_toolbar{nullptr};
    int                 m_vscroll_width{0};
};


class CardEdit : public WorkingNoteEdit
{
public:
    CardEdit(ard::TopicWidget* w);
protected:
    void focusInEvent(QFocusEvent * e)override;
protected:
    ard::TopicWidget* m_w;
};


#pragma once

#include <QWidget>
#include <QIcon>
#include <QTimer>
#include "anfolder.h"

class QBoxLayout;
class NoteFrameWidget;
class TabControl;
class ImagePreview;
class EmailPreview;
class CardHeader;
class EmbededAnnotationCard;
class QTabWidget;
class QTabBar;
class PopupTabBarButtons;
class PopupTabBar;
class QTextEdit;
class NoteEditBase;

#define MAX_PAGES_NUMBER    12
#define MAX_PINUP_NUMBER    16

#define ARD_TOOLBAR_HEIGHT 32

namespace ard
{
    class BlackBoard;
    class TopicWidget : public QWidget 
    {
    public:
        enum class ECommand
        {
            unknown,
            add_page,
            view_net_traffic,
			lock_page,
            close,
        };

        virtual topic_ptr topic() = 0;
        virtual topic_cptr topic()const;
        
        virtual void closeTopicWidget(TopicWidget*);
        virtual void select_topic_widget(TopicWidget*) {};
        virtual void zoomView(bool zoom_in) = 0;        
        virtual void saveModified() = 0;
        virtual void reloadContent() = 0;
		virtual void refreshTopicWidget() {};
        virtual void locateFilterText() = 0;
        virtual void setFocusOnContent() = 0;
        virtual void detachCardOwner() = 0;
        virtual void updateCardHeader() = 0;
        virtual void setupAnnotationCard(bool edit_it);
        virtual void resetAnnotationCardPos();
        virtual void hideAnnotationCard();
        /// returns widget that serves the topic, should be overriden in tabbed
        virtual TopicWidget* findTopicWidget(ard::topic*);
        virtual void makeActiveInPopupLayout() = 0;
        virtual void closeWidget() { close(); };

        virtual bool toJson(QJsonObject& js)const;
    protected:
        void moveEvent(QMoveEvent *e)override;
        void resizeEvent(QResizeEvent *e)override;
        void closeEvent(QCloseEvent *e)override;
    protected:
        EmbededAnnotationCard*     m_annotation_card{ nullptr };
        const quint8 CONST_DRAG_BORDER_SIZE = 8;
    };
}

class PopupCard : public ard::TopicWidget
{
public:
    PopupCard();
    virtual ~PopupCard();

    virtual void    init_popup();
    virtual void    selectNote(topic_ptr c) = 0;

    QString         customPopupTitle()const { return m_custom_popup_title; }
    void            setCustomPopupTitle(QString s) { m_custom_popup_title = s; }

    void makeActiveInPopupLayout()override;

protected:  

    QBoxLayout*         m_content_box{ nullptr };
    QString             m_custom_popup_title;

    void changeEvent(QEvent *event)override;
    bool eventFilter(QObject *obj, QEvent *event)override;
    void mousePressEvent(QMouseEvent *event)override;
    void mouseReleaseEvent(QMouseEvent *e)override;
    void checkBorderDragging(QMouseEvent *event);

    QRect m_StartGeometry;
    bool m_bMousePressed{ false };
    bool m_bDragTop{ false };
    bool m_bDragLeft{ false };
    bool m_bDragRight{ false };
    bool m_bDragBottom{ false };
    bool m_cursor_set{ false };
    QTimer m_cursor_timer;

    bool leftBorderHit(const QPoint &pos);
    bool rightBorderHit(const QPoint &pos);
    bool topBorderHit(const QPoint &pos);
    bool bottomBorderHit(const QPoint &pos);
    bool anyBorderHit(const QPoint &pos);

	virtual void styleWindow();
};

namespace ard 
{
	class AnnotationPopupCard : public PopupCard
	{
	public:
		AnnotationPopupCard(topic_ptr);

		topic_ptr topic()override { return nullptr; };
		topic_cptr topic()const override { return nullptr; };

		static AnnotationPopupCard* findAnnotationCard(ard::topic* f);
		static void					saveAllCards();

		topic_ptr annotated_topic() { return m_annotated_topic; };

		void    init_popup()override;

		void    selectNote(topic_ptr)override {};//?
		void setupAnnotationCard(bool)override {};
		void resetAnnotationCardPos()override {};
		void hideAnnotationCard()override {};
		void zoomView(bool)override {};
		void locateFilterText()override;
		void updateCardHeader()override {};
		void setFocusOnContent()override;
		void saveModified()override;
		void reloadContent()override;
		void detachCardOwner()override;
	protected:
		NoteEditBase*   m_edit{ nullptr };
		topic_ptr       m_annotated_topic;
		static std::unordered_map<ard::topic*, AnnotationPopupCard*>	m_t2a;
	};
};
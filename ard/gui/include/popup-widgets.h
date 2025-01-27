#pragma once

#include "workspace.h"


class PicturePreview;


namespace ard 
{
    class BlackBoard;
    class email;
    class email_draft;
    class fileref;
	class picture;
    class AnnotationPopupCard;
};

#ifdef ARD_BIG
using CARDS_ARR = std::vector<PopupCard*>;


class EmbededAnnotationCard : public QWidget
{
public:
    EmbededAnnotationCard(ard::TopicWidget* w);
    void                editAnnotation(bool animate_pos, const QPoint* ptClick = nullptr);
    ard::TopicWidget*   twidget() {return m_w;}
    topic_ptr           topic() { return m_w->topic(); };
    void                resizeToText();

protected:
    void  paintEvent(QPaintEvent *e)override;
    void mousePressEvent(QMouseEvent *e)override;

protected:    
    ard::TopicWidget*   m_w;
};

namespace ard {
	class tda_view;

    class EmailTabPage : public topic_tab_page
    {
    public:
        EmailTabPage(ard::email* e);
        void		attachTopic(ard::email*);
        topic_ptr	topic()override;
        void		refreshTopicWidget()override;
        void		reloadContent()override;
        void		zoomView(bool zoom_in)override;
        void		detachCardOwner()override;
		void		findText();
    protected:
        EmailPreview*       m_email_view{ nullptr };
        ard::email*         m_email{nullptr};       
    };

    class NoteTabPage : public topic_tab_page
    {
    public:
        NoteTabPage(topic_ptr f);
        void attachTopic(topic_ptr);
        topic_ptr topic()override { return m_topic; };
        void zoomView(bool zoom_in)override;
        void saveModified()override;
        void reloadContent()override;
        void locateFilterText()override;
        void setFocusOnContent()override;
        void detachCardOwner()override;
    protected:
        void closeEvent(QCloseEvent *e)override;

        NoteFrameWidget*    m_text_edit{nullptr};
        topic_ptr           m_topic{ nullptr };
        bool                m_modifications_saved{ false };
    };

    class EmailDraftTabPage : public topic_tab_page
    {
    public:
        EmailDraftTabPage(ard::email_draft* d);

        topic_ptr topic()override;
        void zoomView(bool zoom_in)override;
        void saveModified()override;
        void reloadContent()override;
        void locateFilterText()override;
        void setFocusOnContent()override;
        void detachCardOwner()override;
    protected:
        void closeEvent(QCloseEvent *e)override;
        NoteFrameWidget*    m_text_edit{ nullptr };
        ard::email_draft*   m_draft{ nullptr };
        bool                m_modifications_saved{ false };
    };

	class PicturePage : public topic_tab_page
	{
	public:
		PicturePage(ard::picture* p);
		void attachTopic(ard::picture*);
		topic_ptr topic()override;
		void reloadContent()override;
		void zoomView(bool )override {};
		void detachCardOwner()override;
	protected:
		ard::picture*   m_pic{ nullptr };
		PicturePreview*	m_pview{ nullptr };
	};

	class TdaPage : public topic_tab_page
	{
	public:
		TdaPage(ard::fileref* p);
		void attachTopic(ard::fileref*);
		topic_ptr topic()override;
		void reloadContent()override {};
		void zoomView(bool)override {};
		void detachCardOwner()override {};
	protected:
		ard::fileref*   m_ref{ nullptr };
		tda_view*		m_tda_view{ nullptr };
	};
}

#endif //ARD_BIG

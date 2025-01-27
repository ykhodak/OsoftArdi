#pragma once

#include <QWidget>
#include "email.h"
#include "email_draft.h"

#ifdef ARD_CHROME
    #include <QWebEngineView>
#else
    #include <QTextBrowser>
    class ArdQTextBrowser;
#endif

class QTextEdit;
class QWebEngineView;
class EmailTitleWidget;
class QLineEdit;
class QLabel;
class QPushButton;
class QToolButton;
class QProgressBar;
class NoteFrameWidget;
class StaticEmailTitleWidget;

namespace ard 
{
    class TopicWidget;
}

class EmailPreview : public QWidget
{
public:
    EmailPreview(ard::TopicWidget*);
    ~EmailPreview();
    void        attachEmail(email_ptr e);
    void        detach();
    void        reloadContent();
    void        zoomView(bool zoom_in);
	void		findText();
	void		updateTitle();
protected:
#ifdef ARD_CHROME    
    QWebEngineView* m_web_view;
#else
    ArdQTextBrowser*        m_web_view;
#endif
    StaticEmailTitleWidget*     m_title_widget;
    email_ptr                   m_email{nullptr};
};


class DraftEmailTitleWidget : public QWidget
{
public:
    DraftEmailTitleWidget(NoteFrameWidget* text_edit);
    void saveTitleModified();
    void attachDraft(ard::email_draft*);
    void detachDraft();

protected:
    void updateAttachmentButton();
    void runAB();

    ard::email_draft						*m_draft{ nullptr };
    QLineEdit								*m_edit_subject;
    QTextEdit								*m_edit_to{ nullptr }, *m_edit_cc{ nullptr };
    QToolButton								*m_att_btn;
	ard::email_draft_ext::attachement_file_list	m_attachementList;
    bool									m_attachment_info_modified;
    NoteFrameWidget*						m_text_edit{nullptr};
};

#ifdef ARD_CHROME
class ArdQWebEnginePage : public QWebEnginePage
{
public:
    ArdQWebEnginePage();
    bool acceptNavigationRequest(const QUrl & url,
        QWebEnginePage::NavigationType type,
        bool)override;
    QWebEnginePage* createWindow(WebWindowType type)override;
};

#else
class ArdQTextBrowser : public QTextBrowser
{
};
#endif//ARD_CHROME

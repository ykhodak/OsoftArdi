#pragma once

#include <functional>
#include <QBoxLayout>
#include "GoogleClient.h"
#include "gcontact/GcontactRoutes.h"
#include "gcontact/GcontactCache.h"

using FetchCallback = std::function<void(QNetworkReply*)>;

class QGraphicsSceneDragDropEvent;

namespace ard {
	class topic;
    class email_model;
    class KRingKey;
	class topic_tab_page;	
	class locus_folder;
}
class RootTopic;

namespace ard
{    
    ///returns true when current DB is open and data loaded
    bool       isDbConnected();
    ///current open DB (shouldn't be used in sync code)
    ArdDB*     db();
    ///root topic of current open DB
    RootTopic* root();
    ///default threads space
    topic_ptr thread_root();
    ///top topic currently outlined
    topic_ptr   hoisted();

    ///topic currently selected in outline
    topic_ptr   currentTopic();
    ///if current topic is an email, we return typecast
    email_ptr   currentEmail();
    ///if current topic is an thread of emails, we return typecast
    ethread_ptr     currentThread();
    
    ///current active popup board
    ard::selector_board*    currentBoard();
    ard::KRingKey*			currentKRingKey();
    ard::locus_folder*		Sortbox();
	ard::locus_folder*		Maybe();
	ard::locus_folder*		Reference();
	ard::locus_folder*		Trash();
	ard::locus_folder*		Delegated();
	ard::locus_folder*		ensureCustomSorterByTitle(QString name);
	ard::locus_folder*		Backlog();
	ard::locus_folder*		BoardTopicsHolder();
	ard::locus_folder*		CustomSortersRoot();
    topic_ptr   lookup(DB_ID_TYPE id);

    ///create new topic, if 'parent' is not provided
    ///it will be created in context of current outline
    topic_ptr   insert_new_topic(topic_ptr parent = nullptr);
    ///same as insert_new_topic but popup topic
    topic_ptr   insert_new_popup_topic(topic_ptr parent = nullptr);

    template <class T> T* lookupAs(DB_ID_TYPE id);

    topic_ptr   hoistedInFilter();
    void        setHoistedInFilter(topic_ptr);
    QString getAttachmentDownloadDir();

	topic_tab_page*		open_page(topic_ptr, bool focusOnContext = false);
    void				popup_annotation(topic_ptr);    
    void				close_popup(topic_ptr);
    void				close_popup(TOPICS_LIST);
	void				selectArdiDbFile();
	void				closeArdiDb();
	bool				isSelectorLinked();
	void				linkSelector(bool link);

    void        save_all_popup_content();
    void        edit_selector_annotation();///we pickup current selector topic
    void        update_topic_card(topic_ptr);
    void        search(QString text = "");
    void        show_selector_context_menu(topic_ptr, const QPoint& pt);
    void        focusOnOutline();
    void        focusOnTSpace();

	topic_tab_page*     edit_note(topic_ptr, bool in_new_tab);
    void				updateGItem(topic_ptr);
    void        formatNotes(TOPICS_LIST& lst, const QTextCharFormat* fmt);

    void        selectCustomFolders(LOCUS_LIST& plist);
    ///this is expensive call
    snc::SyncProgressStatus* syncProgressStatus();

	googleQt::gclient_ptr	google();
    googleQt::GmailRoutes*  gmail();
    
    ard::email_model*       gmail_model();
    googleQt::mail_cache::GMailSQLiteStorage* gstorage();
    bool                    isGoogleConnected();
	bool                    isGmailConnected();
    void                    disconnectGoogle();
    bool                    reconnectGoogle(QString userId);
    bool                    authAndConnectNewGoogleUser();
    bool                    hasGoogleToken();
    bool                    revokeGoogleToken();
 //   void                    initGoogleBatchUpdate();
    /// first(true) - token is found, second(true) Google authorization request confirmed
    std::pair<bool, bool>   guiConditionalyCheckAuthorizeGoogle();
    bool                    revokeGoogleTokenWithConfirm();
    void                    resolveGoogleException(googleQt::GoogleException& e);
    void                    sendAllDrafts();
    bool                    deleteGoogleCache();

    TOPICS_LIST             reduce2ancestors(TOPICS_LIST& lst);
	ESET					reduce2emails(TOPICS_LIST& lst);
	template<class IT>
	THREAD_SET              reduce2ethreads(IT begin, IT end);
	THREAD_SET				select_ethreads(TOPICS_LIST& lst);

    void                    moveTopics(TOPICS_LIST& lst, 
                                        topic_ptr dest, 
                                        int& moved_count, 
                                        int& move_err_count,
                                        QString& errDescriptionOnMove,
                                        QString& errDescriptionOnLabel);

    ///call moveTopics and show result in case of warning in message box
    bool                    guiInterpreted_moveTopics(TOPICS_LIST& lst,
                                    topic_ptr dest,
                                    int& moved_count,
                                    bool showReportOnSuccess = true);
    void                    guiInterpreted_moveTopic(topic_ptr t, topic_ptr dest);
    bool                    guiEditUFolder(topic_ptr f, QString boxHeader = "");
    void                    guiEditProjectRoot(topic_ptr f);
    
    //@todo:starting from here clean up "gui" namespace
	QWidget*				mainWnd();
    ///popup message box
    void					messageBox(QWidget* parent, QString msg);
	///popup error box
	void					errorBox(QWidget* parent, QString msg);
	///popup confirmation box
	bool					confirmBox(QWidget* parent, QString s);
	///popup confirmation box with option
	YesNoConfirm			confirmBoxWithOption(QWidget* parent, QString s, QString optionStr);

    YesNoConfirm YesNoCancel(QWidget* parent, QString msg, bool default2confirm = true);
    ///popup choice box with radion buttons
    int        choice(std::vector<QString>& options_list, int selected = -1, QString label = "");
    ///popup choice box with combo options
    int        choiceCombo(std::vector<QString>& options_list, int selected = -1, QString label = "", std::vector<QString>* buttons_list=nullptr);
    ///add button with default size to box layout, assign event handler
    QPushButton* addBoxButton(QBoxLayout* lt, QString text, std::function<void(void)> released_slot);
    ///log root directory, includes log files, sync log subdirs etc.
    QString     logDir();
	void		killTopicsSilently(TOPICS_LIST& lst);


    ///create new topic, if 'parent' is not provided
    ///it will be created in context of current outline
    ///return root topic of outline
    topic_ptr           buildOutlineSample(OutlineSample o, topic_ptr parent = nullptr);

    ///generates some outline in provided board, functions creates new topics in board holder
    ///and then puts them or board and connects in some way
    ard::board_item*    buildBBoardSample(BoardSample smpl, ard::selector_board* b, int root_band_index, int root_yDelta);

    void        sleep(int msec);

    /// fonts ///
    QSize               calcSize(QString s, QFont* font = nullptr);
    void                resetAllFonts(int defaultFontSize);
    void                resetNotesFonts();
    QFont*              defaultFont();
    QFont*              defaultBoldFont();
    QFont*              defaultSmallFont();
    QFont*              defaultSmall2Font();
    QFont*              graphNodeHeaderFont();
    QFont*              defaultOutlineLabelFont();
    int                 graphNodeHeaderHeight();
    QFont*              defaultNoteFont();
    int                 defaultFallbackFontSize();

    QIcon               iconFromTheme(QString name);

    void         asyncExec(EAsyncCallRequest cmd, topic_ptr t1 = nullptr, topic_ptr t2 = nullptr);
    void         asyncExec(EAsyncCallRequest cmd, int param, int param2 = 0, int param3 = 0);
    void         asyncExec(EAsyncCallRequest cmd, QString param, int param2 = 0, int param3 = 0);
    void         delayed_asyncExec(EAsyncCallRequest cmd, topic_ptr t1 = nullptr, topic_ptr t2 = nullptr);

    QByteArray		encryptBarray(QString pwd, const QByteArray& b);
    QByteArray		decryptBarray(QString pwd, const QByteArray& b);
    QByteArray		encryptBarray(const std::pair<uint64_t, uint64_t>& k, const QByteArray& b);
    QByteArray		decryptBarray(const std::pair<uint64_t, uint64_t>& k, const QByteArray& b);

    TOPICS_LIST		insertClipboardData(topic_ptr destination_parent, int destination_pos, const QMimeData* mm, bool dragMoveContent);
	void			dragEnterEvent(QGraphicsSceneDragDropEvent *e);
	bool			dragMoveEvent(ard::topic* target, QGraphicsSceneDragDropEvent *e);
    topic_ptr		dropTextFile(topic_ptr destination_parent, int destination_pos, const QUrl& url);
	topic_ptr		dropImageFile(topic_ptr destination_parent, int destination_pos, const QUrl& url);
    topic_ptr		dropHtml(topic_ptr destination_parent, int destination_pos, const QString& ml);
    topic_ptr		dropText(topic_ptr destination_parent, int destination_pos, const QString& s);
    topic_ptr		dropUrl(topic_ptr destination_parent, int destination_pos, const QUrl& u);
	topic_ptr		dropImage(topic_ptr destination_parent, int destination_pos, const QImage& u);

    /// select bookmarks html-file and import into sortbox
    void gui_import_html_bookmarks();

    void        setup_menu(QMenu* m);

	void		fetchHttpData(const QUrl& url, FetchCallback cb, std::map<QString, QString>* rawHeaders = nullptr);
	void		downloadHttpData(const QUrl& url, QString destinationFile, FetchCallback cb, std::map<QString, QString>* rawHeaders = nullptr);

    ///we generate autotest and want to skip some gui interactions, like activating windows, set focus, layout popup etc.
    bool        autotestMode();

	void		rebuildMailBoard(ard::q_param* q);
	void		rebuildFoldersBoard(ard::locus_folder* f);
	ard::topic* moveInsideParent(ard::topic* it, bool moveUp);

	QString		recoverEmailAddress(QString name_ref);
	/// spacer - empty widget with h-expanding policy
	QWidget*	newSpacer(bool expandingY = false);
	template<class T>
	QBoxLayout*	newLayout(QWidget *parent = nullptr);
	template<class T>
	T scale(T num, T in_min, T in_max, T out_min, T out_max);
};

template <class T> T* ard::lookupAs(DB_ID_TYPE id) {
    T* rv = nullptr;
    auto f = lookup(id);
    if (f) {
        rv = dynamic_cast<T*>(f);
    }
    return rv;
};

template<class T>
QBoxLayout*	ard::newLayout(QWidget *parent)
{
	T* rv = new T(parent);
	rv->setSpacing(0);
	rv->setMargin(0);
	return rv;
};


template<class T>
T scale(T num, T in_min, T in_max, T out_min, T out_max) 
{
	return (num - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
};


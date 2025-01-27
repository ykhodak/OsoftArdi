#pragma once

#include <QDialog>
#include "gmail/GmailRoutes.h"
#include "utils.h"
#include "snc-tree.h"
#include "anfolder.h"
#include "custom-widgets.h"
#include "OutlineView.h"
#include "contact.h"
#include "email.h"
#include "email_draft.h"

class anItem;
class QFormLayout;
class QVBoxLayout;
class QTabWidget;
class QCheckBox;
class FlowLayout;

namespace ard {
	class scene_view_dlg : public QDialog
	{
	public:
		scene_view_dlg(QWidget *parent = nullptr);
	protected:
		template<class T = ard::topic>
		T*						current_v_topic();

		ProtoGItem*				currentGI();
		OutlineView*			currentView();
		scene_view*				current_scene_view();
		void					rebuild_current_scene_view();

		void					setupDialog(QSize sz, bool addCloseButton = true);

		QVBoxLayout*			m_main_layout;
		QTabWidget*				m_main_tab;
		QHBoxLayout*			m_buttons_layout;
		scene_view::map_idx2v	m_index2view;
	};

	/**
	list of email attachments with options to download
	*/
	class attachements_dlg : public scene_view_dlg,
							public googleQtProgressControl
	{
		Q_OBJECT
	public:
		static void runIt(email_ptr e);

		~attachements_dlg();
	protected:
		attachements_dlg(email_ptr e);

		public slots:
		void currentMarkPressed(int c, ProtoGItem*);
		void attachmentsDownloaded(googleQt::mail_cache::msg_ptr, googleQt::mail_cache::att_ptr);
		void allAttachmentsDownloaded(ard::email*);

	protected:
		email_ptr               m_email{ nullptr };
		QLabel*					m_download_status{ nullptr };
	protected:
		void addAttachmentsTab();
	};

	/**
	list of draft email attachments with options to download
	*/
	class draft_attachements_dlg : public scene_view_dlg
	{
		Q_OBJECT
	public:
		static void runIt(ard::email_draft* d, QWidget *parent);
	protected:
		draft_attachements_dlg(ard::email_draft* d, QWidget *parent);

		public slots:
		void currentMarkPressed(int c, ProtoGItem*);

	protected:
		ard::email_draft*		m_draft{ nullptr };

	protected:
		void addDraftAttachmentsTab();
	};

	/**
	list of gmail accounts
	*/
	class accounts_dlg : public scene_view_dlg
	{
		Q_OBJECT

	public:
		static void runIt();
	protected:
		accounts_dlg();

		public slots:
		void currentMarkPressed(int c, ProtoGItem*);

	protected:
		void addAccountsTab();
	};

	/**
	move items dialog - select target destination
	and move there current topic
	*/
	class move_dlg : public ard::scene_view_dlg
	{
		Q_OBJECT
	public:
		static void moveIt(TOPICS_LIST& move_it);
	protected:
		move_dlg(TOPICS_LIST& move_it);

		public slots:
		void currentMarkPressed(int c, ProtoGItem*);

	protected:
		void                    addFolderTab();
		void                    move2folder(topic_ptr dest);

		TOPICS_LIST				m_topics2move;
		QCheckBox*              m_follow_dest_folder{ nullptr };
		topic_ptr               m_DestinationFolder{ nullptr };
		bool                    m_got_moved{ false };
	};

	/**
		show selector folders
	*/
	class folders_dlg : public ard::scene_view_dlg
	{
		Q_OBJECT
	public:
		static void showFolders();
	protected:
		folders_dlg();

	public slots:
	void currentMarkPressed(int, ProtoGItem*) {};

	protected:
		void   addFolderTab();
	};

	class files_dlg : public ard::scene_view_dlg
	{
		Q_OBJECT
	public:
		static void showFiles();
	protected:
		files_dlg();

		public slots:
		void currentMarkPressed(int c, ProtoGItem*);

	protected:
		void	addFilesTab();
		void	cmdFileMore();
		void	processMoreEx(QAction* a);

		QPushButton* m_moreBtn{nullptr};
	};


	/**
	list of contact groups
	*/
	class contact_groups_dlg : public scene_view_dlg
	{
		Q_OBJECT

	public:
		static void runIt();
	protected:
		contact_groups_dlg();

		public slots:
		void currentMarkPressed(int c, ProtoGItem*);

	protected:
		void addGroupsTab();
	};
}

template<class T>
T* ard::scene_view_dlg::current_v_topic() 
{
	ProtoGItem* g = currentGI();
	if (g) {
		return dynamic_cast<T*>(g->topic()->shortcutUnderlying());
	}
	return nullptr;
};




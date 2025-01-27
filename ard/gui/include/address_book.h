#pragma once

#include "contact.h"
#include "small_dialogs.h"

namespace ard {
	/**
	adress book
	*/
	class address_book_dlg : public ard::scene_view_dlg
	{
		Q_OBJECT
	public:
		static ard::ContactsLookup select_receiver(QWidget* parent, ard::ContactsLookup* predef_lst = nullptr, bool attached_option = false);
		static ard::ContactsLookup select_contacts(QWidget* parent);
		static ard::contact* open_book();

		public slots:
		void currentMarkPressed(int c, ProtoGItem*);
		void topicDoubleClick(void*);

	protected:
		address_book_dlg(bool single_select, bool attached_option = false, bool has_cc = true);
		ard::contact*			selected_contact();
		QPushButton*			create_edit_contact_button();
		QPushButton*			create_new_contact_button();
		QPushButton*			create_delete_contact_button();		
	protected:
		ard::ContactsLookup		m_result;
		QTextEdit               *m_edit_to{ nullptr }, *m_edit_cc{ nullptr };
		QCheckBox*              m_chk_attached{ nullptr };
		bool                    m_single_contact_select{ false };
		QLineEdit               *m_search_edit;
		QString                 m_str_search;
		QPushButton*			m_btnTo{nullptr};
	protected:
		void addABTab();
	};

	/**
	edit contact
	*/
	class contact_dlg : public scene_view_dlg
	{
		Q_OBJECT

	public:
		static void runIt(ard::contact* c);
	protected:
		contact_dlg(ard::contact* c);

	protected:
		void add_contact_tab();
		ard::contact*	m_c;
	};

	extern QTextEdit* buildEmailListEditor();
	void lookupList2editor(const ard::LOOKUP_LIST& lookup_list, QTextEdit* e);
	void editor2lookupList(QTextEdit* e, STRING_SET* emails_set, ard::LOOKUP_LIST* lookup_list);
	void addContactToListEditor(ard::EitherContactOrEmail c, QTextEdit* e);
}
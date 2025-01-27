#pragma once

#include <set>
#include "small_dialogs.h"
class QLineEdit;
class QListWidget;
class QPlainTextEdit;

namespace ard 
{
	class rule;
	/// sungle rule dialog
	class rule_dlg : public QDialog
	{
		Q_OBJECT
	public:
		static bool addRule(QString name = "", std::set<QString>* from = nullptr);
		static bool editRule(ard::rule* r, std::set<QString>* from = nullptr);
	public slots:
		void currentMarkPressed(int c, ProtoGItem*);
		void currentGChanged(ProtoGItem*);
	protected:
		rule_dlg(ard::rule* r, QString name, std::set<QString>* from);
		void		processOK();
		void		processCancel();
		void		addSender();
		void		addSubject();

		void		buildSendersView(ard::rule* r, std::set<QString>* from, QBoxLayout* box);
		void		buildSubjectView(ard::rule* r, QBoxLayout* box);

		rule*		m_rule{nullptr};

		scene_view::ptr 	m_senders_view;
		scene_view::ptr 	m_subject_view;
		TOPICS_LIST			m_senders_list;
		TOPICS_LIST			m_subject_list;
		QLineEdit*			m_name{nullptr};
		QPlainTextEdit*		m_msg_phrase{ nullptr };
		QCheckBox*			m_exclusion_filter{nullptr};
		bool				m_accepted{ false };
	};

	/**
	list of mail rules
	*/
	class rules_dlg : public scene_view_dlg
	{
		Q_OBJECT
	public:
		static void run_it(ard::q_param* r2select = nullptr);
	public slots:
		void currentMarkPressed(int c, ProtoGItem*);
		void currentGChanged(ProtoGItem*);
	protected:
		rules_dlg(ard::q_param* r2select);
		void addRulesTab();
		void selectRule(ard::q_param* r2select);
		void editRule();
		void removeRule();

		QPlainTextEdit*     m_rule_str{ nullptr };
		bool				m_modified{ false };
	};
};
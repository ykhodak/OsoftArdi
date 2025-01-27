#pragma once
#include "anfolder.h"

namespace ard
{
	/**
		tool_topic - sometime we want to generate items for sceen on the fly out of some data
		it can have title and wrap extra data in derived classes if needed
	*/
	class tool_topic : public ard::topic
	{
	public:
		tool_topic(QString title = "") :ard::topic(title) {};
		virtual QString				objName()const override { return "OToolItem"; };
		virtual QPixmap				getIcon(OutlineContext)const override { return m_IconPixmap; };
		virtual QPixmap				getSecondaryIcon(OutlineContext)const override { return QPixmap(); };
		virtual EOBJ				otype()const override { return objToolItem; };
		virtual cit_primitive*		create()const override { ASSERT(0, "unsupported"); return NULL; };
		void						fatFingerSelect()override {};

		void						setIconPixmap(QPixmap pm) { m_IconPixmap = pm; }
		template <class IT>
		static TOPICS_LIST			wrapList(IT b, IT e, COMMANDS_SET commands, bool can_rename);
		bool						hasCurrentCommand(ECurrentCommand c)const override;
		bool						canRename()const override{ return m_can_rename; }
		void						setCommands(COMMANDS_SET cset) { m_available_commands = cset; }
		void						setCanRename(bool val) { m_can_rename = val; }
	protected:
		QPixmap             m_IconPixmap;
		COMMANDS_SET		m_available_commands;
		bool				m_can_rename{false};
	};

	/**
		shortcut - a wrapper for outline
	*/
	class shortcut : public ard::tool_topic
	{
	public:
		shortcut(topic_ptr underlying);
		virtual ~shortcut();
		virtual QString     objName()const override { return "Shortcut"; };
		topic_ptr   shortcutUnderlying()override { return m_underlying; };
		topic_cptr  shortcutUnderlying()const override { return m_underlying; };
		QPixmap     getIcon(OutlineContext c)const override;
		QPixmap     getSecondaryIcon(OutlineContext c)const override;
		QString     title()const override;
		void        setTitle(QString title, bool guiRecalc = false)override;
		std::pair<TernaryIconType, int>  ternaryIconWidth()const override { return m_underlying->ternaryIconWidth(); };
		DB_ID_TYPE  underlyingDbid()const override;
		///create list of shortcutst out of list of topic
		static TOPICS_LIST wrapList(TOPICS_LIST& lst, EOutlinePolicy hot_select_policy);
		template <class IT>
		static TOPICS_LIST			wrapList(IT b, IT e, EOutlinePolicy hot_select_policy);
		void        fatFingerSelect()override;
		bool        canRename()const override;
		QString     dbgHint(QString s = "")const override;
		const QPixmap*          thumbnail()const override;
	protected:
		topic_ptr					m_underlying{ nullptr };
		EOutlinePolicy				m_hot_select_policy{ outline_policy_Uknown };
	};
}


/**
    one column of an composed topic.
    it gets flippet 90 so columns become fields and are displayed as field:value
*/
class FormFieldTopic : public ard::shortcut
{
public:

    using ptr = FormFieldTopic*;
    using cptr = const FormFieldTopic*;

    FormFieldTopic(topic_ptr f, EColumnType column, QString type_label);
    QString     title()const override;
    QString     formValue()const override;
    QString     fieldMergedValue(EColumnType column_type, QString type_label)const override;
    EColumnType treatFieldEditorRequest(EColumnType column_type)const override;
    EColumnType columnType()const { return m_column; }
    QString     formValueLabel()const override{ return m_type_label; }
    FieldParts  fieldValues(EColumnType column_type, QString type_label)const override;
    InPlaceEditorStyle inplaceEditorStyle(EColumnType)const override;
    void        setFieldValues(EColumnType column_type, QString type_label, const FieldParts& parts)override;
    bool        canRename()const override;
    bool        hasText4SearchFilter(const TextFilterContext& fc)const override;
protected:
    EColumnType m_column;
    QString     m_type_label;
};

template <class IT>
TOPICS_LIST	ard::tool_topic::wrapList(IT b, IT e, COMMANDS_SET commands, bool can_rename)
{
	TOPICS_LIST rv;
	IT i = b;
	while(i != e) 
	{
		auto t = new tool_topic(*i);
		t->m_available_commands = commands;
		t->m_can_rename = can_rename;
		rv.push_back(t);
		i++;
	}
	return rv;
};

template <class IT>
TOPICS_LIST ard::shortcut::wrapList(IT b, IT e, EOutlinePolicy hot_select_policy)
{
	TOPICS_LIST rv;
	IT i = b;
	while (i != e)
	{
		auto sh = new shortcut(*i);
		sh->m_hot_select_policy = hot_select_policy;
		rv.push_back(sh);
		i++;
	}
	return rv;
};

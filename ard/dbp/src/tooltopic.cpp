#include "a-db-utils.h"
#include "tooltopic.h"

/**
	tool_topic
*/
bool ard::tool_topic::hasCurrentCommand(ECurrentCommand c)const
{
	auto i = m_available_commands.find(c);
	if (i != m_available_commands.end())
		return true;
	return false;
}



/**
	shortcut
*/
ard::shortcut::shortcut(topic_ptr underlying)
{
    ASSERT(underlying, "expected object");
    m_underlying = underlying;
    LOCK(m_underlying);
};

ard::shortcut::~shortcut()
{
    if (m_underlying) {
        m_underlying->release();
    }
};

QPixmap ard::shortcut::getSecondaryIcon(OutlineContext c)const
{
    return m_underlying->getSecondaryIcon(c);
};

QPixmap ard::shortcut::getIcon(OutlineContext c)const
{
    if(c != OutlineContext::normal){
        return m_underlying->getIcon(c);
    }
    
    QPixmap rv;
    if (items().size() > 0)
    {
        rv = isExpanded() ? getIcon_TopicExpanded() : getIcon_TopicCollapsed();
    }
    return rv;
};

QString ard::shortcut::title()const
{
    return m_underlying->title();
};

void ard::shortcut::setTitle(QString s, bool guiRecalc)
{
    if (m_underlying) {
        m_underlying->setTitle(s, guiRecalc);
    }
};

bool ard::shortcut::canRename()const
{
    bool rv = false;
    if (m_underlying) {
        rv = m_underlying->canRename();
    }
    return rv;
};

QString ard::shortcut::dbgHint(QString s /*= ""*/)const
{
    QString rv;
    if (m_underlying) {
        rv = "shortcut to " + m_underlying->dbgHint(s);
    }
    return rv;
};

DB_ID_TYPE ard::shortcut::underlyingDbid()const
{
    return m_underlying->underlyingDbid();
};

const QPixmap* ard::shortcut::thumbnail()const
{
	return m_underlying->thumbnail();
};

TOPICS_LIST ard::shortcut::wrapList(TOPICS_LIST& lst, EOutlinePolicy hot_select_policy)
{
    TOPICS_LIST rv;
    rv.clear();
    for (auto& i : lst) {
        auto sh = new ard::shortcut(i);
        sh->m_hot_select_policy = hot_select_policy;
        rv.push_back(sh);
    }
    return rv;
};

void ard::shortcut::fatFingerSelect()
{
    topic_ptr f = shortcutUnderlying();
    if (outline_policy_Uknown != m_hot_select_policy) {
        gui::outlineFolder(f, m_hot_select_policy);
    }
    else {
        ard::open_page(f);
    }
};

/**
    FormFieldTopic
*/
FormFieldTopic::FormFieldTopic(topic_ptr f, EColumnType column, QString type_label)
    :ard::shortcut(f), m_column(column), m_type_label(type_label)
{

};

QString FormFieldTopic::title()const
{
    return columntype2label(m_column);
};

QString FormFieldTopic::formValue()const 
{
    QString rv;
    if (m_underlying) {
        rv = m_underlying->fieldMergedValue(m_column, m_type_label);
    }
    return rv;
};

QString FormFieldTopic::fieldMergedValue(EColumnType column_type, QString /*type_label*/)const
{
    QString rv;
    if (m_underlying) {
        switch (column_type)
        {
            case EColumnType::FormFieldName:
            {
                rv = columntype2label(m_column);
            }break;
            case EColumnType::FormFieldValue: 
            {
                rv = m_underlying->fieldMergedValue(m_column, m_type_label);
            }break;
            default:
            {
                ASSERT(0, "NA");
                return rv;
            }
        }
    }
    return rv;
};

bool FormFieldTopic::canRename()const
{
    if (m_underlying)
        return true;
    return false;
};

EColumnType FormFieldTopic::treatFieldEditorRequest(EColumnType )const 
{
    return EColumnType::FormFieldValue;
};

FieldParts FormFieldTopic::fieldValues(EColumnType, QString)const
{
    if (m_underlying) {
        return m_underlying->fieldValues(m_column, m_type_label);
    }

    FieldParts r;
    return r;
};

void FormFieldTopic::setFieldValues(EColumnType, QString type_label, const FieldParts& parts)
{
    if (m_underlying) {
        m_underlying->setFieldValues(m_column, type_label, parts);
    }
};

InPlaceEditorStyle FormFieldTopic::inplaceEditorStyle(EColumnType c)const
{
    InPlaceEditorStyle rv = InPlaceEditorStyle::regular;
    if (c == EColumnType::FormFieldValue) {
        if (m_column == EColumnType::ContactNotes ||
            m_column == EColumnType::KRingNotes) 
        {
            rv = InPlaceEditorStyle::maximized;
        }
    }
    return rv;
};

bool FormFieldTopic::hasText4SearchFilter(const TextFilterContext& fc)const 
{
    QString s = formValue();
    int idx = s.indexOf(fc.key_str, 0, Qt::CaseInsensitive);
    if (idx != -1) return true;
    return false;
};

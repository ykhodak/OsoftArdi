#pragma once

#ifdef ARD_GD
#include <memory>
#include "anfolder.h"
#include "tooltopic.h"
#include "ansyncdb.h"
#include "gcontact/GcontactRoutes.h"
#include "gcontact/GcontactCache.h"

class ArdDB;

/**
    contacts in ardi space
*/
namespace ard {
    class contact_ext;
    struct ContactsLookup;
    class ArdiCsv;
	class contact_group;

    struct contacts_merge_result
    {
        CONTACT_LIST        contacts;
        CONTACT_GROUP_LIST  groups;
        int                 skipped_existing_contacts{ 0 };
        int                 skipped_tmp_contacts{ 0 };
        bool                merge_canceled{ false };
    };

    class contacts_model
    {
    public:
        contacts_model(ArdDB* db);
		~contacts_model();

        ard::ContactsRoot*             croot();
        ard::ContactGroupsRoot*        groot();

        const ard::ContactsRoot*       croot()const;
        const ard::ContactGroupsRoot*  groot()const;

        std::pair<int, int>             mergeContacts(const contacts_model& other);
        std::pair<int, int>             storeMergedContacts(ard::contacts_merge_result);
    protected:      
        ard::ContactsRoot*				m_contacts_root{ nullptr };
        ard::ContactGroupsRoot*			m_contact_groups_root{ nullptr };
    };

    class ContactsRoot : public RootTopic
    {
        friend class ard::contacts_model;
    public:
        DECLARE_ROOT(ContactsRoot, objContactRoot, ESingletonType::contactsHolder);
        QString                 title()const override { return "Contacts"; };
        ard::contact*			addContact(QString firstName, QString lastName, std::vector<QString>* groups = nullptr);
        CONTACT_LIST            contacts();
        CCONTACT_LIST           contacts()const;
        TOPICS_LIST             filteredItems(QString groupFilterId, QString indexStr = "*");

        void setContactGroupMembership(ard::contact* c, const std::vector<QString>& groups);

        static QString  trimPhoneNumber(QString s);
        static QString  trimEmail(QString s);
        static bool     isTmpContactEmail(QString s);

        /// phone -> [c], phone lower trimmed
        S_2_CSET        phoneGroupedContacts()const;
        /// phone -> [c], email lower trimmed
        S_2_CSET        emailGroupedContacts()const;
        /// last -> [c], last lower trimmed
        S_2_CSET        familyNameGroupedContacts()const;

        /// returns (imported, skipped) if imported == -1 means error
        //std::pair<int, int> 
        contacts_merge_result   mergeContacts(const CONTACT_LIST& new_contacts, const GROUP_MAP& new_groups)const;      

        void enrichLookupListFromCache(ard::ContactsLookup& res_lst);

        static contact*		hoistedContactInFilter();
        static void        setHoistedContactInFilter(contact*);
    protected:
        S_2_CSET  groupContactBy(std::function<QString(contact_cptr)> broup_by_val)const;
        ard::contacts_model* m_cmodel{nullptr};
    };

    class ContactGroupsRoot : public RootTopic
    {
        friend class ard::contacts_model;
    public:
        DECLARE_ROOT(ContactGroupsRoot, objContactGroupRoot, ESingletonType::contactGroupsHolder);
        ~ContactGroupsRoot();
        QString                     title()const override { return "ContactGroups"; };
        CONTACT_GROUP_LIST          groups();
        CONTACT_CGROUP_LIST         groups()const;
        ard::contact_group*       addGroup(QString name);
        ///list of groups with contacts
        TOPICS_LIST getAsGroupListModel(std::set<contact_group*>* gfilter = nullptr);

		contact_group* allGroupMarker();

        ard::contacts_model*    cmodel() {return m_cmodel;}
    protected:
        ard::contacts_model*	m_cmodel{ nullptr };
		contact_group*			m_all_groups_marker{nullptr};
    };


    class contact : public ard::topic {
        friend class ContactsRoot;
        friend class ArdiCsv;
    public:

        contact();

        contact_ext*           cext();
        const contact_ext*     cext()const;
        contact_ext*           ensureCExt();
        QString                 objName()const override;

        QString     contactFamilyName()const;
        QString     contactGivenName()const;
        QString     contactFullName()const;
        QString     contactEmail()const;
        QString     contactPhone()const;
        QString     contactAddress()const;
        QString     contactOrganization()const;
        QString     contactNotes()const;

        QString     title()const override;
        QString     impliedTitle()const override;
        bool        canRename()const override { return true; }

        std::set<QString> groupsMember()const;
        bool        isMemberOfGroup(QString groupId)const;

        FieldParts          fieldValues(EColumnType column_type, QString type_label)const override;
        QString             fieldMergedValue(EColumnType column_type, QString type_label)const override;
        void                setFieldValues(EColumnType column_type, QString type_label, const FieldParts& parts)override;
        EColumnType         formNotesColumn()const override{ return EColumnType::ContactNotes; }
        EColumnType         treatFieldEditorRequest(EColumnType column_type)const override;
        TOPICS_LIST         produceFormTopics(std::set<EColumnType>* include_columns = nullptr,
                            std::set<EColumnType>* exclude_columns = nullptr)override;
        bool                hasText4SearchFilter(const TextFilterContext& fc)const override;

        bool        canBeMemberOf(const topic_ptr)const override;
        static contact*  createNewContact(QString firstName, QString lastName, std::vector<QString>* groups = nullptr);

        EOBJ                otype()const override { return objContact; };
        cit_prim_ptr        create()const override;
        ENoteView           noteViewType()const override{ return ENoteView::None; };
        void                fatFingerSelect()override;
        bool                hasFatFinger()const override;
        bool                isEmptyTopic()const override { return false; }
        QSize               calcBlackboardTextBoxSize()const override;
    protected:
        static contact*  createNewContact(std::unique_ptr<googleQt::gcontact::ContactInfo>&& c);
        void mapExtensionVar(cit_extension_ptr e)override;

        contact_ext*       m_cext{ nullptr };
    };

    class contact_group : public ard::topic {
        friend class ContactsRoot;
    public:
		contact_group(QString title = "");

        bool isStandardLocusable()const override { return false; };
        static contact_group*	createNewGroup(QString name);
        QString                 objName()const override;

        bool                canBeMemberOf(const topic_ptr)const override;           
        EOBJ                otype()const override { return objContactGroup; };
        cit_prim_ptr        create()const override;
        ENoteView           noteViewType()const override{ return ENoteView::None; };
        bool                isEmptyTopic()const override { return false; }
        CITEMS              prepareBBoardInjectSubItems()override;
		EColor				colorIndex()const override { return ard::EColor::none; };
		bool				hasCurrentCommand(ECurrentCommand c)const override;
    };

    /**
    anQExt - contains details on gmail query filter
    */
    class contact_ext : public ArdPimplWrapper<ardiExtension<contact_ext, ard::contact>, googleQt::gcontact::ContactInfo>
    {
        friend class contact;
        friend class ContactsRoot;

        DECLARE_DB_EXTENSION_PERSISTANT(snc::EOBJ_EXT::extContact, "contact-ext", "ard_ext_contact");
    public:
        ///default constructor
        contact_ext();
        ///for recovering from DB
        contact_ext(contact* _owner, QSqlQuery& q);

        QString             toXml()const;
        bool                isAtomicIdenticalTo(const cit_primitive* other, int& iflags)const override;
        void                assignSyncAtomicContent(const cit_primitive* other)override;
        snc::cit_primitive* create()const override;
        QString             calcContentHashString()const override;
        uint64_t            contentSize()const override;
        void                setGroupMembership(const std::vector<QString>& groups);
    };

    /**
    can have user name and email or just email address
    */
    struct DisplayEmailAddress
    {
        DisplayEmailAddress() {}
        DisplayEmailAddress(QString name, QString email) :display_name(name), email_addr(email) {}

        QString     display_name;
        QString     email_addr;

        QString     name()const { if (!display_name.isEmpty())return display_name; return  email_addr; }
    };

    /**
    used in contact lokoup, if 'contact' is null, see value in email_addr
    */
    struct EitherContactOrEmail
    {
        EitherContactOrEmail(contact* c) :contact1(c) {}
        EitherContactOrEmail(DisplayEmailAddress a) :display_email(a) {}
        EitherContactOrEmail(QString name, QString email) :display_email(name, email) {}

        contact*                contact1{ nullptr };
        DisplayEmailAddress     display_email;

        bool                    isValid()const;
        QString                 toEmailAddress()const;
    };

    using LOOKUP_LIST = std::vector<EitherContactOrEmail>;
    struct ContactsLookup
    {
        LOOKUP_LIST to_list;
        LOOKUP_LIST cc_list;
        bool        lookup_succeeded{ false };
		bool		include_attached4forward{ false };
        static QString  toAddressStr(const LOOKUP_LIST& lst);
    };
};

#endif //ARD_GD

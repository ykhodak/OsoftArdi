#include <QFontMetrics>
#include "a-db-utils.h"
#include "contact.h"
#include "email.h"
#include "tooltopic.h"
#include "ansyncdb.h"


#define RETURN_ON_EXT(F) auto e = cext(); QString rv;if (e) {rv = e->optr()->F;}return rv;

#define TAKE_PRIMARY(P, F, R)  {auto idx = P.findPrimary();\
                            if(idx == -1 && !P.items().empty()) idx = 0;\
                            if (idx != -1) {\
                                auto entry = P.items()[idx];    \
                                R = entry.F();\
                            }}\

#define FIND_PRIM_OR_FIRST(C, I) I = C.findPrimary();if(C.items().size() != 0){ I=0; }


IMPLEMENT_ROOT(ard, ContactsRoot, "Contacts", ard::contact);
IMPLEMENT_ROOT(ard, ContactGroupsRoot, "ContactGroups", ard::contact_group);

ard::contacts_model::contacts_model(ArdDB* db)
{
    m_contacts_root = new ard::ContactsRoot(db);
    m_contact_groups_root = new ard::ContactGroupsRoot(db);
    m_contacts_root->m_cmodel = this;
    m_contact_groups_root->m_cmodel = this;
};

ard::contacts_model::~contacts_model() 
{
	if (m_contacts_root)
		m_contacts_root->release();
	if (m_contact_groups_root)
		m_contact_groups_root->release();
};

ard::ContactsRoot*      ard::contacts_model::croot() { return m_contacts_root; }
ard::ContactGroupsRoot* ard::contacts_model::groot() { return m_contact_groups_root; }
const ard::ContactsRoot*       ard::contacts_model::croot()const { return m_contacts_root; }
const ard::ContactGroupsRoot*  ard::contacts_model::groot()const { return m_contact_groups_root; }

std::pair<int, int> ard::contacts_model::mergeContacts(const ard::contacts_model& _other)
{
    std::pair<int, int> rv;

    ard::contacts_model& otherNonConst = (ard::contacts_model&)_other;

    auto clst   = otherNonConst.croot()->itemsAs<contact>();
    auto glst   = otherNonConst.groot()->itemsAs<contact_group>();
    auto s2g    = ardi_functional::toSyidMap(glst);

    auto res    = croot()->mergeContacts(clst, s2g);
    rv = storeMergedContacts(res);
    return rv;
};

std::pair<int, int> ard::contacts_model::storeMergedContacts(ard::contacts_merge_result res)
{
    std::pair<int, int> rv;
    rv.first = res.contacts.size();
    rv.second = res.skipped_existing_contacts + res.skipped_tmp_contacts;
    auto cr = croot();
    auto gr = groot();

    if (rv.first > 0) {
        for (auto c : res.contacts) {
            if (c->isTmpSelected()) {
                cr->addItem(c);
            }
        }
        for (auto g : res.groups) {
            gr->addItem(g);
        }
        gr->ensurePersistant(1);
        for (auto g : res.groups)
        {
            ///@todo: don't really know why we are doing this
            if (g->isSyncDbAttached())
            {
                g->setSyncModified();
                g->ask4persistance(np_SYNC_INFO);
                g->ensurePersistant(1);
            }
        }
    }

    cr->ensurePersistant(1);
    gr->ensurePersistant(1);

    return rv;
};


/**
    ContactGroupsRoot
*/
ard::ContactGroupsRoot::~ContactGroupsRoot()
{
    if(m_all_groups_marker){
        m_all_groups_marker->release();
        m_all_groups_marker = nullptr;
    }
};

ard::contact_group* ard::ContactGroupsRoot::allGroupMarker()
{
    if(!m_all_groups_marker){
        m_all_groups_marker = new contact_group("All");
    }
    return m_all_groups_marker;
};

CONTACT_GROUP_LIST ard::ContactGroupsRoot::groups()
{
    ASSERT_VALID(this);
    CONTACT_GROUP_LIST rv = itemsAs<ard::contact_group>();
    return rv;
};

CONTACT_CGROUP_LIST ard::ContactGroupsRoot::groups()const
{
    ASSERT_VALID(this);
    CONTACT_CGROUP_LIST rv = itemsAs<ard::contact_group>();
    return rv;
};

ard::contact_group* ard::ContactGroupsRoot::addGroup(QString name)
{
    ASSERT_VALID(this);
    auto* rv = contact_group::createNewGroup(name);
    addItem(rv);
    ensurePersistant(1);
    if (isSyncDbAttached()) {
        //we have to do it after object serialized
        rv->setSyncModified();
        rv->ask4persistance(np_SYNC_INFO);
        rv->ensurePersistant(1);
    }
    return rv;
};

TOPICS_LIST ard::ContactGroupsRoot::getAsGroupListModel(std::set<contact_group*>* gfilter /*= nullptr*/)
{
    ASSERT_VALID(this);
    TOPICS_LIST result_topic_list;
    if (!ard::isDbConnected()) {
        ASSERT(0, "no DB");
        return result_topic_list;
    }

	DB_ID_TYPE lastExpandedGroup = 0;//dbp::configLastSelectedContactGroupOID();

    using ID_2_GSHORTCUT = std::map<QString, ard::shortcut*>;
    ID_2_GSHORTCUT id2g_shorcut;

    auto gr_list = items();
    for (auto t : gr_list) {
		auto g = dynamic_cast<contact_group*>(t);
        ASSERT(g, "expected group");
        if (g) {
            if (gfilter) {
                auto it = gfilter->find(g);
                if (it == gfilter->end())
                    continue;
            }

            auto sh = new ard::shortcut(g);
            result_topic_list.push_back(sh);
            id2g_shorcut[g->syid()] = sh;

            if (lastExpandedGroup == g->underlyingDbid()) {
                sh->setExpanded(true);
            }
        }
    }

    auto c_list = m_cmodel->croot()->items();
    for (auto f : c_list) {
		auto c = dynamic_cast<ard::contact*>(f);
        auto gmem = c->groupsMember();
        for (auto gid : gmem) {
            auto j = id2g_shorcut.find(gid);
            if (j != id2g_shorcut.end()) {
                auto g_sh = j->second;
                auto c_sh = new ard::shortcut(c);
                g_sh->addItem(c_sh);
            }
        }
    }

    return result_topic_list;
};

/**
    ContactsRoot
*/
CONTACT_LIST ard::ContactsRoot::contacts()
{
    ASSERT_VALID(this);
    CONTACT_LIST rv = itemsAs<ard::contact>();
    return rv;
};

CCONTACT_LIST ard::ContactsRoot::contacts()const
{
    ASSERT_VALID(this);
    CCONTACT_LIST rv = itemsAs<ard::contact>();
    return rv;
};


TOPICS_LIST ard::ContactsRoot::filteredItems(QString groupFilterId, QString indexStr /*= "*"*/) 
{
    ASSERT_VALID(this);
    bool useIdx = (indexStr != "*" && !indexStr.isEmpty());
    TOPICS_LIST rv;
    auto c_list = items();
    for (auto& i : c_list)
    {
        auto c = dynamic_cast<ard::contact*>(i);

        bool addit = true;
        if (!groupFilterId.isEmpty()) {
            addit = c->isMemberOfGroup(groupFilterId);
        }
        if (addit && useIdx) {
            QString s = c->impliedTitle().left(1).toLower();
            if (s.isEmpty()) {
                addit = false;
            }
            else {
                addit = (indexStr.indexOf(s) != -1);
            }
        }

        if (!addit) {
            if (c == ard::ContactsRoot::hoistedContactInFilter()) {
                addit = true;
            }
        }

        if (addit) {
            rv.push_back(c);
        }
    }
    return rv;
};

ard::contact* ard::ContactsRoot::addContact(QString firstName, QString lastName, std::vector<QString>* groups)
{
    ASSERT_VALID(this);
    auto* rv = contact::createNewContact(firstName, lastName, groups);
    addItem(rv);
    ensurePersistant(1);
    return rv;
};

QString ard::ContactsRoot::trimPhoneNumber(QString s)
{
    s = s.remove(QRegExp("[^\\d]"));
    return s;
};

QString ard::ContactsRoot::trimEmail(QString s)
{
    s = s.toLower();
    return s;
};

bool ard::ContactsRoot::isTmpContactEmail(QString s)
{
    bool rv = false;
    auto idx = s.indexOf("@reply.linkedin.com");
    if (idx != -1) {
        auto s1 = s.mid(0, idx);
        if (s1.length() > 32) {
            rv = true;
        }
    }
    return rv;
};

void ard::ContactsRoot::setContactGroupMembership(ard::contact* c, const std::vector<QString>& groups)
{
    ASSERT_VALID(this);
    auto e = c->ensureCExt();
    if (e) {
        e->setGroupMembership(groups);
        e->ask4persistance(np_ATOMIC_CONTENT);
        e->ensureExtPersistant(dataDb());
    }
};


S_2_CSET ard::ContactsRoot::groupContactBy(std::function<QString(contact_cptr)> broup_by_val)const
{
    ASSERT_VALID(this);
    S_2_CSET rv;
    auto c_list = contacts();
    for (auto& c : c_list)
    {
        auto s = broup_by_val(c);
        if (!s.isEmpty()) {
            auto j = rv.find(s);
            if (j == rv.end()) {
                std::set<contact_cptr> res;
                res.insert(c);
                rv[s] = res;
            }
            else {
                j->second.insert(c);
            }
        }
    }
    return rv;
};

S_2_CSET ard::ContactsRoot::phoneGroupedContacts()const
{
    ASSERT_VALID(this);
    return groupContactBy([](contact_cptr c)
    {
        return ard::ContactsRoot::trimPhoneNumber(c->contactPhone());
    });
};

S_2_CSET ard::ContactsRoot::emailGroupedContacts()const
{
    ASSERT_VALID(this);
    return groupContactBy([](contact_cptr c)
    {
        return ard::ContactsRoot::trimEmail(c->contactEmail());
    });
};

S_2_CSET ard::ContactsRoot::familyNameGroupedContacts()const
{
    ASSERT_VALID(this);
    return groupContactBy([](contact_cptr c)
    {
        QString s = c->contactFamilyName().toLower().trimmed();
        if (s.isEmpty()) {
            s = c->contactFullName().toLower().trimmed();
        }
        return s;
    });
};

ard::contacts_merge_result ard::ContactsRoot::mergeContacts(const CONTACT_LIST& new_contacts, const GROUP_MAP& new_groups)const
{
    ASSERT_VALID(this);
    contacts_merge_result rv;

    if (!ard::db()) {
        return rv;
    }

    const ard::ContactGroupsRoot* gr = m_cmodel->groot();
    using CGROUP_MAP = std::unordered_map<QString, const contact_group*>;

    int imported_contacts = 0;
    CGROUP_MAP g_by_name;
    for (auto g : gr->groups()) {
        g_by_name[g->title().toLower()] = g;
    }

    auto c_byphone = phoneGroupedContacts();
    auto c_by_email = emailGroupedContacts();
    auto c_by_lastName = familyNameGroupedContacts();

    for (auto& nc : new_contacts) 
    {
        auto e = nc->ensureCExt();
        if (!e) {
            ASSERT(0, "expected contact extension");
            continue;
        }
        googleQt::gcontact::ContactInfo* optr = e->optr();
        if (!optr) {
            ASSERT(0, "expected contact private IMPL object");
            continue;
        }


        bool skip_contact = false;

        QString lastName, main_phone, main_email_address;
        lastName = nc->contactFamilyName().toLower().trimmed();
        if (lastName.isEmpty()) {
            lastName = nc->contactFullName().toLower().trimmed();
        }
        TAKE_PRIMARY(optr->phones(), number, main_phone);
        main_phone = ard::ContactsRoot::trimPhoneNumber(main_phone);
        TAKE_PRIMARY(optr->emails(), address, main_email_address);
        main_email_address = ard::ContactsRoot::trimEmail(main_email_address);


        auto it_last = c_by_lastName.find(lastName);
        if (it_last != c_by_lastName.end()) 
        {
            /// if last names are same and phone # or email same we consider contacts identical
            if (!main_phone.isEmpty()) {
                auto j = c_byphone.find(main_phone);
                if (j != c_byphone.end()) {
                    for (auto k : it_last->second) {
                        if (j->second.find(k) != j->second.end()) {
                            //qDebug() << "merge/contact with same last-name and phone skipped" << lastName << main_phone;
                            skip_contact = true;
                            continue;
                        }
                    }
                }
            }

            if (!main_email_address.isEmpty()) {
                auto j = c_by_email.find(main_email_address);
                if (j != c_by_email.end()) {
                    for (auto k : it_last->second) {
                        if (j->second.find(k) != j->second.end()) {
                            //qDebug() << "merge/contact with same last-name and email skipped" << lastName << main_email_address;
                            skip_contact = true;
                            continue;
                        }
                    }
                }
            }
            //}//email

            if (skip_contact) {
                rv.skipped_existing_contacts++;
                continue;
            }

            if (main_email_address.isEmpty() &&
                main_phone.isEmpty())
            {
                /// see if we can compare more data
                QString addr;// = nc->contactAddress();
                QString org;// = nc->contactOrganization();
                QString content;
                TAKE_PRIMARY(optr->addresses(), formattedAddress, addr);
                addr = addr.trimmed();
                org = optr->organization().name().trimmed();
                content = optr->content().trimmed();
                for (auto k : it_last->second) {
                    auto our_c = k;
                    if (our_c->contactAddress().compare(addr) == 0 &&
                        our_c->contactOrganization().compare(org) == 0 &&
                        our_c->contactNotes().compare(content) == 0)
                    {
                        skip_contact = true;
                        continue;
                    }
                }//last loop
            }//empty email&phone


            if (skip_contact) {
                rv.skipped_existing_contacts++;
                continue;
            }
        }//last name
        else 
        {
            if (lastName.isEmpty()) 
            {
                if (!main_email_address.isEmpty()) {
                    auto j = c_by_email.find(main_email_address);
                    if (j != c_by_email.end()) {
                        //qDebug() << "merge/contact with empty last-name and same email skipped" << lastName << main_email_address;
                        skip_contact = true;
                        continue;
                    }
                }

                if (!main_phone.isEmpty()) {
                    auto j = c_byphone.find(main_phone);
                    if (j != c_byphone.end()) {
                        //qDebug() << "merge/contact with empty last-name and same phone skipped" << lastName << main_phone;
                        skip_contact = true;
                        continue;
                    }
                }
            }

            if (skip_contact) {
                rv.skipped_existing_contacts++;
                continue;
            }
        }

        if (!main_email_address.isEmpty()) 
        {
            if (ard::ContactsRoot::isTmpContactEmail(main_email_address)) 
            {
                //qDebug() << "skipped tmp contact" << main_email_address;
                rv.skipped_tmp_contacts++;
                continue;
            }
        }

 //       if (lastName.isEmpty() && main_email_address.isEmpty() && main_phone.isEmpty()) {
 //           qDebug() << "possibly empty contact..";
 //       }

        auto o2 = optr->clone();
        auto c = contact::createNewContact(std::move(o2));
        rv.contacts.push_back(c);
        c->setTmpSelected(true);

        //qDebug() << "merge/contact accepted" << lastName << main_email_address << main_phone;

        //...
        std::vector<QString> imported_groups;
        ///make sure we have all the groups
        auto n_groups = nc->groupsMember();
        for (auto& ng : n_groups) 
        {
            auto j = new_groups.find(ng);
            if (j != new_groups.end()) 
            {
                auto& g2 = j->second;
                auto new_group_name = g2->title();
                auto k = g_by_name.find(new_group_name.toLower());
                if (k == g_by_name.end()) 
                {
                    auto new_g = contact_group::createNewGroup(new_group_name);
                    rv.groups.push_back(new_g);
                    //ykh-need-code
                    /*
                    gr->addItem(new_g);
                    gr->ensurePersistant(1);
                    if (new_g->isSyncDbAttached()) {
                        new_g->setSyncModified();
                        new_g->ask4persistance(np_SYNC_INFO);
                        new_g->ensurePersistant(1);
                    }
                    */
                    g_by_name[new_group_name.toLower()] = new_g;
                    imported_groups.push_back(new_g->syid());
                    //qDebug() << "merge/contact group created" << new_group_name << new_g->syid();
                }
                else {
                    imported_groups.push_back(k->second->syid());
                }
            }
            else {
                ASSERT(0, "failed to locate group by id in imported contacts") << ng;
            }
        }//for groups
        auto c_ext = c->ensureCExt();
        if (!imported_groups.empty()) {
            c_ext->setGroupMembership(imported_groups);
        }
        imported_contacts++;
    }//for


    return rv;
};

void ard::ContactsRoot::enrichLookupListFromCache(ard::ContactsLookup& res_lst)
{
    ASSERT_VALID(this);
    //auto cache = ard::contacts_cache();
    //assert_return_void(cache, "expected gcache");
    //auto cl = cache->contacts();
    using MAIL_MAP = std::map<QString, contact*>;
    MAIL_MAP mm;
    auto c_list = items();
    for (auto& i : c_list) {
        auto c = dynamic_cast<contact*>(i);
        //if (c->optr()->isRemoved() || c->optr()->isRetired())
        //  continue;

        QString email_addr = c->contactEmail().toUpper();
        mm[email_addr] = c;
    }

    std::function<void(ard::LOOKUP_LIST&)> lookup_function = [&mm](ard::LOOKUP_LIST& lst)
    {
        for (auto& c : lst) {
            if (!c.contact1) {
                QString s1 = c.display_email.email_addr.trimmed().toUpper();
                auto i = mm.find(s1);
                if (i != mm.end()) {
                    c.contact1 = i->second;
                }
            }
        }
    };

    lookup_function(res_lst.to_list);
    lookup_function(res_lst.cc_list);
};

ard::contact* _hoistedContactInFilter = nullptr;

ard::contact* ard::ContactsRoot::hoistedContactInFilter()
{
    return _hoistedContactInFilter;
};

void ard::ContactsRoot::setHoistedContactInFilter(contact* c)
{
    _hoistedContactInFilter = c;
};

/**
    ard::contact
*/
ard::contact::contact()
{
    
};

cit_prim_ptr ard::contact::create()const
{
    return new contact;
};

void ard::contact::fatFingerSelect()
{
};

bool ard::contact::hasFatFinger()const 
{
    auto rv = false;
#ifndef ARD_BIG
    rv = true;
#endif
    return rv;
};

ard::contact* ard::contact::createNewContact(QString firstName, QString lastName, std::vector<QString>* groups /*= nullptr*/)
{
    auto rv = new ard::contact;
    auto e = rv->ensureCExt();

    googleQt::gcontact::NameInfo n;
    n.setGivenName(firstName);
    n.setFamilyName(lastName);
    n.setFullName(firstName + " " + lastName);
    if (e) {
        e->optr()->setName(n);
        e->optr()->setTitle(n.fullName());
        if (groups) {
            e->optr()->setGroups(dbp::configEmailUserId(), *groups);
        }
    }

    return rv;
};

ard::contact* ard::contact::createNewContact(std::unique_ptr<googleQt::gcontact::ContactInfo>&& c)
{
    auto rv = new ard::contact;
    auto e = rv->ensureCExt();
    e->reset_pimpl_obj(std::move(c));
    return rv;
};

void ard::contact::mapExtensionVar(cit_extension_ptr e)
{
    ard::topic::mapExtensionVar(e);
    if (e->ext_type() == snc::EOBJ_EXT::extContact) {
        ASSERT(!m_cext, "duplicate contact ext");
        m_cext = dynamic_cast<contact_ext*>(e);
        ASSERT(m_cext, "expected contact ext");
    }
};


ard::contact_ext* ard::contact::ensureCExt()
{
    ASSERT_VALID(this);
    if (m_cext)
        return m_cext;

    ensureExtension(m_cext);
    return m_cext;
};

ard::contact_ext* ard::contact::cext()
{
    return ensureCExt();
};

const ard::contact_ext* ard::contact::cext()const
{
    ard::contact* ThisNonCost = (ard::contact*)this;
    return ThisNonCost->ensureCExt();
};

QString ard::contact::objName()const 
{
    return "contact";
};

QString ard::contact::contactFamilyName()const
{
    RETURN_ON_EXT(name().familyName());
};

QString ard::contact::contactGivenName()const 
{
    RETURN_ON_EXT(name().givenName());
};

QString ard::contact::contactFullName()const
{
    RETURN_ON_EXT(name().fullName());
};

QString ard::contact::contactEmail()const
{
    ASSERT_VALID(this);
    QString rv;
    auto e = cext();
    if (e) {
        TAKE_PRIMARY(e->optr()->emails(), address, rv);
    }
    return rv;
};

QString ard::contact::contactPhone()const
{
    ASSERT_VALID(this);
    QString rv;
    auto e = cext();
    if (e) {
        TAKE_PRIMARY(e->optr()->phones(), number, rv);
    }
    return rv;
};

QString ard::contact::contactAddress()const
{
    ASSERT_VALID(this);
    QString rv;
    auto e = cext();
    if (e) {
        TAKE_PRIMARY(e->optr()->addresses(), formattedAddress, rv);
    }
    return rv;
};

QString ard::contact::contactOrganization()const
{
    QString rv;
    auto e = cext();
    if (e) {
        auto o = m_cext->optr()->organization();
        rv = o.name();// +" " + o.title();
        if (!rv.isEmpty()) {
            QString org_title = o.title();
            if (!org_title.isEmpty()) {
                rv += ", ";
                rv += org_title;
            }
        }
    }
    return rv;
};

QString ard::contact::contactNotes()const
{
    RETURN_ON_EXT(content());
};


QString ard::contact::title()const
{
    RETURN_ON_EXT(title());
};

QString ard::contact::impliedTitle()const
{
    ASSERT_VALID(this);
    QString rv = title();
    if (!rv.isEmpty())
        return rv;

    auto e = cext();
    if (e) { 
        rv = contactEmail();
    }
    return rv;
};

std::set<QString> ard::contact::groupsMember()const
{
    ASSERT_VALID(this);
    std::set<QString> rv;
    auto e = cext();
    if (e) {
        auto gm = e->optr()->groups();
        for (auto& m : gm.items()) {
            rv.insert(m.groupId());
        }
    }
    return rv;
};

bool ard::contact::isMemberOfGroup(QString groupId)const
{
    ASSERT_VALID(this);
    auto e = cext();
    if (e) {
        auto gm = e->optr()->groups();
        for (auto& m : gm.items()) {
            if (groupId.compare(m.groupId()) == 0) {
                return true;
            }
        }
    }
    return false;
};

FieldParts ard::contact::fieldValues(EColumnType column_type, QString type_label)const
{
    ASSERT_VALID(this);
    FieldParts rv;
    if (m_cext) {
        auto o = m_cext->optr();
        if (!o) {
            ASSERT(0, "expected PIMPL on contact/1");
            return rv;
        }

        switch (column_type) {
        case EColumnType::ContactTitle:
        {
            auto n = o->name();
            rv.add(FieldParts::FirstName, n.givenName());
            rv.add(FieldParts::LastName, n.familyName());
        }break;
        case EColumnType::ContactEmail:
        {
            if (type_label.isEmpty()) {
                rv.add(FieldParts::Email, contactEmail());
            }
            else {
                auto elst = o->emails();
                auto idx = elst.findByLabel(type_label);
                if (idx != -1) {
                    const auto& e = elst.items()[idx];
                    rv.add(FieldParts::Email, e.address());
                }
            }
        }break;
        case EColumnType::ContactPhone:
        {
            if (type_label.isEmpty()) {
                rv.add(FieldParts::Phone, contactPhone());
            }
            else {
                auto plst = o->phones();
                auto idx = plst.findByLabel(type_label);
                if (idx != -1) {
                    const auto& p = plst.items()[idx];
                    rv.add(FieldParts::Phone, p.number());
                }
            }
        }break;
        case EColumnType::ContactAddress:
        {
            auto idx = o->addresses().findPrimary();
            if (idx == -1) {
                googleQt::gcontact::PostalAddress addr;
                addr.setPrimary(true);
                o->addAddress(addr);
                o->markAsModified();
            }
            idx = o->addresses().findPrimary();
            if (idx != -1) {
                const googleQt::gcontact::PostalAddress& a = o->addresses().items()[idx];
                rv.add(FieldParts::AddrStreet, a.street());
                rv.add(FieldParts::AddrCity, a.city());
                rv.add(FieldParts::AddrRegion, a.region());
                rv.add(FieldParts::AddrZip, a.postcode());
                rv.add(FieldParts::AddrCountry, a.country());
            }
        }break;
        case EColumnType::ContactOrganization:
        {
            const googleQt::gcontact::OrganizationInfo& o2 = o->organization();
            rv.add(FieldParts::OrganizationName, o2.name());
            rv.add(FieldParts::OrganizationTitle, o2.title());
        }break;
        case EColumnType::ContactNotes:
        {
            rv.add(FieldParts::Notes, o->content());
        }break;
        default:
        {
            ASSERT(0, "Not handled field") << (int)column_type;
        }break;
        }
    }
    return rv;
};

QString ard::contact::fieldMergedValue(EColumnType column_type, QString type_label)const
{
    ASSERT_VALID(this);
    QString rv;
    if (m_cext) {
        auto o = m_cext->optr();
		assert_return_empty(o, "expected PIMPL on contact/2");
        switch (column_type)
        {
        case EColumnType::ContactTitle:
        {
            rv = title();
        }break;
        case EColumnType::ContactEmail:
        {
            if (type_label.isEmpty()) {
                rv = contactEmail();
            }
            else {
                auto elst = o->emails();
                auto idx = elst.findByLabel(type_label);
                if (idx != -1) {
                    const auto& e = elst.items()[idx];
                    rv = e.address();
                }
            }
        }break;
        case EColumnType::ContactPhone:
        {
            if (type_label.isEmpty()) {
                rv = contactPhone();
            }
            else {
                auto plst = o->phones();
                auto idx = plst.findByLabel(type_label);
                if (idx != -1) {
                    const auto& e = plst.items()[idx];
                    rv = e.number();
                }
            }
        }break;
        case EColumnType::ContactAddress:
        {
            rv = contactAddress();
        }break;
        case EColumnType::ContactOrganization:
        {
            rv = contactOrganization();
        }break;
        case EColumnType::ContactNotes:
        {
            rv = contactNotes();
        }break;
        default:ASSERT(0, QString("Unsupported column %1").arg(static_cast<int>(column_type)));
        }
    }
    return rv;
};

void ard::contact::setFieldValues(EColumnType column_type, QString type_label, const FieldParts& fp)
{
    ASSERT_VALID(this);
#define VALIDATE_PART_TYPE(I, T) assert_return_void((fp.parts()[I].first == FieldParts::T), QString("expected %1 part for email %2").arg(#T).arg(static_cast<int>(fp.parts()[I].first)));

#define SET_PART_VALUE(I, G, S)    str = fp.parts()[I].second.trimmed();    \
    if (str.compare(p.G()) != 0) {             \
        p.S(str);                              \
        o->markAsModified();                  \
    }                                           \

    QString str = "";

    if (m_cext) {
        auto o = m_cext->optr();
        assert_return_void(o, "expected PIMPL on contact/3");
        switch (column_type)
        {
        case EColumnType::ContactTitle:
        {
            assert_return_void((fp.parts().size() == 2), QString("expected two part for contact title %1").arg(fp.parts().size()));
            VALIDATE_PART_TYPE(0, FirstName);
            VALIDATE_PART_TYPE(1, LastName);

            googleQt::gcontact::NameInfo& n = o->nameRef();
            n.setGivenName(fp.parts()[0].second);
            n.setFamilyName(fp.parts()[1].second);
            n.setFullName(n.givenName() + " " + n.familyName());
            o->markAsModified();
            o->setTitle(n.fullName());
        }break;
        case EColumnType::ContactEmail:
        {
            assert_return_void((fp.parts().size() == 1), QString("expected one part for contact email %1").arg(fp.parts().size()));
            VALIDATE_PART_TYPE(0, Email);

            googleQt::gcontact::EmailInfoList& lst = o->emailsRef();
            if (lst.items().size() == 0) {
                googleQt::gcontact::EmailInfo e;
                o->addEmail(e);
                lst = o->emailsRef();
            }

            QString email_address = fp.parts()[0].second.trimmed();

            if (lst.items().size() > 0) {
                //..
                bool locatedByLabel = false;
                if (!type_label.isEmpty()) {
                    auto idx = lst.findByLabel(type_label);
                    if (idx != -1) {
                        auto& e = lst.items()[idx];
                        e.setAddress(email_address);
                        o->markAsModified();
                        locatedByLabel = true;
                    }
                }
                if (!locatedByLabel) {
                    auto idx = -1;
                    FIND_PRIM_OR_FIRST(lst, idx);
                    if (idx != -1) {
                        auto& p = lst.items()[idx];
                        SET_PART_VALUE(0, address, setAddress);
                        if (!type_label.isEmpty()) {
                            p.setTypeLabel(type_label);
                        }
                    }
                }

                /*
                if (type_label.isEmpty()) {
                    auto idx = -1;
                    FIND_PRIM_OR_FIRST(lst, idx);
                    if (idx != -1) {
                        auto& p = lst.items()[idx];
                        SET_PART_VALUE(0, address, setAddress);
                    }
                }
                else {
                    //..
                    auto idx = lst.findByLabel(type_label);
                    if (idx != -1) {
                        auto& e = lst.items()[idx];
                        e.setAddress(email_address);
                        o->markAsModified();
                    }
                }*/
            }
        }break;

        case EColumnType::ContactPhone:
        {
            assert_return_void((fp.parts().size() == 1), QString("expected one part for contact phone %1").arg(fp.parts().size()));
            VALIDATE_PART_TYPE(0, Phone);

            googleQt::gcontact::PhoneInfoList& lst = o->phonesRef();
            if (lst.items().size() == 0) {
                googleQt::gcontact::PhoneInfo ph;
                o->addPhone(ph);
                lst = o->phonesRef();
            }

            QString phone_number = fp.parts()[0].second.trimmed();

            if (lst.items().size() > 0) {
                bool locatedByLabel = false;
                if (!type_label.isEmpty()) {
                    auto idx = lst.findByLabel(type_label);
                    if (idx != -1) {
                        auto& e = lst.items()[idx];
                        e.setNumber(phone_number);
                        o->markAsModified();
                        locatedByLabel = true;
                    }
                }
                if (!locatedByLabel) {
                    auto idx = -1;
                    FIND_PRIM_OR_FIRST(lst, idx);
                    if (idx != -1) {
                        auto& p = lst.items()[idx];
                        SET_PART_VALUE(0, number, setNumber);
                        if (!type_label.isEmpty()) {
                            p.setTypeLabel(type_label);
                        }
                    }
                }

                //...
                /*
                if (type_label.isEmpty()) {
                    auto idx = -1;
                    FIND_PRIM_OR_FIRST(lst, idx);
                    if (idx != -1) {
                        auto& p = lst.items()[idx];
                        SET_PART_VALUE(0, number, setNumber);
                    }
                }
                else {
                    auto idx = lst.findByLabel(type_label);
                    if (idx != -1) {
                        auto& e = lst.items()[idx];
                        e.setNumber(phone_number);
                        o->markAsModified();
                    }
                }
                */
            }
        }break;

        case EColumnType::ContactAddress:
        {
            assert_return_void((fp.parts().size() == 5), QString("expected 5 parts for contact address %1").arg(fp.parts().size()));
            VALIDATE_PART_TYPE(0, AddrStreet);
            VALIDATE_PART_TYPE(1, AddrCity);
            VALIDATE_PART_TYPE(2, AddrRegion);
            VALIDATE_PART_TYPE(3, AddrZip);
            VALIDATE_PART_TYPE(4, AddrCountry);

            googleQt::gcontact::PostalAddressList& lst = o->addressesRef();
            if (lst.items().size() == 0) {
                googleQt::gcontact::PostalAddress a;
                o->addAddress(a);
                lst = o->addressesRef();
            }

            if (lst.items().size() > 0) {
                auto idx = -1;
                FIND_PRIM_OR_FIRST(lst, idx);
                if (idx != -1) {
                    auto& p = lst.items()[idx];
                    SET_PART_VALUE(0, street, setStreet);
                    SET_PART_VALUE(1, city, setCity);
                    SET_PART_VALUE(2, region, setRegion);
                    SET_PART_VALUE(3, postcode, setPostcode);
                    SET_PART_VALUE(4, country, setCountry);
                    o->markAsModified();

                    QString s = p.street();
#define ADD_FPART(N)  if (!p.N().isEmpty()) {\
                    if (!s.isEmpty()) {\
                        s += ", ";\
                    }\
                    s += p.N();\
                    }\

                    ADD_FPART(city);
                    ADD_FPART(region);
                    ADD_FPART(postcode);
                    ADD_FPART(country);
#undef ADD_FPART

                    p.setFormattedAddress(s);
                }
            }

        }break;
        case EColumnType::ContactOrganization:
        {
            assert_return_void((fp.parts().size() == 2), QString("expected two part for contact organization %1").arg(fp.parts().size()));
            VALIDATE_PART_TYPE(0, OrganizationName);
            VALIDATE_PART_TYPE(1, OrganizationTitle);
            googleQt::gcontact::OrganizationInfo& o2 = o->organizationRef();
            o2.setName(fp.parts()[0].second);
            o2.setTitle(fp.parts()[1].second);
            o->markAsModified();
        }break;
        //...
        case EColumnType::ContactNotes:
        {
            assert_return_void((fp.parts().size() == 1), QString("expected one part for contact notes %1").arg(fp.parts().size()));
            VALIDATE_PART_TYPE(0, Notes);
            o->setContent(fp.parts()[0].second);
            o->markAsModified();
        }break;

        //....

        default:ASSERT(0, QString("Unsupported column type '%1'").arg(static_cast<int>(column_type)));
        }//switch (column_type)

        if (isSyncDbAttached()) {
            m_cext->setSyncModified();
            m_cext->ask4persistance(np_SYNC_INFO);
        }
        m_cext->ask4persistance(np_ATOMIC_CONTENT);
        m_cext->ensureExtPersistant(dataDb());
    }

#undef SET_PART_VALUE
#undef VALIDATE_PART_TYPE
};

EColumnType ard::contact::treatFieldEditorRequest(EColumnType column_type)const
{
    EColumnType rv = column_type;
    if (column_type == EColumnType::Title) {
        rv = EColumnType::ContactTitle;
    }
    return rv;
};


TOPICS_LIST ard::contact::produceFormTopics(std::set<EColumnType>* include_columns /*= nullptr*/,
    std::set<EColumnType>* exclude_columns /*= nullptr*/)
{
    ASSERT_VALID(this);
    ///
    /// we take all column from include_columns, except for listed in exclude_columns
    /// if include_columns is NULL we take all default except for listed in exclude_columns
    ///

    using COL_TYPE_LIST = std::vector<EColumnType>;
    static COL_TYPE_LIST default_columns;
    if (default_columns.empty()) {///do it one in program lifetime
        default_columns.push_back(EColumnType::ContactTitle);
        default_columns.push_back(EColumnType::ContactEmail);
        default_columns.push_back(EColumnType::ContactPhone);
        default_columns.push_back(EColumnType::ContactAddress);
        default_columns.push_back(EColumnType::ContactOrganization);
        default_columns.push_back(EColumnType::ContactNotes);
    }

#define PUSH_FIELD(C, T, L) lst.push_back(new FormFieldTopic(C, T, L));
#define PUSH_ALL_LABELED_FIELDS(L) \
for (auto& p : L.items()) {\
    if (p.isPrimary()) {\
        PUSH_FIELD(c, ct, p.typeLabel());\
    }\
}\
for (auto& p : L.items()) {\
    if (!p.isPrimary()) {\
        PUSH_FIELD(c, ct, p.typeLabel());\
    }\
}\


    std::function<void(ard::contact*, EColumnType, TOPICS_LIST&)> addFieldWithLabes = [](ard::contact* c, EColumnType ct, TOPICS_LIST& lst)
    {
        switch (ct) {
        case EColumnType::ContactPhone:
        {
            auto o = c->m_cext->optr();
            if (o) {
                auto plst = o->phones();
                PUSH_ALL_LABELED_FIELDS(plst);
            }
        }break;
        case EColumnType::ContactEmail:
        {
            auto o = c->m_cext->optr();
            if (o) {
                auto plst = o->emails();
                PUSH_ALL_LABELED_FIELDS(plst);
            }
        }break;
        default:
        {
            PUSH_FIELD(c, ct, "");
        }
        }
    };

#undef PUSH_FIELD
#undef PUSH_ALL_LABELED_FIELDS

    TOPICS_LIST lst;

    if (m_cext) {
        if (include_columns) {
            for (auto c : *include_columns) {
                if (!exclude_columns || (exclude_columns && exclude_columns->find(c) == exclude_columns->end())) {
                    addFieldWithLabes(this, c, lst);
                }
            }
        }
        else {
            for (auto c : default_columns) {
                if (!exclude_columns || (exclude_columns && exclude_columns->find(c) == exclude_columns->end())) {
                    addFieldWithLabes(this, c, lst);
                }
            }
        }
    }

    return lst;
};

bool ard::contact::canBeMemberOf(const topic_ptr f)const
{
    ASSERT_VALID(this);
    bool rv = false;
    if (dynamic_cast<const ard::ContactsRoot*>(f) != nullptr) {
        rv = true;
    }
    return rv;
};

QSize ard::contact::calcBlackboardTextBoxSize()const
{
    int lineH = gui::shortLineHeight();
    auto email = contactEmail();
    if (!email.isEmpty()) {
        lineH += gui::shortLineHeight();
    }

    auto phone = contactPhone();
    if (!phone.isEmpty()) {
        lineH += gui::shortLineHeight();
    }

    QSize rv{ DEFAULT_MAIN_WND_WIDTH, lineH };

    return rv;
};

bool ard::contact::hasText4SearchFilter(const TextFilterContext& fc)const 
{
    bool rv = ard::topic::hasText4SearchFilter(fc);
    if (!rv) 
    {
        if (m_cext) 
        {
            auto s = contactEmail();
            rv = (s.indexOf(fc.key_str, 0, Qt::CaseInsensitive) != -1);
        }
    }
    return rv;
};

/**
    contact_group
*/
ard::contact_group::contact_group(QString title) :ard::topic(title)
{

}

QString ard::contact_group::objName()const
{
    return "cgroup";
};


bool ard::contact_group::canBeMemberOf(const topic_ptr f)const
{
    bool rv = false;
    if (dynamic_cast<const ard::ContactGroupsRoot*>(f) != nullptr) {
        rv = true;
    }
    return rv;
};

ard::contact_group* ard::contact_group::createNewGroup(QString name)
{
    auto rv = new contact_group(name.trimmed());
    rv->setInLocusTab(true);
    rv->m_syid = "ard" + cit_base::make_new_syid(1);
    return rv;
};

cit_prim_ptr ard::contact_group::create()const
{
    return new contact_group;
};

CITEMS ard::contact_group::prepareBBoardInjectSubItems()
{
    CITEMS rv;

    ContactGroupsRoot* gr = dynamic_cast<ContactGroupsRoot*>(parent());
    if (!gr) {
        ASSERT(0, "expected group root");
        return rv;
    }

    auto m = gr->cmodel();
    if (!gr) {
        ASSERT(0, "expected group model");
        return rv;
    }

    auto c_list = m->croot()->items();
    for (auto f : c_list) 
    {
		auto c = dynamic_cast<ard::contact*>(f);
        auto gmem = c->groupsMember();
        for (auto gid : gmem) {
            if (gid == syid()) {
                rv.push_back(c);
            }
        }
    }
    return rv;
};

bool ard::contact_group::hasCurrentCommand(ECurrentCommand c)const 
{
	return (c == ECurrentCommand::cmdEdit || c == ECurrentCommand::cmdDelete);
};

/**
* contact_ext
*/
ard::contact_ext::contact_ext()
{
};

ard::contact_ext::contact_ext(ard::contact* _owner, QSqlQuery& q)
{
    attachOwner(_owner);

    auto xml = q.value(1).toString();
    if (!optr()->parseXml(xml)) {
        qWarning() << "failed to parse contact entry db record size="
            << xml.size()
            << "dbid=" << q.value(0).toInt();
        qWarning() << "-------------";
    }

    m_mod_counter = q.value(2).toInt();
    _owner->addExtension(this);
};


QString ard::contact_ext::toXml()const
{
    QString rv = optr()->toXml(dbp::configEmailUserId());
    return rv;
};

bool ard::contact_ext::isAtomicIdenticalTo(const cit_primitive* _other, int& )const
{
    assert_return_false(_other, "expected item [1]");
    auto other = dynamic_cast<const contact_ext*>(_other);
    assert_return_false(other, "expected item [2]");
    bool rv = (*optr() == *(other->optr()));
    return rv;
};

void ard::contact_ext::assignSyncAtomicContent(const cit_primitive* _other)
{
    assert_return_void(_other, QString("expected contact ext "));
    auto other = dynamic_cast<const contact_ext*>(_other);
    assert_return_void(other, QString("expected contact %1").arg(_other->dbgHint()));
    optr()->assignContent(*(other->optr()));
    ask4persistance(np_ATOMIC_CONTENT);
};

snc::cit_primitive* ard::contact_ext::create()const
{
    return new contact_ext;
};

QString ard::contact_ext::calcContentHashString()const
{
    QString tmp = optr()->toXml("4hash");
    QString rv = QCryptographicHash::hash(tmp.toUtf8(), QCryptographicHash::Md5).toHex();
    return rv;
};

uint64_t ard::contact_ext::contentSize()const
{
    return sizeof(googleQt::gcontact::ContactInfo);
};

void ard::contact_ext::setGroupMembership(const std::vector<QString>& groups) 
{
    std::vector<QString> groups_in;
    for (auto& m : optr()->groups().items()){
        groups_in.push_back(m.groupId());
    }

    bool v_equal = (groups_in.size() == groups.size());
    if (v_equal) {
        v_equal = std::equal(groups_in.begin(), groups_in.end(), groups.begin());
        if (v_equal) {
            qDebug() << "ignoring equal group lists" << groups.size();
            return;
        }
    }

//#ifdef _DEBUG
//    auto s = *(groups.begin());
//    qDebug() << "ykh-set-group" << s;
//#endif

    optr()->setGroups(dbp::configEmailUserId(), groups);
    if (hasDBRecord() && cit_owner())
    {
        setSyncModified();
        ask4persistance(np_SYNC_INFO);
        cit_owner()->ensurePersistant(1);
    }
};


/**
EitherContactOrEmail
*/
bool ard::EitherContactOrEmail::isValid()const
{
    if (contact1)
        return true;
    bool rv = (display_email.email_addr.indexOf("@") != -1);
    return rv;
};

QString ard::EitherContactOrEmail::toEmailAddress()const
{
    QString str_email;
    if (contact1) {
        if (contact1->title().isEmpty()) {
            str_email = contact1->contactEmail();
        }
        else {
            str_email = QString("\"%1\" <%2>")
                .arg(contact1->title())
                .arg(contact1->contactEmail());
        }
    }
    else {
        if (display_email.display_name.isEmpty()) {
            str_email = display_email.email_addr;
        }
        else {
            str_email = QString("\"%1\" <%2>")
                .arg(display_email.display_name)
                .arg(display_email.email_addr);
        }
    }
    return str_email;
};

QString ard::ContactsLookup::toAddressStr(const ard::LOOKUP_LIST& lst)
{
    QString rv;
    for (auto& s1 : lst)
    {
        auto s = s1.toEmailAddress();
        if (!s.isEmpty()) {
            rv += s;
            rv += ";";
        }
    }
    rv.remove(rv.size() - 1, 1);
    return rv;
};


;


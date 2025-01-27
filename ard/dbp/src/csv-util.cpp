#include <QFileDialog>
#include "a-db-utils.h"
#include "csv-util.h"
#include "contact.h"
#include "csv-parser.h"

ard::ArdiCsv::ArdiCsv() 
{

};

bool ard::ArdiCsv::loadCsv(QString fileName)
{
    m_columns.clear();
    m_rows.clear();
    m_name2column_index.clear();
    std::ifstream f(fileName.toStdString());
    if (!doLoadCsv(f)) {
        qWarning() << "error reading CSV file" << fileName;
        return false;
    }
    return true;;
};

bool ard::ArdiCsv::loadCsv(std::istream& input)
{
    m_columns.clear();
    m_rows.clear();
    m_name2column_index.clear();
    if (!doLoadCsv(input)) {
        qWarning() << "error reading CSV file";
        return false;
    }
    return true;;
};

bool ard::ArdiCsv::doLoadCsv(std::istream& f)
{
    try {
        aria::csv::CsvParser parser(f);
        bool headerLoaded = false;
        for (auto& row : parser) {
            if (!headerLoaded) 
            {
                if (!row.empty()) 
                {
                    if (m_name2column_index.empty() && m_columns.empty()) 
                    {
                        for (auto& field : row) {
                            QString s = field.c_str();
                            auto s2 = s.toLower();
                            auto it = m_name2column_index.find(s2);
                            ASSERT(it == m_name2column_index.end(), QString("duplicate column name detected [%1]").arg(s));
                            m_name2column_index[s2] = m_columns.size();
                            m_columns.push_back(s);
                        }
                    }
                    headerLoaded = true;
                }
            }
            else 
            {
                if (!row.empty())
                {
                    if (row.size() >= m_columns.size()) {
                        STRING_LIST r;
                        for (auto& field : row) {
                            QString s = field.c_str();
                            r.push_back(s);
                        }
                        m_rows.push_back(r);
                    }
                    else {
                        qWarning() << QString("skipped cvs row [%1/%2]").arg(m_columns.size()).arg(row.size());
                        //ASSERT(0, QString("skipped cvs row [%1/%2]").arg(m_columns.size()).arg(row.size()));
                    }
                }
            }
        }
    }
    catch (const std::ifstream::failure& e) {
        qWarning() << "ArdiCsv/Exception opening/reading file" << e.what();
        return false;
    }
    catch (const std::exception& e) {
        qWarning() << "ArdiCsv/Generic Exception" << e.what();
        return false;
    }

    return true;
};

bool ard::ArdiCsv::loadCsvFiles(QStringList files2import)
{
    for (auto& s : files2import)
    {
        std::ifstream f(s.toStdString());
        if (!doLoadCsv(f)) {
            qWarning() << "error reading CSV file" << s;
            return false;
        }
    }
    return true;
};

QString ard::ArdiCsv::val(QString s, STRING_LIST& r)const
{
    auto column_name = s.toLower();
    QString rv = "";
    auto i = m_name2column_index.find(column_name);
    if (i != m_name2column_index.end()) {
        auto idx = i->second;
        if (idx >= 0 && idx < static_cast<int>(r.size())) {
            rv = r[idx];
        }
    }
    return rv;
};

extern bool guiSelectContacts2Import(ard::contacts_merge_result& mr);
std::pair<int, int> ard::ArdiCsv::storeAsOutlookContacts(bool withGuiconfirmationBox)
{
    std::pair<int, int> rv{-1, 0};

    if (!ard::isDbConnected()) {
        ASSERT(0, "expected open DB");
        return rv;
    }

    CONTACT_LIST lst;
    GROUP_MAP gr_map;
    GROUP_MAP name2gr_map;
    for (auto& r : m_rows) {
        auto main_email_address = val("E-mail Address", r);
        auto main_phone = val("Primary Phone", r);
        auto mobile_phone = val("Mobile Phone", r);
        auto home_phone = val("Home Phone", r);
        auto lastName = val("Last Name", r);


        auto syid = "ard" + cit_base::make_new_syid(1);
        //googleQt::gcontact::ContactInfo::ptr 
        auto new_c = googleQt::gcontact::ContactInfo::createWithId(syid);

        googleQt::gcontact::NameInfo n;

        auto firstName = val("First Name", r);
        auto fullName = firstName + " " + lastName;

        n.setGivenName(firstName);
        n.setFamilyName(lastName);
        n.setFullName(fullName);

        new_c->setName(n).setTitle(fullName);

        if (!main_email_address.isEmpty()) {
            googleQt::gcontact::EmailInfo e;
            e.setAddress(main_email_address);
            e.setDisplayName(fullName);
            e.setPrimary(true);
            e.setTypeLabel("home");
            new_c->addEmail(e);
        }

        auto v = val("E-mail 2 Address", r);
        if (!v.isEmpty()) {
            googleQt::gcontact::EmailInfo e;
            e.setAddress(v);
            e.setDisplayName(fullName);
            e.setPrimary(false);
            e.setTypeLabel("other");
            new_c->addEmail(e);
        }

        v = val("E-mail 3 Address", r);
        if (!v.isEmpty()) {
            googleQt::gcontact::EmailInfo e;
            e.setAddress(v);
            e.setDisplayName(fullName);
            e.setPrimary(false);
            e.setTypeLabel("other3");
            new_c->addEmail(e);
        }

        if (!main_phone.isEmpty()) {
            googleQt::gcontact::PhoneInfo p;
            p.setNumber(main_phone);
            p.setPrimary(true);
            p.setTypeLabel("primary");
            new_c->addPhone(p);
        }

        if (!home_phone.isEmpty()) {
            googleQt::gcontact::PhoneInfo p;
            p.setNumber(home_phone);
            p.setPrimary(false);
            p.setTypeLabel("home");
            new_c->addPhone(p);
        }

        v = val("Home Phone 2", r);
        if (!v.isEmpty()) {
            googleQt::gcontact::PhoneInfo p;
            p.setNumber(v);
            p.setPrimary(false);
            p.setTypeLabel("other");
            new_c->addPhone(p);
        }

        if (!mobile_phone.isEmpty()) {
            googleQt::gcontact::PhoneInfo p;
            p.setNumber(mobile_phone);
            p.setPrimary(false);
            p.setTypeLabel("mobile");
            new_c->addPhone(p);
        }

        v = val("Home Street", r);
        if (!v.isEmpty()) {
            googleQt::gcontact::PostalAddress a;
            a.setCity(val("Home City", r));
            a.setStreet(v);
            a.setRegion(val("Home State", r));
            a.setPostcode(val("Home Postal Code", r));
            a.setCountry(val("Home Country", r));
            a.setPrimary(true);
            new_c->addAddress(a);
        }

        v = val("Home Street 2", r);
        if (!v.isEmpty()) {
            googleQt::gcontact::PostalAddress a;
            a.setStreet(v);
            a.setPrimary(false);
            new_c->addAddress(a);
        }

        v = val("Home Street 3", r);
        if (!v.isEmpty()) {
            googleQt::gcontact::PostalAddress a;
            a.setStreet(v);
            a.setPrimary(false);
            new_c->addAddress(a);
        }

        v = val("Company", r);
        if (!v.isEmpty()) {
            googleQt::gcontact::OrganizationInfo o;
            o.setName(v);
            o.setTitle(val("Job Title", r));
            new_c->setOrganizationInfo(o);
        }

        auto c = ard::contact::createNewContact(std::move(new_c));
        lst.push_back(c);

        v = val("Categories", r);
        if (!v.isEmpty()) {
            std::vector<QString> glst;

            auto it = name2gr_map.find(v);
            if (it == name2gr_map.end()) {
                auto new_g = ard::contact_group::createNewGroup(v);
                ASSERT(!new_g->syid().isEmpty(), "expected valid syid on group");
                gr_map[new_g->syid()] = new_g;
                name2gr_map[v] = new_g;
                glst.push_back(new_g->syid());
                //qDebug() << "ykh-new-groupid:" << new_g->syid();
            }
            else {
                ard::contact_group* g = it->second;
                glst.push_back(g->syid());
                //qDebug() << "ykh-used-groupid:" << g->syid();
            }

            auto e = c->ensureCExt();
            if (e) {
                e->setGroupMembership(glst);
            }       
        }
    }

    auto m = ard::db()->cmodel();
    auto cr = m->croot();
    auto res = cr->mergeContacts(lst, gr_map);
    if (withGuiconfirmationBox) {
        if (guiSelectContacts2Import(res)) {
            rv = m->storeMergedContacts(res);
        }
    }
    else
    {
        rv = m->storeMergedContacts(res);
    }
    return rv;
};

std::pair<int, int> ard::ArdiCsv::importContactsCsvFiles(QStringList files2import, bool withGuiConfirmation)
{
    std::pair<int, int> rv = { 0,0 };
    ArdiCsv a;
    if (a.loadCsvFiles(files2import)) 
    {
        rv = a.storeAsOutlookContacts(withGuiConfirmation);
    };
    return rv;
};

void ard::ArdiCsv::guiSelectAndImportContactsCsvFiles()
{
    if (!ard::confirmBox(ard::mainWnd(), "Please confirm contacts import. In the next window you will select Contacs CSV-file(s) created from Google Contacts or Outlook."))
        return;

  QStringList selectedFiles = QFileDialog::getOpenFileNames(gui::mainWnd(),
                             "Select one or more CSV file",
                            dbp::configFileLastShellAccessDir(),
                            "CSV Files  (*.csv)");
    if (selectedFiles.size() <= 0) {
        return;
    }

    dbp::configFileSetLastShellAccessDir(selectedFiles[0], true);

    int total_imported = 0;
    int total_duplicates = 0;

    auto r = importContactsCsvFiles(selectedFiles, true);
    total_imported = r.first;
    total_duplicates = r.second;

    QString msg(QString("Imported %1 contact(s).").arg(total_imported));
    if (total_duplicates > 0) {
        msg += QString("Ignored %1 duplicate").arg(total_duplicates);
    }
    qWarning() << msg;
	ard::messageBox(gui::mainWnd(), msg);
};


bool ard::ArdiCsv::guiExportAllContactsGroups(QWidget* parent)
{
    assert_return_false(ard::isDbConnected(), "expected DB");

    QString dir_path = QFileDialog::getExistingDirectory(parent, "Select Export Directory",
        dbp::configFileLastShellAccessDir(),
        QFileDialog::ShowDirsOnly
        | QFileDialog::DontResolveSymlinks);
    if (!dir_path.isEmpty())
    {
        dbp::configFileSetLastShellAccessDir(dir_path, false);
        

        auto gr_list = ard::db()->cmodel()->groot()->groups();
        for (auto g : gr_list) 
        {
            auto filename = g->title();
            filename.replace(QRegExp("[" + QRegExp::escape("\\/:*?\"<>|") + "]"), QString("_"));
            filename += ".csv";
            QString file_path = dir_path + "/" + filename;
            qDebug() << "======= processing group" << file_path;

            exportContactsGroup(g, file_path);
        }
    }
    return true;
};

bool ard::ArdiCsv::exportContactsGroup(ard::contact_group* g, QString fileName)
{
    assert_return_false(ard::isDbConnected(), "expected DB");

    QString columns = "First Name,Middle Name,Last Name,Title,Suffix,Initials,Web Page,Gender,Birthday,Anniversary,Location,Language,Internet Free Busy,Notes,E-mail Address,E-mail 2 Address,E-mail 3 Address,Primary Phone,Home Phone,Home Phone 2,Mobile Phone,Pager,Home Fax,Home Address,Home Street,Home Street 2,Home Street 3,Home Address PO Box,Home City,Home State,Home Postal Code,Home Country,Spouse,Children,Manager's Name,Assistant's Name,Referred By,Company Main Phone,Business Phone,Business Phone 2,Business Fax,Assistant's Phone,Company,Job Title,Department,Office Location,Organizational ID Number,Profession,Account,Business Address,Business Street,Business Street 2,Business Street 3,Business Address PO Box,Business City,Business State,Business Postal Code,Business Country,Other Phone,Other Fax,Other Address,Other Street,Other Street 2,Other Street 3,Other Address PO Box,Other City,Other State,Other Postal Code,Other Country,Callback,Car Phone,ISDN,Radio Phone,TTY/TDD Phone,Telex,User 1,User 2,User 3,User 4,Keywords,Mileage,Hobby,Billing Information,Directory Server,Sensitivity,Priority,Private,Categories";
    auto c_arr = columns.split(",");

    QFile file(fileName);
    if(!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;
    QTextStream exp_out(&file);
    exp_out << columns;
    exp_out << "\n";

    std::set<ard::contact_group*> gfilter;
    gfilter.insert(g);
    auto glst = ard::db()->cmodel()->groot()->getAsGroupListModel(&gfilter);
    for (auto g : glst) 
    {
        for (auto f : g->items())
        {
            auto sh = dynamic_cast<ard::shortcut*>(f);
            assert_return_false(sh, "expected shortcut");
            qDebug() << g->title() << f->title();
            auto c = dynamic_cast<ard::contact*>(sh->shortcutUnderlying());
            if (c) 
            {
                googleQt::gcontact::PostalAddress addr;

                bool has_main_address = false;
                googleQt::gcontact::PostalAddressList& lst = c->ensureCExt()->optr()->addressesRef();
                if (lst.items().size() > 0) 
                {
                    has_main_address = true;
                    addr = lst.items()[0];
                }

                QString line = "";
                for(int col_num = 0; col_num < c_arr.size(); col_num++)
                {
                    QString col_name = c_arr[col_num];
                    QString col_val = "";
                    if (col_name == "First Name")
                    {
                        col_val = c->contactGivenName();
                    }
                    else if (col_name == "Last Name") {
                        col_val = c->contactFamilyName();
                    }
                    else if (col_name == "E-mail Address") {
                        col_val = c->contactEmail();
                    }
                    else if (col_name == "Primary Phone") {
                        col_val = c->contactPhone();
                    }
                    else if (col_name == "Home Street") {
                        if (has_main_address) {
                            col_val = addr.street();
                        }
                    }
                    else if (col_name == "Home City") {
                        if (has_main_address) {
                            col_val = addr.city();
                        }
                    }
                    else if (col_name == "Home State") {
                        if (has_main_address) {
                            col_val = addr.region();
                        }
                    }
                    else if (col_name == "Home Postal Code") {
                        if (has_main_address) {
                            col_val = addr.postcode();
                        }
                    }
                    else if (col_name == "Home Country") {
                        if (has_main_address) {
                            col_val = addr.country();
                        }
                    }
                    else if (col_name == "Categories") {
                        col_val = g->title();
                    }

                    line += col_val;
                    line += ",";
                }

                line += "\n";
                exp_out << line;
            }
        }
    }

    return true;
};

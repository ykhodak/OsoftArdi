#pragma once

#include "anfolder.h"

namespace ard
{
	class contact_group;

    using VALUES_LIST = std::vector<std::vector<QString>>;
    class ArdiCsv
    {
    public:
        ArdiCsv();

        static void guiSelectAndImportContactsCsvFiles();
        /// pair (imported, duplicates)
        static std::pair<int, int> importContactsCsvFiles(QStringList files2import, bool withGuiConfirmation);

        bool loadCsvFiles(QStringList files2import);
        bool loadCsv(QString fileName);
        bool loadCsv(std::istream& input);

        ///return number of recovered Outlook style contacts and number of skipped (duplicate) or -1 in case an error
        std::pair<int, int> storeAsOutlookContacts(bool withGuiconfirmationBox);

        STRING_LIST&    columns() { return m_columns; }
        VALUES_LIST&    rows() { return m_rows; }
        QString         val(QString column_name, STRING_LIST& r)const;

        static bool guiExportAllContactsGroups(QWidget* parent);
        static bool exportContactsGroup(ard::contact_group* g, QString fileName);

    protected:
        bool doLoadCsv(std::istream& input);

        using COL2INDEX = std::map<QString, int>;

        STRING_LIST m_columns;
        COL2INDEX   m_name2column_index;
        VALUES_LIST m_rows;
    };
}
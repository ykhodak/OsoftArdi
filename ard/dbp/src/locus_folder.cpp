#include "a-db-utils.h"
#include "locus_folder.h"
#include "ethread.h"
#include "email.h"
#include "ansearch.h"

ard::locus_folder::locus_folder(QString name, EFolderType ftype)
    :ard::topic(name, ftype)
{
    setExpanded(true);
};

ard::locus_folder::~locus_folder()
{
}


STRING_SET ard::locus_folder::allowedTitles()const
{
    STRING_SET st;
    auto ss = suggestSingletonLabelName();
    for (auto s : ss) {
        st.insert(s);
    }
    return st;
};

STRING_LIST ard::locus_folder::suggestSingletonLabelName()const
{
    STRING_LIST label_names;

    auto st = ard::topic::getSingletonType();
    switch (st) {
    case ESingletonType::gtdSortbox: 
    {
        label_names.push_back("Sortbox");
        label_names.push_back("INBOX");
    }break;
    case ESingletonType::gtdInkubator: {
        label_names.push_back("Someday");
        label_names.push_back("Someday/Maybe");
        label_names.push_back("Maybe");
        label_names.push_back("Inkubator");
    }break;
    case ESingletonType::gtdReference: {
        label_names.push_back("Reference");
        label_names.push_back("Library");
    }break;
    case ESingletonType::gtdDelegated: {
        label_names.push_back("Delegated");
    }break;
    case ESingletonType::gtdDrafts: {
        label_names.push_back("Backlog");
    }break;
    default:break;
    }

    return label_names;
};

QPixmap ard::locus_folder::getIcon(OutlineContext c)const
{
    if(c == OutlineContext::check2select){
        if(isDefaultLocused()){
            return getIcon_CheckedGrayedBox();
        }
        
        return ard::topic::getIcon(c);
    }
    return QPixmap();
}


QString PrimaryGmailQStr() 
{
    return "category:{personal updates forums} OR label:SENT";
    //return "category:{personal updates}";
    //return "in:inbox -category:{social promotions updates forums}";
};

QString DraftGmailQStr()
{
    return "label:DRAFT";
}

bool ard::locus_folder::isDefaultLocused()const
{
    switch (folder_type()) {
    case EFolderType::folderSortbox:
    case EFolderType::folderMaybe:
    case EFolderType::folderReference:
        return true;
    default:break;
    }
    return false;
};

bool ard::locus_folder::canCloseLocusTab()const
{
    return !isDefaultLocused();
};

bool ard::locus_folder::isInLocusTab()const
{
    if (isDefaultLocused()) {
        return true;
    }

    return ard::topic::isInLocusTab();
};

void ard::locus_folder::setInLocusTab(bool val /*= true*/)
{
    if (isDefaultLocused()) {
        ASSERT(0, QString("Can't modify locus property of constant folder type '%1'").arg(static_cast<int>(folder_type())));
        return;
    }
    ard::topic::setInLocusTab(val);
};

ard::locus_folder* ard::locus_folder::getLocusFolder()
{
    return this;
};


int	ard::locus_folder::boardBandWidth()const
{
	return m_board_band_width;
};

void ard::locus_folder::setBoardBandWidth(int val)
{
	if (val >= BBOARD_BAND_MIN_WIDTH && val <= BBOARD_BAND_MAX_WIDTH)
	{
		m_board_band_width = val;
		dbp::configStoreBoardFolderBandWidth(id(), val);
	}
};

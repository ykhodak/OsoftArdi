#include "a-db-utils.h"
#include "fileref.h"

ard::fileref::fileref() 
{
};

ard::fileref::fileref(QString title):ard::topic(title)
{

};


QString ard::fileref::objName()const 
{
    return "fref";
};

ENoteView ard::fileref::noteViewType()const
{
	auto idx = m_title.indexOf("statement:");
	if (idx == 0) {
		return ENoteView::TDAView;
	}
	return ENoteView::None;
};

bool ard::fileref::canAcceptChild(cit_cptr)const
{
    return false;
};

QPixmap ard::fileref::getIcon(OutlineContext)const
{
    return getIcon_AttachmentFile();
}

std::pair<QString, QString> ard::fileref::getRefFileName()const
{
    std::pair<QString, QString> rv;
    ASSERT_VALID(this);
    auto s = title();
    auto idx = s.indexOf(":");
    if (idx == -1) {
        qWarning() << "failed to load TDA, invalid format" << title();
        return rv;
    }
    rv.first = s.mid(idx + 1);
    idx = rv.first.indexOf(" ");
    if (idx != -1) 
    {
        rv.first = rv.first.left(idx);
    }
    idx = s.indexOf(":", idx+1);
    if (idx != -1) {
        rv.second = s.mid(idx + 1);
    }
    return rv;
};

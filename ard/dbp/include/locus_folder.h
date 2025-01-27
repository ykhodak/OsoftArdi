#pragma once

#include "anfolder.h"

namespace ard
{
	class locus_folder : public ard::topic
	{
	public:
		locus_folder(QString name = "", EFolderType ftype = EFolderType::folderUserSorter);
		~locus_folder();
		cit_prim_ptr        create()const override { return new locus_folder; }

		STRING_LIST suggestSingletonLabelName()const;
		QPixmap     getIcon(OutlineContext)const override;

		bool        canRenameToAnything()const override { return false; }
		STRING_SET  allowedTitles()const override;

		bool        isInLocusTab()const override;
		bool        isDefaultLocused()const;
		void        setInLocusTab(bool val = true)override;
		bool        canCloseLocusTab()const override;

		ENoteView   noteViewType()const override { return ENoteView::None; };
		bool        canAttachNote()const override { return false; };
		ard::locus_folder*   getLocusFolder()override;
		int			boardBandWidth()const;
		void		setBoardBandWidth(int val);
	protected:
		int			m_board_band_width{ BBOARD_BAND_DEFAULT_WIDTH };
	};
}
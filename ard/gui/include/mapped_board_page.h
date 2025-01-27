#pragma once

#include "board_page.h"

namespace ard 
{
	template<class BB, class C>
	class mapped_board_page : public board_page<BB>
	{
	public:
		void		rebuildBoard()override;
		void		rebuildBoardBand(C* band_col);
		void		rebuildBoardBands(const std::unordered_set<C*>& bands2rebuild);
		void        applyTextFilter()override;
        void		removeSelected(bool )override{};
		void		setupAnnotationCard(bool)override {};
		void		zoomView(bool)override {};
		void		saveModified()override {};
		void		reloadContent()override {};
		void		detachCardOwner()override {};
		void		setFocusOnContent()override {};
        void	    onMovedInBands(const std::unordered_set<int>&)override;;
	protected:
		virtual int	layoutBand(ard::board_band_info*, C*, int) { return 0; };
		bool		m_make_thumb_on_rebuild{false};
	};

};


template<class BB, class C>
void ard::mapped_board_page<BB, C>::rebuildBoard() 
{
	board_page<BB>::clearBoard();
	board_page<BB>::m_bb->rebuildMappedTopicsBoard();
	board_page<BB>::rebuildBands();

	int xstart = 0;
	board_page<BB>::m_max_ybottom = 0;
	auto& bls = board_page<BB>::m_bb->bands();
	auto& rls = board_page<BB>::m_bb->topics();
	for (size_t i = 0; i < rls.size(); i++)
	{
		auto band = bls[i];
		auto band_width = band->bandWidth();
		auto yband = 0;
		auto r = rls[i];
		if (r && r->isValid()) {
			yband = layoutBand(band, r, xstart);
		}
		xstart += band_width;
		if (yband > board_page<BB>::m_max_ybottom)
			board_page<BB>::m_max_ybottom = yband;
	}

	board_page<BB>::m_max_ybottom += BBOARD_DELTA_EXPAND_HEIGHT;
	board_page<BB>::alignBandHeights();

	//if(m_make_thumb_on_rebuild)makeThumbnail();
};

template<class BB, class C>
void ard::mapped_board_page<BB, C>::rebuildBoardBands(const std::unordered_set<C*>& bands2rebuild) 
{
	board_page<BB>::pushSelected();
	board_page<BB>::clearSelectedGBItems();
	bool h_increased = false;
	int xstart = 0;
	auto& bls = board_page<BB>::m_bb->bands();
	auto& rls = board_page<BB>::m_bb->topics();
	for (size_t i = 0; i < rls.size(); i++)
	{
		auto band = bls[i];
		auto bindex = band->bandIndex();
		auto band_width = band->bandWidth();
		auto yband = 0;
		auto r = rls[i];
		if (r && r->isValid() && bands2rebuild.find(r) != bands2rebuild.end()) 
		{
			auto old_num = board_page<BB>::removeBandGBItems(bindex);
			yband = layoutBand(band, r, xstart);
			if (yband > board_page<BB>::m_max_ybottom) {
				board_page<BB>::m_max_ybottom = yband;
				h_increased = true;
			}
			auto new_num = board_page<BB>::m_band2glst[bindex].size();
			ard::trail(QString("rebuild-band [%1] [%2][%3][%4]->[%5]").arg(board_page<BB>::m_bb->title()).arg(r->title()).arg(bindex).arg(old_num).arg(new_num));
		}
		xstart += band_width;
	}

	if(h_increased) board_page<BB>::m_max_ybottom += BBOARD_DELTA_EXPAND_HEIGHT;
	board_page<BB>::alignBandHeights();
	board_page<BB>::popSelected();
	if (m_make_thumb_on_rebuild)board_page<BB>::makeThumbnail();
};

template<class BB, class C>
void ard::mapped_board_page<BB, C>::rebuildBoardBand(C* band_col) 
{
	std::unordered_set<C*> bands2rebuild;
	bands2rebuild.insert(band_col);
	rebuildBoardBands(bands2rebuild);
};

template<class BB, class C>
void ard::mapped_board_page<BB, C>::applyTextFilter()
{
	//ard::trail(QString("mp-board/apply-text-filter [%1][%2]").arg(m_text_filter->isActive() ? "Y" : "N").arg(m_text_filter->fcontext().key_str));

	auto key = board_page<BB>::m_filter_edit->text().trimmed();
	bool refilter = true;
	if (board_page<BB>::m_text_filter->isActive()) {
		auto s2 = board_page<BB>::m_text_filter->fcontext().key_str;
		if (key == s2) {
			refilter = false;
		}
	}

	if (refilter)
	{
		board_page<BB>::clearSelectedGBItems();

		TextFilterContext fc;
		fc.key_str = key;
		fc.include_expanded_notes = false;
		board_page<BB>::m_text_filter->setSearchContext(fc);
		bool active_filter = !key.isEmpty();

		for (size_t idx = 0; idx < board_page<BB>::m_band2glst.size(); idx++)
		{
			auto band = board_page<BB>::m_bb->bandAt(idx);
			assert_return_void(band, "expected band");
			int xstart = band->xstart();
			int ystart = 0;
			auto& lst = board_page<BB>::m_band2glst[idx];
			for (auto& j : lst)
			{
				auto f = j->refTopic();
				if (f)
				{
					if (active_filter)
					{
						if (f->hasText4SearchFilter(fc))
						{
							j->show();
							board_page<BB>::repositionInBand(band, j, xstart, ystart);
							ystart += j->boundingRect().height();
						}
						else {
							j->hide();
						}
					}
					else
					{
						j->show();
						j->update();
						board_page<BB>::repositionInBand(band, j, xstart, ystart);
						ystart += j->boundingRect().height();
					}
				}
			}
		}
	}

	board_page<BB>::m_view->verticalScrollBar()->setValue(0);
};

template<class BB, class C>
void ard::mapped_board_page<BB, C>::onMovedInBands(const std::unordered_set<int>& bands) 
{
	std::unordered_set<C*> bands2rebuild;
	auto lst = board_page<BB>::m_bb->topics();
	for (auto& i : bands) 
	{
		if (i >= 0 && i < static_cast<int>(lst.size())) 
		{
			auto f = lst[i];
			bands2rebuild.insert(f);
		}
	}

	if(!bands2rebuild.empty()) rebuildBoardBands(bands2rebuild);
};

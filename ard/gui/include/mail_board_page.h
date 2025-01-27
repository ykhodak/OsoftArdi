#pragma once
#include "workspace.h"
#include "custom-widgets.h"
#include "mapped_board_page.h"
#include "ethread.h"

namespace ard 
{
	class mail_board_g_topic;
	using ETH2GB = std::unordered_map<ethread*, mail_board_g_topic*>;

	class mail_board_page : public mapped_board_page<ard::mail_board, ard::q_param>
	{
	public:
		mail_board_page(ard::mail_board*);
		void        removeSelected(bool silently)override;
		void        onBandControl(board_band_header<ard::mail_board>* g_band)override;
		//void        applyTextFilter()override;
		void		markAsRead(bool bval)override;
        void	    onMovedInBands(const std::unordered_set<int>&)override {};
#ifdef _DEBUG
		void		debugFunction();
#endif
	protected:
		ard::board_band_header<ard::mail_board>* produceBandHeader(ard::board_band_info* band)override;
		int						layoutBand(ard::board_band_info* b, ard::q_param*, int xstart)override;
		mail_board_g_topic*		register_topic(ard::ethread*, board_band_info* band);	
	};

	class mail_board_g_topic : public ard::board_box_g_topic<ard::mail_board, ard::ethread> 
	{
	public:
		mail_board_g_topic(mail_board_page* bb, ard::ethread* f);
	protected:
		void		onContextMenu(const QPoint& pt)override;
	};

	class mail_board_band_header : public board_band_header<ard::mail_board> 
	{
	protected:
		void    paint(QPainter * p, const QStyleOptionGraphicsItem *, QWidget *)override;
	private:
		mail_board_band_header(mail_board_page* bb, ard::board_band_info* bi, ard::q_param* q);
		~mail_board_band_header();
	
		ard::q_param*	m_rule{ nullptr };
		friend class mail_board_page;
	};
};

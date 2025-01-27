#pragma once
#include "workspace.h"
#include "custom-widgets.h"
#include "mapped_board_page.h"
#include "utils.h"

namespace ard
{
	class folders_board_g_topic;
	class dnd_g_mark;

	class folders_board_page : public mapped_board_page<ard::folders_board, ard::locus_folder>
	{
	public:
		struct drag_target 
		{
			ard::topic* dest{nullptr};
			int			pos{-1};
			QPointF		pt;
			QSizeF		sz;
		};
	
		folders_board_page(ard::folders_board*);
		void					clearBoard()override;
		void					removeSelected(bool silently)override;
		void					applyTextFilter()override;
		void					pasteFromClipboard()override;
		std::pair<bool, QString>renameCurrent()override;
		void					onBandControl(board_band_header<ard::folders_board>* g_band)override;
        //		void					onMovedInBands(const std::unordered_set<int>& bands)override;		
		int						removeBandGBItems(int bindex)override;
		void					onBandDragEnter(board_band<ard::folders_board>*, QGraphicsSceneDragDropEvent *)override;
		void					onBandDragMove(board_band<ard::folders_board>*, QGraphicsSceneDragDropEvent *)override;
		void					onBandDragDrop(board_band<ard::folders_board>*, QGraphicsSceneDragDropEvent *)override;
		void					onDragMove(const drag_target& dt);
		void					onDragDrop(const drag_target& dt, QGraphicsSceneDragDropEvent *e);
		void					onDragDisabled();
	protected:
		int			layoutBand(ard::board_band_info* b, ard::locus_folder*, int xstart)override;
		folders_board_g_topic*	register_topic(ard::topic*, board_band_info* band, int depth);
		DEPTH2LINES				m_depth2lines;
		dnd_g_mark*				m_dnd_mark{nullptr};
	};


	class folders_board_g_topic : public ard::board_box_g_topic<ard::folders_board, ard::topic>
	{
	public:
		folders_board_g_topic(folders_board_page* bb, ard::topic* f);

		void    dragEnterEvent(QGraphicsSceneDragDropEvent *e)override;
		void    dropEvent(QGraphicsSceneDragDropEvent *e)override;
		void	dragMoveEvent(QGraphicsSceneDragDropEvent *e);

	protected:
		void		onContextMenu(const QPoint& pt)override;
		folders_board_page::drag_target calcDropTarget(QGraphicsSceneDragDropEvent *e);
		folders_board_page* fboard();
	};

	class dnd_g_mark : public board_g_mark<ard::folders_board>
	{
	public:
		dnd_g_mark(folders_board_page* bb);
		void	paint(QPainter * p, const QStyleOptionGraphicsItem *, QWidget *)override;
		void	updateDndPos(const QPointF& pt, const QSizeF& sz);
	};
};



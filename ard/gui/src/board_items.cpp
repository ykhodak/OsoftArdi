#include "utils.h"
#include "board_items.h"
#include "BlackBoardItems.h"

ard::board_band_splitter::board_band_splitter()
{
	setLine(0, 0, 5, BBOARD_DEFAULT_HEIGHT);
};

void ard::board_band_splitter::updateXPos(int x)
{
	setLine(x, 0, x, BBOARD_DEFAULT_HEIGHT);
	show();
	update();
};

void ard::board_band_splitter::paint(QPainter * p, const QStyleOptionGraphicsItem *, QWidget *)
{
	PGUARD(p);
	p->setPen(QPen(Qt::white, 6));
	p->drawLine(line());
};

/**
board_selector_rect
*/
ard::board_selector_rect::board_selector_rect()
{

};

void ard::board_selector_rect::updateSelectRect(QRectF rc)
{
	setRect(rc);
	if (rc.isEmpty()) {
		hide();
	}
	else {
		show();
	}
};

void ard::board_selector_rect::paint(QPainter * p, const QStyleOptionGraphicsItem *, QWidget *)
{
	PGUARD(p);
	p->setPen(QPen(Qt::white, 6, Qt::DotLine));
	p->drawRect(rect());
};

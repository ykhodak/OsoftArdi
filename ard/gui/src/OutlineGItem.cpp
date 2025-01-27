#include <QApplication>
#include "OutlineGItem.h"
#include "utils.h"
#include "MainWindow.h"
#include "anfolder.h"
#include "ardmodel.h"
#include "OutlineMain.h"
#include "ProtoScene.h"
#include "TopicPanel.h"
#include "OutlinePanel.h"
#include "TablePanel.h"
#include "email.h"
#include "ansearch.h"
#include "kring.h"
#include "extnote.h"
#include "anurl.h"
#include "contact.h"
#include "rule_runner.h"

/*
  OutlineGItem
*/
OutlineGItem::OutlineGItem()
{

};

OutlineGItem::OutlineGItem(int outline_ident)
{
    m_attr.flag = 0;
    m_attr.outline_ident = outline_ident;
};

OutlineGItem::~OutlineGItem()
{

};

void OutlineGItem::drawToDoColumn(const QRectF& rc, QPainter *painter)
{
    topic_ptr it = proto()->topic();

    if (it->isToDo() &&
        proto()->p()->hasProp(ProtoPanel::PP_ToDoColumn))
    {
        PGUARD(painter);
#ifdef Q_OS_ANDROID
        QPen penBox(Qt::gray, 5);
#else
        QPen penBox(Qt::gray, 2);
#endif
        painter->setPen(penBox);
        painter->setRenderHint(QPainter::Antialiasing, true);

        QRectF rc1(rc.topRight().x() - gui::lineHeight(),
            rc.topRight().y(),
            gui::lineHeight(),
            gui::lineHeight());

        if (it->hasToDoPriority())
        {
            QRect rcp;
            rectf2rect(rc1, rcp);
            utils::drawPriorityIcon(it, rcp, painter);
        }
        utils::drawCompletedBox(it, rc1, painter);
    }
};

void OutlineGItem::drawDateLabel(QPainter *painter, 
                                 const QStyleOptionGraphicsItem *, 
                                 const QRectF& rc)
{
    utils::drawDateLabel(proto()->topic(), painter, rc);
};

void OutlineGItem::drawLabels(QPainter *painter, const QStyleOptionGraphicsItem*)
{
    QRectF rc = identedRect();
    utils::drawLabels(proto()->topic(), painter, rc);
};

const QRectF& OutlineGItem::identedRect()const 
{
    static QRectF rv;
    rv = m_rect;
    rv.setLeft(rv.left() + m_attr.outline_ident + m_attr.ident_adjustment);
    rv.setTop(rv.top() + m_attr.annotation_height);
    rv.setBottom(rv.bottom() - m_attr.expanded_note_height);
    return rv;
};

const QRectF& OutlineGItem::nonidentedRect()const
{
    static QRectF rv;
    rv = m_rect;
    rv.setTop(rv.top() + m_attr.annotation_height);
    rv.setBottom(rv.bottom() - m_attr.expanded_note_height);
    // + m_attr.expanded_note_height
    return rv;
};

const QRectF& OutlineGItem::annotationRect()const
{
    static QRectF rv;
    rv = m_rect;
    rv.setLeft(rv.left() + m_attr.outline_ident + m_attr.ident_adjustment);
    rv.setBottom(rv.top() + m_attr.annotation_height);
    return rv;
};

const QRectF& OutlineGItem::expandedNoteRect()const
{
    static QRectF rv;
    rv = m_rect;
    rv.setLeft(rv.left() + m_attr.outline_ident + m_attr.ident_adjustment);
    rv.setTop(rv.bottom() - m_attr.expanded_note_height);
    return rv;
};


void OutlineGItem::recalcGeometry()
{
	auto f = proto()->topic();

	auto w = proto()->p()->panelWidth();
	int h = 0;

	if (/*!force_recalc &&*/ f->lastOutlinePanelWidht() == w &&
		f->lastOutlineHeight() > 10) {
		h = f->lastOutlineHeight();
	}
	else
	{
		h = guessOutlineHeight(proto(), (int)proto()->p()->textColumnWidth() - outlineIdent(), proto()->p()->ocontext()) + 1;
		//h = gui::lineHeight();

		f->setLastOutlinePanelWidht(w);
		f->setLastOutlineHeight(h);
	}

	//...
	if (proto()->p()->hasProp(ProtoPanel::PP_Annotation)) {
		if (f->canHaveAnnotation()) {
			m_attr.annotation_height = 0;
			auto s = f->annotation4outline().trimmed();
			if (!s.isEmpty()) {
				m_attr.annotation_height = guessOutlineAnnotationHeight(s, w);
				h += m_attr.annotation_height;
			}
		}
	}
	//...

	if (!m_attr.asRootTopic) {
		if (proto()->p()->hasProp(ProtoPanel::PP_CheckSelectBox)) {
			m_attr.ident_adjustment = gui::lineHeight();
		}

		if (proto()->p()->hasProp(ProtoPanel::PP_ExpandNotes)) {
			if (f->hasNote()) {
				m_attr.expanded_note_height = 0;
				auto n = f->mainNote();
				if (n) {
					auto s = n->plain_text4draw();
					if (!s.isEmpty()) {
						m_attr.expanded_note_height = guessOutlineExpandedNoteHeight(s, w);
						h += m_attr.expanded_note_height;
					}
				}
			}
		}
	}

	m_rect = QRectF(0, 0,
		w,
		h);
};

void OutlineGItem::resetOutlineItemGeometry()
{
    assert_return_void(proto()->p(), "expected panel");
    proto()->request_prepareGeometryChange();
    m_rect.setRight(proto()->p()->panelWidth());
    m_attr.ident_adjustment = 0;
    if (!m_attr.asRootTopic && proto()->p()->hasProp(ProtoPanel::PP_CheckSelectBox))
        {
            m_attr.ident_adjustment = gui::lineHeight();
        }
};


void OutlineGItem::drawTitle(const QRectF& rc, QPainter *painter, const QStyleOptionGraphicsItem *option)
{
    topic_ptr it = proto()->topic()->shortcutUnderlying();
    PGUARD(painter);

    QString stitle = it->impliedTitle();

    ProtoPanel* ppanel = proto()->p();
    if (isRootTopic())
    {
        QFont f(*ard::defaultFont());
        f.setBold(true);
        painter->setFont(f);
    }
    else
    {
        if (it->isRetired())
        {
            QFont f(*ard::defaultFont());
            f.setStrikeOut(true);
            painter->setFont(f);
        }
        else
        {
            painter->setFont(*ard::defaultFont());
        }
    }

    bool as_selected = (option->state & QStyle::State_Selected);
    if (as_selected)
    {
        QPen penText(Qt::black);
        painter->setPen(penText);
    }
    else
    {
        QPen pen(QColor(color::invert(painter->background().color().rgb())));
        painter->setPen(pen);
    }

    QRectF rcText = rc;

    QSize rv = { 0,0 };
    bool drawMLine = ppanel->hasProp(ProtoPanel::PP_MultiLine);
    if (drawMLine)
    {
        if (proto()->topic()->hasCustomTitleFormatting())
        {
            QList<QTextLayout::FormatRange> fr;
            proto()->topic()->prepareCustomTitleFormatting(painter->font(), fr);
            QString date_label = it->dateColumnLabel();
            rv = globalTextFilter().drawTextMLine(painter,
                painter->font(),
                rcText,
                stitle,
                false,
                nullptr,
                &fr);
            if (!date_label.isEmpty()) {
                drawDateLabel(painter, option, rcText);
            }
        }
        else
        {
            rv = globalTextFilter().drawTextMLine(painter, painter->font(), rcText, stitle, false);
        }
    }
    else
    {
        auto r = globalTextFilter().drawTextLine(painter, rcText, stitle);
        rv.setHeight(r.height());
        rv.setWidth(r.width());
    }

    if (ppanel->hasProp(ProtoPanel::PP_ToDoColumn))
    {
        //if (!as_selected) {
            drawToDoColumn(rc, painter);
        //}
    }
    if (ppanel->hasProp(ProtoPanel::PP_Labels))
    {
        drawLabels(painter, option);
    }
};


void OutlineGItem::drawActionBtn(QPainter *p)
{
    PGUARD(p);
    p->setRenderHint(QPainter::Antialiasing, true);

    auto rcpm2 = nonidentedRect();
    rcpm2.setTop(rcpm2.top() + ARD_MARGIN);
    rcpm2.setRight(rcpm2.left() + proto()->p()->actionBtnWidth());
    rcpm2.setBottom(rcpm2.top() + gui::lineHeight() - 2 * ARD_MARGIN);

    //QColor cl_btn(COLOR_PANEL_SEP);
    QColor cl_btn(qRgb(121,0,0));
    QBrush br(cl_btn);
    p->setBrush(br);

    cl_btn = cl_btn.darker(200);
    QPen pn(cl_btn);
    p->setPen(pn);

    int radius = 5;
    p->drawRoundedRect(rcpm2, radius, radius);

    static int flags = Qt::AlignHCenter | Qt::AlignVCenter;
    QPen pn1(color::White);
    p->setPen(pn1);
    p->drawText(rcpm2, flags, proto()->p()->actionBtnText());
};

void OutlineGItem::drawAnnotation(const QRectF& rc, QPainter *p, bool as_selected, bool hover)
{
    PGUARD(p);
    p->setRenderHint(QPainter::Antialiasing, true);
    QColor cl_btn = color::Yellow;

    if (as_selected) {
        cl_btn = cl_btn.darker(110);
    }
    else 
    {
        if (hover) {
            cl_btn = color::HOVER_ITEM_BK;
            cl_btn = cl_btn.darker(110);
        }
    }

    QBrush br(cl_btn);
    p->setBrush(br);

    cl_btn = cl_btn.darker(200);
    QPen pn(cl_btn);
    p->setPen(pn);

    int radius = 5;
    p->drawRoundedRect(rc, radius, radius);

    auto s = proto()->topic()->annotation4outline();

    QFont f(*utils::annotationFont());  
    globalTextFilter().drawTextMLine(p, f, rc, s, false);
};

void OutlineGItem::drawExpandedNote(const QRectF& rc, QPainter *p, bool as_selected)
{
    PGUARD(p);
    p->setRenderHint(QPainter::Antialiasing, true);

    QColor cl_btn = qRgb(232, 239, 255);

    if (as_selected) {
        cl_btn = color::SELECTED_ITEM_BK;
        cl_btn = cl_btn.lighter(110);
    }

    QBrush br(cl_btn);
    p->setBrush(br);
    p->setPen(Qt::NoPen);
    /*
    cl_btn = cl_btn.darker(200);
    QPen pn(cl_btn);
    p->setPen(pn);

    int radius = 5;
    p->drawRoundedRect(rc, radius, radius);
    */
    p->drawRect(rc);

    cl_btn = cl_btn.darker(200);
    QPen pn(cl_btn);
    p->setPen(pn);


    QString s = "";
    auto n = proto()->topic()->mainNote();
    if (n) {
        s = n->plain_text4draw();
    }

    QFont f(*ard::defaultSmallFont());
    globalTextFilter().drawTextMLine(p, f, rc, s, false);
};

void OutlineGItem::drawIcons(QRectF& rc, QPainter *painter, const QStyleOptionGraphicsItem *option)
{    
    Q_UNUSED(option);
    PGUARD(painter);
    painter->setRenderHint(QPainter::Antialiasing,true);

    auto topic = proto()->topic();

    QRect rcpm((int)rc.topLeft().x(), (int)rc.topLeft().y(), 
               (int)ICON_WIDTH, (int)ICON_WIDTH);

    QPixmap pm;
    
    if (proto()->p()->hasProp(ProtoPanel::PP_CheckSelectBox))
        {
            if (proto()->isCheckSelected())
                {
                    QRect rcpm2 = rcpm;
                    rcpm2.setLeft(nonidentedRect().left());
                    rcpm2.setRight(rcpm2.left() + gui::lineHeight());
                    pm = getIcon_CheckSelect();
                    painter->drawPixmap(rcpm2, pm);
                }
        }


    pm = topic->getIcon(proto()->p()->ocontext());
    bool hasMainIcon = !pm.isNull();
    if(hasMainIcon)
        {
            rc.setLeft(rc.left() + ICON_WIDTH);
            painter->drawPixmap(rcpm, pm);
            rcpm.translate((int)ICON_WIDTH, 0);
        }

    if (topic->hasColorByHashIndex()) {
        auto ch = topic->getColorHashIndex();
        if (ch.second != 0) {
            QRect rcpm2clr_hash = rcpm;
            if (hasMainIcon) {              
                rcpm2clr_hash = QRect((int)rc.topLeft().x() - ICON_WIDTH, (int)rc.topLeft().y() + ICON_WIDTH,
                    (int)ICON_WIDTH, (int)ICON_WIDTH);
            }
            else {
                rc.setLeft(rc.left() + ICON_WIDTH);
                rcpm2clr_hash = rcpm;
                rcpm.translate((int)ICON_WIDTH, 0);
            }

            QColor rgb = color::getColorByHashIndex(ch.first);
            rgb = rgb.darker(150);
            PGUARD(painter);
            painter->setBrush(QBrush(rgb));
            painter->setPen(Qt::NoPen);
            painter->setFont(*ard::defaultOutlineLabelFont());
            painter->drawEllipse(rcpm2clr_hash);
            painter->setPen(Qt::white);
            painter->drawText(rcpm2clr_hash, Qt::AlignCenter | Qt::AlignHCenter, QString(ch.second));
        }
    }


    auto it = topic->shortcutUnderlying();
    auto clidx = it->colorIndex();
    if (clidx != ard::EColor::none)
    {
        PGUARD(painter);
        QRect rc_clr;
        rectf2rect(rc, rc_clr);
        if(rc_clr.height() > 48){
            rc_clr.setTop(rc_clr.bottom() - 48);
        }
        rc_clr.setRight(rc_clr.left() + 200);
        auto pm = utils::colorMarkSelectorPixmap(clidx);
        if (pm) 
        {
            painter->drawPixmap(rc_clr, *pm);
        }
    }

    pm = topic->getSecondaryIcon(proto()->p()->ocontext());
    if (!pm.isNull())
    {
        //pt.setX(rc.topLeft().x());
        rc.setLeft(rc.left() + ICON_WIDTH);
        painter->drawPixmap(rcpm, pm);
        rcpm.translate((int)ICON_WIDTH, 0);
    }

    auto t3 = topic->ternaryIconWidth();
    if (t3.first != TernaryIconType::none && t3.second > 0)
    {
        if (t3.first == TernaryIconType::gmailFolderStatus) 
        {
            auto* q = dynamic_cast<ard::rule_runner*>(proto()->topic());
            if (q) {
                QRect rct = rcpm;
                utils::drawQFolderStatus(q, painter, rct);
            }
        }
        rc.setLeft(rc.left() + t3.second);
    }
};

void OutlineGItem::drawText(QString s, QRectF& rc, QPainter *painter, const QStyleOptionGraphicsItem *option)
{
    if (!s.isEmpty())
    {
        PGUARD(painter);
        painter->setRenderHint(QPainter::Antialiasing, true);

        if (option->state & QStyle::State_Selected)
        {
            QPen penText(Qt::black);
            painter->setPen(penText);
        }
        else
        {
            QPen pen(QColor(color::invert(painter->background().color().rgb())));
            painter->setPen(pen);
        }
        painter->setFont(*ard::defaultFont());
        globalTextFilter().drawTextLine(painter, rc, s);
    }
};

bool OutlineGItem::isThumbnailDraw()const
{
	auto f = proto()->topic()->shortcutUnderlying();
	bool rv = proto()->p()->hasProp(ProtoPanel::PP_Thumbnail) && f->hasThumbnail();
	return rv;
};


void OutlineGItem::drawTitleWithThumbnail(const QRectF& rc, QPainter *painter, const QStyleOptionGraphicsItem *option)
{
	QRectF rc2 = rc;
	auto pxm = proto()->topic()->thumbnail();
	rc2.setLeft(rc.left() + MP1_WIDTH);
	if (pxm && !pxm->isNull()) {
		QRect rc_pxmp(rc.left(), rc.top(), MP1_WIDTH, MP1_HEIGHT);
		painter->drawPixmap(rc_pxmp, *pxm);
	}
	drawTitle(rc2, painter, option);
};

void OutlineGItem::drawThumbnail(QRectF& rc, QPainter *painter, const QStyleOptionGraphicsItem *option)
{
	if (!m_attr.asRootTopic) {
		if (proto()->p()->hasProp(ProtoPanel::PP_ActionButton)) {
			drawActionBtn(painter);
		}
	}

	drawIcons(rc, painter, option);
	drawTitleWithThumbnail(rc, painter, option);
};


#define DRAW_FIELD(N)    auto it = proto()->topic();\
                        QString s_label = it->N();\
                        if (!s_label.isEmpty())\
                        {                       \
                            drawText(s_label, rc, painter, option);\
                        }   \


#define DRAW_CONTACT_FIELD(N)    auto it = dynamic_cast<ard::contact*>(proto()->topic());\
                        if(it){QString s_label = it->N();\
                        if (!s_label.isEmpty())\
                        {                       \
                            drawText(s_label, rc, painter, option);\
                        }}   \


void OutlineGItem::drawFormFieldName(QRectF& rc, QPainter *painter, const QStyleOptionGraphicsItem *option) 
{
    DRAW_FIELD(title);
};

void OutlineGItem::drawFormFieldValue(QRectF& rc, QPainter *painter, const QStyleOptionGraphicsItem *option)
{
    auto it = proto()->topic();
    QString s_label = it->formValue();
    if(s_label.isEmpty()){
        auto f = dynamic_cast<FormFieldTopic*>(it);
        if (f) {
            auto ct = f->columnType();
            QString s_label = QString("<%1>").arg(columntype2label(ct));
            PGUARD(painter);
            painter->setPen(model()->penGray());
            painter->setFont(*ard::defaultSmallFont());
            globalTextFilter().drawTextLine(painter, rc, s_label);
        }
    }
    else {        
        drawText(s_label, rc, painter, option);
    }
};

void OutlineGItem::drawEmail(QRectF& rc, QPainter *painter, const QStyleOptionGraphicsItem *option)
{
	DRAW_CONTACT_FIELD(contactEmail);
};

void OutlineGItem::drawContactPhone(QRectF& rc, QPainter *painter, const QStyleOptionGraphicsItem *option)
{
	DRAW_CONTACT_FIELD(contactPhone);
};

void OutlineGItem::drawContactAddress(QRectF& rc, QPainter *painter, const QStyleOptionGraphicsItem *option)
{
	DRAW_CONTACT_FIELD(contactAddress);
};

void OutlineGItem::drawContactOrganization(QRectF& rc, QPainter *painter, const QStyleOptionGraphicsItem *option)
{
	DRAW_CONTACT_FIELD(contactOrganization);
};

void OutlineGItem::drawContactNotes(QRectF& rc, QPainter *painter, const QStyleOptionGraphicsItem *option)
{
	DRAW_CONTACT_FIELD(contactNotes);
};

#define DRAW_KRING_FIELD(N)    auto it = proto()->topic();\
                        auto k = dynamic_cast<ard::KRingKey*>(it);\
                        if(k){\
                        QString s_label = k->N();\
                        if (!s_label.isEmpty())\
                        {                       \
                            drawText(s_label, rc, painter, option);\
                        }}   \

/*
void OutlineGItem::drawKRingField(ard::KRingKey* k, QRectF& rc, QPainter *painter, const QStyleOptionGraphicsItem *option)
{
    QString s_label = k->N();
    if (!s_label.isEmpty())
    {
        drawText(s_label, rc, painter, option);
    }
}
*/

void OutlineGItem::drawKRingLogin(QRectF& rc, QPainter *painter, const QStyleOptionGraphicsItem *option) 
{
    auto it = proto()->topic();
        auto k = dynamic_cast<ard::KRingKey*>(it);
        if (k) {
                QString s_label = k->keyLogin();
                if (!s_label.isEmpty())
                {
                    drawText(s_label, rc, painter, option);
                }
        }
    //DRAW_KRING_FIELD(keyLogin);
};

void OutlineGItem::drawKRingPwd(QRectF& rc, QPainter *painter, const QStyleOptionGraphicsItem *option) 
{
    DRAW_KRING_FIELD(keyPwd);
};

void OutlineGItem::drawKRingLink(QRectF& rc, QPainter *painter, const QStyleOptionGraphicsItem *option) 
{
    DRAW_KRING_FIELD(keyLink);
};

void OutlineGItem::drawKRingNotes(QRectF& rc, QPainter *painter, const QStyleOptionGraphicsItem *option) 
{
    DRAW_KRING_FIELD(keyNote);
};

void OutlineGItem::drawCompletedLabel(QRectF& rc, QPainter *painter, const QStyleOptionGraphicsItem *)
{
    PGUARD(painter);
    painter->setRenderHint(QPainter::Antialiasing,true);
    utils::drawCompletedBox(proto()->topic(), rc, painter);
};

void OutlineGItem::drawFatFingerSelect(const QRectF& rc, QPainter *painter)
{
    PGUARD(painter);
    QBrush brush2(model()->brushHotSelectedItem());
    painter->setBrush(brush2);
    int hot_height = rc.height();
    if (hot_height > FAT_FINGER_MAX_H) {
        hot_height = FAT_FINGER_MAX_H;
    }
    int halfHot = hot_height / 2;
    int hotDrawingWidth = FAT_FINGER_WIDTH - halfHot;
    //QRect rct(0, 0, hotDrawingWidth, hot_height);
    //QRect rct(rc.right() - hotDrawingWidth, 0, hotDrawingWidth, hot_height);
    auto rct = nonidentedRect();
    rct.setLeft(rct.right() - hotDrawingWidth);
    rct.setWidth(hotDrawingWidth);
    rct.setHeight(hot_height);
    painter->drawRect(rct);
    //auto rcico = rct;
    /*
    QRect rcico;
    rectf2rect(rct, rcico);
    rcico.setLeft(rcico.right() - ICON_WIDTH);
    rcico.setBottom(rcico.top() + ICON_WIDTH);
    rcico.setWidth(ICON_WIDTH);
    rcico.setHeight(ICON_WIDTH);
    painter->drawRect(rct);
    auto pm = getIcon_VDetails();
    painter->drawPixmap(rcico, pm);
    */
    //painter->drawPixmap(rcico, getIcon_VDetails());

    rct.setRect(rct.left() - halfHot, rct.top(), hot_height, hot_height);
    static int degree90 = 90 * 16;
    painter->drawPie(rct, degree90, 2 * degree90);

	if (m_attr.Hover)
	{
		if (!ard::isSelectorLinked()) {
			QRect rcico;
			rectf2rect(rct, rcico);
			painter->drawPixmap(rcico, getIcon_PopupUnlocked());
		}
	}
};

EHitTest OutlineGItem::hitTestIdentedRect(const QRectF& rc1, const QPointF& pt, SHit& hit)
{
    Q_UNUSED(hit);
    topic_ptr it = proto()->topic();

    QRectF rc2 = rc1;
    QRectF rc_check = rc2;
    bool hasMainIcon = !it->getIcon(proto()->p()->ocontext()).isNull();
    if (hasMainIcon)
    {
        rc_check.setRight(rc_check.left() + ICON_WIDTH);
        if (rc_check.contains(pt)){
            return hitMainIcon;
        }
        rc2.setLeft(rc2.left() + ICON_WIDTH);
        rc_check = rc2;
    }

    bool hasSecIcon = !it->getSecondaryIcon(proto()->p()->ocontext()).isNull();
    if (hasSecIcon)
    {
        rc_check.setRight(rc_check.left() + ICON_WIDTH);
        if (rc_check.contains(pt)) {
            return hitSecondaryIcon;
        }
        rc2.setLeft(rc2.left() + ICON_WIDTH);
        rc_check = rc2;
    }

    //..
    if (it->hasColorByHashIndex()) {
        auto ch = it->getColorHashIndex();
        if (ch.second != 0) {
            if (hasMainIcon) {
                //has to be something else
            }
            else {
                rc_check.setRight(rc_check.left() + ICON_WIDTH);
                if (rc_check.contains(pt)) {
                    return hitColorHash;
                }
                rc2.setLeft(rc2.left() + ICON_WIDTH);
                rc_check = rc2;
            }
        }
    }
    //...
    /*
    if (!it->IsUtilityFolder() && it->colorIndex() != 0)
    {
        rc_check.setRight(rc_check.left() + ICON_WIDTH);
        if (rc_check.contains(pt)) {
            return hitColorBox;
        }
        rc2.setLeft(rc2.left() + ICON_WIDTH);
        rc_check = rc2;
    }*/
    //...
    auto t3 = it->ternaryIconWidth();
    if (t3.first != TernaryIconType::none)
    {
        rc_check = rc2;
        rc_check.setRight(rc_check.left() + t3.second);
        if (rc_check.contains(pt)) {
            return hitTernaryIconArea;
        }
        rc2.setLeft(rc2.left() + t3.second);
        //rc.setLeft(rc.left() + t3.second);
    }

    if (!isRootTopic() && 
        proto()->p()->hasProp(ProtoPanel::PP_FatFingerSelect) &&
        proto()->topic()->hasFatFinger())
    {
        if (proto()->g()->isSelected()) 
        {
            int hot_height = nonidentedRect().height();
            if (hot_height > FAT_FINGER_MAX_H) {
                hot_height = FAT_FINGER_MAX_H;
            }
            //QRectF rct = QRectF(rc2.right() - ICON_WIDTH, nonidentedRect().top(), ICON_WIDTH, hot_height);
            //if (rct.contains(pt)) 
            //{
            //  return hitFatFingerDetails;
            //}
            auto rct = QRectF(rc2.right() - FAT_FINGER_WIDTH, nonidentedRect().top(), FAT_FINGER_WIDTH, hot_height);
            if (rct.contains(pt))
            {
                return hitFatFingerSelect;
            }
        }
    }

	if (m_attr.rbutton_width > 0)
	{
		rc_check = rc2;
		rc_check.setLeft(rc_check.right() - m_attr.rbutton_width);
		if (rc_check.contains(pt))
		{
			return hitRIcon;
		}
	}

    rc_check = rc2;
    rc_check.setRight(rc_check.left() + utils::calcWidth(it->title()));
    if (rc_check.contains(pt))
    {
        return hitTitle;
    }



    return hitAfterTitle;
}

void OutlineGItem::implement_hoverEnterEvent(QGraphicsSceneHoverEvent *)
{
    m_attr.Hover = 1;
    proto()->g()->update();
};

void OutlineGItem::implement_hoverLeaveEvent(QGraphicsSceneHoverEvent *)
{
    m_attr.Hover = 0;
    proto()->g()->update();
}

void OutlineGItem::implement_hoverMoveEvent(QGraphicsSceneHoverEvent * e)
{
    proto()->processHoverMoveEvent(e);
};

bool OutlineGItem::implement_preprocessMousePress(const QPointF& p, bool wasSelected)
{
#define    EXEC_AND_RETURN(F)proto()->F(); return true;

    QPointF pt = proto()->g()->mapFromScene(p);
    SHit sh;
    EHitTest hit = proto()->hitTest(pt, sh);
    switch(hit)
        {
        case hitToDo:EXEC_AND_RETURN(on_clickedToDo);
        case hitFatFingerSelect:   EXEC_AND_RETURN(on_clickedFatFingerSelect);
        case hitFatFingerDetails: 
        {
            auto v1 = proto()->p()->s()->v()->v();
            auto pt1 = v1->mapFromScene(p);
            auto pt2 = v1->mapToGlobal(pt1);
            proto()->on_clickedFatFingerDetails(pt2); 
            return true; //EXEC_AND_RETURN(on_clickedFatFingerDetails);
        }
        case hitActionBtn:    EXEC_AND_RETURN(on_clickedActionBtn);
        case hitAnnotation: 
        {
            proto()->on_clickedAnnotation(p, wasSelected); 
            return true;
        }break;
        case hitExpandedNote: 
        {
            proto()->on_clickedExpandedNote(p, wasSelected);
            return true;
        }break;
        case hitHotSpot:          
            {
                auto spot_item = sh.item;
                auto f = proto()->topic();
                ASSERT(f, "Invalid folder object");
                if(!f->isExpanded())
                    {
                        gui::setupMainOutlineSelectRequestOnRebuild(spot_item);
                        f->setExpanded(true);
                        model()->setAsyncCallRequest(AR_rebuildOutline);
                    }
                else
                    {
                        gui::ensureVisibleInOutline(spot_item);
                    } 
            }
            return true;
        case hitMainIcon: 
        {
            if(proto()->p()->ocontext() == OutlineContext::check2select){
                auto f = proto()->topic()->shortcutUnderlying();
                if (!f->canCloseLocusTab()) {
					ard::messageBox(gui::mainWnd(), "Selected Folder is required in locused tab. It can't be unchecked in current edition.");
                    return true;
                }

                f->setInLocusTab(!f->isInLocusTab());
                ard::asyncExec(AR_RebuildLocusTab);
                proto()->g()->update();
                return true;
            }
            
            proto()->p()->s()->storeMSelected();
            EXEC_AND_RETURN(processDefaultAction);
        }break;
        case hitSecondaryIcon: 
        {
            auto f = proto()->topic()->shortcutUnderlying();
            auto u = dynamic_cast<ard::anUrl*>(f);
            if (u) {
                u->openUrl();
            }
        }break;
		case hitRIcon:break;
        case hitColorHash:
        {
            EXEC_AND_RETURN(on_clickedFatFingerSelect);
        }return true;
        case hitTernaryIconArea: {
            EXEC_AND_RETURN(on_clickedTernary);
        }break;

        case hitAfterTitle:
        case hitTitle:   
           // m_renameInit = wasSelected;
            return false;
        case hitUrl:              EXEC_AND_RETURN(on_clickedUrl);
        case hitTableColumn:
            {
                TablePanel* tp = dynamic_cast<TablePanel*>(proto()->p());
                if(tp)
                    {
                        assert_return_false(tp->columns().size() > (unsigned int)sh.column_number && sh.column_number > -1, "invalid column index");
                        TablePanelColumn* c = tp->columns()[sh.column_number];
                        if(onHitColumn(c, sh, p, wasSelected))return true;
                    }
                else      
                    {
                        ASSERT(0, "expected table panel");
                    }
            };break;
        //case hitColorBox: {EXEC_AND_RETURN(on_clickedColorBox); }break;
        default:break;  
        }
    return false;
#undef EXEC_AND_RETURN
};

bool OutlineGItem::implement_mousePressEvent(QGraphicsSceneMouseEvent *e) 
{
    auto g = proto()->g();
    auto pt_scene = g->mapToScene(e->pos());

    bool wasSelected = g->isSelected();    
    if (!wasSelected) {
        proto()->p()->s()->s()->clearSelection();
        g->setSelected(true);
    }
    if (e->button() == Qt::LeftButton && proto()->preprocessMousePress(pt_scene, wasSelected)) {
        return true;
    }

    
    if (e->button() == Qt::RightButton) {
        auto v1 = proto()->p()->s()->v()->v();
        auto pt1 = v1->mapFromScene(pt_scene);
        auto pt2 = v1->mapToGlobal(pt1);
        proto()->mouseRightClick(pt2);
        return true;
    }

    return false;
};

void OutlineGItem::implement_mouseReleaseEvent(QGraphicsSceneMouseEvent *e) 
{
    proto()->g()->setCursor(Qt::ArrowCursor);
    if (m_renameInit) {
        auto g = proto()->g();
        auto pt_scene = g->mapToScene(e->pos());
        proto()->on_clickedTitle(pt_scene, true);
    }
	proto()->p()->s()->installCurrentItemView(true);
};

void OutlineGItem::implement_mouseMoveEvent(QGraphicsSceneMouseEvent *e) 
{
    if (canStartDnD(e)) {
        auto scene = proto()->p()->s();
        m_renameInit = false;
        auto *drag = new QDrag(scene->v()->v());
        TOPICS_LIST lst = scene->mselected();
        if (!lst.empty()) {
            drag->setMimeData(new ard::TopicsListMime(lst.begin(), lst.end()));
        }
        else {
            drag->setMimeData(new ard::TopicsListMime(proto()->topic()));
        }
        Qt::DropAction dropAction = drag->exec();
        Q_UNUSED(dropAction);
    }
};


bool OutlineGItem::onHitColumn(TablePanelColumn* c, const SHit& sh, const QPointF& pt, bool wasSelected)
{
    TablePanel* tp = dynamic_cast<TablePanel*>(proto()->p());
    assert_return_false(tp, "expected table panel");

    tp->setLastActiveColumn(EColumnType::Uknown);
    
    switch(c->type())
        {
    case EColumnType::Selection:
            {
                if(tp->topic() == proto()->topic())
                    return false;
    
#define TMP_SELECTION_METHOD_NAME "outlineTmpSelectionBoxPressed"

                //proto()->topic()->toggleTmpSelected();
                proto()->topic()->setTmpSelected(!proto()->topic()->isTmpSelected());
                QWidget* w = proto()->p()->s()->v()->v()->parentWidget();
                if(w)
                    {
                        bool handlerFound = false;
                        while(w && !handlerFound)
                            {
                                const QMetaObject* mo = w->metaObject();
                                int idx = mo->indexOfSlot("outlineTmpSelectionBoxPressed(int,int)");
                                if(idx == -1)
                                    {
                                        w = w->parentWidget();
                                    }
                                else
                                    {
                                        handlerFound = true;
                                    }
                            }

                        if(w)
                            {
                                QMetaObject::invokeMethod(w, TMP_SELECTION_METHOD_NAME,
                                                          Qt::QueuedConnection,
                                                          Q_ARG(int, proto()->topic()->underlyingDbid()),
                                                          Q_ARG(int, sh.column_number));
                            }
                        else
                            {
                                //ASSERT(0, "failed to find parent with slot:") << TMP_SELECTION_METHOD_NAME;
                            }
                    }
                else
                    {
                        ASSERT(0, "expected parent widget");
                    }
            }return true;
        case EColumnType::Title:
        case EColumnType::ContactTitle:
        case EColumnType::ContactEmail:
        case EColumnType::ContactPhone:
        case EColumnType::ContactAddress:
        case EColumnType::FormFieldValue:
            {
                proto()->p()->setLastActiveColumn(c->type());
                if(wasSelected)
                    {
                        proto()->p()->s()->v()->renameSelected(c->type(), &pt);
                    }
            }break;

        case EColumnType::KRingTitle:
        case EColumnType::KRingLogin:
        case EColumnType::KRingLink:
        case EColumnType::KRingNotes:
        case EColumnType::KRingPwd:
        {
            if (ard::isDbConnected()) {
                auto r = ard::db()->kmodel()->keys_root();
                if (r->isRingLocked()) {
                    r->guiUnlockRing();
                    return false;
                }

                proto()->p()->setLastActiveColumn(c->type());
                if (wasSelected)
                {
                    proto()->p()->s()->v()->renameSelected(c->type(), &pt);
                }
            }
        }break;
        default:break;
        }
    return false;
};

void OutlineGItem::implement_keyPressEvent(QKeyEvent * e)
{
    topic_ptr it = proto()->topic();
    ProtoScene* ps = proto()->p()->s();
    bool AltPressed = e->modifiers() & Qt::AltModifier;
    bool keymod = AltPressed;

    
    switch (e->key())
    {
    case Qt::Key_Down:
    {
        if (keymod)
        {
            ard::moveInsideParent(it, false);
            model()->setAsyncCallRequest(AR_rebuildOutline);
        }
        else
        {
            ps->selectNext(false);
        }
    }break;

    case Qt::Key_Up:
    {
        if (keymod)
        {
            ard::moveInsideParent(it, true);
            model()->setAsyncCallRequest(AR_rebuildOutline);
        }
        else
        {
            ps->selectNext(true);
        }
    }break;

    case Qt::Key_Left:
    {
        if (keymod)
        {
            outline()->toggleMoveLeft();
        }
        else
        {
            if (it->isExpanded())
            {
                it->setExpanded(false);
                model()->setAsyncCallRequest(AR_rebuildOutline, 0, 0, 0, proto()->p()->s()->v()->sceneBuilder());
            }
        }
    }break;

    case Qt::Key_Right:
    {
        if (keymod)
        {
            outline()->toggleMoveRight();
            //NA
            //dbp::moveInsideParent(it, false);
            //ps->setAsyncCallRequest(AR_rebuildOutline);
        }
        else
        {
            if (!it->isExpanded())
            {
                it->setExpanded(true);
                model()->setAsyncCallRequest(AR_rebuildOutline, 0, 0, 0, proto()->p()->s()->v()->sceneBuilder());
            }
        }
    }break;
    case Qt::Key_Return:
    case Qt::Key_Delete:
    {
        proto()->on_keyPressed(e);
    }break;
    case Qt::Key_F2: 
    {
        QPointF pt{0,0};
        proto()->p()->s()->v()->renameSelected(EColumnType::Title, &pt);
    }break;
    case Qt::Key_N:
    {        
        if (keymod)
        {
            ard::insert_new_topic();
        }
    }break;
    }
};


EDragOver OutlineGItem::calcDragItemOver(topic_cptr , QGraphicsSceneDragDropEvent *)
{
    return dragUnknown;
}

EDragOver OutlineGItem::calcDragGenericOver(QGraphicsSceneDragDropEvent *e)
{
    EDragOver dv = dragUnknown;
    QMimeData* md = (QMimeData*)e->mimeData();

    auto this_topic = proto()->topic();

    auto tmd = qobject_cast<const ard::TopicsListMime*>(md);
    if (tmd) {
        auto lst = tmd->topics();
        for (auto& i : lst) {
			if (i == this_topic) {
				//qDebug() << "dragUnknown/this";
				return dragUnknown;
			}
			if (i->isAncestorOf(this_topic)) {
				//qDebug() << "dragUnknown/isAncestorOf";
				return dragUnknown;
			}
        }
    }

    if (md->hasUrls() ||
        md->hasText() ||
        md->hasHtml() ||
		md->hasImage() ||
        tmd)
    {
        const QPointF ptInSceneCoord = e->scenePos();
        QPointF  pt2 = proto()->g()->mapFromScene(ptInSceneCoord);

        QRectF rc = proto()->g()->boundingRect();

        qreal dy = (rc.bottom() - rc.top()) / 3;
        if (pt2.y() < rc.top() + dy)
        {
			//qDebug() << "dragAbove";
            dv = dragAbove;
        }
        else if (pt2.y() > rc.top() + 2 * dy)
        {
			//qDebug() << "dragBelow";
            dv = dragBelow;
        }
        else
        {
			//qDebug() << "dragInside";
            dv = dragInside;
        }
    }

	//if (dv == dragUnknown) {
	//	qDebug() << "dragUnknown";
	//}

    return dv;
};

EDragOver OutlineGItem::calcDragOverType(QGraphicsSceneDragDropEvent *event)
{
    EDragOver dv = dragUnknown;

    auto our_parent = proto()->topic()->parent();
    if(!our_parent)
        {
            return dragUnknown;
        }

    QMimeData* md = (QMimeData*)event->mimeData();
    if(!md)
        return dragUnknown;

	auto generic_md = md->hasUrls() ||
		md->hasText() ||
		md->hasHtml() ||
		md->hasImage();

    auto tmd = qobject_cast<const ard::TopicsListMime*>(md);

    if (generic_md || tmd)
    {
        dv = calcDragGenericOver(event);
    }

    return dv;
};

void OutlineGItem::dropGeneric(QGraphicsSceneDragDropEvent *e)
{
    QMimeData* md = (QMimeData*)e->mimeData();
    if (!md)
        return;

    EDragOver dv = calcDragGenericOver(e);

    auto this_topic = proto()->topic();
    assert_return_void(this_topic, "expected topic");
    auto grand_parent = this_topic->parent();
    int this_topic_index = -1;

    if (dv != dragInside)
    {
        if (!grand_parent)
        {
            dv = dragInside;
        }
        else
        {
            this_topic_index = grand_parent->indexOf(this_topic);
        }
    }

    if (dv != dragInside)
    {
        assert_return_void(grand_parent, "expected parent topic");
        assert_return_void(this_topic_index != -1, "expected valid topic index");
    }

    topic_ptr destination_parent = nullptr;
    int destination_pos = -1;

    switch (dv)
    {
    case dragInside:
    {
        destination_parent = this_topic;
        destination_pos = 0;
    }break;
    case dragAbove:
    {
        destination_parent = grand_parent;
        destination_pos = this_topic_index;
    }break;
    case dragBelow:
    {
        destination_parent = grand_parent;
        destination_pos = this_topic_index + 1;
    }break;
    default:
        assert_return_void(0, "undefined type");
    }


    auto lst = ard::insertClipboardData(destination_parent, destination_pos, md, true);
	gui::rebuildOutline();
	if (!lst.empty())
	{
		auto created = *lst.begin();
		gui::ensureVisibleInOutline(created);
	}
};


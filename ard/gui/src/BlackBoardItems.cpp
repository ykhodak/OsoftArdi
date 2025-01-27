#include <QApplication>
#include <QToolTip>
#include <QWidgetAction>
#include "BlackBoardItems.h"
#include "BlackBoard.h"
#include "popup-widgets.h"
#include "ardmodel.h"
#include "utils.h"

#include "bezierinterpolator.h"
#include "custom-boxes.h"
#include "contact.h"
#include "custom-menus.h"
#include "picture.h"

QFont* defaultBoardLabelFont() {
    return ard::defaultSmallFont();
}

extern QString textFilesExtension();
extern QString imageFilesExtension();

QSize calcAnnotationSize(int band_width, QString annotation)
{
    QSize sz;
    QRect rc(0, 0, band_width, 3 * gui::lineHeight());
    QFontMetrics fm(*ard::defaultSmallFont());
    QRect rc2 = fm.boundingRect(rc, Qt::TextWordWrap, annotation);
    auto maxLineH = 2 * gui::lineHeight();
    if (rc2.height() > maxLineH) {
        rc2.setHeight(maxLineH);
    }
    sz = rc2.size();
    return sz;
}


int approximateDefaultHeightInBoardBand(topic_ptr f, int band_width)
{
    int height = 0;
    auto annotation = f->annotation4outline();
    if (!annotation.isEmpty()) {
        QSize szAnnotation = calcAnnotationSize(band_width, annotation);
        height += szAnnotation.height();
    }
    /*
    auto s = f->title().trimmed();
    if (s.isEmpty()) {
        s = "W";
    }

    auto szTitle = calcBTextSize(s, ard::defaultSmallFont(), 7);
    */
    auto szTitle = f->calcBlackboardTextBoxSize();
    auto title_height = szTitle.height();
    if (title_height == 0) {
        title_height = gui::lineHeight();
    }

    height += title_height;
    height += 2 * BOXING_MARGIN;

    return height;
};


/**
GBLink
*/
ard::GBLink::GBLink(BlackBoard* bb, ard::board_link_list* link_list, ard::board_link* blnk, black_board_g_topic* origin, black_board_g_topic* target)
    :m_bb(bb), m_link_list(link_list), m_blnk(blnk), m_origin(origin), m_target(target)
{
    setPen(QPen(Qt::white, 2));
    setFlags(QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemIsFocusable);
    setZValue(BBOARD_ZVAL_EDGE - 0.01 * blnk->linkPindex());
    LOCK(m_blnk);
};

ard::GBLink::~GBLink() 
{
    m_blnk->release();
};

bool ard::GBLink::isIdentityLink()const 
{
    bool rv = false;
    if (m_origin && m_target) {
        rv = (m_origin->bitem() == m_target->bitem());
    }
    return rv;
};

class distance_calc 
{
public:
    distance_calc
	(const QPointF& o, const QPointF& t, qreal preferance = 1.0):m_o(o), m_t(t), m_preferance(preferance)
    {
        m_distance = preferance * std::hypot(m_o.x() - m_t.x(), m_o.y() - m_t.y());
        //qDebug() << "preferance=" << preferance << "dist=" << m_distance;
    };

    qreal distance()const {return m_distance;}
    const QPointF& originPt()const { return m_o; }
    const QPointF& targetPt()const { return m_t; }

protected:
    QPointF m_o, m_t;
    qreal   m_preferance{1.0};
    qreal   m_distance;

};

using DCALC_ARR = std::vector<distance_calc>;

void ard::GBLink::recalcStartEndPoints() 
{
    QRectF rcOrigin, rcTarget;
    //  QPointF ptOrigin, ptTarget;

    auto origin_bidx = m_origin->bitem()->bandIndex();
    auto target_bidx = m_target->bitem()->bandIndex();

    if (ard::isBoxlikeShape(m_origin->bitem()->bshape())) {
        rcOrigin = m_origin->mapRectToScene(m_origin->box_rect());
    }
    else{   
        rcOrigin = m_origin->mapRectToScene(m_origin->boundingRect());
    }
    qreal origin_my = (rcOrigin.top() + rcOrigin.bottom()) / 2;
    qreal origin_mx = (rcOrigin.left() + rcOrigin.right()) / 2;

    if (isIdentityLink()) {
        m_originPt = QPointF(rcOrigin.left(), origin_my);
        m_targetPt = QPointF(origin_mx, rcOrigin.top());
        return;
    }

    if (ard::isBoxlikeShape(m_target->bitem()->bshape())) {
        rcTarget = m_target->mapRectToScene(m_target->box_rect());
    }
    else{
        rcTarget = m_target->mapRectToScene(m_target->boundingRect());
    }
    qreal target_my = (rcTarget.top() + rcTarget.bottom()) / 2;
    qreal target_mx = (rcTarget.left() + rcTarget.right()) / 2;


    bool l2r = (origin_bidx < target_bidx);
    bool t2b = (rcOrigin.bottom() < rcTarget.bottom());

    QPointF pt1_origin;
    QPointF pt1_target;
    QPointF pt2_origin;
    QPointF pt2_target;

    if (l2r)
    {
        pt1_origin = QPointF(rcOrigin.right(), origin_my);
        pt1_target = QPointF(rcTarget.left(), target_my);
    }
    else {
        pt1_origin = QPointF(rcOrigin.left(), origin_my);
        pt1_target = QPointF(rcTarget.right(), target_my);
    }

    if (t2b) {
        pt2_origin = QPointF(origin_mx, rcOrigin.bottom());
        pt2_target = QPointF(target_mx, rcTarget.top());
    }
    else {
        pt2_origin = QPointF(origin_mx, rcOrigin.top());
        pt2_target = QPointF(target_mx, rcTarget.bottom());
    }


    DCALC_ARR darr;
    darr.emplace_back(distance_calc(pt1_origin, pt1_target));
    //darr.emplace_back(distance_calc(pt1_origin, pt2_target, 0.9));//.
    darr.emplace_back(distance_calc(pt2_origin, pt1_target));
    //darr.emplace_back(distance_calc(pt2_origin, pt2_target, 0.9));//..
    if (origin_bidx == target_bidx) {
        darr.emplace_back(distance_calc(pt1_origin, pt2_target, 0.9));//.
        darr.emplace_back(distance_calc(pt2_origin, pt2_target, 0.9));//..
    }
    std::sort(darr.begin(), darr.end(), [](const distance_calc& d1, const distance_calc& d2)
    {
        return (d1.distance() < d2.distance());
    });

    //qDebug() << m_blnk->linkLabel() << "min-dist" << darr[0].distance() << darr[0].originPt() << darr[0].targetPt();

    m_originPt = darr[0].originPt();
    m_targetPt = darr[0].targetPt();
};

std::pair<QPointF, QPointF> ard::GBLink::recalcControlPoints()
{
    std::pair<QPointF, QPointF> rv;

    auto kx = std::abs(m_originPt.x() - m_targetPt.x());
    auto ky = std::abs(m_originPt.y() - m_targetPt.y());

    qreal len = std::hypot(kx, ky);
    if (len > 0) {
        if (isIdentityLink()) {
            auto at = 1.57079;//pi/2
            if (ky > 0.0) {
                at = atan(kx / ky);
            }
            len += m_blnk->linkPindex() * 50;
            qreal dx = len * cos(at);
            qreal dy = len * sin(at);
            //qDebug() << "identity-ctrl" << at << len << dx << dy;
            rv.first = QPointF(m_originPt.x() - dx, m_originPt.y() - dy);
            rv.second = QPointF(m_targetPt.x() - dx, m_targetPt.y() - dy);
            return rv;
        }


        qreal cx1 = (2.0 * m_originPt.x() / 3.0 + m_targetPt.x() / 3.0);
        qreal cx2 = (m_originPt.x() / 3.0 + 2.0 * m_targetPt.x() / 3.0);
        qreal cy1 = (2.0 * m_originPt.y() / 3.0 + m_targetPt.y() / 3.0);
        qreal cy2 = (m_originPt.y() / 3.0 + 2.0 * m_targetPt.y() / 3.0);
        

        /*
        qreal cx1 = (m_originPt.x() / 4.0 + m_targetPt.x() * 3.0/ 4.0);
        qreal cx2 = (3 * m_originPt.x() / 4.0 + m_targetPt.x() / 4.0);
        qreal cy1 = (m_originPt.y() / 4.0 + 3.0 * m_targetPt.y() / 4.0);
        qreal cy2 = (3.0 * m_originPt.y() / 4.0 + m_targetPt.y() / 4.0);
        */

        qreal diff_y = m_targetPt.y() - m_originPt.y();
        qreal diff_x = m_targetPt.x() - m_originPt.x();
        qreal sin_angle = std::abs(diff_y) / len;
        qreal cos_angle = std::abs(diff_x) / len;
        //qreal sin_angle = (m_targetPt.y() - m_originPt.y()) / len;
        //qreal cos_angle = (m_targetPt.x() - m_originPt.x()) / len;

        qreal control_delta = 50;
        if (m_blnk->linkPindex() > 1) {
            int idx_mult = m_blnk->linkPindex() / 2;
            control_delta += idx_mult * 30;
        }
        if (m_blnk->linkPindex() % 2) {
            control_delta = -control_delta;
        }

        qreal dx = control_delta * sin_angle;
        qreal dy = control_delta * cos_angle;

        
        /*
        if (diff_y < 0 && diff_x < 0) {
            dx *= -1;
            dy *= -1;
        }*/
        

        /*
        if (diff_y < 0) {
            dy *= -1;
        }

        if (diff_x < 0) {
            dx *= -1;           
        }
        */
        if (diff_x < 0) {
            dx *= -1;
            dy *= -1;
        }



        //qDebug() << "len=" << len << "dx=" << dx << "dy=" << dy << m_target->bitem()->refTopic()->title();

        rv.first = QPointF(cx1 - dx, cy1 - dy);
        rv.second = QPointF(cx2 - dx, cy2 - dy);

    }
    return rv;
};

void ard::GBLink::rebuildPathWithNewCurveControlPoints(const std::pair<QPointF, QPointF>& c) 
{
    if (m_cp1) {
        m_cp1->setPos(c.first);
    }
    if (m_cp2) {
        m_cp2->setPos(c.second);
    }

    auto& bz = ard::BezierController::instance();
    QVector<QPointF> cpts;
    cpts.push_back(m_originPt);
    cpts.push_back(c.first);
    cpts.push_back(c.second);
    cpts.push_back(m_targetPt);
    QPolygonF boorNetPoints;
    bz.bz_intp.CalculateBoorNet(cpts, bz.knotVector, boorNetPoints);
    QPolygonF ipts;
    ipts.push_back(cpts.first());
    for (int counter = 0; counter < boorNetPoints.size() - 3; counter += 3)
        bz.bz_intp.InterpolateBezier(boorNetPoints[counter],
            boorNetPoints[counter + 1],
            boorNetPoints[counter + 2],
            boorNetPoints[counter + 3],
            ipts);
    ipts.push_back(cpts.last());

    QPainterPath link_path;
    link_path.addPolygon(ipts);
    setPath(link_path);

    if (m_arrow) {
        QPointF ptArrowEnd = link_path.pointAtPercent(1.0);
        QPointF ptArrowBegin = link_path.pointAtPercent(0.95);
        m_arrow->recalcArrow(ptArrowBegin, ptArrowEnd);
    }

    if (m_ctrl) {
        qreal pp = 0.01 + 0.05 * m_blnk->linkPindex();
        QPointF ptCtrl = link_path.pointAtPercent(pp);
        const int dx = BBOARD_LINK_CONTROL_WIDTH / 2;
        ptCtrl.setX(ptCtrl.x() - dx);
        ptCtrl.setY(ptCtrl.y() - dx);
        m_ctrl->setPos(ptCtrl);
    }

    rebuildLabel();
};

void ard::GBLink::rebuildLabel() 
{
    auto link_path = path();
    if (m_label) {
        if (isIdentityLink()) {
            //qreal pp = 0.2 + 0.1 * m_blnk->linkPindex();
            qreal pp = 0.5;
            QPointF pt = link_path.pointAtPercent(pp);
            auto agle = link_path.angleAtPercent(pp);
            m_label->resetLabel(pt, agle);
        }
        else 
        {
            bool target_to_the_right = (m_target->bitem()->bandIndex() > m_origin->bitem()->bandIndex());

            qreal pp = 0.2 + 0.1 * m_blnk->linkPindex();
            if (target_to_the_right) {
                pp = pp + m_link_list->rpos_index() * 0.05;
            }
            else {
                pp = pp + m_link_list->lpos_index() * 0.05;
            }

			if (pp > 1.0) {
				pp = 1.0;
				//ASSERT(0, "invalid pointAtPercent") << pp;
			}

            QPointF pt = link_path.pointAtPercent(pp);
            if (!m_blnk->linkLabel().isEmpty())
            {
                if (m_blnk->linkPindex() == 0) {
                    QPointF pt_end = link_path.pointAtPercent(0.0);
                    QLineF lbl_line(pt, pt_end);
                    auto lblw = utils::calcWidth(m_blnk->linkLabel(), defaultBoardLabelFont());
                    auto line_len = lbl_line.length();
                    if (lblw > line_len) {
                        pt = link_path.pointAtPercent(0.4);
                    }
                    //qDebug() << "<<< blink-label" << m_blnk->linkLabel() << "l-len=" << line_len << "lblw=" << lblw << pt << link_path.length();
                }
            }

            /*
            if (target_to_the_right) {
                qDebug() << "<<< blink/r" << m_blnk->linkLabel() << m_blnk->linkPindex() << "r-idx" << m_link_list->rpos_index() << pp;
            }
            else {
                qDebug() << "<<< blink/l" << m_blnk->linkLabel() << m_blnk->linkPindex() << "l-idx" << m_link_list->lpos_index() << pp;
                }*/


            if (target_to_the_right) {
                //pp = pp + 0.1 + m_link_list->rpos_index() * 0.2;
                //qDebug() << "<<< blink/r" << m_blnk->linkLabel() << m_blnk->linkPindex() << m_link_list->rpos_index() << pp;
				auto pp1 = pp + 0.1;
				if (pp1 > 1.0) {
					pp1 = 1.0;
				}
				auto agle = link_path.angleAtPercent(pp1);
                m_label->resetLabel(pt, agle);
            }
            else {
                //pp = pp - (0.1 + m_link_list->lpos_index() * 0.2);
                //qDebug() << "<<< blink/l" << m_blnk->linkLabel() << m_blnk->linkPindex() << m_link_list->lpos_index() << pp;
                
				auto pp1 = pp - 0.1;
				if (pp1 < 0.0) {
					pp1 = 0.0;
				}
				auto agle = link_path.angleAtPercent(pp1);
                m_label->resetLabel(pt, agle + 180);
            }
            /*
            if (m_target->bitem()->bandIndex() < m_origin->bitem()->bandIndex()) {
                //qreal pp = 0.9 - 0.1 * m_blnk->linkPindex();
                //QPointF pt = link_path.pointAtPercent(pp);
                auto agle = link_path.angleAtPercent(pp - 0.1);
                m_label->resetLabel(pt, agle + 180);
            }
            else {
                //qreal pp = 0.2 + 0.1 * m_blnk->linkPindex();
                //QPointF pt = link_path.pointAtPercent(pp);
                auto agle = link_path.angleAtPercent(pp + 0.1);
                m_label->resetLabel(pt, agle);
            }
            */
        }
    }
};

void ard::GBLink::resetLink() 
{
    recalcStartEndPoints();
    auto c = recalcControlPoints();
    //qDebug() << "curve-point" << c.first << c.second;

    /*
    qreal cx1 = (m_originPt.x()/3.0 + 2*m_targetPt.x()/3.0);
    qreal cx2 = (2 * m_originPt.x() / 3.0 + m_targetPt.x() / 3.0);
    qreal cy1 = (m_originPt.y() / 3.0 + 2 * m_targetPt.y() / 3.0);
    qreal cy2 = (2 * m_originPt.y() / 3.0 + m_targetPt.y() / 3.0);

    QPointF c1(cx1, cy1); 
    QPointF c2(cx2, cy2);
    */

    /*
    qreal mx = (m_originPt.x() + m_targetPt.x()) / 2;
    qreal my = (m_originPt.y() + m_targetPt.y()) / 2;

    QPointF c1, c2;
    if (m_blnk->linkPindex() == 0) {
        c1 = QPointF(mx, m_originPt.y());
        c2 = QPointF(m_targetPt.x(), my);
    }
    else if (m_blnk->linkPindex() == 1) {
        c1 = QPointF(m_originPt.x(), my);
        c2 = QPointF(mx, m_targetPt.y());
    }   

    std::pair<QPointF, QPointF> c{ c1,c2 };
    */

    rebuildPathWithNewCurveControlPoints(c);
};

void ard::GBLink::updateLink()
{
    update();
    if (m_arrow) {
        m_arrow->update();
    }
    if (m_label) {
        m_label->update();
    }
};

void ard::GBLink::markAsSelected()
{
    m_sel_mode = ESelectionMode::selected;
    updateLink();
    if (m_cp1)
        m_cp1->show();
    if (m_cp2)
        m_cp2->show();
    if (m_ctrl)
        m_ctrl->show();
};

void ard::GBLink::markAsControlSelected()
{
    m_sel_mode = ESelectionMode::control_selected;
    updateLink();
    if (m_cp1)
        m_cp1->show();
    if (m_cp2)
        m_cp2->show();
    if (m_ctrl)
        m_ctrl->show();
};

void ard::GBLink::markAsNormal()
{
    m_sel_mode = ESelectionMode::normal;
    updateLink();
    if (m_cp1)
        m_cp1->hide();
    if (m_cp2)
        m_cp2->hide();
    if (m_ctrl)
        m_ctrl->hide();
};

void ard::GBLink::onMovedGLinkCurveControl() 
{
    if (m_cp1 && m_cp2) 
    {
        QPointF c1 = m_cp1->scenePos();
        QPointF c2 = m_cp2->scenePos();
        std::pair<QPointF, QPointF> c{ c1,c2 };
        rebuildPathWithNewCurveControlPoints(c);
    }
};

void ard::GBLink::processKey(int key) 
{
    qDebug() << "got-link-key" << key;
    switch (key)
    {
    case Qt::Key_Return:
    case Qt::Key_Enter:
    {
        bb()->editLink(this);
    }break;
    case Qt::Key_Delete:
    {
        bb()->removeCurrentControlLink();
    }break;
    };
};

void ard::GBLink::processDoubleClick() 
{
    bb()->editLink(this);
};

void ard::GBLink::keyPressEvent(QKeyEvent * e) 
{
    QGraphicsPathItem::keyPressEvent(e);
    processKey(e->key());
};

void ard::GBLink::paint(QPainter * p, const QStyleOptionGraphicsItem* , QWidget* ) 
{
    PGUARD(p);
    p->setRenderHint(QPainter::Antialiasing, true);

    switch (m_sel_mode) 
    {
    case ESelectionMode::normal: 
    {
        p->setPen(QPen(Qt::white, 3));
    }break;
    case ESelectionMode::selected: 
    {
        p->setPen(QPen(BBOARD_CURR_MARK_COLOR, 4));
    }break;
    case ESelectionMode::control_selected: 
    {
        p->setPen(QPen(BBOARD_CURR_LINK_CTRL_COLOR, 4));
    }break;
    }

    p->drawPath(path());
};

void ard::GBLink::mousePressEvent(QGraphicsSceneMouseEvent* e)
{
    QGraphicsPathItem::mousePressEvent(e);
    bb()->setCurrentControlLink(blink());
}

void ard::GBLink::mouseDoubleClickEvent(QGraphicsSceneMouseEvent* e)
{
    QGraphicsPathItem::mouseDoubleClickEvent(e);
    processDoubleClick();
};

std::vector<QGraphicsItem*> ard::GBLink::produceLinkItems()
{
    ASSERT(m_arrow == nullptr, "create arrow olny once");
    std::vector<QGraphicsItem*> rv;
    m_arrow = new GBLinkArrow(this);
    rv.push_back(m_arrow);
    //..
#ifdef  SHOW_CURVE_CTRL
    m_cp1 = new GBLinkCurvePoint(this);
    m_cp2 = new GBLinkCurvePoint(this);
    rv.push_back(m_cp1);
    rv.push_back(m_cp2);
    
    //..
    m_ctrl = new GBLinkControl(this);
    rv.push_back(m_ctrl);
#endif//SHOW_CURVE_CTRL

    m_label = new GBLinkLabel(this);
    rv.push_back(m_label);
    return rv;
};

void ard::GBLink::removeLinkFromScene(ArdGraphicsScene* scene)
{
    if (m_arrow) {
        scene->removeItem(m_arrow);
    }

    if (m_cp1) {
        scene->removeItem(m_cp1);
    }

    if (m_cp2) {
        scene->removeItem(m_cp2);
    }

    if (m_ctrl) {
        scene->removeItem(m_ctrl);
    }

    if (m_label) {
        scene->removeItem(m_label);
    }

    scene->removeItem(this);
};

/**
GBLinkArrow
*/
ard::GBLinkArrow::GBLinkArrow(GBLink* bl):m_blnk(bl)
{
};

void ard::GBLinkArrow::paint(QPainter * p, const QStyleOptionGraphicsItem* , QWidget* ) 
{
    PGUARD(p);
    p->setRenderHint(QPainter::Antialiasing, true);
    switch (m_blnk->selectionMode())
    {
    case GBLink::ESelectionMode::normal:
    {
        p->setPen(QPen(Qt::white, 3));
    }break;
    case GBLink::ESelectionMode::selected:
    {
        p->setPen(QPen(BBOARD_CURR_MARK_COLOR, 4));
    }break;
    case GBLink::ESelectionMode::control_selected:
    {
        p->setPen(QPen(BBOARD_CURR_LINK_CTRL_COLOR, 4));
    }break;
    }

    /*
    if (m_blnk->isMarkedAsSelected()) {
        p->setPen(QPen(BBOARD_CURR_MARK_COLOR, 4));
    }
    else {
        p->setPen(QPen(Qt::white, 3));
    }*/ 
    
    p->drawPath(path());
};

void ard::GBLinkArrow::recalcArrow(const QPointF& ptBegin, const QPointF& ptEnd)
{
    qreal dx = ptEnd.x() - ptBegin.x();
    qreal dy = ptEnd.y() - ptBegin.y();
    qreal polar_angel = atan2(dy, dx);
    qreal delta_angle = 3.14159265 * 10.0 / 180.0;
#ifdef ARD_BIG  
    qreal radius = 8.0;
#else
    qreal radius = is_big_screen() ? 16.0 : 8.0;
#endif

    QPainterPath pp;
    QPointF pt1(ptEnd.x() - radius*cos(polar_angel + delta_angle),
        ptEnd.y() - radius*sin(polar_angel + delta_angle));
    QPointF pt2(ptEnd.x() - radius*cos(polar_angel - delta_angle),
        ptEnd.y() - radius*sin(polar_angel - delta_angle));
    pp.moveTo(pt1);
    pp.lineTo(ptEnd);
    pp.lineTo(pt2);

    setPath(pp);
};

/**
GBLinkCurvePoint
*/
ard::GBLinkCurvePoint::GBLinkCurvePoint(GBLink* bl)
    :m_blnk(bl)
{
    setRect(0,0,10,10);
    setFlags(QGraphicsItem::ItemIsSelectable | 
        QGraphicsItem::ItemIsFocusable | 
        QGraphicsItem::ItemIsMovable | 
        QGraphicsItem::ItemSendsGeometryChanges);
    hide();
};

void ard::GBLinkCurvePoint::paint(QPainter * p, const QStyleOptionGraphicsItem* , QWidget* )
{
    QRectF rc = boundingRect();
    PGUARD(p);
    p->setRenderHint(QPainter::Antialiasing, true);
    p->setPen(Qt::NoPen);
    p->setBrush(QBrush(Qt::red));
    p->drawRect(rc);
};

void ard::GBLinkCurvePoint::mouseReleaseEvent(QGraphicsSceneMouseEvent * e) 
{
    QGraphicsRectItem::mouseReleaseEvent(e);
    m_blnk->onMovedGLinkCurveControl();
};


/**
GBLinkControl
*/
ard::GBLinkControl::GBLinkControl(GBLink* bl) :m_blnk(bl) 
{
    setRect(0, 0, BBOARD_LINK_CONTROL_WIDTH, BBOARD_LINK_CONTROL_WIDTH);
    setFlags(QGraphicsItem::ItemIsSelectable|QGraphicsItem::ItemIsFocusable);
    hide();
}

void ard::GBLinkControl::mousePressEvent(QGraphicsSceneMouseEvent* e)
{
    QGraphicsRectItem::mousePressEvent(e);  
    m_blnk->bb()->setCurrentControlLink(m_blnk->blink());
}

void ard::GBLinkControl::keyPressEvent(QKeyEvent * e) 
{
    QGraphicsRectItem::keyPressEvent(e);
    m_blnk->processKey(e->key());
};

void ard::GBLinkControl::mouseDoubleClickEvent(QGraphicsSceneMouseEvent * e)
{
    QGraphicsRectItem::mouseDoubleClickEvent(e);
    m_blnk->processDoubleClick();
};

void ard::GBLinkControl::paint(QPainter * p, const QStyleOptionGraphicsItem* , QWidget* )
{
    QRectF rc = boundingRect();
    PGUARD(p);
    p->setRenderHint(QPainter::Antialiasing, true);
    switch (m_blnk->selectionMode())
    {
    case GBLink::ESelectionMode::normal:
    {
        p->setPen(QPen(Qt::white, 3));
        p->setBrush(QBrush(Qt::white));
    }break;
    case GBLink::ESelectionMode::selected:
    {
        p->setPen(QPen(BBOARD_CURR_MARK_COLOR, 4));
        p->setBrush(QBrush(BBOARD_CURR_MARK_COLOR));
    }break;
    case GBLink::ESelectionMode::control_selected:
    {
        p->setPen(QPen(BBOARD_CURR_LINK_CTRL_COLOR, 4));
        p->setBrush(QBrush(BBOARD_CURR_LINK_CTRL_COLOR));
    }break;
    }
    p->drawEllipse(rc);
};

/**
GBLinkLabel
*/
ard::GBLinkLabel::GBLinkLabel(GBLink* bl):m_blnk(bl)
{
    setFlags(QGraphicsItem::ItemIsSelectable|QGraphicsItem::ItemIsFocusable|QGraphicsItem::ItemSendsGeometryChanges);
    hide();
};

/*
QRectF ard::GBLinkLabel::boundingRect() const
{
    return m_bounding_rect;
};*/

void ard::GBLinkLabel::resetLabel(const QPointF& pt, qreal angle)
{
    setPos(pt);
    if (angle > 90 && angle < 270) {
        angle -= 180;
    }
    m_angle = angle;// -90;
    //m_angle = 270;
    /*
    qreal len = 100.0;
    auto angle_in_radiant = 0.0174532 * angle - 1.57079;//pi/2
    auto dx = len * sin(angle_in_radiant);
    auto dy = len * cos(angle_in_radiant);

    //auto x = pt.x() + dx;
    //auto y = pt.y() + dy;

    qDebug() << "label-calc" << m_angle;// << dx << dx;

    QPainterPath pp;
    pp.moveTo(QPointF(0,0));
    pp.lineTo(QPointF(100, 100));
    //pp.lineTo(QPointF(-dx, -dy));
    setPath(pp);
    */
    //QString s = QString("hello world %1").arg(m_angle);
    auto s = m_blnk->blink()->linkLabel().trimmed();
    if (s.isEmpty()) {
        //qDebug() << "disabled link-label";
        hide();
        setEnabled(false);
    }
    else {
        //qDebug() << "enabled link-label" << s;
        show();
        setEnabled(true);
        auto sz = utils::calcSize(s, ard::defaultSmallFont());
        QRect rc(0, - sz.height(), sz.width(), sz.height());
        //setRect(rc);

        QTransform t = QTransform().rotate(-angle);
        m_rotatedRect = t.mapToPolygon(rc);
        QPainterPath pp;
        pp.addPolygon(m_rotatedRect);
        setPath(pp);
        /*
        auto angle_in_radiant = 0.0174532 * angle - 1.57079;//pi/2
        auto dx = std::abs(sz.width() * sin(angle_in_radiant));
        auto dy = std::abs(sz.height() * cos(angle_in_radiant));
        m_bounding_rect = rc;
        m_bounding_rect.setLeft(m_bounding_rect.left() - dx);
        m_bounding_rect.setWidth(m_bounding_rect.width() + 2 * dx);
        m_bounding_rect.setTop(m_bounding_rect.top() - dy);
        m_bounding_rect.setHeight(m_bounding_rect.height() + 2 * dy);
        //m_bounding_rect.setRight(m_bounding_rect.right() + std::abs(dx));
        //m_bounding_rect.setBottom(m_bounding_rect.bottom() + std::abs(dy));
        qDebug() << "enabled link-label" << s << dx << dy;
        */
    }
    update();
};

void ard::GBLinkLabel::paint(QPainter * p, const QStyleOptionGraphicsItem* , QWidget* ) 
{
    if (isEnabled()) {
        PGUARD(p);
        p->setRenderHint(QPainter::Antialiasing, true);
        
        //.. ykh -- draw rect around label
        //p->drawPolygon(m_rotatedRect);
        //....

        switch (m_blnk->selectionMode())
        {
        case GBLink::ESelectionMode::normal:
        {
            p->setPen(QPen(Qt::gray));
        }break;
        case GBLink::ESelectionMode::selected:
        {
            p->setPen(QPen(BBOARD_CURR_MARK_COLOR));
        }break;
        case GBLink::ESelectionMode::control_selected:
        {
            p->setPen(QPen(BBOARD_CURR_LINK_CTRL_COLOR));
        }break;
        }

        p->rotate(-m_angle);
        p->setFont(*defaultBoardLabelFont());
        auto s = m_blnk->blink()->linkLabel();
        //ykh show ange
        //s += " " + QString("%1").arg(m_angle);
        //ykh draw-label - working
        //p->drawText(QPointF(0, -10), s);
        
        auto sz = utils::calcSize(s, defaultBoardLabelFont());
        QRect rc(0, -sz.height(), sz.width(), sz.height());
        m_blnk->bb()->text_filter()->drawTextMLine(p, *defaultBoardLabelFont(), rc, s, true);
    }
};

void ard::GBLinkLabel::mousePressEvent(QGraphicsSceneMouseEvent* e) 
{
    QGraphicsPathItem::mousePressEvent(e);
    m_blnk->bb()->setCurrentControlLink(m_blnk->blink());
};

void ard::GBLinkLabel::mouseDoubleClickEvent(QGraphicsSceneMouseEvent * e)
{
    QGraphicsPathItem::mouseDoubleClickEvent(e);
    m_blnk->processDoubleClick();
};

void ard::GBLinkLabel::keyPressEvent(QKeyEvent * e) 
{
    QGraphicsPathItem::keyPressEvent(e);
    m_blnk->processKey(e->key());
};



/**
black_board_g_topic
*/
ard::black_board_g_topic::black_board_g_topic(BlackBoard* bb, ard::board_item* b) 
	:board_g_topic(bb), m_bitem(b)
{   
    LOCK(m_bitem);
	setFlags(QGraphicsItem::ItemIsSelectable |
		QGraphicsItem::ItemIsFocusable |
		QGraphicsItem::ItemIsMovable |
		QGraphicsItem::ItemSendsGeometryChanges);
}

ard::black_board_g_topic::~black_board_g_topic() 
{
    m_bitem->release();
};

ard::topic*	ard::black_board_g_topic::refTopic() 
{
	if (m_bitem) {
		return m_bitem->refTopic();
	}
	return nullptr;
};

const ard::topic* ard::black_board_g_topic::refTopic()const 
{
	if (m_bitem) {
		return m_bitem->refTopic();
	}
	return nullptr;
};

ard::BoardItemShape	ard::black_board_g_topic::bshape()const
{
	if (m_bitem) {
		return m_bitem->bshape();
	}
	return ard::BoardItemShape::box;
};

int ard::black_board_g_topic::ypos()const 
{
	if (m_bitem) {
		return m_bitem->yPos();
	}
	return 0;
};

ard::BlackBoard* ard::black_board_g_topic::selector_board() 
{
	ard::BlackBoard* rv = dynamic_cast<BlackBoard*>(m_bb);
	return rv;
};



void ard::black_board_g_topic::onContextMenu(const QPoint& pt)
{
	QAction* a = nullptr;
	QMenu m;
	ard::setup_menu(&m);
	ADD_CONTEXT_MENU_IMG_ACTION("target-mark", "Locate", [&]() {m_bb->locateInSelector(); });
	ADD_CONTEXT_MENU_IMG_ACTION("x-trash", "Del", [&]() {m_bb->removeSelected(false); });
	ADD_TEXT_MENU_ACTION("Show Links", [&]() {m_bb->showLinkProperties(); });
	ADD_TEXT_MENU_ACTION("Properties", [&]() {m_bb->showProperties(); });
	m.exec(pt);
}

/**
GCurrLinkOriginMark
*/
ard::GCurrLinkOriginMark::GCurrLinkOriginMark(BlackBoard* bb) :board_g_mark<ard::selector_board>(bb)
{
};


void ard::GCurrLinkOriginMark::paint(QPainter * p, const QStyleOptionGraphicsItem *, QWidget *)
{
    QRectF rc = boundingRect();
    PGUARD(p);
    p->setPen(QPen(BBOARD_CURR_MARK_COLOR, 6));
    const int m = 4 * ARD_MARGIN;
    auto rc1 = rc.marginsRemoved(QMarginsF(m, m, m, m));
    p->drawRect(rc1);
}

/**
GBActionButton
*/
ard::GBActionButton::GBActionButton(BlackBoard* bb) :board_g_mark<ard::selector_board>(bb)
{
    setZValue(BBOARD_ZVAL_ACTIVE_BTN);
    QFontMetrics fm(*ard::defaultSmallFont());
    QRect rc(0, 0, 3 * gui::lineHeight(), gui::lineHeight());
    //QRect rc2 = fm.boundingRect("Add Link");
    QRect rc2 = fm.boundingRect("Insert Template");
    if (rc2.width() > rc.width()) {
        rc.setWidth(rc2.width());
    }
    setRect(rc);
};

void ard::GBActionButton::paint(QPainter * p, const QStyleOptionGraphicsItem *, QWidget *)
{
    QRectF rc = boundingRect();
    PGUARD(p);
    p->setPen(QPen(BBOARD_CURR_MARK_COLOR, 2));
    p->setBrush(Qt::black);
    p->drawRect(rc);
    p->setPen(QPen(BBOARD_CURR_MARK_COLOR, 1));
    p->setFont(*ard::defaultSmallFont());
    QString s;
    switch (m_type) 
    {
    case add_link:          s = "Add link"; break;
    case insert_template:   s = "Insert Template"; break;
    case insert_item:       s = "Add"; break;
    case find_item:         s = "Find"; break;
    case show_item_links:   s = "Arrows"; break;
    case properties:        s = "Properties"; break;
    case none:              s = "----"; break;
    }
    p->drawText(rc, Qt::AlignCenter | Qt::AlignVCenter, s);
};


void ard::GBActionButton::updateMarkPos(board_g_topic<ard::selector_board>* gi)
{
    auto rc_this = boundingRect();
    auto pt = gi->pos();
    //pt.setX(pt.x() + xoffset);
    //int rv = rc_this.width();
	auto new_y = pt.y() - gui::lineHeight();
	//qDebug() << "<<< action/updateCurrPos" << new_y;
    pt.setY(new_y);
    setPos(pt);
    update();
	//return rv;
};

void ard::GBActionButton::mousePressEvent(QGraphicsSceneMouseEvent* e)
{
	board_g_mark<ard::selector_board>::mousePressEvent(e);
    e->accept();
};

ard::BlackBoard* ard::GBActionButton::selector_board()
{
	ard::BlackBoard* rv = dynamic_cast<BlackBoard*>(m_bb);
	return rv;
};

void ard::GBActionButton::mouseReleaseEvent(QGraphicsSceneMouseEvent * e) 
{
    switch (m_type)
    {
    case add_link:          selector_board()->addLink(); break;
    case insert_template:   selector_board()->completeCreateFromTemplate(e->scenePos()); break;
    case insert_item:       selector_board()->completeAddInInsertMode(); break;
    case find_item:         selector_board()->locateInSelector(); break;
    case show_item_links:   selector_board()->showLinkProperties(); break;
    case properties:        selector_board()->showProperties();break;
    case none:              ASSERT(0, "NA"); break;
    }
};

void ard::GBActionButton::setType(EType t) 
{
    m_type = t;
    update();
};

/**
BoardView
*/
ard::BoardView::BoardView(BlackBoard* bb) :board_page_view<ard::selector_board>(bb)
{
};

bool ard::BoardView::process_keyPressEvent(QKeyEvent *e) 
{
    bool rv = false;
    switch (e->key())
    {
    case Qt::Key_Delete:
    {
		auto g1 = m_bpage->firstSelected();
        if (g1) {
			m_bpage->removeSelected(false);
        }
        else {
			auto bb = dynamic_cast<BlackBoard*>(m_bpage);
            if (bb->currrentControlLink()) {
				bb->removeCurrentControlLink();
            }
        }
        rv = true;
    }break;
    case Qt::Key_Escape:
    {
		m_bpage->exitCustomEditMode();
        rv = true;
    }break;
    case Qt::Key::Key_A:
    {
        if (e->modifiers() == Qt::ControlModifier)
        {
			m_bpage->selectAllGBItems();
            rv = true;
        }
    }break;
    }
    return rv;
};




#include <QPaintEvent>
#include <QPainter>
#include <QScrollBar>
#include <QScroller>
#include <QTimer>
#include <QDesktopWidget>
#include <QApplication>
#include <QFormLayout>
#include <QPushButton>
#include <QCalendarWidget>
#include <QProgressBar>
#include <QComboBox>
#include <QStylePainter>
#include <QLabel>
#include <csignal>
#include <QCheckBox>
#include <QFileInfo>
#include <QFileDialog>
#include <QListView>
#include <QStandardItemModel>
#include <QClipboard>
#include <QMenu>

#include "anfolder.h"
#include "custom-widgets.h"
#include "registerbox.h"
#include "gmail/GmailRoutes.h"
#include "email.h"
#include "ethread.h"
#include "custom-menus.h"

using namespace googleQt;

#define LOCATOR_ANI_FRAMES      10
#define LOCATOR_ANI_DURATION_MS 1000

/**
   ProgressSlider

ProgressSlider::ProgressSlider(topic_ptr it, double width_mdelta)
{  
    QSize textSize = ard::calcSize("Completed: 100%");
    textSize.setWidth(static_cast<int>(textSize.width() * width_mdelta));
    textSize.setHeight(static_cast<int>(textSize.height() * 1.5));

    QRect rec = QApplication::desktop()->screenGeometry();
    int set_width = rec.width() - 60;
    if(set_width < textSize.width()){
            textSize.setWidth(set_width);
    }
  
    setMinimumSize(textSize);
    setMaximumSize(textSize);

    m_isModified = false;

    m_item = it;
    m_CompletedPercent = m_item->getToDoDonePercent4GUI();
    m_isModified = false;
};

void ProgressSlider::paintEvent(QPaintEvent *)
{
    if(m_item){
            QRect rc(0,0,width(),height());

            QPainter p(this);
            p.setRenderHint(QPainter::Antialiasing);
            p.setPen(gui::colorTheme_FgColor());

            QColor clBkDef = gui::colorTheme_BkColor();
            QBrush brush(clBkDef, isEnabled() ?  Qt::SolidPattern : Qt::CrossPattern);
            p.setBrush(brush);
            p.drawRect(rc);

            QFont f(*ard::defaultFont());
            p.setFont(f);

            qreal px_per_val = (qreal)rc.width() / 100;

            int draw_flags = Qt::AlignHCenter|Qt::AlignVCenter;
            int done_perc = m_CompletedPercent;
            if(done_perc > 0)
                {
                    int xcompl = (int)(px_per_val * done_perc);
                    QRect rc_prg = rc;
                    rc_prg.setRight(rc_prg.left() + xcompl);
                    if(rc_prg.right() >= rc.right())
                        rc_prg.setRight(rc.right() - 1);

                    QColor cl = color::LightGreen;

                    QBrush brush(cl, isEnabled() ?  Qt::SolidPattern : Qt::CrossPattern);
                    p.setBrush(brush);
                    p.drawRect(rc_prg);
                }

            QString s = QString("Completed:%1%").arg(done_perc);

            p.drawText(rc, draw_flags, s);

            QPoint pt1 = rc.bottomLeft();
            QPoint pt2 = pt1;     
            pt2.setY(pt2.y() - 5);

            for(int i = 1; i < 10; i++)
                {
                    pt1.setX((int)(rc.left() + 10 * i * px_per_val));
                    pt2.setX(pt1.x());
                    p.drawLine(pt1, pt2);
                }
        }
};

void ProgressSlider::mousePressEvent(QMouseEvent *e)
{
    if(m_item)
        {
            QRect rc(0,0,width(),height());
            qreal px_per_val = (qreal)rc.width() / 100;
            qreal per_val = (e->x() / px_per_val);
            m_CompletedPercent = 10 + 10 * (int)(per_val / 10.0);
            m_isModified = true;
            qDebug() << "<< per_val" << per_val << m_CompletedPercent;
            update();
            emit progressChanged();
        }
};
*/

/**
   ArdGraphicsView
*/
ArdGraphicsView::ArdGraphicsView(QWidget *parent)
    :QGraphicsView(parent)   
{
    m_resetHScrollOnRebuildRequest = false;
	setupKineticScroll(EScrollType::scrollKineticNone);
};

ArdGraphicsView::ArdGraphicsView(QGraphicsScene * scene, QWidget * parent)
    :QGraphicsView(scene, parent), 
     m_resetHScrollOnRebuildRequest(false)
{
	setupKineticScroll(EScrollType::scrollKineticNone);
}

ArdGraphicsView::~ArdGraphicsView()
{

};

void ArdGraphicsView::setNoScrollBars()
{
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
};

void ArdGraphicsView::setupKineticScroll(EScrollType s)
{
    setAlignment(Qt::AlignLeft | Qt::AlignTop);
	m_kineticScroll = s;
   // m_enableKineticScroll = enableKineticScroll;

    if (m_kineticScroll != EScrollType::scrollKineticNone) {
        setNoScrollBars();
        QScroller::grabGesture(this, QScroller::LeftMouseButtonGesture);
    }
};

void ArdGraphicsView::mousePressEvent(QMouseEvent *e)
{
    m_ptMouseDownPos = e->pos();
    QGraphicsView::mousePressEvent(e);
};

void ArdGraphicsView::mouseReleaseEvent(QMouseEvent *e)
{
    QGraphicsView::mouseReleaseEvent(e);
    if (m_kineticScroll != EScrollType::scrollKineticNone) {
        int dx = e->pos().x() - m_ptMouseDownPos.x();
        int dy = e->pos().y() - m_ptMouseDownPos.y();
        int abs_dx = dx;
        int abs_dy = dy;
        if (abs_dx < 0) abs_dx *= -1;
        if (abs_dy < 0) abs_dy *= -1;
        if (abs_dx > 20 /*&& abs_dy < 10*/)
        {
            swipeX(dx);
        }
    }
};

void ArdGraphicsView::swipeX(int )
{

};

bool ArdGraphicsView::event(QEvent* e)
{  
    if (m_kineticScroll != EScrollType::scrollKineticNone) {
        switch (e->type()) {
        case QEvent::ScrollPrepare:
        {

            QScrollPrepareEvent *se = static_cast<QScrollPrepareEvent *>(e);
            se->setViewportSize(QSizeF(size()));
            switch (m_kineticScroll)
            {
            case scrollKineticVerticalOnly:
            {
                auto v_max = verticalScrollBar()->maximum();
                auto v_val = verticalScrollBar()->value();
                //v_max = 100;
                se->setContentPosRange(QRectF(0.0, 0.0, 0.0, v_max));
                se->setContentPos(QPointF(0.0, v_val));
            }break;
            case scrollKineticHorizontalOnly:
            {
                se->setContentPosRange(QRectF(0.0, 0.0, horizontalScrollBar()->maximum(), 0.0));
                se->setContentPos(QPointF(horizontalScrollBar()->value(), 0.0));
            }break;
            case scrollKineticVerticalAndHorizontal:
            {
                se->setContentPosRange(QRectF(0.0, 0.0, horizontalScrollBar()->maximum(), verticalScrollBar()->maximum()));
                se->setContentPos(QPointF(horizontalScrollBar()->value(), verticalScrollBar()->value()));
            }break;
            case scrollKineticNone:
            {
                se->ignore();
                return false;
            }break;
            }

            se->accept();
        }break;
        //......
        case QEvent::Scroll:
        {
#define OVERSHOOT_CUTOFF 40

            QScrollEvent *se = static_cast<QScrollEvent *>(e);

            QString str_state;
            switch (se->scrollState()) {
            case QScrollEvent::ScrollStarted:str_state = "started"; m_overshoot = overshootNone; break;
            case QScrollEvent::ScrollUpdated:
            {
                str_state = "updated";
                if (se->overshootDistance().y() < -OVERSHOOT_CUTOFF) {
                    m_overshoot = overshootTop;
                }
                else if (se->overshootDistance().y() > OVERSHOOT_CUTOFF) {
                    m_overshoot = overshootBottom;
                }
            }break;
            case QScrollEvent::ScrollFinished:
            {
                if (m_overshoot == overshootTop) {
                    emit overshooted_Y(true);
                }
                else if (m_overshoot == overshootBottom) {
                    emit overshooted_Y(false);
                }
                str_state = "finished";
                m_overshoot = overshootNone;
            } break;
            }
            return QGraphicsView::event(e);
        }break;
        //.....
        default:
            return QGraphicsView::event(e);
        }

        return true;
    }

    return QGraphicsView::event(e);
//#endif // ARD_KINETIC_SCROLL
};

void ArdGraphicsView::emit_onResetSceneBoundingRect()
{
    QTimer::singleShot(10, this, SLOT(delayedResetSceneBoundingRect()));
};

void ArdGraphicsView::delayedResetSceneBoundingRect()
{
	if (m_resetHScrollOnRebuildRequest)
	{
		m_resetHScrollOnRebuildRequest = false;
		QScrollBar* hsc = horizontalScrollBar();
		if (hsc != NULL)
		{
			hsc->setValue(0);
		}
	}
	else
	{
		if (m_kineticScroll == scrollKineticVerticalOnly)
		{
			QScrollBar* hsc = horizontalScrollBar();
			if (hsc != NULL)
			{
				if (hsc->maximum() > hsc->value())
				{
					hsc->setValue(0);
				}
				//hsc->setValue(0);
			}
		}
	}

	emit onResetSceneBoundingRect();
};

/**
   ArdTableView
*/
QTableView* NewArdTableView() {
    return new ArdTableView();
}

ArdTableView::ArdTableView()
{
#ifndef ARD_BIG
    QScroller::grabGesture(this, QScroller::LeftMouseButtonGesture);
    setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
#endif
};

bool ArdTableView::event(QEvent* e)
{
#ifndef ARD_BIG
    switch (e->type()) {
    case QEvent::ScrollPrepare:
        {

            QScrollPrepareEvent *se = static_cast<QScrollPrepareEvent *>(e);
            se->setViewportSize(QSizeF(size()));
            se->setContentPosRange(QRectF(0.0, 0.0, 0.0, verticalScrollBar()->maximum()));
            se->setContentPos(QPointF(0.0, verticalScrollBar()->value()));
            se->accept();                       
        }break;
    default:
        return QTableView::event(e);
    }

    return true;
#else
    return QTableView::event(e);
#endif
};

void ArdTableView::mousePressEvent(QMouseEvent *e) 
{
    if (e->button() == Qt::RightButton) {
        QMenu m(this);
        ard::setup_menu(&m);
        m.addAction(new QAction("Copy", nullptr));

        QString selected_text;

        connect(&m, &QMenu::triggered, [&](QAction*)
        {
            auto clb = QApplication::clipboard();
            if (clb) {
                auto m = model();
                auto selection = selectionModel();
                auto indexes = selection->selectedIndexes();
                QModelIndex previous = indexes.first();
                QVariant data = m->data(previous);
                QString s = data.toString();
                selected_text.append(s);
                selected_text.append('\n');
                indexes.removeFirst();
                foreach(const QModelIndex &current, indexes)
                {
                    data = m->data(current);
                    s = data.toString();
                    selected_text.append(s);

                    if (current.row() != previous.row())
                    {
                        selected_text.append('\n');
                    }
                    // Otherwise it's the same row, so append a column separator, which is a tab.
                    else
                    {
                        selected_text.append('\t');
                    }
                    previous = current;
                }

                clb->setText(selected_text);
            }
        });
        m.exec(QCursor::pos());
    }

    QTableView::mousePressEvent(e);
};

ArdTextBrowser::ArdTextBrowser()
{
    QScroller::grabGesture(this, QScroller::LeftMouseButtonGesture);
    //  setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
};

bool ArdTextBrowser::event(QEvent* e)
{
    switch (e->type()) {
    case QEvent::ScrollPrepare:
        {

            QScrollPrepareEvent *se = static_cast<QScrollPrepareEvent *>(e);
            se->setViewportSize(QSizeF(size()));
            se->setContentPosRange(QRectF(0.0, 0.0, 0.0, verticalScrollBar()->maximum()));
            se->setContentPos(QPointF(0.0, verticalScrollBar()->value()));
            se->accept();                       
        }break;
    default:
        return QTextBrowser::event(e);
    }

    return true;
};

/**
   ClickableLabel
*/
ClickableLabel::ClickableLabel(QString text, QWidget* parent)
    : QLabel(parent)
{
    setText(text);
}
 
ClickableLabel::~ClickableLabel()
{
}
 
void ClickableLabel::mousePressEvent(QMouseEvent* )
{
    emit clickedOnLabel();
}

QString rectf2str(QRectF rcf)
{
    QString rv = QString("(%1 %2 %3 %4)")
        .arg(rcf.left())
        .arg(rcf.top())
        .arg(rcf.width())
        .arg(rcf.height());
    return rv;
}

QString rect2str(QRect rcf)
{
    QString rv = QString("(%1 %2 %3 %4)")
        .arg(rcf.left())
        .arg(rcf.top())
        .arg(rcf.width())
        .arg(rcf.height());
    return rv;
}

void MarkedPlainTextEdit::paintEvent(QPaintEvent *e)
{
    QWidget *vwp = viewport();
    if(vwp)
        {
            QPainter pd(vwp);
            QPen pn(color::Olive);
            pn.setWidth(3);
            pd.setPen(pn);

            QRect rc = vwp->contentsRect();
            rc.setHeight(rc.height() - 1);
            rc.setWidth(rc.width() - 1);      
            pd.drawRect(rc);
        }
  
    QPlainTextEdit::paintEvent(e);
};


/**
    ArdGraphicsScene
*/
ArdGraphicsScene::ArdGraphicsScene()
{
    QObject::connect(&m_locatorTimeLine, &QTimeLine::frameChanged, [&](int f)
    {
        if (m_locator_item) {
            auto delta = (1 - (double)f / LOCATOR_ANI_FRAMES);
            m_locator_item->setScale(delta);
            if (delta < 0.1) {
                m_locator_item->hide();
            }
        }
    });


    m_locatorTimeLine.setFrameRange(1, LOCATOR_ANI_FRAMES);
    m_locatorTimeLine.setDuration(LOCATOR_ANI_DURATION_MS);
};

void ArdGraphicsScene::clearArdScene() 
{
    m_locator_item = nullptr;
    clear();    
};

void ArdGraphicsScene::animateLocator(const QPointF& pt)
{
    if (!m_locator_item) {
        m_locator_item = new QGraphicsRectItem(0, 0, gui::lineHeight() * 2, gui::lineHeight() * 2);
        m_locator_item->setZValue(2.0);
        QPen pn(Qt::gray);
        m_locator_item->setPen(pn);
        m_locator_item->setBrush(QBrush(color::LOCATOR_ANI));
        m_locator_item->setPos(pt);
        addItem(m_locator_item);
    }
    else {
        m_locator_item->setPos(pt);
        m_locator_item->show();
    }
    m_locatorTimeLine.start();
};

googleQtProgressControl::googleQtProgressControl() 
{

};

googleQtProgressControl::~googleQtProgressControl() 
{
    if (m_googleQt_upload_signal_connection) {
        QObject::disconnect(m_googleQt_upload_signal_connection);
    }
    if (m_googleQt_download_signal_connection) {
        QObject::disconnect(m_googleQt_download_signal_connection);
    }
};


void googleQtProgressControl::addProgressBar(QBoxLayout* l, bool downloadProgress)
{
    if (m_progress_bar) {
        ASSERT(0, "progress_bar object already created");
        return;
    }
    if (m_googleQt_upload_signal_connection) {
        ASSERT(0, "unexpected connection object 1");
        QObject::disconnect(m_googleQt_upload_signal_connection);
    }

    if (m_googleQt_download_signal_connection) {
        ASSERT(0, "unexpected connection object 2");
        QObject::disconnect(m_googleQt_download_signal_connection);
    }


    m_progress_bar = new QProgressBar();
    m_progress_bar->setVisible(false);
    l->addWidget(m_progress_bar);

    std::function<void(qint64, qint64)> prg = [&](qint64 bytesProcessed, qint64 total) {
        if (bytesProcessed > 30000 && total != -1) 
		{
			if(!m_progress_bar->isVisible())
				m_progress_bar->setVisible(true);
            if (total != m_progress_bar->maximum()) {
                m_progress_bar->setMaximum(total);
            }
			m_progress_bar->setValue(bytesProcessed);
			qDebug() << "m_progress_bar" << bytesProcessed << total;
        }
		else 
		{
			if (m_progress_bar->isVisible())
				m_progress_bar->setVisible(false);
		}

        //if(bytesProcessed == total){
        //    m_progress_bar->setVisible(false);
        //}
    };

    auto gc = ard::google();
    if (gc) {
        if (downloadProgress) {
            m_googleQt_download_signal_connection = QObject::connect(gc.get(), &googleQt::ApiClient::downloadProgress, prg);
        }
        else {
            m_googleQt_upload_signal_connection = QObject::connect(gc.get(), &googleQt::ApiClient::uploadProgress, prg);
        }
        /*
        auto g_prg = new googleQt::ApiClientProgress;
        gc->setApiClientProgress(g_prg);
        if(downloadProgress){
            m_googleQt_download_signal_connection = QObject::connect(g_prg, &googleQt::ApiClientProgress::downloadProgress, prg);
        }
        else{
            m_googleQt_upload_signal_connection = QObject::connect(g_prg, &googleQt::ApiClientProgress::downloadProgress, prg);
        }*/
    }
};

/**
    PanelHeader
*/
PanelHeader::PanelHeader(ArdGraphicsView* ctrlView):m_ctrlView(ctrlView)
{
    connect(m_ctrlView, &ArdGraphicsView::onResetSceneBoundingRect,
        this, &PanelHeader::onControlViewResetSceneBoundingRect);
};

void PanelHeader::resetHeader() 
{
    setMaximumHeight(gui::headerHeight());
    setMinimumHeight(gui::headerHeight());

    rebuildHeader();
};

void PanelHeader::onControlViewResetSceneBoundingRect() 
{
    if (isEnabled()) {
        rebuildHeader();
    }
};



QString EitherLabelOrQ::toString()
{
    QString res = QString("%1/%2")
        .arg(isLabel() ? "L" : "Q")
        .arg(value);

    return res;
};


EitherLabelOrQ EitherLabelOrQ::empty_lbl()
{
    EitherLabelOrQ r;
    r.value = "";
    r.wrapper_type = t_label;
    return r;
};

EitherLabelOrQ EitherLabelOrQ::lbl(QString lid)
{
    EitherLabelOrQ r;
    r.value = lid;
    r.wrapper_type = t_label;

    return r;
};

EitherLabelOrQ EitherLabelOrQ::q(QString qs) 
{
    EitherLabelOrQ r;
    r.value = qs;
    r.wrapper_type = t_q;
    return r;
};

bool EitherLabelOrQ::equalTo(const EitherLabelOrQ& el)const
{
    bool rv = (wrapper_type == el.wrapper_type) && value.compare(el.value, Qt::CaseInsensitive) == 0;
    return rv;
};

/**
    class LabelCombo
*/
EitherLabelOrQ LabelCombo::currentLabel()const
{
    return QComboBox::currentData().value<EitherLabelOrQ>();
};

void LabelCombo::addLabel(QString name, const EitherLabelOrQ& lbl)
{
    QVariant v;
    v.setValue(lbl);
    QComboBox::addItem(name, v);
};

int LabelCombo::findLabel(const EitherLabelOrQ& lbl)const 
{
    for (int i = 0; i < count(); i++) {
        //auto s = itemText(i);
        auto d = itemData(i).value<EitherLabelOrQ>();
        if (d.equalTo(lbl)) {
            return i;
        }
    }
    return -1;
    /*
    QVariant v;
    v.setValue(lbl);
    return QComboBox::findData(v);
    */
};

int LabelCombo::findFirstQ()const 
{
    for (int i = 0; i < count(); i++) {
        auto el = QComboBox::itemData(i).value<EitherLabelOrQ>();
        if (el.isQ()) {
            return i;
        }
    }
    return -1;
};

void LabelCombo::updateQ(int idx, QString q)
{
    auto el = QComboBox::itemData(idx).value<EitherLabelOrQ>();
    if (el.isQ()) {
        el.value = q;

        QVariant v;
        v.setValue(el);
        QComboBox::setItemText(idx, q);
        QComboBox::setItemData(idx, v);
    }
    else 
    {
        ASSERT(0, "expected Q-item");
    }
};

int LabelCombo::addQ(QString q)
{
    QComboBox::addItem(q);
    int idx = QComboBox::count() - 1;
    QVariant v;
    v.setValue(EitherLabelOrQ::q(q));
    QComboBox::setItemData(idx, v);
    return idx;
};

#ifdef _DEBUG
void LabelCombo::debugPrint() 
{
    for (int i = 0; i < count(); i++) {
        auto s = itemText(i);
        auto d = itemData(i).value<EitherLabelOrQ>();
        qDebug() << "l-combo-data" << s << d.toString();
    }
};
#endif

/**
    LabelTabBar
*/
EitherLabelOrQ LabelTabBar::currentTabLabel()const
{
    auto idx = QTabBar::currentIndex();
    if (idx == -1) {
        ASSERT(0, "expected current tab index");
        return EitherLabelOrQ::empty_lbl();
    }

    return QTabBar::tabData(idx).value<EitherLabelOrQ>();
};

int LabelTabBar::addSysLabelTab(googleQt::mail_cache::SysLabel l)
{
    int idx = -1;
    auto lname = googleQt::mail_cache::sysLabelName(l);
    auto lid = googleQt::mail_cache::sysLabelId(l);
    if (!lname.isEmpty() && !lid.isEmpty()) {
        idx = QTabBar::addTab(lname);
        EitherLabelOrQ lbl = EitherLabelOrQ::lbl(lid);

        QVariant v;
        v.setValue(lbl);
        QTabBar::setTabData(idx, v);
    }
    else {
        ASSERT(0, "Invalid SysLabel") << (int)l;
    }
    return idx;
};

int LabelTabBar::addLabelTab(QString name, const EitherLabelOrQ& lbl)
{
    int idx = QTabBar::addTab(name);
    QVariant v;
    v.setValue(lbl);
    QTabBar::setTabData(idx, v);
    return idx;
};

int LabelTabBar::addQ(QString q) 
{
    int idx = QTabBar::addTab(q);
    QVariant v;
    v.setValue(EitherLabelOrQ::q(q));
    QTabBar::setTabData(idx, v);
    return idx;
};

int LabelTabBar::findLabelTab(const EitherLabelOrQ& lbl)const
{
    for (int i = 0; i < count(); i++) {
        auto el = QTabBar::tabData(i).value<EitherLabelOrQ>();
        if (el.equalTo(lbl)) {
            return i;
        }
    }
    return -1;
};

int LabelTabBar::findFirstQ()const
{
    for (int i = 0; i < count(); i++) {
        auto el = QTabBar::tabData(i).value<EitherLabelOrQ>();
        if (el.isQ()) {
            return i;
        }
    }
    return -1;
};

void LabelTabBar::updateQ(int idx, QString q) 
{
    auto el = QTabBar::tabData(idx).value<EitherLabelOrQ>();
    if (el.isQ()) {     
        el.value = q;

        QVariant v;
        v.setValue(el);
        QTabBar::setTabText(idx, q);
        QTabBar::setTabData(idx, v);
    }
    else
    {
        ASSERT(0, "expected Q-tab");
    }
};


#define BTN_H gui::lineHeight()

class ColorButton : public QToolButton
{
public:
    ColorButton(ColorButtonPanel* b, ard::EColor idx, bool selected) :
        m_cbox(b),
        m_color_idx(idx),
        m_selected(selected)
    {
        resetButton();
        setMinimumHeight(BTN_H);
        setMinimumWidth(BTN_H);

        connect(this, &QAbstractButton::pressed, [&]()
        {
            m_cbox->m_cbutton_changed = true;
            for (auto d : m_cbox->m_cbuttons) {
                d->m_selected = false;
            }
            m_selected = true;
            m_cbox->m_selected_color_idx = m_color_idx;
            for (auto d : m_cbox->m_cbuttons) {
                d->resetButton();
                d->repaint();
            }
        });
    };

    void resetButton()
    {
        if (m_selected) {
            setText("[x]");
        }
        else {
            setText("");
        }
        setStyleSheet(QString("QToolButton{background-color: rgb(%1);%2}").arg(ard::cidx2color_str(m_color_idx)).arg(m_selected ? "border: 2px solid black" : "border: 1px solid gray"));
    };

	ard::EColor colorIndex()const { return m_color_idx; }
    bool selected()const { return m_selected; }
protected:
    ColorButtonPanel*	m_cbox;
	ard::EColor			m_color_idx{ ard::EColor::none };
    bool				m_selected{ false };
};

/**
ColorButtonPanel
*/
ColorButtonPanel::ColorButtonPanel(ard::EColor idx) :m_selected_color_idx(idx)
{
    auto *color_box = new QHBoxLayout;
    setLayout(color_box);

    QWidget* spacer = new QWidget();
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    color_box->addWidget(spacer);

    ColorButton* b;

#define ADD_CLR_BUTTON(I) b = new ColorButton(this, I, m_selected_color_idx == I);\
    color_box->addWidget(b);\
    m_cbuttons.push_back(b);\

    ADD_CLR_BUTTON(ard::EColor::purple);
    ADD_CLR_BUTTON(ard::EColor::red);
    ADD_CLR_BUTTON(ard::EColor::blue);
    ADD_CLR_BUTTON(ard::EColor::green);
#undef ADD_CLR_BUTTON

    bool as_default_white = (m_selected_color_idx == ard::EColor::none);
    /*switch (m_selected_color_idx)
    {
    case 5:
    case 9:
    case 12:
    case 2:
        as_default_white = false;
    }*/

    b = new ColorButton(this, ard::EColor::none, as_default_white);
    color_box->addWidget(b);
    m_cbuttons.push_back(b);
    b->setMinimumWidth(2 * gui::lineHeight());
};

/**
* NoteBarColorButtons
*/
NoteBarColorButtons::NoteBarColorButtons(int height) 
{   
    m_button_width_height = height - 2 * ARD_MARGIN;
    m_select_btn_width = m_button_width_height *1.5;

    bmargin = ARD_MARGIN;
    int buttons_count = sizeof(m_buttons) / sizeof(int);

    int buttons_area_width = buttons_count * m_button_width_height + (buttons_count - 1) * bmargin;
    m_rcButtons = QRect(bmargin, bmargin, buttons_area_width, m_button_width_height);
    m_rcButtonsSelect = QRect(m_rcButtons.right() + 2 * bmargin, bmargin, m_select_btn_width, m_button_width_height);

    setMaximumHeight(height);
    setMaximumWidth(m_rcButtonsSelect.right() + ARD_MARGIN);
    setMinimumWidth(m_rcButtonsSelect.right() + ARD_MARGIN);
};

void NoteBarColorButtons::paintEvent(QPaintEvent *) 
{
    QPainter p(this);
    QRect rc(0, 0, width() - 1, height() - 1);

    QPen pen(gui::colorTheme_BkColor());
    QBrush brush(gui::colorTheme_BkColor());
    p.setBrush(brush);
    p.setPen(pen);
    p.drawRect(rc);

    auto f = QApplication::font("QMenu");
    p.setFont(f);
    p.setPen(QPen(Qt::gray));

    /// color buttons ///
    rc = QRect(bmargin, bmargin, m_button_width_height, m_button_width_height);
    for (auto& idx : m_buttons) {
        QBrush brush(color::getColorByClrIndex(idx), isEnabled() ? Qt::SolidPattern : Qt::CrossPattern);
        p.setBrush(brush);
        p.drawRect(rc);

        if (idx == m_selected_color_idx) {
            PGUARD(&p);
            p.setPen(Qt::black);
            p.drawText(rc, Qt::AlignCenter | Qt::AlignVCenter, "[x]");
        }

        rc.translate(m_button_width_height + bmargin, 0);
    }

    //QBrush brush(Qt::white);
    p.setBrush(QBrush(Qt::white));
    p.drawRect(m_rcButtonsSelect);
    p.drawText(m_rcButtonsSelect, Qt::AlignCenter | Qt::AlignVCenter, "..");
    /*if (m_selected_color_idx == 15) {
        p.drawText(m_rcButtonsSelect, Qt::AlignCenter | Qt::AlignVCenter, "..");
    }*/
};

void NoteBarColorButtons::mousePressEvent(QMouseEvent *e) 
{
    QPoint pt = e->pos();
    auto idx = hit_test(e->pos());
    if (idx == -1) {
        emit onColorSelectRequested();
    }
    else{
        m_selected_color_idx = idx;
        emit onColorSelected(color::getColorByClrIndex(idx));
        update();
    }   
};

void NoteBarColorButtons::selectColor(const QColor& cl) 
{
    m_selected_color_idx = color::getColorIndexByColor(cl.rgb());
    update();
};

int NoteBarColorButtons::hit_test(const QPoint& pt) 
{
    if (m_rcButtonsSelect.contains(pt)) {
        return -1;
    }

    if (m_rcButtons.contains(pt)) 
    {
        QRect rc(bmargin, bmargin, m_button_width_height, m_button_width_height);
        for (auto& idx : m_buttons) {
            if (rc.contains(pt)) {
                return idx;
            }
            rc.translate(m_button_width_height + bmargin, 0);
        }
    }

    return -1;
};

/**
* LocationAnimator
*/
static int mark_dx = 5;

void LocationAnimator::installLocationAnimator()
{
    QObject::connect(&m_cursorAniTimeLine, &QTimeLine::frameChanged, [&](int f)
    {
        updateCursorAniRect();
        recalcCursorAniRect(f);
        if (f >= LOCATOR_ANI_FRAMES) {
            m_rcCursorAniRect.setRect(0, 0, 0, 0);
            m_cursorAniTimeLine.stop();
        }
        else
        {
            recalcCursorAniRect(f);
        }
    });
    m_cursorAniTimeLine.setFrameRange(0, LOCATOR_ANI_FRAMES);
    m_cursorAniTimeLine.setDuration(LOCATOR_ANI_DURATION_MS);
};

void LocationAnimator::animateLocator()
{
    if (m_cursorAniTimeLine.state() == QTimeLine::Running) {
        m_cursorAniTimeLine.stop();
    }

    if (!m_rcCursorAniRect.isEmpty())
    {
        updateCursorAniRect();
    }

    recalcCursorAniRect(0);
    m_cursorAniTimeLine.start();
};


void LocationAnimator::updateCursorAniRect() 
{
    QRect rc = m_rcCursorAniRect.adjusted(-mark_dx, -mark_dx, 2 * mark_dx, 2 * mark_dx);
    int dy = (m_cursorAniVScrollPos - locatorVerticalScrollValue()/*verticalScrollBar()->value()*/);
    if (dy != 0)
    {
        rc.translate(0, dy);
    }

    locatorUpdateRect(rc);
};

void LocationAnimator::recalcCursorAniRect(int f)
{
    //QTextCursor cr = textCursor();
    m_rcCursorAniRect = locatorCursorRect();// cursorRect(cr);
    int aw = LOCATOR_ANI_FRAMES - f;
    m_rcCursorAniRect = m_rcCursorAniRect.adjusted(-aw, -aw,
        2 * aw, 2 * aw);

    m_cursorAniVScrollPos = locatorVerticalScrollValue();
};

void LocationAnimator::drawLocator(QPainter& p) 
{
    if (!m_rcCursorAniRect.isEmpty()) {
        p.setBrush(QBrush(color::LOCATOR_ANI));
        QPen pn(qRgb(108, 0, 21));
        pn.setWidth(1);
        p.setPen(pn);
        p.drawRect(m_rcCursorAniRect);
    }
};

/**
    NoteEditBase
*/
NoteEditBase::NoteEditBase() 
{
    installLocationAnimator();
};

void NoteEditBase::locatorUpdateRect(const QRect& rc) 
{
    viewport()->update(rc);
};

QRect NoteEditBase::locatorCursorRect()const 
{
    QTextCursor cr = textCursor();
    auto rv = cursorRect(cr);
    return rv;
};

int NoteEditBase::locatorVerticalScrollValue()const 
{
    int rv = 0;
    auto b = verticalScrollBar();
    if (b) {
        rv = b->value();
    }
    return rv;
};

void NoteEditBase::paintEvent(QPaintEvent *e) 
{
    QPainter pd(viewport());
    drawLocator(pd);
    QTextEdit::paintEvent(e);
};

/**
    AccountButton
*/
AccountButton::AccountButton() 
{
    //QFontMetrics fm(*ard::defaultFont());
    //return fm.boundingRect(s).width();

    //auto w = utils::calcWidth("AAA", utils::defaultBoldFont());
};

void AccountButton::updateAccountButton()
{
    QFontMetrics fm(*ard::defaultFont());
    QSize calc_sz(fm.boundingRect("AAA").width(), ICON_WIDTH);
    setFixedHeight(ICON_WIDTH);
    //QSize calc_sz(fm.boundingRect("AAA")).width(), ICON_WIDTH);
    //setFixedSize(calc_sz);

    //  extern std::pair<char, char> calcColorHashIndex(QString emailAddress);
    //bool g_ok = ard::isGoogleConnected();
    std::pair<char, char> ch;
    auto email = dbp::configEmailUserId();
    bool is_gmail_connected = ard::isGoogleConnected() && !email.isEmpty();

    if (!is_gmail_connected) {
        ch.first = 1;
        ch.second = '-';
        
    }
    else {
        ch = color::calcColorHashIndex(email);
        if (ch.second == 0) {
            ch.first = 1;
            ch.second = '-';
        }
    }

    //if (is_gmail_connected)
    {
        //auto w = utils::calcWidth("AAA", utils::defaultBoldFont());
        //b->setFixedWidth(w);

        m_pxmap = QPixmap(calc_sz.width(), ICON_WIDTH);
        QPainter p(&m_pxmap);

        //-------------
        QRect rcBtn(0, 0, calc_sz.width(), ICON_WIDTH);
        QPointF pt1(0, 0);
        QPointF pt2(calc_sz.width(), 0);
        QColor clBkDef = gui::darkSceneBk();
        QColor clBkDef2 = clBkDef.lighter(150);
        QLinearGradient lgrad(pt1, pt2);
        lgrad.setColorAt(0.0, clBkDef);
        lgrad.setColorAt(1.0, clBkDef2);
        p.setBrush(lgrad);
        p.drawRect(rcBtn);
        //-------------

        QColor rgb = is_gmail_connected ? color::getColorByHashIndex(ch.first) : color::Gray;
        rgb = rgb.darker(150);
        //p->save();
        p.setBrush(QBrush(rgb));
        p.setPen(Qt::NoPen);
        p.setFont(*ard::defaultOutlineLabelFont());
        //QRect rcBtn(0,0,ICON_WIDTH, ICON_WIDTH);
        QRect rcIcon(0, 0, ICON_WIDTH, ICON_WIDTH);
        int dx = (rcBtn.width() - rcIcon.width()) / 2;
        rcIcon.translate(dx, 0);
        //rcIcon.setX(dx);
        p.drawEllipse(rcIcon);
        p.setPen(Qt::white);
        p.drawText(rcIcon, Qt::AlignCenter | Qt::AlignHCenter, QString(ch.second));
        //QIcon icon(pix);
        //b->setIcon(icon);
        //b->setIconSize(QSize(w, ICON_WIDTH));
    }
    /*else 
    {
        m_pxmap = QPixmap();
    }*/

    update();
};

void AccountButton::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    auto rc = rect();
    if (!m_pxmap.isNull()) {
        p.drawPixmap(rc, m_pxmap);
    }
};

void AccountButton::mousePressEvent(QMouseEvent *e)
{
    QWidget::mousePressEvent(e);
    ard::asyncExec(AR_SelectGmailUser);
};

HintLineEdit::HintLineEdit(QString hint_str) :m_hint_str(hint_str)
{

};

void HintLineEdit::paintEvent(QPaintEvent * e)
{
    QLineEdit::paintEvent(e);
    auto s = text();
    if (s.isEmpty()) 
    {
        auto rc = geometry();
        QPainter p(this);
        p.setBrush(QBrush(qRgb(251, 196, 199)));
        p.drawRect(rc);
        QPen pn(Qt::gray);
        p.setPen(pn);
        p.setFont(*ard::defaultFont());
        QString s = QString("<%1>").arg(m_hint_str);
        p.drawText(rc, Qt::AlignLeft | Qt::AlignVCenter, s);
    }
};
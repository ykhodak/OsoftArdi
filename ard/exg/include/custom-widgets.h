#pragma once

#include <QDate>
#include <QToolButton>
#include <QGraphicsView>
#include <QLabel>
#include <QTableView>
#include <QTextBrowser>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDialog>
#include <QLineEdit>
#include <QMainWindow>
#include <QTimeLine>
#include <QGraphicsRectItem>
#include <QSplitter>
#include <QComboBox>

#include "a-db-utils.h"

class QCalendarWidget;
class QProgressBar;
class QBoxLayout;
class QCheckBox;
class QListView;

namespace ard 
{
	class topic;
};

namespace dbp
{
    class Resource;
};

namespace googleQt
{
    class GoogleException;
};

#include <QPlainTextEdit>

/**
   ArdGraphicsView - base class for most graphic views
   in application, slightly modifyes QGraphicsView by
   hiding scrollbars and enabling scroll gestures
*/
class ArdGraphicsView: public QGraphicsView
{
    Q_OBJECT
public:
    enum EScrollType
        {
            scrollKineticNone,
            scrollKineticVerticalOnly,
            scrollKineticHorizontalOnly,
            scrollKineticVerticalAndHorizontal
        };

    enum EOvershootType 
        {
            overshootNone,
            overshootTop,
            overshootBottom
        };

    explicit ArdGraphicsView(QWidget *parent = nullptr);
    ArdGraphicsView(QGraphicsScene * scene, QWidget * parent = 0 );
    ~ArdGraphicsView();

    bool     event(QEvent*);
    void     emit_onResetSceneBoundingRect();
    bool     resetHScrollOnRebuildRequest()const{return m_resetHScrollOnRebuildRequest;}
    void     setResetHScrollOnRebuildRequest(){m_resetHScrollOnRebuildRequest = true;}
   // virtual EScrollType kscroll()const{return scrollKineticVerticalOnly;}
	virtual void resetGItemsGeometry() {};
    virtual void updateTitleEditFramePos() {};
    void setNoScrollBars();
	void setupKineticScroll(EScrollType s);
protected:
    virtual void swipeX(int dx);
    
    //void setup(bool enableKineticScroll);

    void mousePressEvent(QMouseEvent *e);
    void mouseReleaseEvent(QMouseEvent *e);

signals:
    void onResetSceneBoundingRect();
    void overshooted_Y(bool ontop);  

protected slots:
    virtual void delayedResetSceneBoundingRect();

protected:
    bool		m_resetHScrollOnRebuildRequest;
	EScrollType	m_kineticScroll{ EScrollType::scrollKineticNone };
  //  bool    m_enableKineticScroll{false};
    QPoint m_ptMouseDownPos;    
    EOvershootType m_overshoot{ overshootNone };
};

/**
    ArdGraphicsScene - QGraphicsScene with extra items to animate
    tap on screen and possible other custom properties
*/
class ArdGraphicsScene : public QGraphicsScene 
{
    Q_OBJECT
public:
    ArdGraphicsScene();
    void clearArdScene();
    void animateLocator(const QPointF& pt);
protected:
    QTimeLine               m_locatorTimeLine;
    QGraphicsRectItem*      m_locator_item{ nullptr };
};

/**
*   ArdTableView - table with kinnetick scroll and
*   clipboard copy of selected cells
*/
class ArdTableView : public QTableView
{
    Q_OBJECT
public:
    ArdTableView();
protected:
    bool        event(QEvent*)override;
    void        mousePressEvent(QMouseEvent *e)override;
};

/**
   ArdTextBrowser - text browser with kinnetick scroll
*/
class ArdTextBrowser : public QTextBrowser
{
    Q_OBJECT
public:
    ArdTextBrowser();
    bool     event(QEvent*);
};


class ClickableLabel : public QLabel
{
    Q_OBJECT
public:
    explicit ClickableLabel(QString text="", QWidget* parent = nullptr);
    ~ClickableLabel();
signals:
    void clickedOnLabel();
protected:
    void mousePressEvent(QMouseEvent* event);
};

class TitleThumbDialogHelper
{
public:
    TitleThumbDialogHelper();

    void makeTitleThumbLayout(topic_ptr it, QDialog* dlg, QWidget* firstWidgetInLayout = nullptr, bool v_layout = true);
    void processTitleLabelClicked(topic_ptr it);
    bool processSaveTitleIfModified(topic_ptr it);

protected:
    void showTitleEdit();
  
protected:
    ClickableLabel*     m_title_label{ nullptr };
    QPlainTextEdit*     m_title_edit{ nullptr };
    QBoxLayout*         m_edit_layout{ nullptr };
};


class MarkedPlainTextEdit : public QPlainTextEdit
{
    Q_OBJECT
protected:
    void paintEvent(QPaintEvent *e);
};

/**
    googleQtProgressControl - show/update progress on download/upload
*/
class googleQtProgressControl
{
public:
    googleQtProgressControl();
    virtual ~googleQtProgressControl();
    void addProgressBar(QBoxLayout* l, bool downloadProgress);

protected:
    QProgressBar*               m_progress_bar{nullptr};
    QMetaObject::Connection     m_googleQt_upload_signal_connection;
    QMetaObject::Connection     m_googleQt_download_signal_connection;
};

/**
PanelHeader - static header ctrl over custom panel
*/
class PanelHeader : public QWidget
{
    Q_OBJECT
public:
    PanelHeader(ArdGraphicsView* ctrlView);
    //virtual void attachControlView(OutlineView* v) { m_ctrlView = v; };
    virtual void resetHeader();
    
private slots:
    void    onControlViewResetSceneBoundingRect();

protected:
    virtual void rebuildHeader() = 0;
protected:
    ArdGraphicsView* m_ctrlView;
};

///................

/**
    we can be either label wrapper or query string
*/
struct EitherLabelOrQ
{
    enum Etype 
    {
        t_label = 0,
        t_q = 1
    };

    EitherLabelOrQ() :value(""), wrapper_type(t_label) {}

    QString value{ "" };
    Etype   wrapper_type{ t_label };

    static EitherLabelOrQ empty_lbl();
    static EitherLabelOrQ lbl(QString lid);
    static EitherLabelOrQ q(QString qs);

    bool isLabel()const { return (wrapper_type == t_label); }
    bool isQ()const { return wrapper_type == t_q; }
    bool equalTo(const EitherLabelOrQ& el)const;

    QString toString();
};

Q_DECLARE_METATYPE(EitherLabelOrQ)

/**
    combo box to show labels or queries
*/
class LabelCombo : public QComboBox 
{
public:
    EitherLabelOrQ  currentLabel()const;
    void            addLabel(QString name, const EitherLabelOrQ& lbl);
    int             findLabel(const EitherLabelOrQ& lbl)const;
    int             findFirstQ()const;
    void            updateQ(int idx, QString q);
    int             addQ(QString q);

    void        addItem(const QString &text, const QVariant &userData = QVariant()) = delete;
    void        addItem(const QIcon &icon, const QString &text, const QVariant &userData = QVariant()) = delete;
    void        addItems(const QStringList &texts) = delete;
    QVariant    currentData(int role = Qt::UserRole) const = delete;
    int         findData(const QVariant &data, int role = Qt::UserRole, Qt::MatchFlags flags = static_cast<Qt::MatchFlags>(Qt::MatchExactly | Qt::MatchCaseSensitive)) const = delete;
#ifdef _DEBUG
    void        debugPrint();
#endif
};

/**
tabbar to show labels or queries
*/
class LabelTabBar : public QTabBar 
{
public:
    EitherLabelOrQ  currentTabLabel()const;
    int         addSysLabelTab(googleQt::mail_cache::SysLabel);
    int         addLabelTab(QString name, const EitherLabelOrQ& lbl);
    int         addQ(QString q);
    int         findLabelTab(const EitherLabelOrQ& lbl)const;
    int         findFirstQ()const;
    void        updateQ(int idx, QString q);

    int addTab(const QString &text) = delete;
    int addTab(const QIcon &icon, const QString &text) = delete;
    void setTabData(int index, const QVariant &data) = delete;
    QVariant tabData(int index) const = delete;
    void setTabText(int index, const QString &text) = delete;
};

class ColorButton;
using CLR_BUTTONS = std::vector<ColorButton*>;

class ColorButtonPanel : public QWidget
{
    Q_OBJECT;
public:
    ColorButtonPanel(ard::EColor idx);

	ard::EColor colorIndex()const { return m_selected_color_idx; };
    void setColorIndex(ard::EColor idx) { m_selected_color_idx = idx; };
protected:
    CLR_BUTTONS     m_cbuttons;
    bool            m_cbutton_changed{ false };
	ard::EColor     m_selected_color_idx{ ard::EColor::none };
    friend class ColorButton;
};

/**
    we give option to change text background in note view
*/
class NoteBarColorButtons : public QWidget 
{
    Q_OBJECT;
public:
    NoteBarColorButtons(int height);

    int     currentColorIndex()const { return m_selected_color_idx; }
    void    selectColor(const QColor& cl);

signals:
    void    onColorSelected(const QColor& cl);
    void    onColorSelectRequested();

protected:
    void    paintEvent(QPaintEvent *e)override;
    void    mousePressEvent(QMouseEvent *e)override;
    int     hit_test(const QPoint& pt);
protected:
    const int m_buttons[5]{ 5, 9, 12, 2, 11};
    int     bmargin;
    int     m_selected_color_idx{ 15 };
    QRect   m_rcButtons, m_rcButtonsSelect;
    int     m_button_width_height{ 0 }, m_select_btn_width{0};
};

/**
* we draw rectange around cursor in editor or around item in outline
*/
class LocationAnimator 
{
public:
    void installLocationAnimator();
    void animateLocator();
protected:
    virtual void locatorUpdateRect(const QRect& rc) = 0;
    virtual QRect locatorCursorRect()const = 0;
    virtual int  locatorVerticalScrollValue()const { return 0; }
    
    void drawLocator(QPainter& p);
    void recalcCursorAniRect(int f);
    void updateCursorAniRect();

    QRect        m_rcCursorAniRect;
    int          m_cursorAniVScrollPos;
    QTimeLine    m_cursorAniTimeLine;
};

/**
    text editor with a way to animate cursor position
*/
class NoteEditBase : public QTextEdit,
                     public LocationAnimator
{
public:
    NoteEditBase();
protected:
    void locatorUpdateRect(const QRect& rc)override;
    QRect locatorCursorRect()const override;
    int  locatorVerticalScrollValue()const override;
    void paintEvent(QPaintEvent *e)override;
};

/**
    AccountButton - show menu to switch account
*/
class AccountButton : public QWidget
{
    Q_OBJECT
public:
    AccountButton();

    void updateAccountButton();

protected:
    void paintEvent(QPaintEvent *e)override;
    void mousePressEvent(QMouseEvent *e)override;

protected:
    QPixmap m_pxmap;
};


class HintLineEdit : public QLineEdit
{
    friend class OutlineTitleEdit;
public:
    HintLineEdit(QString hint_str);

protected:
    void paintEvent(QPaintEvent * event)override;

    QString     m_hint_str;
};
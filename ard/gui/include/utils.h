#ifndef UTILS_H
#define UTILS_H

#include <QtCore>
#include <QtGui>
#include <QThread>
#include <QProgressDialog>
#include <QFutureWatcher>
#include <QPointF>
#include <QIcon>
#include <QAction>
#include <QGraphicsSceneDragDropEvent>
#include <QStyleOptionGraphicsItem>

#include "snc.h"
#include "a-db-utils.h"
#ifdef Q_OS_WIN32
#include "StackWalker.h"
#endif

class QPainter;
class QFont;
class QObject;
class ProtoGItem;
class QBoxLayout;
class QComboBox;
class QToolBar;


using ACTION_LIST = std::vector<QAction*>;
using ACTION_CLR_MAP = std::map<ard::EColor, QAction*>;

class DnDHelper
{
public:
    DnDHelper():m_dragOver(dragUnknown){}
    virtual ~DnDHelper(){};

protected:
    virtual EDragOver calcDragOverType(QGraphicsSceneDragDropEvent *e) = 0;
    virtual void      updateGuiAfterDnd() = 0;
    virtual QRectF    dndRect()const = 0;

    void process_dragEnterEvent(QGraphicsSceneDragDropEvent *e);
    void process_dragLeaveEvent(QGraphicsSceneDragDropEvent *e);
    virtual bool canStartDnD(QGraphicsSceneMouseEvent * e);
    void drawDnDMark(QPainter * p);
protected:
    EDragOver     m_dragOver;
    bool          m_renameInit{false};
};

#ifdef Q_OS_WIN32
namespace ard {
    class rule_runner;

    class ArdStackWalker : public StackWalker
    {
    public:
        ArdStackWalker() {};
        ArdStackWalker(ExceptType extype) :StackWalker(extype) {};
    protected:
        virtual void OnOutput(LPCSTR szText)
        {
            if (szText) {
                QString s = szText;
                qWarning() << s.trimmed();
            }
            StackWalker::OnOutput(szText);
        }
    };
}
#endif

#define ADD_TAB_CTRL_ACTION tb->addAction(a);m_actions.push_back(a);
#define ADD_TAB_CTRL_GROUP_ACTION(a) tb->addAction(a);m_actions.push_back(a);grp->addAction(a);

#define ADD_TOOLBAR_OUTLINE_IMG_ACTION(I, T, M) { QString s2 = T;/*if(!is_big_screen())s2 = "";*/ \
        a = new QAction(utils::fromTheme(I), s2, this);                 \
        connect(a, SIGNAL(triggered()), this, SLOT(M()));               \
        tb->addAction(a);}                                              \


#define ADD_TOOLBAR_OUTLINE_ACTION(T, M)      a = new QAction(T, this); \
    connect(a, SIGNAL(triggered()), outline(), SLOT(M()));              \
    tb->addAction(a);                                                   \




class TextFilter
{
public:

    TextFilter();
    bool filterIn(topic_cptr c)const;
    void setSearchContext(const TextFilterContext& fc);
    const TextFilterContext& fcontext()const{return m_fc;}
    bool  isActive()const;
    QRectF  drawTextLine(QPainter *p, const QRectF& rc, QString s);
    QSize drawTextMLine(QPainter *p, 
        const QFont& font, 
        const QRectF& rc, 
        QString s, 
        bool alignCenter, 
        const QRectF* rc_thumb = nullptr,
        QList<QTextLayout::FormatRange>* custom_format = nullptr);

protected:
    QRectF drawDefaultTextLine(QPainter *p, const QRectF& rc, QString s, QList<QTextLayout::FormatRange>* fl = nullptr);
    QSize drawDefaultTextMLine(QPainter *p, const QFont& font, const QRectF& rc, QString s, bool alignCenter , QList<QTextLayout::FormatRange>* fl = nullptr, const QRectF* rc_clip = nullptr);
    void prepareFormatRangeList(const QFont& font, QString s, int idx, QList<QTextLayout::FormatRange>&);
    void prepareOutlineLabelsFormatRangeList(const QFont& font, int labelLen, QList<QTextLayout::FormatRange>&);

protected:
    TextFilterContext m_fc;
};

namespace utils
{
    void        drawImagePixmap(const QPixmap& pm, QPainter *painter, const QRect& rc, QPoint* ptLT = nullptr);

    void        drawCheckBox(QPainter *painter, const QRectF& rc, bool checked, int CompletedPercentages = 0, bool drawEmptyBox = false);
    void    drawCompletedBox(topic_ptr it, QRectF& rc, QPainter *painter);
    void    drawPriorityIcon(topic_ptr it, QRect& rc, QPainter *painter);
  

    QFont*      defaultBoldFont();
    QFont*      defaultSmallBoldFont();
    QFont*      defaultTinyFont();
    QFont*      defaultSmall2Font();
    QFont*      defaultNoteFont();
    QString     defaultFontFamily();
    int         defaultFontSize();
    QFont*      groupHeaderFont();
    QFont*      annotationFont();
    QFont*      nheaderFont();
    QFont*      graphNodeHeaderFont();

    int      outlineDefaultHeight();
    int      outlineSmallHeight();
    int      outlineTinyHeight();
    int      projectHeaderHeight();
    int      outlineShortDateWidth();

    qreal    nheaderFontHeight();

    unsigned    calcWidth(QString s, QFont* font = nullptr);
    unsigned    calcHeight(QString s, const QFont* font = nullptr);
    QSize       calcSize(QString s, QFont* font = nullptr);
    unsigned    node_width(QString, unsigned minWidth);
    unsigned    node_width(topic_ptr, unsigned minWidth);
    QSize       calc_text_svg_node_size(const QSizeF& szOriginalImg, QString s);
    void        svg2pixmap(QPixmap* pm, QString svg_resource, const QSize& sz);
  
    bool        resetFonts();
    void        clean();
    void        applySettings();
    void        prepareGuiAfterTopicsMoved(const TOPICS_LIST& topics_moved);

    void       setupCentralWidget(QWidget *parent, QWidget* child);
    void       setupBoxLayout(QLayout* l);
    void       expandWidget(QWidget* w);
    void       setupProgressBar(QProgressBar* b);

    void        drawDateLabel(topic_ptr t, QPainter *p, const QRectF& rc);
    void        drawLabels(topic_ptr t, QPainter *p, const QRectF& rc);
    void        drawGmailLabels(topic_ptr t, QPainter *p, const QRectF& rc1);

    QString     get_program_appdata_log_dir_path();

    QIcon       fromTheme(QString name);
    QAction*    actionFromTheme(QString theme, QString label, QObject* parent, QAction::Priority priority = QAction::LowPriority);
    QPixmap     colorMarkIcon(ard::EColor clr_idx, bool as_checked);
    QPixmap*    colorMarkSelectorPixmap(ard::EColor c);


    ///removeEmptySubfolders - returns list of empty folders inside some parent
    void removeEmptySubfolders(QString parentPath);

    int        choice(std::vector<QString>& options_list, int selected = -1, QString label = "");
    int        choiceCombo(std::vector<QString>& options_list, int selected = -1, QString label = "", std::vector<QString>* buttons_list=nullptr);
    int        outlinePadding();
    //void       drawProjectPie(topic_ptr f, QPainter* p, const QRect& rct);
    void       drawQFolderStatus(ard::rule_runner* f, QPainter* p, const QRect& rct);
    void       setupToolButton(QPushButton* b);
};


extern int guessOutlineHeight(ProtoGItem* git, int textWidth, OutlineContext c);
extern int guessOutlineAnnotationHeight(QString annotation, int textWidth);
extern int guessOutlineExpandedNoteHeight(QString annotation, int textWidth);
extern TextFilter& globalTextFilter();

#ifdef Q_WS_WIN
#define strncpy_s(D, DS, S, N)strncpy_s(D, S, N)
#define sprintf_s2(B, BS, F, P1, P2)sprintf_s(B, F, P1, P2)
#define sprintf_s3(B, BS, F, P1, P2, P3)sprintf_s(B, F, P1, P2, P3)
#else
#define strncpy_s(D, DS, S, N)strncpy(D, S, N)
#define sprintf_s2(B, BS, F, P1, P2)sprintf(B, F, P1, P2)
#define sprintf_s3(B, BS, F, P1, P2, P3)sprintf(B, F, P1, P2, P3)
#endif

#endif // UTILS_H

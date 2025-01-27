#pragma once

#include <QTimeLine>

#include "snc.h"
#include "a-db-utils.h"
#include "custom-g-items.h"
#include "anfolder.h"
#include "utils.h"

#define PARAM_SYNC_SELECT "sync-sel-2-main-outline"
#define PARAM_GCONFIG_ID  "gconf-id"
#define PARAM_MAX_GNODES  "max-gnodes"
#define PARAM_GRAPH_RANK  "graph-rank"

class ArdGraphicsScene;
class QGraphicsView;
class ProtoScene;
class ProtoPanel;
class ProtoGItem;
class QGraphicsItem;
class ProtoGSecondaryItem;
class HudButton;

typedef std::unordered_map<topic_ptr, ProtoGItem*>  ITEM_2_GITEM;
typedef std::vector<ProtoGItem*>                    GITEMS_VECTOR;
typedef std::vector<ProtoGSecondaryItem*>             GSEC_ITEMS_LIST;
typedef std::unordered_map<ProtoGItem*, int>        GITEM_2_IDX;
typedef std::vector<ProtoGItem*>                      GITEMS_LIST;
typedef std::vector<CurrentMark*>                     GCURRENT_MARKS;
typedef std::vector<HudButton*>                     GHUD_BUTTONS;
typedef std::vector<QGraphicsLineItem*>               LINES_LIST;
typedef std::vector<QGraphicsPixmapItem*>             GPMAP_LIST;

Q_DECLARE_METATYPE(ProtoGItem*);

struct SHit
{
    topic_ptr item;
    int     column_number;
};

/**
   ProtoGItem - and abstruct item of ProtoScene.
   ProtoScene keeps track of ProtoGItem objects
   Designed as aggregator of item data object,
   used in multiple inheritance for all QGraphicsItem
   derived object in ProtoScene.
*/

class ProtoGItem
{
public:
    ProtoGItem();
    ProtoGItem(topic_ptr item, ProtoPanel*);
    virtual ~ProtoGItem();

    void invalidate_proto();
    
    virtual QGraphicsItem*       g() = 0;
    virtual const QGraphicsItem* g()const = 0;
    ProtoPanel*                  p() {return m_p;}
    const ProtoPanel*            p()const{return m_p;}

    topic_ptr            topic() { return m_item; };
    topic_cptr           topic()const { return m_item; };
    virtual void         onBecomeCurrent(bool propagate2workspace);
    virtual ECurrentMarkType currentMarkType()const { return ECurrentMarkType::typeOutlineCurr; }

    virtual bool         isCheckSelected()const { return false; };
    virtual void         setCheckSelected(bool ){};
    virtual bool         asHeaderTopic()const{return false;}

    virtual QRectF       secondaryRect()const{return g()->boundingRect();}
    virtual EHitTest     hitTest(const QPointF&, SHit&){return hitTitle;};
    virtual void         getOutlineEditRect(EColumnType column_type, QRectF& rc);
    //QString              pathLabel()const { return m_path_label; }
    //void                 setPathLabel(QString s) { m_path_label = s; }
    virtual void         setAsRootTopic() {};

    virtual void         recalcSize();
    virtual void         resetGeometry();
    virtual void         regenerateCurrentActions();
    virtual void         processDefaultAction();
    virtual void         processDoubleClickAction();
    virtual bool         preprocessMousePress(const QPointF&, bool wasSelected);
    virtual void         mouseRightClick(const QPoint&) {};
    virtual bool         preselectPreprocessMousePress(const QPointF&) { return false; };
    virtual void         request_prepareGeometryChange() = 0;
    virtual void         processHoverMoveEvent(QGraphicsSceneHoverEvent * e);

    virtual void        on_clickedToDo();
    virtual void        on_clickedActionBtn();
    //virtual void        on_clickedDateColumn();
    virtual void        on_clickedUrl();
    virtual void        on_clickedTitle(const QPointF& p, bool wasSelected);
    virtual void        on_clickedAnnotation(const QPointF& p, bool wasSelected);
    virtual void        on_clickedExpandedNote(const QPointF& p, bool wasSelected);
    virtual void        on_clickedFatFingerSelect();
    virtual void        on_clickedFatFingerDetails(const QPoint& p);
    virtual void        on_keyPressed(QKeyEvent * e);
    virtual void        on_clickedTernary();

#ifdef _DEBUG
    QString             dbgMark()const { return m_dbg_mark; };
    void                setDbgMark(QString s) { m_dbg_mark = s; }
#endif

protected:
    virtual void        updateGeometryWidth();
    virtual void        optAddCurrentCommand(ECurrentCommand c);
protected:
    topic_ptr            m_item;
    ProtoPanel*          m_p;
#ifdef _DEBUG
    QString             m_dbg_mark;
#endif
};

/**
   ProtoGSecondaryItem - derived secondary item, generated
   by ProtoGItem, used in multiple inheritance with QGraphicsItem
*/
class ProtoGSecondaryItem
{
public:
    ProtoGSecondaryItem(ProtoGItem* prim);
    virtual ~ProtoGSecondaryItem();

    virtual QGraphicsItem*       g() = 0;
    virtual const QGraphicsItem* g()const = 0;

    virtual ProtoGItem*       primary(){return m_prim;};
    virtual const ProtoGItem* primary()const{return m_prim;};
    virtual void              rebuild() = 0;
protected:
    ProtoGItem* m_prim;
};


/**
   One ProtoPanel per ProtoScene
*/
class ProtoPanel
{
    friend class ProtoScene;
public:
    ProtoPanel(ProtoScene* s);
    virtual ~ProtoPanel();

    enum EProp{
        xx1    = 0x1,
        PP_ActionButton     = 0x2,  // big button with label for every item in outline, on the left side
        PP_Annotation       = 0x4,  // yellow label text next to title in outline
        PP_InplaceEdit      = 0x8, //can have title editor in place
        xx2   = 0x10,
        PP_RTF              = 0x20, 
        xx3          = 0x40,
        xx4             = 0x80,
        PP_DnD              = 0x100, 
        PP_DateLabel        = 0x200, //small date label on the right for email views
        PP_Thumbnail        = 0x400, 
        PP_Filter           = 0x800, 
        PP_CheckSelectBox   = 0x1000,
        PP_ToDoColumn       = 0x2000, 
        PP_ExpandNotes      = 0x4000,
        PP_Labels           = 0x8000, 
        PP_MultiLine        = 0x10000, 
        XX5        = 0x20000,
        XX6     = 0x40000,
        PP_CurrDownload     = 0x80000,
        PP_CurrDelete       = 0x100000,
        PP_CurrOpen         = 0x200000,
        PP_CurrFindInShell  = 0x400000,
        PP_CurrSpot         = 0x800000,
        PP_CurrMoveTarget   = 0x1000000,
        PP_CurrSelect       = 0x2000000,
        PP_CurrEdit         = 0x4000000,
        PP_FatFingerSelect  = 0x8000000,//fat finger cloud around item for tapping
        XX7      = 0x10000000
    }; 

    typedef std::set<ProtoPanel::EProp> PANEL_PROPERTIES;

    virtual QString       pname()const { return "panel"; };

    ProtoScene*           s(){return m_s;}
    const ProtoScene*     s()const{return m_s;}
    GITEMS_VECTOR&        outlined(){return m_outlined;}
    virtual void          registerGI(topic_ptr it, ProtoGItem*, GITEMS_VECTOR* registrator = nullptr);
    virtual ProtoGItem*   findGI      (topic_ptr it);
   // virtual ProtoGItem*   selectNext  (bool go_up, ProtoGItem* gi);
    void                  shiftDown(ProtoGItem* gi, qreal dy);
    virtual QRectF        boundingRectInSceneCoord()const;
    virtual bool          isLastInOutline(ProtoGItem* gi);
    virtual void          freeItems   ();
    virtual void          clear       ();
    virtual void          updateHudItemsPos();
    virtual void          updateAuxItemsPos();
    virtual void          createAuxItems();
    qreal                 panelWidth()const{return m_panelWidth;}
    virtual qreal         textColumnWidth()const;
    void                  setPanelWidth(qreal val);
    qreal                 panelIdent()const{return m_panelIdent;}
 
    inline bool           hasProp(EProp p)const { return ((m_panel_prop & p) != 0); };
    void                  setProp(std::set<EProp>* prop2set, std::set<EProp>* prop2remove = nullptr);
    /// some aux outline in dlg wit option to select topic to proceed
    bool                  isSelectorControl()const {return m_selector_control;}
    void                  clearAllProp();
    static PANEL_PROPERTIES   allProperties();
    qreal                 calcHeight();
    virtual void          rebuildPanel() = 0;// { ASSERT(0, "not-implemented"); }
    template<class T>
    qreal                 rebuildAsListPanel(std::vector<T*>& items, bool autoCreateAuxItems = true);
    template<class T>
    qreal                 rebuildAsListPanel(std::vector<T*>& items, QString search_str, bool autoCreateAuxItems = true);
    virtual void          resetGeometry();
    virtual void          resetPosAfterGeometryReset() { /*resetPosAfterGeometryResetForOutlines();*/ }
    void                  resetPosAfterGeometryResetForOutlines();

    virtual void          applyParamMap();

    void                  showHint(QString text, const QPointF& pt);
    void                  hideHint();
    virtual bool          hasCloudMarks()const{ return false; }

    EColumnType           lastActiveColumn()const{return m_last_active_column;}
    void                  setLastActiveColumn(EColumnType c){m_last_active_column = c;}
    
    virtual ProtoGItem*   footer() { return nullptr; }
    virtual void          onDefaultItemAction(ProtoGItem* ) {};
#ifdef _DEBUG
    std::pair<int, int>     dbgPrint(QString prefix_label);
#endif
    virtual int             actionBtnWidth();
    virtual QString         actionBtnText();
    OutlineContext       ocontext()const{return m_ocontext;}
    void                 setOContext(OutlineContext c);
	virtual ProtoGItem*  produceOutlineItems(topic_ptr it,
		const qreal& x_ident,
		qreal& y_pos,
		GITEMS_VECTOR* registrator = nullptr);

protected:

    void resetClassicOutlinerGeometry();
    HierarchyBranchMark*  createHierarchyBranchMark();
    QString shortTitle(QString s);

protected:
    OutlineContext       m_ocontext{OutlineContext::normal};
    quint64              m_panel_prop;    
    ProtoScene*          m_s;
    ITEM_2_GITEM         m_it2gi;
    GITEMS_VECTOR        m_outlined;
    qreal                m_panelWidth{ 50.0 }, m_panelIdent{ 0.0 };
    int                  m_action_btn_with{ -1 };

    QGraphicsLineItem   *m_vline1{nullptr}, *m_vline2{nullptr}, 
                        *m_check_select_vline{nullptr};
    QGraphicsLineItem   *m_header_line{nullptr};
    QGraphicsTextItem   *m_hint_item{nullptr};
    AUX2_ITEMS           m_aux2;
    bool                 m_multi_G_per_item{false};
    bool                 m_selector_control{false};
    EColumnType          m_last_active_column{EColumnType::Uknown};
};

class CallbackObj;

/**
   ProtoView is used in QGraphicsView derived 
   (multiple inheritance) and works in pair with
   ProtoScene
*/
class ProtoView
{
public:
    virtual ~ProtoView(){}
    virtual void            rebuild() = 0;

    virtual QGraphicsView* v() = 0;
    virtual const QGraphicsView* v()const = 0;
    virtual void    emit_inDerived_onResetSceneBoundingRect()=0;
    void ensureVisibleGItem(ProtoGItem* pg);

    QObject*          sceneBuilder(){return m_sceneBuilder;}
    virtual void      renameSelected(EColumnType field2edit = EColumnType::Title, 
                                    const QPointF* ptClick = nullptr,
                                    bool selectContent = false) = 0;
    virtual bool      isInTitleEditMode()const = 0;


protected:
    QObject*              m_sceneBuilder;
};

/**
   ProtoScene for QGraphicsScene
*/
class ProtoScene
{
public:

    ProtoScene();
    virtual ~ProtoScene();

    virtual ArdGraphicsScene* s() = 0;
    virtual const ArdGraphicsScene* s()const = 0;
    virtual ProtoView* v() = 0;
    virtual const ProtoView* v()const = 0;  


    void                    selectGI(ProtoGItem* pg, bool propagate2workspace = true);
    virtual ProtoGItem*     currentGI(bool acceptTool = true) = 0;
    virtual ProtoGItem*     selectNext(bool go_up, ProtoGItem* gi = nullptr);   
    ProtoGItem*             footer();
    virtual void            freePanels();
    virtual void            clearOutline() = 0;
    virtual void            installCurrentItemView(bool propagate2workspace) = 0;
    ProtoPanel*             panel();
    bool                    hasPanel()const;
    void                    addPanel(ProtoPanel*);
    ///return outlined gitems between g1 and g2
    std::vector<ProtoGItem*> gitemsInRange(ProtoGItem* gi1, ProtoGItem* gi2);
    ProtoGItem*             findGItem(topic_ptr it);
    ProtoGItem*             findGItemByUnderlying(topic_ptr it);
    ProtoGItem*             findGItemByEid(QString eid);
    ProtoGItem*             findGItem(std::function<bool (topic_ptr)> findBy);
  //  void                    removeSelItem();
    void                    gui_delete_mselected();
    void                    setupCurrentSpot(ProtoGItem* pg, bool force = false);

    void                    freeCurrMarks();    
    void                    hideCurrMarks();
    TOPICS_LIST             mselected();
    void                    clearMSelected();
    void                    mselectAll(std::function<bool(topic_ptr)> pred = nullptr);
    void                    storeMSelected();
    void                    restoreMSelected();
    void                    registerMSelectedByMouse(topic_ptr f);
    topic_ptr               lastMselectedByMouse();

    bool                    isMainScene()const{return m_main_scene;}
    void                    setMainScene(bool val){m_main_scene = val;}
    
    const QSize&            viewportSize(){return m_viewport_size;}
    EOutlinePolicy  outlinePolicy()const{return m_outline_policy;}
    virtual void    setOutlinePolicy(EOutlinePolicy p);

    QRectF          enabledItemsBoundingRect () const;
    virtual void    resetSceneBoundingRect();
    void            updateAuxItemsPos();
    virtual void    resetPanelsGeometry();

    virtual void     updateItem(topic_ptr it) = 0;
    /// param map is passed to panel during creating
    /// it's up to the panel to interprete the map
    PARAM_MAP& paramMap(){return m_param_map;}
    bool       hasParam(QString name)const;
  
    ///setINproperties/getOUTproperties - properties
    ///that overwrite all panel properties during rebuild
    const ProtoPanel::PANEL_PROPERTIES&   setINproperties()const{return m_setINproperties;}
    const ProtoPanel::PANEL_PROPERTIES&   getOUTproperties()const{return m_getOUTproperties;}

    OutlineContext                      enforced_ocontext()const {return m_enforced_ocontext;}
    void                                set_enforced_ocontext(OutlineContext c) { m_enforced_ocontext = c; }

    //OutlineContext            m_enforced_ocontext{ OutlineContext::none };

    void                      assignSetINproperties(ProtoPanel::PANEL_PROPERTIES& p){m_setINproperties = p;};
    void                      assignGetOUTproperties(ProtoPanel::PANEL_PROPERTIES& p){m_getOUTproperties = p;};
public:
    //virtual void    processHoverLeaveEvent(topic_ptr);
    void            setupHint(EHitTest, ProtoGItem*){};
    //void            clearHint(){};
    void            resetVisibleItemsOpacity(QGraphicsItem* g_except, qreal opacity);

    GCURRENT_MARKS&  current_marks(){return m_current_marks;}
    GHUD_BUTTONS     top_right_huds(){return m_top_right_huds;}
    GHUD_BUTTONS     left_aligned_huds(){return m_left_aligned_huds;}
    void             addTopRightHudButton(HudButton* h);
    void             addLeftAlignedHudButton(HudButton* h);
    void             addBottomCenterHudButton(HudButton* h);
    void             showEditFrame(bool show, QRectF rc = QRectF(), EColumnType c = EColumnType::Title);

protected:
    virtual void     updateSceneHudItemsPos();

protected:
    EOutlinePolicy          m_outline_policy;
    ProtoPanel*             m_panel{nullptr};
    GCURRENT_MARKS          m_current_marks;
    GHUD_BUTTONS            m_top_right_huds;
    GHUD_BUTTONS            m_left_aligned_huds;
    GHUD_BUTTONS            m_bottom_center_huds;
    PARAM_MAP               m_param_map;
    QSize                   m_viewport_size;
    bool                    m_main_scene{false};
    TOPICS_SET              m_stored_mselected;
    topic_ptr               m_last_mselected_by_mouse{nullptr};///we don't lock pointer
    
    ProtoPanel::PANEL_PROPERTIES     m_setINproperties;
    ProtoPanel::PANEL_PROPERTIES     m_getOUTproperties;
    OutlineContext          m_enforced_ocontext{ OutlineContext::none};

    TitleEditFrame*      m_edit_frame{nullptr};
};


#define ZVALUE_BACK               -1.0
#define ZVALUE_NORMAL              0.0
#define ZVALUE_BELOW_NORMAL       -0.1
#define ZVALUE_FRONT               0.05
#define ZVALUE_FRONT_1             0.1

#define FREE_GITEM(A)if(A){s()->s()->removeItem(A);delete(A);A = nullptr;}

/**
   HudButton - member of ProtoScene, suppose to
   stick in defined position on scene and ignore scrolling
*/

class HudButton: public QGraphicsRectItem
{
public:
    enum E_ID
        {
            idUnknown = -1,
            idOK,
            idCancel,
            idCreateObject,
            idFilter,
            idNext,
            idPrev
        };
  
    enum class EHudType
        {
            Button = 0,
            LineBreak,
            Break,
            Label
        };

    HudButton(QObject* owner, E_ID _id, QString _label, QString imageResource);
    HudButton(QObject* owner, QString _label, int _data, QString imageResource);

    HudButton(QObject* owner, EHudType t, QString _label = "", E_ID _id = idUnknown, int _data = 0, int _width = 0);

    E_ID getID()const{return m_id;}
    int data()const{return m_data;}
    void setMinSize(QSize sz);
    void setHeight(int h);
    void setWidth(int w);
    void setImageStyleSize();

    EHudType hudType()const{return m_type;}
    bool isBreak()const{return (m_type == EHudType::LineBreak || m_type == EHudType::Break);}

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);
    virtual bool preprocessMouseEvent();

    QString  getLabel()const{return m_label;}

protected:
    void setupButton();
    void drawAsButton(QPainter *painter);
    void drawAsLine(QPainter *painter);
    void drawAsLabel(QPainter *painter);

protected:
    QObject* m_owner{nullptr};
    E_ID     m_id;
    QString  m_label;
    QString  m_imageRes;
    int      m_data;
    EHudType m_type;
    bool     m_asImageOnly;
    int      m_force_width;
};

template<class T>
qreal ProtoPanel::rebuildAsListPanel(std::vector<T*>& items, bool autoCreateAuxItems)
{
    if (m_panelWidth < 150 || m_panelWidth > 1200)
        m_panelWidth = 700;

    bool filterIn = true;
    qreal x_ident = 0;
    qreal y_pos = 0.0;
    if (!m_outlined.empty()) {
        ProtoGItem* g = *(m_outlined.rbegin());
        QPointF br = g->g()->mapToScene(g->g()->boundingRect().bottomRight());
        y_pos = br.y();
    }


    auto begin_list = items.begin();
    auto end_list = items.end();

    for (auto i = begin_list; i != end_list; i++)
    {
        auto it = *i;
        if (globalTextFilter().isActive())
        {
            filterIn = globalTextFilter().filterIn(it);
        }
        else
        {
            filterIn = true;
        }
        if (filterIn)
        {
            produceOutlineItems(it, x_ident, y_pos);
        }
    }

    if (autoCreateAuxItems) {
        createAuxItems();
    }

    return y_pos;
};

template<class T>
qreal ProtoPanel::rebuildAsListPanel(std::vector<T*>& items, QString search_str, bool autoCreateAuxItems)
{
    if (m_panelWidth < 150 || m_panelWidth > 1200)
        m_panelWidth = 700;

    bool filterIn = true;
    qreal x_ident = 0;
    qreal y_pos = 0.0;
    if (!m_outlined.empty()) {
        ProtoGItem* g = *(m_outlined.rbegin());
        QPointF br = g->g()->mapToScene(g->g()->boundingRect().bottomRight());
        y_pos = br.y();
    }

    TextFilterContext fc;
    fc.key_str = search_str;
    fc.include_expanded_notes = hasProp(PP_ExpandNotes);

    bool active_filter = !search_str.isEmpty();

    auto begin_list = items.begin();
    auto end_list = items.end();

    for (auto i = begin_list; i != end_list; i++)
    {
        auto it = *i;
        if (active_filter)
        {
            filterIn = it->hasText4SearchFilter(fc);
        }
        else
        {
            filterIn = true;
        }
        if (filterIn)
        {
            produceOutlineItems(it, x_ident, y_pos);
        }
    }

    if (autoCreateAuxItems) {
        createAuxItems();
    }

    return y_pos;
};


#define SET_PPP(S, P) S.insert(ProtoPanel::P);

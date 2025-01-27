#pragma once

#include <QWidget>
#include <QGraphicsView>
#include <QGraphicsRectItem>
#include <QGraphicsPathItem>
#include "utils.h"
#include "custom-widgets.h"

class anItem;
class TabG;
class TabGMark;
class TabView;
class WorkingNoteEdit;

class  TabControl: public QWidget
{
    Q_OBJECT

public:
    class tab;

signals :
    void        tabSelected(int);
    void        currentTabClicked();

public:

    enum class EType
        {
            Right,
            Left,
            ABC,
            //LocusedToobar,
            RLocusedToobar
        };


    TabControl(TabControl::EType t, QString hint = "");
    virtual ~TabControl();

    tab*        addTab(QString _label, int _data);
    tab*        addTab(QPixmap _icon, QString _label, int _data);
    void        setLocusTopics(const TOPICS_LIST& lst);
    tab*        addLocusTopic(topic_ptr f);
	bool		hasLocusTopics()const;
    tab*        addSecondaryTab(QPixmap _icon, QString _label, int _data);
    bool        hasSecondaryTabs()const{return !m_secondary_tabs.empty();}
    bool        isEmpty()const;
    int         count()const;
    tab*        tabAt(int pos);

    QString     hint()const { return m_hint; }
    void        setHint(QString s) { m_hint = s; }

    void        removeAllTabs();
    qreal       tabWidth()const{return m_tab_wh;}
    void        setTabWidth(qreal w);
    void        setTabHeight(qreal h);
    void        rebuildTabs();
    void        update();
    void        asyncRebuild();

    void        detachGui();
    int         indexOfTopic(topic_cptr f)const;
    int         indexOfTopic(DB_ID_TYPE fid)const;
    void        ensureVisible(DB_ID_TYPE fid);

    TabView*            v(){return m_view;}
    ArdGraphicsScene*   s(){return m_scene;}

    static TabControl* createMainTabControl();
    static TabControl* createGrepTabControl();
    static TabControl* createABCTabControl();
    static TabControl* createLocusedToobar(EOutlinePolicy pol);
    static TabControl* createKRingTabControl();

    void        resetTabControl();
    void        setSideTabDefaultTabWidth();
    bool        vert_text_by_symbol()const {return m_vert_text_by_symbol;}

    int         calcBoundingHeight()const;

    EOutlinePolicy      policy()const { return m_policy; }
    const WorkingNoteEdit*    noteEdit()const { return m_note_edit; }

#ifdef _DEBUG
    void        debug_print();
#endif

public:
    class tab
    {
    public:
        tab(TabControl* _tc, 
            QString _label, int _data):
            m_tab_control(_tc), 
            m_label(_label), 
            m_data(_data){};

        virtual ~tab();

        QString label()const{return m_label;}
        void    setLabel(QString s) { m_label = s; }
        int     data()const{return m_data;}
		QString mark_label()const;

		TabControl *		tc(){return m_tab_control;}
		const TabControl * tc()const { return m_tab_control; }
		bool              isEnabledTab()const{return m_enabled; }
		void              setEnabledTab(bool val) { m_enabled = val; }

		void				attach_topic(ard::topic* f);

        QPixmap             icon()const{return m_icon;}
        void                setIcon(QPixmap c){m_icon = c;}

        ard::EColor			colorIndex()const {return m_color_index;}
        void                setColorIndex(ard::EColor  c) { m_color_index = c; }
        
        bool                isOptTab()const{return m_is_opt;}
        bool                isVertical()const {return m_is_vertical_text;}
        void                setVertical(bool val) { m_is_vertical_text = val; }

        bool                isChecked()const{return m_is_checked;}
        void                setChecked(bool v){m_is_checked = v;};

        bool                isSpaceButton()const { return m_is_space_button; }

        int                widthMultiplier()const { return m_width_multiplier; }
        void               setWidthMultiplier(int val) { m_width_multiplier = val; }

        QRectF              calcBoundingRect()const;        
    protected:
        TabControl			*m_tab_control;
        QString				m_label;
        mutable QString		m_view_header;
        int					m_data{ 0 };
        bool				m_enabled{true};
        QPixmap				m_icon;
        bool				m_is_checked{false};
        bool				m_is_opt{false};
        bool				m_is_vertical_text{ true };
        bool				m_is_space_button{ false };
        int					m_width_multiplier{ 1 };
        ard::EColor			m_color_index{ ard::EColor::none };
		ard::topic*			m_topic{nullptr};
        friend class TabControl;
    };
    typedef std::vector<tab*>     TABS;
    typedef std::map<tab*, TabG*> TABS2G;
    typedef std::vector<TabG*>    GTABS;
    using TOPIC2TAB = std::map<topic_ptr, tab*>;

    ///select and notify
    void        selectTab(tab*);
    bool        selectTabByData(int d);
    void        setCurrentTab(tab*);
    void        setCurrentTabByData(int d);
    bool        isCurrent(const tab*)const;
    bool        hasCurrent()const;
    int         currentData()const;

    tab*        currentTab();
    tab*        findByData(int d);
    tab*        findSecondaryByData(int d);
    int         indexOfTab(tab*);
    void        setTabData(int idx, int data);
    EType       ttype()const{return m_ttype;}

    tab*        doAddLocusTopic(topic_ptr f);
protected:
    TabG*       registerTab(tab* t);
    void        updateGItem(tab* t);

    void        initAsVerticalTab();
    void        initAsABC();

    void        rebuildAsThumbnailer();
    void        rebuildAsVerticalTab();
    void        rebuildAsABC();

    void        rebuildXDimensionToolbar(qreal x_pos, qreal y_pos);

    void        checkSpacer();
    int         selectNextTab(tab* t);
    void        clearTabControl();
public:
    virtual void onTabSelected(tab* t);
    virtual void onCurrentTabClicked(tab* t);

protected:
    TabView*                m_view{nullptr};
    ArdGraphicsScene*       m_scene{nullptr};
    QBoxLayout*             m_tspace_vlayout{nullptr};
    TABS                    m_tabs, m_secondary_tabs;
    //TOPIC2TAB               m_locked_t2t;
    qreal                   m_tab_wh;
    tab*                    m_curr_tab{nullptr};
    QWidget*                m_empty_tab_spacer{nullptr};
    TABS2G                  m_tabs2gitem;
    EType                   m_ttype;
	bool					m_has_locused{false};
    bool                    m_vert_text_by_symbol{false};
    WorkingNoteEdit*        m_note_edit;
    QString                 m_hint;
    bool                    m_async_rebuild_request{ false };
    EOutlinePolicy          m_policy{ outline_policy_Uknown };
    enum class ESecNoteTbar 
    {
        none,
        font,
        bullets,
        size
    };

    ESecNoteTbar            m_sec_note_tbar{ ESecNoteTbar::none };
};

/**
   TabView
*/

class TabView: public ArdGraphicsView
{
    Q_OBJECT
public:
    TabView(TabControl* tc);
    void            resetGItemsGeometry()override;
protected:
    void mousePressEvent(QMouseEvent * e)override;
    void mouseReleaseEvent(QMouseEvent * e)override;
    void mouseMoveEvent(QMouseEvent* e)override;
 protected:
    TabControl*     m_tc;
	bool            m_splitter_move_request{ false };
    QPoint          m_ptMousePress;
};

struct WrappedTabControl 
{
public:
    QWidget*                                parent_wdg;
    TabControl*                             tab_ctrl;
    std::map<EOutlinePolicy, TabControl*>   locus_space;
    bool                                    expand_to_fill;
};
typedef std::map<EOutlinePolicy, WrappedTabControl> TAB_CONTROLS;


#define T_ICO_MARGIN 3 * ARD_MARGIN
#define TUMB_MARGIN 3 * ARD_MARGIN

#pragma once
#include <memory>
#include <QGraphicsView>
#include "ProtoScene.h"
#include "custom-widgets.h"
#include "OutlineSceneBase.h"

class OutlineTitleEdit;
class OutlineScene;
class QPinchGesture;
class QGestureEvent;

namespace ard 
{
	class topic;
};

class OutlineView : public ArdGraphicsView,
    public ProtoView
{
    friend class OutlineMain;

    Q_OBJECT
public:
    explicit OutlineView(QWidget *parent);
    ~OutlineView();


    QGraphicsView* v()override { return this; };
    const QGraphicsView* v()const override { return this; };

    void            resetGItemsGeometry()override;
    void            rebuild()override;
    void            renameSelected(EColumnType field2edit = EColumnType::Title,
        const QPointF* ptClick = nullptr,
        bool selectContent = false)override;
    bool            isInTitleEditMode()const override;

    OutlineSceneBase* base_scene();
    const OutlineSceneBase* base_scene()const;

    void              updateTitleEditFramePos()override;

    void              setupSelectRequestOnRebuild(topic_ptr it);
    void              clearSelectRequestOnRebuild();
    bool              wasSelectOnRebuildRequested();

    void              setEnableHScrollInClassicOutline(bool val);

    void              enableWithClosedDB() { m_invalidWithClosedDB = false; };
    //EScrollType       kscroll()const override;

protected:
    void resizeEvent(QResizeEvent * event) override;
    void mousePressEvent(QMouseEvent * event)override;
    void mouseReleaseEvent(QMouseEvent * e) override;
    void scrollContentsBy(int dx, int dy)override;
    void focusInEvent(QFocusEvent *e)override;
    void focusOutEvent(QFocusEvent *e)override;
    void emit_inDerived_onResetSceneBoundingRect()override;
    //void contextMenuEvent(QContextMenuEvent *e)override;

    protected slots:
    void verticalScrollValueChanged(int value);
    void horizontalScrollValueChanged(int value);
    void delayedResetSceneBoundingRect()override;
    void onTitleEditDetached();

protected:
    OutlineTitleEdit*     m_title_edit;
    int                   m_invalidWithClosedDB;
    bool                  m_enableHScrollInClassicOutline;
    DB_ID_TYPE            m_selectRequestOnRebuild;
    QPointF             m_ptMousePressPos;
    bool                    m_select_processed{ false };
};

/**
    scene_view - a pair of scene & view
*/
struct scene_view
{
    using ptr = std::unique_ptr<scene_view>;
    using list_ptr = std::vector<ptr>;
	using map_idx2v = std::map<int, ptr>;

    ///create bare bone scene/view pair, builder class can be attached later
    static ptr create(QWidget *parent);
	static ptr create_with_builder(std::function<void (OutlineScene* s)>, std::set<ProtoPanel::EProp> prop, QWidget *parent, OutlineContext enforced_ocontext = OutlineContext::none);
    scene_view() {};
    virtual ~scene_view();

	void	run_scene_builder();

	OutlineSceneBase*						scene{ nullptr };///@todo: make it OutlineScene
    OutlineView*							view{ nullptr };
	std::function<void(OutlineScene* s)>	scene_builder;

protected:

};

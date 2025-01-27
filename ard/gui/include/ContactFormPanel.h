#pragma once
#include "TablePanel.h"


/**
    table with 2 columns to show contacts details as a form view
*/
class FormPanel : public TablePanel 
{
public:
    FormPanel(ProtoScene* s);
    void build_form4topic(topic_ptr f);

    void              createAuxItems()override;
    void              updateAuxItemsPos()override;
    void              freeItems()override;
    void              clear()override;
    virtual ProtoGItem*   footer()override { return m_notes_footer; }

protected:
    ProtoGItem*     produceOutlineItems(topic_ptr it,
        const qreal& x_ident,
        qreal& y_pos,
        GITEMS_VECTOR* registrator = nullptr)override;


protected:
    GPMAP_LIST          m_pxmaps;
    FormPanelNoteFooterGItem* m_notes_footer{nullptr};
    QGraphicsLineItem*  m_notes_footer_line{ nullptr };
};

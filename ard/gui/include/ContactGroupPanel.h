#pragma once

#include "OutlinePanel.h"

class ContactGroupPanel: public OutlinePanel
{
public:
    ContactGroupPanel(ProtoScene* s);
    virtual ~ContactGroupPanel();
    void           rebuildPanel()override;    
    void           onDefaultItemAction(ProtoGItem* g)override;
protected:
    void           rebuildGroupDataModel();
    void           releaseGroupDataModel();

    ProtoGItem*  produceContactCard(ard::contact* c,
        const qreal& x_ident,
        qreal& y_pos);
};


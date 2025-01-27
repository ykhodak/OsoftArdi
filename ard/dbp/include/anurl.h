#pragma once
#include "anfolder.h"

/**
    url as specialized topic
*/
namespace ard {
    class anUrl :public ard::topic
    {
        DECLARE_IN_ALLOC_POOL(anUrl);

        anUrl(QString title = "");
    public:
        
        static anUrl*           createUrl(QString title, QString ref);
        static anUrl*           createUrl();

        EOBJ                    otype()const override { return objUrl; }
		bool					canAttachNote()const override { return false; }
        ENoteView               noteViewType()const override{ return ENoteView::None; };
        QPixmap                 getSecondaryIcon(OutlineContext)const override;
        void                    fatFingerSelect()override;
        void                    openUrl();
        QString                 url()const;
        void                    setUrl(QString u);
        cit_prim_ptr            create()const override;
        QSize                   calcBlackboardTextBoxSize()const override;
    };
};

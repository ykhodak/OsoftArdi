#pragma once

#include "anfolder.h"

namespace ard 
{
    class fileref : public ard::topic
    {
    public:
        fileref();
        fileref(QString title );

        std::pair<QString, QString>     getRefFileName()const;

        cit_primitive*                  create()const override { return new fileref; }
        QString                         objName()const override;
        EOBJ                            otype()const override { return objFileRef; };
        ENoteView                       noteViewType()const override;
        QPixmap                         getIcon(OutlineContext)const override;
        ///we can't have any subitems
        bool                            canAcceptChild(cit_cptr it)const override;      
    };
};

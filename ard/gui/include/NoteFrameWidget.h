#ifndef TEXTEDIT_H
#define TEXTEDIT_H

#include <QWidget>
#include <QMap>
#include <QPointer>

#include "utils.h"
#include "ProtoScene.h"
#include "OutlinePanel.h"

class QAction;
class QComboBox;
class QFontComboBox;
class NoteEdit;
class QTextCharFormat;
class QMenu;
class NoteFrameWidget;
class PopupCard;
class OutlineView;
class OutlineSceneBase;
class TabControl;
class TitleWidget;
class DraftEmailTitleWidget;
class CardEdit;


#ifdef ARD_BIG
class NoteTextToolbar;
#endif

namespace ard 
{
    class TopicWidget;
};

class NoteFrameWidget : public QWidget
{
    Q_OBJECT
public:
    NoteFrameWidget(ard::TopicWidget* w);
  
    virtual ~NoteFrameWidget();

    void    attachTopic(topic_ptr f);
    bool    saveModified();
    void    detachGui();
    void    setFocusOnEdit();
    void    openSearchWindow();
    void    selectAll();
    void    locateFilterText();
    void    searchForText(QString s);
    int     replaceText(QString sFrom, QString sTo);
    void    formatText(const QTextCharFormat *fmt);
    void    zoomIn();
    void    zoomOut();

    CardEdit*           editor() {return m_text_edit1;}
    void                detachOwner();    

#ifdef ARD_BIG
    void                showToolbar(bool val);
#endif //ARD_BIG

public slots:
#ifdef ARD_BIG
    void textBold();
    void textUnderline();
    void textItalic();
    void textFamily(const QString &f);
    void textSize(const QString &p);
    void textStyle(int styleIndex);
    void textColor();
    void bkgColor(const QColor& );
    void bkgColorSelect();
    void textAlign(QAction *a);
    void toggleTable();
    void toggleInsertTable();
    void toggleInsertTableRow();
    void toggleInsertTableColumn();
    void toggleRemoveTableRow();
    void toggleRemoveTableColumn();
    void currentCharFormatChanged(const QTextCharFormat &format);
#endif //ARD_BIG          
  
private:
    CardEdit     *m_text_edit1{nullptr};

#ifdef  ARD_BIG
    void splitIntoTopics();
    void mergeFormatOnWordOrSelection(const QTextCharFormat &format);
    
    void fontChanged(const QFont &f);
    void colorChanged(const QColor &fgColor, const QColor &bgColor);
    void alignmentChanged(Qt::Alignment a);
    NoteTextToolbar*   m_tb{ nullptr };    
#endif

   // TitleWidget*       m_title_widget{nullptr};
    DraftEmailTitleWidget* m_email_title_edit_widget;
};

#endif

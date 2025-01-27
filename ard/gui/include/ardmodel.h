#ifndef ARDMODEL_H
#define ARDMODEL_H

#include <QPen>
#include <QBrush>
#include <QUrl>
#include <QTimer>
#include <memory>
#include "dbp.h"
#include "db-stat.h"
#include "gmail/GmailRoutes.h"

#define GTHUMB_WIDTH getGThumbWidth()

class anToolRootTopic;
class QTextDocument;
class ToolDiagram;
struct EitherLabelOrQ;

namespace ard 
{
	class topic;
    class KRingKey;
    class contact;
    class contact_group;
    class email_model;
};

struct SReqInfo
{
    EAsyncCallRequest   req{ AR_none }; 
    DB_ID_TYPE          id{0};
    DB_ID_TYPE          id2{ 0 };
    DB_ID_TYPE          id3{ 0 };
    QObject*            sceneBuilder{nullptr};
    topic_ptr           t1{ nullptr };
    topic_ptr           t2{ nullptr };
    QString             str{ "" };
};

typedef std::vector<SReqInfo> ASYNC_REQUEST_LIST;
using ASYNC_REQUEST_MAP = std::map<EAsyncCallRequest, SReqInfo>;
typedef std::map<EOutlinePolicy, EOutlinePolicy> POL_2_POL;

class OutlineScene;

class GConfigThumb
{
public:
    GConfigThumb(QPixmap pm, int cid):m_pixmap(pm), m_config_id(cid){}

    QPixmap m_pixmap;
    int     m_config_id;
};
typedef std::vector<GConfigThumb> GCONFIG_THUMB_LIST;

class ArdModel: public QObject
{
    Q_OBJECT
public:
    ArdModel();
    ~ArdModel();

    void detachModelGui();
    void attachGui();

    void            setAsyncCallRequest(EAsyncCallRequest q, DB_ID_TYPE id = 0, DB_ID_TYPE id2 = 0, DB_ID_TYPE id3 = 0, QObject* sceneBuilder = nullptr, int delay=-1);
    void            setAsyncCallRequest(EAsyncCallRequest q, QString str, DB_ID_TYPE id2 = 0, DB_ID_TYPE id3 = 0, QObject* sceneBuilder = nullptr, int delay = -1);
    void            setAsyncCallRequest(EAsyncCallRequest q, topic_ptr t1, topic_ptr t2);
    void            setDelayedAsyncCallRequest(EAsyncCallRequest q, topic_ptr t1, topic_ptr t2);
    void            processDelayedAsyncCallRequests();

    QPen&           penGray               (){return m_penGray;}
    QBrush&         brushSelectedItem     (){return m_brushSelectedItem;}
    QBrush&         brushMSelectedItem() { return m_brushMSelectedItem; }
    QBrush&         brushHotSelectedItem() { return m_brushHotSelectedItem; }
    QBrush&         brushCompletedItem     (){return m_brushCompletedItem;}
    QBrush&         brushOutlineThumb(){return m_brushOutlineThumb;}
    QPen&           penSelectedItem(){return m_penSelectedItem;} 

    void                    selectKRingForm(ard::KRingKey* k);
    ard::KRingKey*          kringInFormView() { return m_kring_in_form_view; }

    void                    selectByText(QString s);
    void                    selectTaskRing();
    void                    selectNotes();
	void                    selectBookmarks();
	void                    selectPictures();
    void                    selectAnnotated();
    void                    selectColored();
  
    topic_ptr             selectedHoistedTopic();
    void                  setSelectedHoistedTopic(topic_ptr h);
    void                  clearSelectedHoistedTopic();

    bool                  locateBySYID(QString syid, bool& dbfound);

    void                  storeSecPolicySelection(EOutlinePolicy secPolicy);
    EOutlinePolicy        getSecondaryPolicy(EOutlinePolicy mainPol);

    QTextDocument*        helperTextDocument();
    void                  debugFunction();              
    void                  onIdle();
    
    GCONFIG_THUMB_LIST&   graphSelectionList(){return m_config_thumb_list;};

    TOPICS_LIST&          getDBFilesList()const;

    QPixmap&              pixItalic(){return m_pmItalic;}
    QPixmap&              pixUnderline(){return m_pmUnderline;}
    QPixmap&              pixBold(){return m_pmBold;}

    QPixmap&              pixFontSize1(){return m_pmFontSize1;}
    QPixmap&              pixFontSize2(){return m_pmFontSize2;}
    QPixmap&              pixFontSize3(){return m_pmFontSize3;}
    QPixmap&              pixFontSize4(){return m_pmFontSize4;}

    QPixmap&              pixBulletCircle(){return m_pmBulletCircle;}
    QPixmap&              pixBulletRect(){return m_pmBulletRect;}
    QPixmap&              pixBulletNumer(){return m_pmBulletNumer;}
    QPixmap&              pixBulletAlpha(){return m_pmBulletAlpha;}

    QPixmap&              pixYesShade(){return m_pmYesShade;}
    QPixmap&              pixNoShade(){ return m_pmNoShade; }
  
    int                   fontSize2Points(int size);
    snc::SyncProgressStatus* syncProgress(){return m_syncProgress;}

    ard::email_model*           gmodel(){return m_gmail_model.get();}

	void		registerPictureWatcher(ard::picture*);
protected:  
    void                 prepareButtonsPixmaps();
    void                 prepareShadesPixmaps();
    void                 releaseDBFilesList()const;
	void				 processPictureWatchers();
    
private slots:
    void                  completeAsyncCall();
  
protected:
    QPen                 m_penGray, m_penSelectedItem;
    QBrush               m_brushSelectedItem,
        m_brushMSelectedItem,
        m_brushHotSelectedItem,
        m_brushCompletedItem,
        m_brushOutlineThumb;


    ard::KRingKey*          m_kring_in_form_view{ nullptr };
    topic_ptr				m_selHoistedTopic{nullptr};
    ASYNC_REQUEST_LIST      m_async_req_list;
    ASYNC_REQUEST_MAP       m_delayed_async_req;
	PICTURES_LIST			m_pictures_watchers;
    int						m_selIndexCardData;
    POL_2_POL				m_mainpol2secpol;
    QTextDocument*			m_helper_text_doc;
  
    OutlineView*         m_helper_gselect_view;
    OutlineScene*        m_helper_gselect_scene;
	QTimer					m_media_watcher;

#ifdef ARD_GD
    std::unique_ptr<ard::email_model> m_gmail_model;
#endif

    GCONFIG_THUMB_LIST   m_config_thumb_list;
    bool                  m_gui_detach_started{false};
    
    mutable TOPICS_LIST   m_db_files_list;

    QPixmap              m_pmItalic, m_pmUnderline, m_pmBold;
    QPixmap              m_pmFontSize1, m_pmFontSize2, m_pmFontSize3, m_pmFontSize4;
    QPixmap              m_pmBulletCircle, m_pmBulletRect, m_pmBulletNumer, m_pmBulletAlpha;
    QPixmap              m_pmYesShade, m_pmNoShade;
    snc::SyncProgressStatus* m_syncProgress;
};

extern ArdModel* model();
extern int getGThumbWidth();

#endif // ARDMODEL_H

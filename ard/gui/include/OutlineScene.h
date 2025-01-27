#include "OutlineSceneBase.h"

class OutlinePanel;

namespace ard 
{
	class email;
	class email_draft;
	class contact;
};

class OutlineScene: public OutlineSceneBase
{
  Q_OBJECT
public:
  OutlineScene(OutlineView* _view);
  QString         name()const{return "outline-scene";};

  static void     build_files(OutlineScene* s);
  static void     build_folders(OutlineScene* s);
  static void     build_rules(OutlineScene* s);
  static void     build_draft_attachements(OutlineScene* s, ard::email_draft* d);
  static void     build_email_attachements(OutlineScene* s, ard::email* m);
  static void     build_single_contact(OutlineScene* s, ard::contact* c);
  static void     build_contact_groups(OutlineScene* s);
  static void     build_contacts(OutlineScene* s, QString group_id, QString search_str);
  static void     build_gmail_accounts(OutlineScene* s);
  static void     build_from_list(OutlineScene* s, TOPICS_LIST& list2outline);

  public slots:
  void	outlineHudButtonPressed(int _id, int _data);
protected:
  void              doRebuild();
  void              rebuildAsOutline();
  void              rebuildAs2SearchView();
  void              rebuildAsTaskRing();
  void              rebuildAsNotesList();
  void              rebuildAsBookmarksList();
  void              rebuildAsPicturesList();
  void              rebuildAsAnnotatedList();
  void              rebuildAsColoredList();


  void            rebuildAsContactTableView();
  void            rebuildAsContactGroupView();
  void            rebuildAsKRingTableView();
  void            rebuildAsKRingFormView();
  void            rebuildAsBoardSelectorView();
  OutlinePanel*   prepareSelectorPanel();

  template <class O> void rebuildTopicPanel(EOutlinePolicy pol, topic_ptr root);
  template <class O> void rebuildEmailTopicPanel(EOutlinePolicy pol/*, topic_ptr root*/);
  void outlineListPanel(EOutlinePolicy pol, topic_ptr root);
};

                                           \


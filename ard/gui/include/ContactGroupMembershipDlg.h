#pragma once

#include "OutlineView.h"
#include "contact.h"

/**
    assign/remove contact to/from a group
*/
class EditContactGroupMembership : public QDialog
{
    Q_OBJECT
public:
    static void editMembership(ard::contact* c);

protected:
    EditContactGroupMembership(ard::contact* c);

public slots:
    void	outlineTmpSelectionBoxPressed(int _id, int column_number);
    void	initPostBuild();
	void	outlineHudButtonPressed(int _id, int _data);

protected:
	void	acceptWindow();
    void    rebuildContactScene();
    void    rebuildGroupScene();    
    void    storeContactMembershipSelection();

    void    rebuildScene();
    bool    onHudCmd(HudButton::E_ID);
	


protected:
    int m_ref_label_width{0};
    scene_view::ptr             m_contact_view;
    TOPICS_LIST                 m_contacts_list;
    TOPICS_LIST::iterator       m_curr_it;
	ard::contact*			m_contact{nullptr};

	OutlineSceneBase*   m_scene{ nullptr };
	OutlineView*        m_view{nullptr};
};


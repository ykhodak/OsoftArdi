#pragma once

#include "a-db-utils.h"
#include <QAction>
#include <QMenu>

namespace ard {
    namespace menu {
        enum class MCmd
        {
            archive = 1,
            synchonize,
            remove,
            move2sbox,
            move2reference,
            move2someday,
            move2folder,
            clear,
            select_all,
            select_all_from,
            select_sbox,
            select_reference,
            select_maybe,
            select_one_folder,
            select_folder,
            select_file,
            merge_notes,
            open_in_blackboard,
            format_notes,
            //open_notes,
            //exit_edit_mode,
            more,
            toggle_column,
            set_as_read,
            set_as_starred,
            unset_as_starred,
            set_as_important,
            unset_as_important,
            view_zoom_in,
            view_zoom_out,
            email_new,
            email_reply,
            email_reply_all,
            email_forward,
            //email_delete,
            email_reload,
            email_search,
            email_mark_as_read,
            email_mark_as_unread,
			email_view_attachement,
            contact_send_email,
            contact_new,
            contact_picture,
            contact_edit_group_membership,
            contact_remove_field,
            contact_edit_field,
            contact_add_email,
            contact_add_phone,
            contact_show_emails,
            view_ufolders,
            edit_ufolder,
            new_ufolder,
            kring_change_master_pwd,
            kring_create_report,
            bboard_add_band,
            bboard_remove_band,
            bboard_rename_band,
#ifdef _DEBUG
            debug1,
#endif
        };

        union UMcmdData
        {
            uint64_t d;
            struct
            {
                uint32_t c;
                uint32_t v;
            };
        };

        std::pair<MCmd, uint32_t> unpackMcmd(QAction* a);
        uint64_t packMcmd(MCmd c, uint32_t v);
       
        int buildTopicsMenu(QMenu& m, TOPICS_LIST& topics2add, TOPICS_SET& topics2skip);        
    }
};

#define ADD_MCMD(T, D) a = new QAction(T, nullptr);             \
    a->setData(static_cast<quint64>(ard::menu::packMcmd(D,0))); \
    m.addAction(a);                                             \

#define ADD_MCMD2(T, D, V) a = new QAction(T, nullptr);         \
    a->setData(static_cast<quint64>(ard::menu::packMcmd(D,V))); \
    m.addAction(a);                                             \



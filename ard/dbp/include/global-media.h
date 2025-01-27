#pragma once

/**
   mp       - original not transformed image

   all cached transformed images use Aspect Ratio 4:3
   note that default aspect ration for DSLRs 3:2 (matches sensor ratio)
   we should use mp2 images to draw pictures by default and to sync

   mp1      - thumbail cache                   100x75
   mp2      - scaled to screen cache           1600x1200  (old=1024x768)
   mp0      - original raw file
   mini-mp1 - mini thumbnail                   33X25
*/


extern QImage load_scaled_media(QString image_source_path, int scale2_w, int scale2_h);
extern QImage load_media(QString image_source_path, int quality = 100);

enum EARD_DIR
    {
        dirUknown = -1,
        dirMediaAux,
        dirMP,
        dirMP1,
        dirMP2,
		dirTMP,
		dirMedia
    };


extern QString ard_dir_curr_root();
extern QString ard_dir(EARD_DIR d);


extern bool is_image_type_supported(QString ext);

#define MP1_WIDTH            100
#define MP1_HEIGHT           75
#define MP2_WIDTH            1600
#define MP2_HEIGHT           1200
#define MP3_WIDTH            300
#define MP3_HEIGHT           225
#define MP1_MINI_WIDTH       33
#define MP1_MINI_HEIGHT      25



extern QPixmap getIcon_Pad();
extern QPixmap getIcon_Bookmark();
extern QPixmap getIcon_TopicExpanded();
extern QPixmap getIcon_TopicCollapsed();
extern QPixmap getIcon_Maybe();
extern QPixmap getIcon_Reference();
extern QPixmap getIcon_Delegated();
extern QPixmap getIcon_Recycle();
extern QPixmap getIcon_Sortbox();
extern QPixmap getIcon_Folder();
extern QPixmap getIcon_NotLoadedEmail();
extern QPixmap getIcon_InProgress();
extern QPixmap getIcon_AttachmentFile();
extern QPixmap getIcon_CheckSelect();
extern QPixmap getIcon_Url();
extern QPixmap getIcon_RulesFilter();
extern QPixmap getIcon_Drafts();
extern QPixmap getIcon_Email();
extern QPixmap getIcon_CloseBtn();
extern QPixmap getIcon_PinBtn();
extern QPixmap getIcon_TabPin();
extern QPixmap getIcon_MoveTabsBtn();
extern QPixmap getIcon_Annotation();
extern QPixmap getIcon_AnnotationWhite();
extern QPixmap getIcon_Star();
extern QPixmap getIcon_StarOff();
extern QPixmap getIcon_Important();
extern QPixmap getIcon_ImportantOff();
extern QPixmap getIcon_CheckedBox();
extern QPixmap getIcon_CheckedGrayedBox();
extern QPixmap getIcon_UnCheckedBox();
extern QPixmap getIcon_GreenCheck();
extern QPixmap getIcon_Open();
extern QPixmap getIcon_Insert();
extern QPixmap getIcon_Remove();
extern QPixmap getIcon_LinkArrow();
extern QPixmap getIcon_BoardShape();
extern QPixmap getIcon_BoardTemplate();
extern QPixmap getIcon_Rename();
extern QPixmap getIcon_Locate();
extern QPixmap getIcon_EmailLocate();
extern QPixmap getIcon_BoardTopicsFolder();
extern QPixmap getIcon_PopupUnlocked();
extern QPixmap getIcon_PopupLocked();
extern QPixmap getIcon_Search();
extern QPixmap getIcon_Find();
extern QPixmap getIcon_Details();
extern QPixmap getIcon_VDetails();
extern QPixmap getIcon_LockTabRoll();
extern QPixmap getIcon_UnlockTabRoll();
extern QPixmap getIcon_EmptyBoard();
extern QPixmap getIcon_EmptyPicture();
extern QPixmap getIcon_MailBoard();
extern QPixmap getIcon_SelectorBoard();
extern QPixmap getIcon_Copy();
extern QPixmap getIcon_Paste();
extern QPixmap getIcon_EmailAsRead();
extern QPixmap getIcon_RuleFilter();
extern QPixmap getIcon_SearchGlass();
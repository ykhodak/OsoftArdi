#pragma once


enum EPersBatchProcess  
    {
        persSubItems  = 1,
        persExtension = 2,
        persAll       = persSubItems | persExtension
    };

///enField - used in identity check
///@todo: we might need isAtomicIdenticalTo functions for extention objects as well
enum enField
    {
        flSyid        = 1 << 0,
        flOtype       = 1 << 1,
        flPSyid       = 1 << 2,
        flTitle       = 1 << 3,
        flFont        = 1 << 4,
        flToDo        = 1 << 5,
        flToDoDone    = 1 << 6,
        flHotSpot     = 1 << 7,
        flRetired     = 1 << 8,
        flSourceRef   = 1 << 9,
        flFileName    = 1 << 10,
        flContent     = 1 << 11,
        flPindex      = 1 << 12,
        flAnnotation  = 1 << 13,
        flCloudId     = 1 << 14,
       // flUploadTime  = 1 << 15,
        flAllocation  = 1 << 16,
        //flCloudIdType = 1 << 17,
        //flDuration    = 1 << 18,
       // flCost        = 1 << 19
    };

enum EOBJ
    {
        objUnknown          = -1,
        objFolder           = 1,
        objEmail            = 2,
        objEmailDraft       = 3,
        objEThread          = 4,
        objPicture          = 5,
        objDataRoot         = 7,
        objToolItem         = 9,
        objEmailLabel       = 10,
		objMailBoard		= 11,
		objFoldersBoard		= 12,
        objPrjResource      = 16,
		objQFilter			= 17,
        objFileRef          = 18,
        objUrl              = 19,
		objQFilterRoot		= 21,
        objEThreadsRoot     = 22,
        objContact          = 23,
        objContactGroup     = 24,
        objContactRoot      = 25,
        objContactGroupRoot = 26,
        objLabelRoot        = 27,
        objKRingRoot        = 28,
        objKRingKey         = 29,
        objBoardRoot        = 30,
        objBoard            = 31,
        objBoardItem        = 32
    };

#define SINGLETON_FOLDERS_COUNT 15

/**
   ESingletonTopic - type of signleton topic,
   if 'none' the topic is not signleton.
   Root topics are signleton, but not only..
*/
enum class ESingletonType
    {
        none,
        dataRoot,
        //resourceRoot,
        etheadsRoot,
        UFoldersHolder,
        qfiltersHolder,
        gtdSortbox,
        gtdInkubator,
        gtdReference,
        gtdDelegated,
        gtdRecycle,
        gtdDrafts,
        contactsHolder,
        contactGroupsHolder,
        labelsHolder,
        keysHolder,
        boardsHolder,
        boardTopicsHolder,
    };


namespace snc
{
    union SYNC_FLAGS
    {
        uint8_t flag;
        struct
        {
            unsigned sync_modified_processed              : 1;
            unsigned sync_moved_processed                 : 1;
            unsigned sync_delete_requested                : 1;
            unsigned sync_ambiguous_mod                   : 1;
            unsigned need_persistance_ATOMIC_CONTENT      : 1;
            unsigned need_persistance_PINDEX              : 1;
            unsigned need_persistance_SYNC_INFO           : 1;
            unsigned need_persistance_POS                 : 1;
        };
    };

    enum ENEED_PERSISTANCE
        {
            np_Uknown = -1,
            np_ALL,
            np_ATOMIC_CONTENT,
            np_PINDEX,
            np_SYNC_INFO,
            np_POS
        };

    enum SyncPointID
        {
            syncpUknown = -1,
            syncpDevice  = 0,
            syncpLocal  = 1,
            syncpGDrive = 2,
            syncpDBox = 3,
        };
  
    enum ESyncUpdateType
        {
            sncupdUknown = -1,
            sncupdCopyLocal2Remote,
            sncupdDelLocal,
            sncupdMoveLocal,
            sncupdMoveRemote
        };


    enum class EOBJ_EXT
        {
            extUnknown          = -1,
            extRingKey          = 3,
            extResourceDetail   = 4,
            extCryptoNote       = 5,
            extEmailDraftDetail = 6,
            extPicture          = 7,
            extQ                = 8,
            extLabel            = 9,
            extThreadOfEmails   = 10,
            extContact          = 11,
            extContactGroup     = 12,
            extBoard            = 13,
            extBoardItem        = 14,
            extNote             = 15
        };


    /// SYNC_PROCESS_FLAG  - used to mark item during sync process
    /// to display progress/status map of items synced in two DBs.
    /// the flatIndex in index in vectore made out of projected tree 
    /// during displaying sync progress we also merge two synced DB
    /// based on SYID (so items with same SYID will be merged into one)
    union SYNC_PROCESS_FLAG 
    {
        uint32_t flag;
        struct 
        {
            unsigned flatIndex : 24;
            unsigned modLocal   : 1;
            unsigned modRemote  : 1;
            unsigned delLocal   : 1;
            unsigned delRemote  : 1;
            unsigned error      : 1;
        };
    };
};

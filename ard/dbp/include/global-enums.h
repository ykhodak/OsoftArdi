#pragma once

#ifdef Q_OS_WIN
#include <cstdint>
#endif

#include <QColor>
#include "global-def.h"
#include "snc.h"

namespace ard 
{
	class topic;
};

enum class EFolderType /// persistant enum, don't change values
{
    folderUnknown = -1, folderGeneric = 0,
    folderRecycle = 1, 
    folderMaybe = 3, folderReference = 4,
    folderDelegated = 5, folderSortbox = 6,
    folderUserSortersHolder = 7,
    folderUserSorter = 8,
    folderBacklog = 9,
    folderBoardTopicsHolder = 11
};

/**
    EColumnType - each topic can maintain fields (columns) with enum id,
    each field can be compound, for example name consists of first & last
*/
enum class EColumnType
{
    Uknown,
    Selection,
    Title,
    ContactTitle, ContactEmail, ContactPhone, ContactAddress, ContactOrganization, ContactNotes,
    FormFieldName, FormFieldValue,
    KRingTitle, KRingLogin, KRingPwd, KRingLink, KRingNotes,
    Annotation
};
using COLUMN_LIST = std::vector<EColumnType>;

extern QString columntype2label(EColumnType t, bool shortLabel = true);

/**
    FieldParts - fields(or columns in table) can be compound,
    like address - is structure of records
*/
class FieldParts 
{
public:
    enum EType
    {
        Uknown,
        Title, Notes, FirstName, LastName,
        Email,
        Phone,
        AddrStreet, AddrCity, AddrRegion, AddrZip, AddrCountry,
        OrganizationName, OrganizationTitle,
        Annotation,
        KRingTitle, KRingLogin, KRingPwd, KRingLink, KRingNotes
    };

    using FILD_PARTS = std::vector<std::pair<EType, QString>>;

    const FILD_PARTS& parts()const { return m_parts; }
    FILD_PARTS& parts(){ return m_parts; }
    void add(EType t, QString str);
    bool isEmptyData()const;
    static QString type2label(EType);
protected:
    FILD_PARTS m_parts;
};

enum class InPlaceEditorStyle 
{
    regular,
    maximized
};

enum EHitTest
    {
        hitUnknown = -1,
        hitTitle,
        hitMainIcon,
        hitSecondaryIcon,
		hitRIcon,
        hitColorHash,
        hitHotSpot,
        hitAnnotation,
        hitExpandedNote,
        hitToDo,
        hitActionBtn,
        hitTernaryIconArea,
        hitCheckSelect,
        hitFatFingerSelect,
        hitFatFingerDetails,
        hitAfterTitle,
        hitTableColumn,
        hitGanttBar,
        hitGanttBarRightBorder,
        hitResourceLabel,
        hitBox,
        hitUrl
    };

enum class ECurrentMarkType
{
    typeUknown = -1,
    typeOutlineCurr
};


enum EAsyncCallRequest
{
    AR_none = 0,
    AR_rebuildOutline,
    AR_freePanelsAndRebuildOutline,
    AR_selectHoisted,
    AR_check4NewEmail,
    AR_insertBBoard,
    AR_ViewTaskRing,
    AR_ViewProperties,
    AR_insertCustomFolder,
    AR_synchronize,
	AR_DataSynchronized,
    AR_BuilderDelegate,
    AR_NoteLoaded,
	AR_PictureLoaded,
    AR_SelectGmailUser,
    AR_ReconnectGmailUser,
    AR_OnChangedGmailUser,
    AR_OnConnectedGmail,
    AR_OnDisconnectedGmail,
    AR_RuleDataReady,
    AR_GoogleRevokeToken,
    AR_GoogleConditianalyGetNewTokenAndConnect,/// reauthorize if no token found, this is needed when user hit 'connect' in the middle of authorization
    AR_GmailErrorDetected,
    AR_UpdateGItem,
    AR_RenameTopic,
    AR_RebuildLocusTab,
    AR_SetKeyRingPwd,
    AR_MovePopupToFolder,
    AR_MovePopupSelectDestination,
    AR_ToggleKRingLock,
    AR_BoardApplyShape,
    AR_BoardCreateTopic,
    AR_BoardRebuild,
    AR_BoardRebuildForRefTopic,
    AR_BoardInsertTemplate,
    AR_ImportSuppliedOutline,
    AR_RebuildGuiOnResolvedCloudCache,
    AR_ShowMessage,
	AR_FindText///<<<replace it with signal
};


union UObjAttrib{ /// can't add any more bits into struct, don't know why
    uint64_t flags;
    struct{
        //-------------- non-persistent ----------------------
		unsigned        lastOutlinePanelWidth		: 10;
		unsigned        lastOutlineHeight			: 8;
		unsigned		isThumbDirty				: 1;       
        unsigned        colorHashIndex				: 4;
        unsigned        colorHashChar				: 8;
        unsigned        optNoteLoadedFromDB			: 1;
        unsigned        isTmpSelected				: 1;
        unsigned        mergeImportSkipMark         : 1;
        //unsigned        isVisibleItemsCalculated    : 1;
        unsigned        isExpanded                  : 1;
		//-------------- serializable ----------------------
		unsigned        FolderType : 4;
		unsigned        ToDo : 4;
		unsigned        ToDoIsDone : 7;
		unsigned		isRetired : 1;
		unsigned		inLocus : 1;
		unsigned		Color : 3;
    };
};

enum EDragOver
    {
        dragUnknown = -1,
        dragAbove,
        dragBelow,
        dragInside,
        dragOverTheEdge
    };

enum class OutlineContext 
{
    none,
    normal,
    check2select,
    grep2list
};

enum EOutlinePolicy
    {
        outline_policy_Uknown     = -1,
        outline_policy_Pad    = 1,
        outline_policy_PadEmail,
        outline_policy_TaskRing,
        outline_policy_Notes,
		outline_policy_Bookmarks,
		outline_policy_Pictures,
        outline_policy_Annotated,
        outline_policy_Colored,
        outline_policy_2SearchView,                
        outline_policy_KRingTable,        
        outline_policy_KRingForm,
        outline_policy_BoardSelector       //Select bboard
    };

enum class ENoteView
    {
        None,
        Edit,
        View,
        EditEmail,
		Picture,
        SelectorBoard,
		MailBoard,
		FoldersBoard,
		TDAView
    };


enum class ToDoPriority 
{
    unknown     = -1,
    notAToDo    = 0,
    normal      = 1,
    important   = 2,
    critical    = 3
};


enum class CloudIdType 
{
    Invalid     = 0,
    GDriveId    = 1,
    LocalDbId   = 2
};

enum class ImageSpaceType
{
    Original = 0,
    Raw      = 1,
    Cropped  = 2,
    Thumbnail= 3
};

enum EDB_MOD
    {
        dbmodUnknown = -1,
        dbmodTitle,
        dbmodToDo,
        dbmodRetired,
        dbmodAnnotation,
        dbmodForceModified,
        dbmodMoved
    };

enum EDestinationFolderType
    {
        destinationUnknown = -1,
        destinationGenericFolder
    };

enum ESearchScope
    {
        search_scopeNone,
        search_scopeTopic,
        search_scopeProject,
        search_scopeGtdFolder,
        search_scopeOpenDatabase
    };


enum SyncAuxCommand
    {
        scmdNONE = 0,
        scmdPrintDataOnHashError,//print data table in case of hash-error
        scmdMAX_ENUM_VALUE
    };
typedef std::vector<SyncAuxCommand> SYNC_AUX_COMMANDS;

enum ETimeConstraint
    {
        tconstraintASAP = 1,
        tconstraintSNET = 2,
        tconstraintMSO  = 3,
    };

enum class ECurrentCommand
{
    cmdSelect,
    cmdSelectMoveTarget,
    cmdEmptyRecycle,
    cmdDownload,
    cmdDelete,
    cmdOpen,
    cmdFindInShell,
    cmdEdit
};

using COMMANDS_SET = std::set<ECurrentCommand>;

enum class TernaryIconType 
{
    none,
    //projectStatus,
    gmailFolderStatus
};

enum class YesNoConfirm
{
    yes,
    no,
    cancel,
	yes_with_option
};

enum class Next2Item 
{
    left,
    right,
    up,
    down
};

enum class OutlineSample 
{
    GreekAlphabet,
    OS,
    Programming,
	Stocks
};

enum class BoardSample
{
    None,
    Greek,
    OS,
    Programming,
    Gtd,
    Greek2Autotest,
    ProductionControl,
	Stocks
};

namespace ard 
{
	enum class EColor 
	{   /// values hardcoded - serializable
		none	= 0,
		red		= 1,
		green	= 2,
		blue	= 3,
		purple  = 4
	};

	QRgb	cidx2color(EColor c);
	QString cidx2color_str(EColor c);
	char	cidx2char(EColor c);
};

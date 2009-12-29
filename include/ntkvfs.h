#ifndef _NTKVFS_H__
#define _NTKVFS_H__

#include "lklddk.h"
#include "util.h"

#define DRIVER_NAME "lklvfs"
#define LKL_FS_NAME  L"\\" DRIVER_NAME
#define LKL_DOS_DEV  L"\\DosDevices\\" DRIVER_NAME
const static UNICODE_STRING lkl_fs_name = RTL_CONSTANT_STRING(LKL_FS_NAME);
const static UNICODE_STRING lkl_dos_dev = RTL_CONSTANT_STRING(LKL_DOS_DEV);


#define IS_SET(x, f)        ((BOOLEAN) ((x) &   (f)))
#define SET_FLAG(x, f)                 ((x) |=  (f))
#define CLR_FLAG(x, f)                 ((x) &= ~(f))



/* structure describing the LKL NTK driver */
typedef struct _LKLVFS {
	PDRIVER_OBJECT driver;
	PDEVICE_OBJECT device;
	VCB_LIST vcb_list;
	NPAGED_LOOKASIDE_LIST       IrpContextLL;
} LKLVFS;

extern LKLVFS *g_lklvfs;

DDKAPI NTSTATUS LklVfsBuildRequest(PDEVICE_OBJECT device, PIRP Irp);



//
// LKLVFS Driver Definitions
//

//
// LKL_IDENTIFIER_TYPE
//
// Identifiers used to mark the structures
//
typedef struct _LKL_MCB  LKL_MCB, *PLKL_MCB;

typedef enum _LKL_IDENTIFIER_TYPE {
#ifdef _MSC_VER
	FGD  = ':DGF',
	VCB  = ':BCV',
	FCB  = ':BCF',
	CCB  = ':BCC',
	ICX  = ':XCI',
	FSD  = ':DSF',
	MCB  = ':BCM',
#else
	FGD  = 0xE2FD0001,
	VCB  = 0xE2FD0002,
	FCB  = 0xE2FD0003,
	CCB  = 0xE2FD0004,
	ICX  = 0xE2FD0005,
	FSD  = 0xE2FD0006,
	MCB  = 0xE2FD0007,
#endif
} LKL_IDENTIFIER_TYPE;



//
// LKL_IDENTIFIER
//
// Header used to mark the structures
//
typedef struct _LKL_IDENTIFIER {
	LKL_IDENTIFIER_TYPE Type;
	ULONG               Size;
} LKL_IDENTIFIER, *PLKL_IDENTIFIER;



typedef struct _LKL_FCBVCB {
	// Command header for Vcb and Fcb
	FSRTL_ADVANCED_FCB_HEADER   Header;
	LKL_IDENTIFIER              Identifier;

	// Locking resources
	ERESOURCE                   MainResource;
	ERESOURCE                   PagingIoResource;
} LKL_FCBVCB, *PLKL_FCBVCB;



//
// LKL_FCB File Control Block
//
// Data that represents an open file
// There is a single instance of the FCB for every open file
//
typedef struct _LKL_FCB {
	// FCB header required by NT
	FSRTL_ADVANCED_FCB_HEADER	   Header;

	// Ext2Fsd identifier
	LKL_IDENTIFIER				 Identifier;
	PLKL_MCB					   Mcb;

} LKL_FCB, *PLKL_FCB;



//
// Mcb Node
//

struct _LKL_MCB {
	// Identifier for this structure
	LKL_IDENTIFIER				 Identifier;

	// Full name with path
	UNICODE_STRING				  FullName;

};

//
// LKL_CCB Context Control Block
//
// Data that represents one instance of an open file
// There is one instance of the CCB for every instance of an open file
//
typedef struct _LKL_CCB {

	// Identifier for this structure
	LKL_IDENTIFIER  Identifier;

	// Flags
	ULONG			 Flags;

	// Mcb of it's symbol link
	PLKL_MCB		 SymLink;

	// State that may need to be maintained
	ULONG			 CurrentByteOffset;
	UNICODE_STRING	DirectorySearchPattern;

} LKL_CCB, *PLKL_CCB;


enum IRP_CONTEXT_FLAG_T {
	IRP_CONTEXT_FLAG_FROM_POOL	= (0x00000001),
	IRP_CONTEXT_FLAG_WAIT		= (0x00000002),
	IRP_CONTEXT_FLAG_WRITE_THROUGH  = (0x00000004),
	IRP_CONTEXT_FLAG_FLOPPY		= (0x00000008),
	IRP_CONTEXT_FLAG_DISABLE_POPUPS	= (0x00000020),
	IRP_CONTEXT_FLAG_DEFERRED	= (0x00000040),
	IRP_CONTEXT_FLAG_VERIFY_READ	= (0x00000080),
	IRP_CONTEXT_STACK_IO_CONTEXT	= (0x00000100),
	IRP_CONTEXT_FLAG_REQUEUED	= (0x00000200),
	IRP_CONTEXT_FLAG_USER_IO	= (0x00000400),
	IRP_CONTEXT_FLAG_DELAY_CLOSE	= (0x00000800),
	IRP_CONTEXT_FLAG_FILE_BUSY	= (0x00001000),
};


//
// LKL_IRP_CONTEXT
//
// Used to pass information about a request between the drivers functions
//
typedef struct _LKL_IRP_CONTEXT {

	// Identifier for this structure
	LKL_IDENTIFIER             Identifier;

	// Pointer to the IRP this request describes
	PIRP                       Irp;

	// Flags
	enum IRP_CONTEXT_FLAG_T    Flags;

	// The major and minor function code for the request
	UCHAR                      MajorFunction;
	UCHAR                      MinorFunction;

	// The device object
	PDEVICE_OBJECT             DeviceObject;

	// The real device object
	PDEVICE_OBJECT             RealDevice;

	// The file object
	PFILE_OBJECT               FileObject;

	PLKL_FCB                   Fcb;
	PLKL_CCB                   Ccb;

	// If the request is top level
	BOOLEAN	                   IsTopLevel;

	// Used if the request needs to be queued for later processing
	WORK_QUEUE_ITEM	           WorkQueueItem;

	// If an exception is currently in progress
	BOOLEAN                    ExceptionInProgress;

	// The exception code when an exception is in progress
	NTSTATUS                   ExceptionCode;

} LKL_IRP_CONTEXT, *PLKL_IRP_CONTEXT;

static inline int LklCanIWait(PLKL_IRP_CONTEXT IrpContext)
{
	return IS_SET(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT);
}



/* debug levels */
#define DL_VIT 0
#define DL_ERR 1
#define DL_USR 2
#define DL_DBG 3
#define DL_INF 4
#define DL_FUN 5
#define DL_LOW 6


#define DL_DEFAULT  DL_USR
#define DL_REN      DL_ERR	   /* renaming operation */
#define DL_RES      DL_DBG	   /* entry reference managment */
#define DL_BLK      DL_LOW	   /* data block allocation / free */
#define DL_CP       DL_LOW	   /* code pages (create, querydir) */
#define DL_EXT      (DL_LOW + 1)   /* extents */
#define DL_MAP      DL_EXT	   /* retrieval points */

extern ULONG DebugLevel;
#define DEBUG(_DL, arg) do {if (_DL <= DL_ERR) DbgPrint arg;} while(0)

PCHAR NtStatusToString(IN NTSTATUS Status );
VOID DbgPrintIrpComplete(IN PIRP Irp, IN BOOLEAN bPrint);
VOID DbgPrintIrpCall(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp, IN PWCHAR FileName);
VOID LklDbgPrintIrpCall(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);

PLKL_IRP_CONTEXT LklAllocateIrpContext(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
VOID LklFreeIrpContext(IN PLKL_IRP_CONTEXT IrpContext);
VOID LklCompleteIrpContext(IN PLKL_IRP_CONTEXT IrpContext, IN NTSTATUS status);


#endif //_NTKVFS_H__


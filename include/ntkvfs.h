#ifndef _NTKVFS_H__
#define _NTKVFS_H__

#include "lklddk.h"
#include "util.h"

#define DRIVER_NAME "lklvfs"
#define LKL_FS_NAME  L"\\" DRIVER_NAME
#define LKL_DOS_DEV  L"\\DosDevices\\" DRIVER_NAME
const static UNICODE_STRING lkl_fs_name = RTL_CONSTANT_STRING(LKL_FS_NAME);
const static UNICODE_STRING lkl_dos_dev = RTL_CONSTANT_STRING(LKL_DOS_DEV);


/* structure describing the LKL NTK driver */
typedef struct _LKLVFS {
	PDRIVER_OBJECT driver;
	PDEVICE_OBJECT device;
	VCB_LIST vcb_list;
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

PCHAR NtStatusToString(IN NTSTATUS Status );
VOID DbgPrintIrpComplete(IN PIRP Irp, IN BOOLEAN bPrint);
VOID DbgPrintIrpCall(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp, IN PWCHAR FileName);
VOID LklDbgPrintIrpCall(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);


#endif //_NTKVFS_H__


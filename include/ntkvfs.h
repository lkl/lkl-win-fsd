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

#endif //_NTKVFS_H__


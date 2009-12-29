#ifndef _LKLVFS_UTIL_H__
#define _LKLVFS_UTIL_H__

#include "lklddk.h"

typedef struct _VCB_LIST {
	LIST_ENTRY list;
	ERESOURCE rwlock;
} VCB_LIST;
struct _LKL_VCB;

NTSTATUS VcbListInit(VCB_LIST *l);
VOID VcbListAdd(VCB_LIST *l, struct _LKL_VCB *vcb);
VOID VcbListDel(VCB_LIST *l, struct _LKL_VCB *vcb);
NTSTATUS VcbListFini(VCB_LIST *l);

VOID Stop(VOID);


/* get the name of the process on behalf of which we're running */
extern ULONG ProcessNameOffset;
ULONG GetProcessNameOffset(VOID);
PCHAR GetCurrentProcessName();


/* string representations for some common file system related constants */
PCHAR IrpMjToString(unsigned int n);
PCHAR FileInformationClassToString(unsigned int n);
PCHAR FsInformationClassToString(unsigned int n);
PCHAR NtStatusToString(IN NTSTATUS Status);

#endif//_LKLVFS_UTIL_H__

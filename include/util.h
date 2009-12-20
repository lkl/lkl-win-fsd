#ifndef _LKLVFS_UTIL_H__
#define _LKLVFS_UTIL_H__

#include "lklddk.h"

typedef struct _VCB_LIST {
	LIST_ENTRY list;
	ERESOURCE rwlock;
} VCB_LIST;
struct _LKL_VCB;

NTSTATUS VcbListInit(VCB_LIST *l);
NTSTATUS VcbListFini(VCB_LIST *l);

VOID Stop(VOID);

#endif//_LKLVFS_UTIL_H__

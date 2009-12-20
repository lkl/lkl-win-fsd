#include "ntkvfs.h"
#include "util.h"

NTSTATUS VcbListInit(VCB_LIST *l)
{
	InitializeListHead(&l->list);
	return ExInitializeResourceLite(&l->rwlock);
}

NTSTATUS VcbListFini(VCB_LIST *l)
{
	ASSERT(IsListEmpty(&l->vcb_list));
	return ExDeleteResourceLite(&l->rwlock);
}

/*
 * Deadlocks the current thread
 *
 * This is usefull in situations in which you want to block a thread
 * at a certain point, but don't want to use 'while(1);' which hogs
 * the CPU.
 */
VOID Stop(VOID)
{
	KSEMAPHORE stopme;
	KeInitializeSemaphore(&stopme, 0, 10);
	KeWaitForSingleObject(&stopme, Executive, KernelMode, FALSE, NULL);
}

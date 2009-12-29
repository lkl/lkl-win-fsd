#include "ntkvfs.h"
#include "util.h"

NTSTATUS VcbListInit(VCB_LIST *l)
{
	InitializeListHead(&l->list);
	return ExInitializeResourceLite(&l->rwlock);
}

VOID VcbListAdd(VCB_LIST *l, struct _LKL_VCB *vcb)
{
	ExAcquireResourceExclusiveLite(&l->rwlock, TRUE);
	InsertTailList(&l->list, &vcb->next);
	ExReleaseResourceLite(&l->rwlock);
}

VOID VcbListDel(VCB_LIST *l, struct _LKL_VCB *vcb)
{
	ExAcquireResourceExclusiveLite(&l->rwlock, TRUE);
	RemoveEntryList(&vcb->next);
	ExReleaseResourceLite(&l->rwlock);
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






/*
 * The PEB structure contains the name of the process on behalf of
 * whom we're executing. This structure was formally undocumented,
 * now it's documented but subject to changes between different
 * Windows kernel versions.
 *
 * The offset at which the process image name is stored my differ
 * between kernel versions/processor architecture, but for a running
 * system it's constant.
 *
 * We know that the image name of the process on behalf of which
 * DriverEntry() is executed is "System". So, in DriverEntry() we scan
 * the first <<PAGE_SIZE-strlen("System")>> bytes for this string and
 * note the offset at which it is found.
 *
 * Note: we only check at most a page of data because we don't have
 * the guarantee (or at least I'm not aware that we have it:) that the
 * next virtual page of memory is mapped in and that we have read
 * access on it.
 */

/*
 * Store the offset in the image name field in the PEB structure
 */
ULONG ProcessNameOffset;

/*
 * Determine the system process' image name offset in the PEB
 * structure. The offset is the same in all PEB structures (it's a C
 * struct after all) and can be used to find the names of other
 * processes.
 *
 * NOTE: this must be called from the context of the "System" process.
 * One such context is the DriverEntry() function of a driver.
 */
ULONG GetProcessNameOffset(VOID)
{
    PEPROCESS Process;
    ULONG i;
    const PCHAR SYSTEM_PROCESS_NAME = "System";
    const int len = strlen(SYSTEM_PROCESS_NAME);

    Process = PsGetCurrentProcess();

    for(i = 0; i < PAGE_SIZE-len; i++)
	    if(!strncmp(SYSTEM_PROCESS_NAME, (PCHAR) Process + i, len))
		    return i;

    DbgPrint("*** GetProcessNameOffset failed. You must call it"
	     " from DriverEntry() or some other System context***\n");
    return 0;
}

/*
 * Get the name of the process on behalf of whom we're running.
 *
 * NOTE: This uses undocumented properties of the PEB struct and may not
 * work on all versions of Windows.
 *
 * NOTE: You must call GetProcessNameOffset() from "System" context
 * before calling this.
 */
PCHAR GetCurrentProcessName()
{
	if (ProcessNameOffset == 0)
		return "*** GetCurrentProcessName() failed. Call"
			" GetProcessNameOffset() from DriverEntry() first!***";

	return ((PCHAR) PsGetCurrentProcess()) + ProcessNameOffset;
}

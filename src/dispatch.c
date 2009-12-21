#include "lklddk.h"
#include "ntkvfs.h"


/*
 * Tell the caller that we have completed all processing for this
 * given I/O request and that we're returning the IRP to the I/O
 * manager.
 */
VOID LklCompleteIrpRequest(IN PIRP Irp, IN BOOLEAN bPrint, IN CCHAR PriorityBoost)
{
	DbgPrintIrpComplete(Irp, bPrint);
	IoCompleteRequest(Irp, PriorityBoost);
}

/*
 * Give the Irp to the I/O manager and free the IrpContext.
 */
VOID LklCompleteIrpContext(IN PLKL_IRP_CONTEXT IrpContext, IN NTSTATUS status)
{
	PIRP    Irp = NULL;
	BOOLEAN bPrint;
	CCHAR PriorityBoost;

	Irp = IrpContext->Irp;
	if (Irp == NULL)
		goto out;

	if (NT_ERROR(status))
		Irp->IoStatus.Information = 0;

	Irp->IoStatus.Status = status;
	bPrint = !IS_SET(IrpContext->Flags, IRP_CONTEXT_FLAG_REQUEUED);

	PriorityBoost = (CCHAR) (NT_SUCCESS(status)? IO_DISK_INCREMENT : IO_NO_INCREMENT);
	LklCompleteIrpRequest(Irp, bPrint, PriorityBoost);

	IrpContext->Irp = NULL;

out:
	LklFreeIrpContext(IrpContext);
}


/*
 * Call the appropriate handler for the Irp enclosed in this
 * IrpContext.  This is called with APCs disabled and a top-level Irp
 * configured (either the enclosed Irp, or some other value set up by
 * someone else - typically as a flag).
 *
 * This must make sure to either free (or plan to be freed by someone
 * else) the IrpContext received on all code paths.
 */
NTSTATUS LklDispatchRequest(IN PLKL_IRP_CONTEXT IrpContext)
{
	ASSERT((IrpContext) &&
	       (IrpContext->Identifier.Type == ICX) &&
	       (IrpContext->Identifier.Size == sizeof(LKL_IRP_CONTEXT)));

	switch (IrpContext->MajorFunction) {
	case IRP_MJ_FILE_SYSTEM_CONTROL:
		return LklFileSystemControl(IrpContext);
	default:
		DEBUG(DL_ERR, ("LklDispatchRequest: Unexpected major function: %xh\n",
			       IrpContext->MajorFunction));

		LklCompleteIrpContext(IrpContext, STATUS_DRIVER_INTERNAL_ERROR);
		return STATUS_DRIVER_INTERNAL_ERROR;
	}
}



/*
 * Do some common processing needed for all file system handler
 * routines and then dispatch to the real handlers.
 */
DDKAPI NTSTATUS LklVfsBuildRequest(IN PDEVICE_OBJECT device, IN PIRP Irp)
{

	BOOLEAN	           AtIrqlPassiveLevel = FALSE;
	BOOLEAN	           IsTopLevelIrp = FALSE;
	PLKL_IRP_CONTEXT   IrpContext = NULL;
	NTSTATUS           status = STATUS_UNSUCCESSFUL;


	LklDbgPrintIrpCall(device, Irp);

	/*
	 * Disables the delivery of normal kernel-mode APCs.
	 *
	 * Normally this should be done just before acquiring a
	 * resource required in performing an I/O request to make sure
	 * that the routine cannot be suspended and block other I/O
	 * requests.
	 *
	 * XXX: We're blocking normal kernel-mode APCs for a longer
	 * time that we need; we should investigate whether it make
	 * sense (performance-wise) to reduce the granularity with
	 * which we disable/re-enable normal kernel-mode APCs.
	 */
	AtIrqlPassiveLevel = (KeGetCurrentIrql() == PASSIVE_LEVEL);
	if (AtIrqlPassiveLevel)
		FsRtlEnterFileSystem();


	/*
	 * IoGetTopLevelIrp returns the value of the TopLevelIrp field
	 * of the current thread. This field can only be set by file
	 * system drivers and file system filter drivers.
	 *
	 * If the current thread holds no resources above the file
	 * system, IoGetTopLevelIrp will return NULL. In that case
	 * we'll mark ourselves (the current file system) as the
	 * top-level component for the current thread,
	 */
	if (!IoGetTopLevelIrp()) {
		IsTopLevelIrp = TRUE;
		IoSetTopLevelIrp(Irp);
	}

	IrpContext = LklAllocateIrpContext(device, Irp);
	if (!IrpContext) {
		/* STATUS_NO_MEMORY would be more descriptive? */
		status = STATUS_INSUFFICIENT_RESOURCES;
		Irp->IoStatus.Status = status;
		/* we finished this call very quicly (just returned an
		 * error), so we tell the caller not to give a
		 * priority boost to the current thread */
		LklCompleteIrpRequest(Irp, TRUE, IO_NO_INCREMENT);
	} else {
		/* dispatch the appropriate handler based on the IRP
		 * major function, with APCs disabled and a top-level
		 * Irp configured.
		 *
		 * NOTE: Either LklDispatchRequest will free the
		 * IrpContext or it will make sure someone else, at a
		 * later point in time, will free it. After this call
		 * returns we can no longer touch IrpContext.
		 */
		status = LklDispatchRequest(IrpContext);
	}

	/*
	 * If we marked this as being the top-level component of the
	 * current thread, we clean that marking, as the file system
	 * will return control
	 */
	if (IsTopLevelIrp)
		IoSetTopLevelIrp(NULL);

	/* enable delivery of normal kernel mode APCs if we disabled it above */
	if (AtIrqlPassiveLevel)
		FsRtlExitFileSystem();

	return status;
}




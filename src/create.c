#include "ntkvfs.h"

NTSTATUS LklCreateVolume(IN PLKL_IRP_CONTEXT IrpContext, PLKL_VCB Vcb)
{
	return STATUS_OBJECT_NAME_NOT_FOUND;
}

NTSTATUS LklCreateFile(IN PLKL_IRP_CONTEXT IrpContext, PLKL_VCB Vcb)
{
	return STATUS_OBJECT_NAME_NOT_FOUND;
}

NTSTATUS LklCreate(IN PLKL_IRP_CONTEXT IrpContext)
{
	NTSTATUS status = STATUS_OBJECT_NAME_NOT_FOUND;
	PIRP Irp = IrpContext->Irp;
	PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
	PLKL_VCB Vcb;
	PLKL_FCBVCB FcbVcb;

	/* AFAIK the Irp's device object can be one of the devices
	 * created for each mounted volume or the global device
	 * created for the file system. */
	if (IrpContext->DeviceObject == g_lklvfs->device) {
		/* There is nothing to be done for the global device
		 * object. Userspace opens this object only to be able
		 * to send IOCTLs at a later time. Just mark the call
		 * as successful and out we go. */
		status = STATUS_SUCCESS;
		Irp->IoStatus.Information = FILE_OPENED;
		LklCompleteIrpContext(IrpContext, status);
		return status;
	}


	/* if it's not the global device object, it's one of the
	 * mounted volume objects */
	Vcb = (PLKL_VCB) IrpContext->DeviceObject->DeviceExtension;
	ASSERT(Vcb->Identifier.Type == VCB);
	ASSERT(LklVcbIsMounted(Vcb));

	if (!ExAcquireResourceExclusiveLite(&Vcb->MainResource, TRUE)) {
		/* this request cannot be performed right now. Just
		 * queue it and it will be taken care of at a later
		 * point */
		status = STATUS_PENDING;
		goto end;
	}

	//TODO: LAG: XXX
	//LklVerifyVcb(IrpContext, Vcb);

	if (IS_SET(Vcb->Flags, VCB_VOLUME_LOCKED)) {
		if (IS_SET(Vcb->Flags, VCB_DISMOUNT_PENDING))
			status = STATUS_VOLUME_DISMOUNTED;
		else
			status = STATUS_ACCESS_DENIED;
		goto release_main_res;
	}

	FcbVcb = (PLKL_FCBVCB) IrpSp->FileObject->FsContext;
	if (((IrpSp->FileObject->FileName.Length == 0) &&
	     (IrpSp->FileObject->RelatedFileObject == NULL)) ||
	    (FcbVcb && FcbVcb->Identifier.Type == VCB))
		status = LklCreateVolume(IrpContext, Vcb);
	else
		status = LklCreateFile(IrpContext, Vcb);

release_main_res:
	ExReleaseResourceLite(&Vcb->MainResource);
end:
	if ((status == STATUS_PENDING) || (status == STATUS_CANT_WAIT))
		return LklQueueRequest(IrpContext);

	LklCompleteIrpContext(IrpContext, status);
	return status;
}

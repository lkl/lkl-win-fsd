#include "ntkvfs.h"

NTSTATUS LklCreate(IN PLKL_IRP_CONTEXT IrpContext)
{
	NTSTATUS status = STATUS_OBJECT_NAME_NOT_FOUND;
	PIRP Irp = IrpContext->Irp;

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

	return status;
}

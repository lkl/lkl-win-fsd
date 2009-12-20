#include "ntkvfs.h"

PLKL_IRP_CONTEXT LklAllocateIrpContext(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
	PIO_STACK_LOCATION  IoStackLocation;
	PLKL_IRP_CONTEXT  IrpContext;
	int irp_ctx_size = sizeof(LKL_IRP_CONTEXT);

	ASSERT(DeviceObject != NULL);
	ASSERT(Irp != NULL);

	IoStackLocation = IoGetCurrentIrpStackLocation(Irp);

	IrpContext = (PLKL_IRP_CONTEXT) (ExAllocateFromNPagedLookasideList(&g_lklvfs->IrpContextLL));
	if (IrpContext == NULL)
		return NULL;

	RtlZeroMemory(IrpContext, irp_ctx_size);

	IrpContext->Identifier.Type = ICX;
	IrpContext->Identifier.Size = irp_ctx_size;

	IrpContext->Irp = Irp;
	IrpContext->MajorFunction = IoStackLocation->MajorFunction;
	IrpContext->MinorFunction = IoStackLocation->MinorFunction;
	IrpContext->DeviceObject = DeviceObject;
	IrpContext->FileObject = IoStackLocation->FileObject;

	if (IrpContext->FileObject != NULL) {
		IrpContext->RealDevice = IrpContext->FileObject->DeviceObject;
	} else if (IrpContext->MajorFunction == IRP_MJ_FILE_SYSTEM_CONTROL) {
		if (IoStackLocation->Parameters.MountVolume.Vpb) {
			IrpContext->RealDevice =
				IoStackLocation->Parameters.MountVolume.Vpb->RealDevice;
		}
	}

	switch(IrpContext->MajorFunction)
	{
	case IRP_MJ_FILE_SYSTEM_CONTROL:
	case IRP_MJ_PNP:
		if (!IoStackLocation->FileObject || IoIsOperationSynchronous(Irp))
			SET_FLAG(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT);
		break;
	case IRP_MJ_CLEANUP:
	case IRP_MJ_CLOSE:
	case IRP_MJ_SHUTDOWN:
		SET_FLAG(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT);
		break;
	default:
		if (IoIsOperationSynchronous(Irp))
			SET_FLAG(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT);
	}

	IrpContext->IsTopLevel = (IoGetTopLevelIrp() == Irp);
	IrpContext->ExceptionInProgress = FALSE;

	return IrpContext;
}

VOID LklFreeIrpContext(IN PLKL_IRP_CONTEXT IrpContext)
{
	ASSERT(IrpContext != NULL);

	ASSERT((IrpContext->Identifier.Type == ICX) &&
	       (IrpContext->Identifier.Size == sizeof(LKL_IRP_CONTEXT)));

	/* free the IrpContext to NonPagedList */
	IrpContext->Identifier.Type = 0;
	IrpContext->Identifier.Size = 0;

	ExFreeToNPagedLookasideList(&(g_lklvfs->IrpContextLL), IrpContext);
}

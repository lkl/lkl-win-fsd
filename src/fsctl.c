#include "ntkvfs.h"

static NTSTATUS LklUserFsRequest(IN PLKL_IRP_CONTEXT IrpContext)
{
	return STATUS_DRIVER_INTERNAL_ERROR;
}

static NTSTATUS LklMountVolume(IN PLKL_IRP_CONTEXT IrpContext)
{
	return STATUS_DRIVER_INTERNAL_ERROR;
}

static NTSTATUS LklVerifyVolume(IN PLKL_IRP_CONTEXT IrpContext)
{
	return STATUS_DRIVER_INTERNAL_ERROR;
}

NTSTATUS LklFileSystemControl(IN PLKL_IRP_CONTEXT IrpContext)
{
	NTSTATUS status;
	switch(IrpContext->MinorFunction) {
	case IRP_MN_USER_FS_REQUEST:
		status = LklUserFsRequest(IrpContext);
		break;
	case IRP_MN_MOUNT_VOLUME:
		status = LklMountVolume(IrpContext);
		break;
	case IRP_MN_VERIFY_VOLUME:
		status = LklVerifyVolume(IrpContext);
		break;
	case IRP_MN_LOAD_FILE_SYSTEM:
	case IRP_MN_KERNEL_CALL:
		status = STATUS_NOT_IMPLEMENTED;
		break;
	default:
		DEBUG(DL_ERR, ("LklFileSystemControl: Invalid minor function %xh.\n",
			       IrpContext->MinorFunction));
		status = STATUS_INVALID_DEVICE_REQUEST;
		break;
	}
	LklCompleteIrpContext(IrpContext, status);
	return status;
}

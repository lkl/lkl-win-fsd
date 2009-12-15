#include "lklddk.h"
#include "ntkvfs.h"

DDKAPI NTSTATUS LklVfsBuildRequest(IN PDEVICE_OBJECT device, IN PIRP Irp)
{
	return STATUS_DRIVER_INTERNAL_ERROR;
}


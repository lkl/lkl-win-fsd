#include <asm/env.h>
#include "lklddk.h"
#include "ntkvfs.h"

const int LKL_MEMORY_SIZE = 64 * 1024 * 1024;


LKLVFS * g_lklvfs = NULL;

/*
 * Free all NTK filesystem driver resources
 */
static NTSTATUS LklFileSystemDriverFini(PDRIVER_OBJECT driver)
{
	IoUnregisterFileSystem(g_lklvfs->device);
	IoDeleteSymbolicLink((PUNICODE_STRING) &lkl_dos_dev);
	ExDeleteNPagedLookasideList(&g_lklvfs->IrpContextLL);
	VcbListFini(&g_lklvfs->vcb_list);
	IoDeleteDevice(g_lklvfs->device);
	ExFreePoolWithTag(g_lklvfs, 'LKLG');
	return STATUS_SUCCESS;
}

/*
 * Called by the system to unload the driver, flush it's data,
 * terminate all pending operations and free all acquired resources.
 * @driver - the NTK driver object for this driver
 */
VOID DDKAPI DriverUnload(PDRIVER_OBJECT driver)
{
	DbgPrint("drvpoc: unload compiled at " __TIME__ " on " __DATE__);
	LklFileSystemDriverFini(driver);
	lkl_env_fini();
	DbgPrint("drvpoc: unload complete");
}


VOID InitializeFunctionPointers(PDRIVER_OBJECT driver)
{
	int i, len;
	int idx[] = {
		/* functions that MUST be supported */
		IRP_MJ_CREATE,
		IRP_MJ_CLOSE,
		IRP_MJ_READ,
		IRP_MJ_WRITE,
		IRP_MJ_DEVICE_CONTROL,
		IRP_MJ_DIRECTORY_CONTROL,
		IRP_MJ_FILE_SYSTEM_CONTROL,
		IRP_MJ_QUERY_INFORMATION,
		IRP_MJ_QUERY_VOLUME_INFORMATION,
		IRP_MJ_SET_INFORMATION,
		IRP_MJ_CLEANUP,

		/* these functions are optional */
		IRP_MJ_FLUSH_BUFFERS,
		IRP_MJ_SHUTDOWN,
		IRP_MJ_SET_VOLUME_INFORMATION,
		IRP_MJ_LOCK_CONTROL,
		IRP_MJ_QUERY_SECURITY,
		IRP_MJ_SET_SECURITY,
		IRP_MJ_QUERY_EA,
		IRP_MJ_SET_EA,

#if (_WIN32_WINNT >= 0x0500)
		IRP_MJ_PNP,
#endif
	};

	/* all calls go through the same build request method */
	len = sizeof(idx)/sizeof(idx[0]);
	for (i = 0; i < len; i++)
		driver->MajorFunction[idx[i]] = LklVfsBuildRequest;
}

static NTSTATUS LklFileSystemDriverInit(PDRIVER_OBJECT driver, PUNICODE_STRING registry)
{
	NTSTATUS status = STATUS_SUCCESS;

	/* allocate memory for the global LKLVFS descriptor */
	g_lklvfs = ExAllocatePoolWithTag(NonPagedPool, sizeof(LKLVFS), 'LKLG');
	if (g_lklvfs == NULL)
		return STATUS_INSUFFICIENT_RESOURCES;
	RtlZeroMemory(g_lklvfs, sizeof(LKLVFS));
	g_lklvfs->driver = driver;

	/* create a file system device */
	status = IoCreateDevice(driver, 0, (PUNICODE_STRING) &lkl_fs_name,
				FILE_DEVICE_DISK_FILE_SYSTEM,
				0, FALSE, &(g_lklvfs->device));
	if (status != STATUS_SUCCESS)
		goto free_g_lklvfs;

	/* initialize the list of mounted volumes */
	status = VcbListInit(&g_lklvfs->vcb_list);
	if (status != STATUS_SUCCESS)
		goto free_device;

	ExInitializeNPagedLookasideList(&(g_lklvfs->IrpContextLL), NULL, NULL, 0,
					sizeof(LKL_IRP_CONTEXT), 'PRIE', 0);


	/* create visible link to the fs device for unloading */
	status = IoCreateSymbolicLink((PUNICODE_STRING) &lkl_dos_dev,
				      (PUNICODE_STRING) &lkl_fs_name);
	if (status != STATUS_SUCCESS)
		goto free_vcb_list;

	InitializeFunctionPointers(driver);
	IoRegisterFileSystem(g_lklvfs->device);

	return status;

free_vcb_list:
	VcbListFini(&g_lklvfs->vcb_list);
free_device:
	IoDeleteDevice(g_lklvfs->device);
free_g_lklvfs:
	ExFreePoolWithTag(g_lklvfs, 'LKLG');
	return status;
}

/*
 * Called by the system to initalize the driver
 * This will initialize LKL and setup the NTK filesystem driver routines.
 * @driver - the NTK driver object for this driver
 * @registry - the path to this driver's registry key
 */
NTSTATUS DDKAPI DriverEntry (PDRIVER_OBJECT driver, PUNICODE_STRING registry)
{
	NTSTATUS status = STATUS_SUCCESS;
	int rc;
	DbgPrint("drvpoc: entry compiled at " __TIME__ " on " __DATE__);
	rc = lkl_env_init(LKL_MEMORY_SIZE);
	if (rc != 0) {
		status = STATUS_UNSUCCESSFUL;
		goto error_out;
	}

	ProcessNameOffset = GetProcessNameOffset();

	status = LklFileSystemDriverInit(driver, registry);
	if (status != STATUS_SUCCESS)
		goto unload_lkl;

	driver->DriverUnload = DriverUnload;
	return STATUS_SUCCESS;

unload_lkl:
	lkl_env_fini();
error_out:
	return status;
}

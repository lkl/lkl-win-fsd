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
	IoDeleteSymbolicLink((PUNICODE_STRING) &lkl_dos_dev);
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

	/* create visible link to the fs device for unloading */
	status = IoCreateSymbolicLink((PUNICODE_STRING) &lkl_dos_dev,
				      (PUNICODE_STRING) &lkl_fs_name);
	if (status != STATUS_SUCCESS)
		goto free_vcb_list;

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

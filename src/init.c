#include <ddk/ntddk.h>
#include <asm/env.h>

const int LKL_MEMORY_SIZE = 64 * 1024 * 1024;
extern int lkl_env_fini(void);

static void stop(void)
{
	KSEMAPHORE stopme;
	KeInitializeSemaphore(&stopme, 0, 10);
	KeWaitForSingleObject(&stopme, Executive, KernelMode, FALSE, NULL);
}

/*
 * Called by the system to unload the driver, flush it's data,
 * terminate all pending operations and free all acquired resources.
 * @driver - the NTK driver object for this driver
 */
void DDKAPI DriverUnload (PDRIVER_OBJECT driver)
{
	DbgPrint("drvpoc: unload compiled at " __TIME__ " on " __DATE__);
	lkl_env_fini();
	DbgPrint("drvpoc: unload complete");
}

/*
 * Called by the system to initalize the driver
 * This will initialize LKL and setup the NTK filesystem driver routines.
 * @driver - the NTK driver object for this driver
 * @registry - the path to this driver's registry key
 */
NTSTATUS DDKAPI DriverEntry (PDRIVER_OBJECT driver, PUNICODE_STRING registry)
{
	DbgPrint("drvpoc: entry compiled at " __TIME__ " on " __DATE__);
	driver->DriverUnload = DriverUnload;
	lkl_env_init(LKL_MEMORY_SIZE);
	return STATUS_SUCCESS;
}

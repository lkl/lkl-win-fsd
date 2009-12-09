#include <ddk/ntddk.h>
#include <asm/env.h>

const int LKL_MEMORY_SIZE = 64 * 1024 * 1024;
extern int lkl_env_fini(void);

void DDKAPI DriverUnload (PDRIVER_OBJECT driver)
{
	DbgPrint("drvpoc: unload compiled at " __TIME__ " on " __DATE__);
	lkl_env_fini();
	DbgPrint("drvpoc: unload complete");
}

NTSTATUS DDKAPI DriverEntry (PDRIVER_OBJECT driver, PUNICODE_STRING registry)
{
	DbgPrint("drvpoc: entry compiled at " __TIME__ " on " __DATE__);
	driver->DriverUnload = DriverUnload;
	lkl_env_init(LKL_MEMORY_SIZE);
	return STATUS_SUCCESS;
}

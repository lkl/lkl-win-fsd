#include <ddk/ntddk.h>


void DriverUnload (PDRIVER_OBJECT driver)
{
	DbgPrint("driver unload");
}

NTSTATUS DriverEntry (PDRIVER_OBJECT driver, PUNICODE_STRING registry)
{
	DbgPrint("driver entry");
	driver->DriverUnload = DriverUnload;
	return STATUS_SUCCESS;
}

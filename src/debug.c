#include <stdarg.h>
#include <stdio.h>
#include "ntkvfs.h"

ULONG  DebugLevel = DL_LOW;
ULONG  ProcessNameOffset = 0;


#define DBG_BUF_LEN 0x100
VOID LklPrintf(PCHAR DebugMessage, ...)
{
	va_list             ap;
	LARGE_INTEGER       CurrentTime;
	TIME_FIELDS         TimeFields;
	CHAR                Buffer[DBG_BUF_LEN];

	RtlZeroMemory(Buffer, DBG_BUF_LEN);
	va_start(ap, DebugMessage);

	KeQuerySystemTime(&CurrentTime);
	RtlTimeToTimeFields(&CurrentTime, &TimeFields);
	vsnprintf(&Buffer[0], DBG_BUF_LEN, DebugMessage, ap);

	DbgPrint(DRIVER_NAME": %2.2d:%2.2d:%2.2d:%3.3d %8.8x: %s",
		 TimeFields.Hour, TimeFields.Minute,
		 TimeFields.Second, TimeFields.Milliseconds,
		 PsGetCurrentThread(), Buffer);

	va_end(ap);
}

VOID LklDbgPrintIrpCall(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
	PIO_STACK_LOCATION              IrpSp;
	PFILE_OBJECT                    FileObject;
	PWCHAR                          FileName;

	IrpSp = IoGetCurrentIrpStackLocation(Irp);
	FileObject = IrpSp->FileObject;
	FileName = L"Unknown";

	if (DeviceObject == g_lklvfs->device) {
		FileName = LKL_FS_NAME;
	} else if (FileObject && FileObject->FsContext) {
		PLKL_FCB Fcb = (PLKL_FCB) FileObject->FsContext;
		if (Fcb->Identifier.Type == VCB)
			FileName = L"\\Volume";
		else if (Fcb->Identifier.Type == FCB && Fcb->Mcb->FullName.Buffer)
			FileName = Fcb->Mcb->FullName.Buffer;
	}

	if (IrpSp->MajorFunction == IRP_MJ_CREATE) {
		FileName = NULL;
		if (DeviceObject == g_lklvfs->device)
			FileName = LKL_FS_NAME;
		else if (IrpSp->FileObject->FileName.Length == 0)
			FileName = L"\\Volume";
	}
	DbgPrintIrpCall(DeviceObject, Irp, FileName);
}


#include <ddk/ntdddisk.h>
#include "ntkvfs.h"
#include <asm/env.h>
#include <asm/disk.h>

static NTSTATUS LklLockVolume(IN PLKL_IRP_CONTEXT IrpContext)
{
	DbgPrint("lock");
	return STATUS_INVALID_DEVICE_REQUEST;
}

static NTSTATUS LklUnLockVolume(IN PLKL_IRP_CONTEXT IrpContext)
{
	DbgPrint("unlock");
	return STATUS_INVALID_DEVICE_REQUEST;
}

static NTSTATUS LklUnmountVolume(IN PLKL_IRP_CONTEXT IrpContext)
{
	DbgPrint("unmont");
	return STATUS_INVALID_DEVICE_REQUEST;
}

static NTSTATUS LklIsVolumeMounted(IN PLKL_IRP_CONTEXT IrpContext)
{
	DbgPrint("is");
	return STATUS_INVALID_DEVICE_REQUEST;
}

static NTSTATUS LklUserFsRequest(IN PLKL_IRP_CONTEXT IrpContext)
{
	PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(IrpContext->Irp);
	PEXTENDED_IO_STACK_LOCATION ExtIrpSp = (PEXTENDED_IO_STACK_LOCATION) IrpSp;

	switch(ExtIrpSp->Parameters.FileSystemControl.FsControlCode)
	{
	case FSCTL_LOCK_VOLUME:
		return LklLockVolume(IrpContext);
	case FSCTL_UNLOCK_VOLUME:
		return LklUnLockVolume(IrpContext);
	case FSCTL_DISMOUNT_VOLUME:
		return LklUnmountVolume(IrpContext);
	case FSCTL_IS_VOLUME_MOUNTED:
		return LklIsVolumeMounted(IrpContext);
	default:
		return STATUS_INVALID_DEVICE_REQUEST;
	}
}


/*
 * Read the partition table and find out the partition size
 */
static NTSTATUS BlockDeviceIoControl(IN PDEVICE_OBJECT DeviceObject,
				     IN ULONG IoctlCode, IN PVOID InputBuffer,
				     IN ULONG InputBufferSize,
				     IN OUT PVOID OutputBuffer,
				     IN OUT PULONG OutputBufferSize)
{
	ULONG OutputBufferSize2 = 0;
	KEVENT Event;
	PIRP Irp;
	IO_STATUS_BLOCK IoStatus;
	NTSTATUS status;

	ASSERT(DeviceObject != NULL);

	if (OutputBufferSize)
		OutputBufferSize2 = *OutputBufferSize;

	KeInitializeEvent(&Event, NotificationEvent, FALSE);
	Irp = IoBuildDeviceIoControlRequest(IoctlCode, DeviceObject,
					    InputBuffer, InputBufferSize,
					    OutputBuffer, OutputBufferSize2,
					    FALSE, &Event, &IoStatus);
	if (!Irp)
		return STATUS_INSUFFICIENT_RESOURCES;

	status = IoCallDriver(DeviceObject, Irp);
	if (status == STATUS_PENDING) {
		KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
		status = IoStatus.Status;
	}

	if (OutputBufferSize)
		*OutputBufferSize = (ULONG) IoStatus.Information;

	return status;
}


static NTSTATUS BlockDeviceDiskGeometry(IN PDEVICE_OBJECT DeviceObject,
					IN OUT PDISK_GEOMETRY DiskGeometry)
{
	ULONG ioctlSize;
	ioctlSize = sizeof(DISK_GEOMETRY);
	return BlockDeviceIoControl(DeviceObject, IOCTL_DISK_GET_DRIVE_GEOMETRY,
				    NULL, 0, DiskGeometry, &ioctlSize);
}

static NTSTATUS BlockDevicePartitionInfo(IN PDEVICE_OBJECT DeviceObject,
					 IN OUT PPARTITION_INFORMATION PInfo)
{
	ULONG ioctlSize;
	ioctlSize = sizeof(PARTITION_INFORMATION);
	return BlockDeviceIoControl(DeviceObject, IOCTL_DISK_GET_PARTITION_INFO,
				    NULL, 0, PInfo, &ioctlSize);
}

static int ilog2(int n)
{
	switch (n)
	{
	case    1: return  0;
	case    2: return  1;
	case    4: return  2;
	case    8: return  3;
	case   16: return  4;
	case   32: return  5;
	case   64: return  6;
	case  128: return  7;
	case  256: return  8;
	case  512: return  9;
	case 1024: return 10;
	case 2048: return 11;
	case 4096: return 12;
	case 8192: return 13;
	default:
		DbgPrint("ilog2() cannot handle large values like %d", n);
		return -1;
	}
}

static NTSTATUS BlockDeviceNrSectors(IN PDEVICE_OBJECT physical_dev,
				     IN PLKL_VCB vcb,
				     OUT int * psectors)
{
	NTSTATUS status;

	status = BlockDeviceDiskGeometry(physical_dev, &vcb->disk_geometry);
	if (!NT_SUCCESS(status))
		return status;

	status = BlockDevicePartitionInfo(physical_dev, &vcb->partition_information);
	if (!NT_SUCCESS(status))
		return status;

	/* we can divide LONGLONG numbers in the kernel,
	 * but we can fake divison with a bit-shift*/
	*psectors = vcb->partition_information.PartitionLength.QuadPart >>
		ilog2(vcb->disk_geometry.BytesPerSector);

	return STATUS_SUCCESS;
}

static void test_list_files(char * path)
{
	int fd = lkl_sys_open(path, O_RDONLY|O_LARGEFILE|O_DIRECTORY, 0);
	if (fd >= 0) {
		char x[4096];
		int count, reclen;
		struct __kernel_dirent *de;

		count = lkl_sys_getdents(fd, (struct __kernel_dirent*) x, sizeof(x));
		ASSERT(count > 0);

		de=(struct __kernel_dirent*)x;
		while (count > 0) {
			reclen = de->d_reclen;
			DbgPrint("%s %ld\n", de->d_name, de->d_ino);
			de = (struct __kernel_dirent*) ((char*)de+reclen);
			count-=reclen;
		}

		lkl_sys_close(fd);
	}
}

/*
 * Mount the device on the LKL side:
 * - add a new LKL disk device
 * - make a device file and a mount point
 * - mount the device over the mount point.
 */
static NTSTATUS LklMount(IN PLKL_VCB vcb, int sectors)
{
	NTSTATUS status = STATUS_SUCCESS;
	__kernel_dev_t devt;
	char * mnt_path = vcb->mount_point.mnt_path;
	int mnt_path_len = sizeof(vcb->mount_point.mnt_path);
	int rc;

	devt = lkl_disk_add_disk(vcb->physical_dev, sectors);
	if (devt == 0)
		return STATUS_DRIVER_INTERNAL_ERROR;
	vcb->mount_point.devt = devt;

	/*
	 * this will:
	 * - create a device file for this devt
	 * - create a mount point
	 * - mount the device over the mount point
	 *
	 * The mount path will be returned in mnt_path.
	 */
	rc = lkl_mount_dev(devt, NULL, 0, NULL, mnt_path, mnt_path_len);
	if (rc) {
		status = STATUS_UNRECOGNIZED_VOLUME;
		goto del_disk;
	}

	test_list_files(mnt_path);
	return STATUS_SUCCESS;

/*
 * this may be usefull later to umount the device

umount_dev:
	rc = lkl_umount_dev(devt, 0);
	if (rc)
		DbgPrint("lkl_umount_dev rc=%d\n", rc);
*/
del_disk:
	rc = lkl_disk_del_disk(devt);
	if (rc)
		DbgPrint("lkl_disk_del_disk rc=%d\n", rc);
	return status;
}

/*
 * Mount an NTK volume:
 * - create a device describing the volume
 * - mount an undelining linux LKL device
 */
static NTSTATUS LklMountVolume(IN PLKL_IRP_CONTEXT IrpContext)
{
	PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(IrpContext->Irp);
	PDEVICE_OBJECT physical_dev;
	PDEVICE_OBJECT vol_dev;
	PLKL_VCB vcb = NULL;
	PVPB vpb;
	NTSTATUS status;
	int sectors;

	vpb = IrpSp->Parameters.MountVolume.Vpb;
	physical_dev = IrpSp->Parameters.MountVolume.DeviceObject;

	if (physical_dev == g_lklvfs->device)
		return STATUS_INVALID_DEVICE_REQUEST;

	/* create a device object for the volume device,
	 * and allocate LKL_VCB in the device */
	status = IoCreateDevice(g_lklvfs->driver, sizeof(LKL_VCB), NULL,
				FILE_DEVICE_DISK_FILE_SYSTEM, 0, FALSE, &vol_dev);
	if (!NT_SUCCESS(status))
		return status;

	if (vol_dev->AlignmentRequirement < physical_dev->AlignmentRequirement)
		vol_dev->AlignmentRequirement = physical_dev->AlignmentRequirement;
	vol_dev->StackSize = (CCHAR)(physical_dev->StackSize+1);

	status = LklVcbInit(vol_dev, physical_dev, vpb, &vcb);
	if (!NT_SUCCESS(status))
		goto free_vol_dev;

	status = BlockDeviceNrSectors(physical_dev, vcb, &sectors);
	if (!NT_SUCCESS(status))
		goto free_vcb;

	status = LklMount(vcb, sectors);
	if (!NT_SUCCESS(status))
		goto free_vcb;

	SET_FLAG(vcb->Flags, VCB_MOUNTED);
	vpb->DeviceObject = vol_dev;
	return STATUS_SUCCESS;


free_vcb:
	LklVcbFini(vcb);
free_vol_dev:
	IoDeleteDevice(vol_dev);
	return status;
}

BOOLEAN LklVcbIsMounted(IN PLKL_VCB vcb)
{
	return IS_SET(vcb->Flags, VCB_MOUNTED);
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

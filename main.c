#include "stdcdrv64.h"

// Device Class GUID for stdcdrv64
static const GUID GUID_DEVCLASS_STDC = {
    0x574F0FC8, 0x1AF4, 0x4B78, { 0x88, 0x2D, 0xAB, 0x62, 0x4F, 0xDD, 0x5E, 0x9F }
};

NTSTATUS StdcCreateClose(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
    UNREFERENCED_PARAMETER(DeviceObject);
    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}

NTSTATUS StdcGetDriverVersion(PIRP Irp, PIO_STACK_LOCATION irpSp) {
    if (irpSp->Parameters.DeviceIoControl.OutputBufferLength < sizeof(STDC_VERSION_RESP)) {
        return STATUS_BUFFER_TOO_SMALL;
    }

    PSTDC_VERSION_RESP pResp = (PSTDC_VERSION_RESP)Irp->AssociatedIrp.SystemBuffer;
    pResp->Major = 2;
    pResp->Minor = 0;
    pResp->Patch = 0;

    Irp->IoStatus.Information = sizeof(STDC_VERSION_RESP);
    return STATUS_SUCCESS;
}

NTSTATUS StdcSetProcAffinity(PIRP Irp, PIO_STACK_LOCATION irpSp) {
    if (irpSp->Parameters.DeviceIoControl.InputBufferLength < sizeof(ULONG)) {
        return STATUS_BUFFER_TOO_SMALL;
    }

    ULONG targetCpu = *(PULONG)Irp->AssociatedIrp.SystemBuffer;
    USHORT maxGroups = KeQueryMaximumGroupCount();
    USHORT group = 0;
    ULONG currentCpu = 0;
    BOOLEAN found = FALSE;
    GROUP_AFFINITY targetAffinity;
    RtlZeroMemory(&targetAffinity, sizeof(targetAffinity));

    for (group = 0; group < maxGroups; group++) {
        KAFFINITY groupAffinity = KeQueryGroupAffinity(group);
        for (ULONG bit = 0; bit < (ULONG)(sizeof(KAFFINITY) * 8); bit++) {
            if ((groupAffinity & ((KAFFINITY)1 << bit)) != 0) {
                if (currentCpu == targetCpu) {
                    targetAffinity.Group = group;
                    targetAffinity.Mask = ((KAFFINITY)1 << bit);
                    found = TRUE;
                    break;
                }
                currentCpu++;
            }
        }
        if (found) {
            break;
        }
    }

    if (found) {
        KeSetSystemGroupAffinityThread(&targetAffinity, NULL);
        Irp->IoStatus.Information = 0;
        return STATUS_SUCCESS;
    }

    return STATUS_INVALID_PARAMETER;
}

NTSTATUS StdcGetProcessorCount(PIRP Irp, PIO_STACK_LOCATION irpSp) {
    if (irpSp->Parameters.DeviceIoControl.OutputBufferLength < sizeof(ULONG)) {
        return STATUS_BUFFER_TOO_SMALL;
    }

    USHORT maxGroups = KeQueryMaximumGroupCount();
    ULONG totalCount = 0;
    for (USHORT group = 0; group < maxGroups; group++) {
        KAFFINITY groupAffinity = KeQueryGroupAffinity(group);
        for (ULONG bit = 0; bit < (ULONG)(sizeof(KAFFINITY) * 8); bit++) {
            if ((groupAffinity & ((KAFFINITY)1 << bit)) != 0) {
                totalCount++;
            }
        }
    }

    *(PULONG)Irp->AssociatedIrp.SystemBuffer = totalCount;
    Irp->IoStatus.Information = sizeof(ULONG);
    return STATUS_SUCCESS;
}

NTSTATUS StdcGetSystemAffinity(PIRP Irp, PIO_STACK_LOCATION irpSp) {
    USHORT maxGroups = KeQueryMaximumGroupCount();
    ULONG requiredSize = (ULONG)maxGroups * (ULONG)sizeof(KAFFINITY);
    if (irpSp->Parameters.DeviceIoControl.OutputBufferLength < requiredSize) {
        return STATUS_BUFFER_TOO_SMALL;
    }

    PKAFFINITY pOut = (PKAFFINITY)Irp->AssociatedIrp.SystemBuffer;
    for (USHORT group = 0; group < maxGroups; group++) {
        pOut[group] = KeQueryGroupAffinity(group);
    }

    Irp->IoStatus.Information = requiredSize;
    return STATUS_SUCCESS;
}

NTSTATUS StdcGetSystemGroupCount(PIRP Irp, PIO_STACK_LOCATION irpSp) {
    if (irpSp->Parameters.DeviceIoControl.OutputBufferLength < sizeof(ULONG)) {
        return STATUS_BUFFER_TOO_SMALL;
    }

    *(PULONG)Irp->AssociatedIrp.SystemBuffer = (ULONG)KeQueryMaximumGroupCount();
    Irp->IoStatus.Information = sizeof(ULONG);
    return STATUS_SUCCESS;
}

NTSTATUS StdcDispatchIoControl(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
    UNREFERENCED_PARAMETER(DeviceObject);
    PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(Irp);
    ULONG ioctl = irpSp->Parameters.DeviceIoControl.IoControlCode;
    NTSTATUS status = STATUS_INVALID_DEVICE_REQUEST;

    Irp->IoStatus.Information = 0;

    switch (ioctl) {
    case IOCTL_STDC_READ_MSR:
        status = StdcReadMsr(Irp, irpSp);
        break;
    case IOCTL_STDC_READ_PHYSICAL_MEM:
        status = StdcReadPhysicalMemory(Irp, irpSp);
        break;
    case IOCTL_STDC_WRITE_PHYSICAL_MEM:
        status = StdcWritePhysicalMemory(Irp, irpSp);
        break;
    case IOCTL_STDC_GET_DRIVER_VERSION:
        status = StdcGetDriverVersion(Irp, irpSp);
        break;
    case IOCTL_STDC_SET_PROC_AFFINITY:
        status = StdcSetProcAffinity(Irp, irpSp);
        break;
    case IOCTL_STDC_GET_PROCESSOR_COUNT:
        status = StdcGetProcessorCount(Irp, irpSp);
        break;
    case IOCTL_STDC_WRITE_MSR:
        status = StdcWriteMsr(Irp, irpSp);
        break;
    case IOCTL_STDC_READ_PCI_CONFIG:
        status = StdcReadPciConfig(Irp, irpSp);
        break;
    case IOCTL_STDC_WRITE_PCI_CONFIG:
        status = StdcWritePciConfig(Irp, irpSp);
        break;
    case IOCTL_STDC_READ_PORT:
        status = StdcReadPort(Irp, irpSp);
        break;
    case IOCTL_STDC_WRITE_PORT:
        status = StdcWritePort(Irp, irpSp);
        break;
    case IOCTL_STDC_READ_CONTROL_REG:
        status = StdcReadControlRegister(Irp, irpSp);
        break;
    case IOCTL_STDC_WRITE_CONTROL_REG:
        status = StdcWriteControlRegister(Irp, irpSp);
        break;
    case IOCTL_STDC_GET_SYSTEM_AFFINITY:
        status = StdcGetSystemAffinity(Irp, irpSp);
        break;
    case IOCTL_STDC_GET_GROUP_COUNT:
        status = StdcGetSystemGroupCount(Irp, irpSp);
        break;
    default:
        status = STATUS_INVALID_DEVICE_REQUEST;
        break;
    }

    Irp->IoStatus.Status = status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return status;
}

VOID StdcUnload(PDRIVER_OBJECT DriverObject) {
    UNICODE_STRING symLink;
    RtlInitUnicodeString(&symLink, STDC_DOS_DEVICE_NAME);
    IoDeleteSymbolicLink(&symLink);

    // FIX: Unconditionally delete device object to prevent resource leak on unload
    if (DriverObject->DeviceObject != NULL) {
        IoDeleteDevice(DriverObject->DeviceObject);
    }

    DbgPrint("TNT STDC Driver Unload\n");
}

NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath) {
    UNREFERENCED_PARAMETER(RegistryPath);
    NTSTATUS status;
    UNICODE_STRING devName;
    UNICODE_STRING symLink;
    UNICODE_STRING sddl;
    PDEVICE_OBJECT devObj = NULL;

    RtlInitUnicodeString(&devName, STDC_DEVICE_NAME);
    RtlInitUnicodeString(&symLink, STDC_DOS_DEVICE_NAME);

    // FIX: Restrict DACL to SYSTEM and Administrators (SDDL: D:P(A;;GA;;;SY)(A;;GA;;;BA))
    // Prevents unprivileged users from sending raw hardware IOCTLs
    RtlInitUnicodeString(&sddl, L"D:P(A;;GA;;;SY)(A;;GA;;;BA)");

    status = IoCreateDeviceSecure(
        DriverObject,
        0,
        &devName,
        FILE_DEVICE_UNKNOWN,
        FILE_DEVICE_SECURE_OPEN,
        FALSE,
        &sddl,
        &GUID_DEVCLASS_STDC,
        &devObj
    );

    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = IoCreateSymbolicLink(&symLink, &devName);
    if (!NT_SUCCESS(status)) {
        // FIX: Delete device object on symbolic link creation failure
        IoDeleteDevice(devObj);
        return status;
    }

    DriverObject->MajorFunction[IRP_MJ_CREATE] = StdcCreateClose;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = StdcCreateClose;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = StdcDispatchIoControl;
    DriverObject->DriverUnload = StdcUnload;

    devObj->Flags |= DO_BUFFERED_IO;
    devObj->Flags &= ~DO_DEVICE_INITIALIZING;

    DbgPrint("TNTDC Driver Entry -- Driver object Created\n");
    return STATUS_SUCCESS;
}

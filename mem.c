#include "stdcdrv64.h"

NTSTATUS StdcReadPhysicalMemory(PIRP Irp, PIO_STACK_LOCATION irpSp) {
    if (irpSp->Parameters.DeviceIoControl.InputBufferLength < 13) {
        return STATUS_BUFFER_TOO_SMALL;
    }

    PSTDC_PHYS_MEM_REQ pReq = (PSTDC_PHYS_MEM_REQ)Irp->AssociatedIrp.SystemBuffer;
    if (irpSp->Parameters.DeviceIoControl.OutputBufferLength < pReq->Size) {
        return STATUS_BUFFER_TOO_SMALL;
    }

    PHYSICAL_ADDRESS physAddr;
    physAddr.LowPart = pReq->LowPart;
    physAddr.HighPart = (LONG)pReq->HighPart;

    PVOID pMapped = MmMapIoSpace(physAddr, pReq->Size, MmNonCached);
    if (!pMapped) {
        return STATUS_UNSUCCESSFUL;
    }

    RtlCopyMemory(Irp->AssociatedIrp.SystemBuffer, pMapped, pReq->Size);
    MmUnmapIoSpace(pMapped, pReq->Size);

    Irp->IoStatus.Information = pReq->Size;
    return STATUS_SUCCESS;
}

NTSTATUS StdcWritePhysicalMemory(PIRP Irp, PIO_STACK_LOCATION irpSp) {
    if (irpSp->Parameters.DeviceIoControl.InputBufferLength < 13) {
        return STATUS_BUFFER_TOO_SMALL;
    }

    PSTDC_PHYS_MEM_REQ pReq = (PSTDC_PHYS_MEM_REQ)Irp->AssociatedIrp.SystemBuffer;
    if (irpSp->Parameters.DeviceIoControl.InputBufferLength < 13 + pReq->Size) {
        return STATUS_BUFFER_TOO_SMALL;
    }

    PHYSICAL_ADDRESS physAddr;
    physAddr.LowPart = pReq->LowPart;
    physAddr.HighPart = (LONG)pReq->HighPart;

    PVOID pMapped = MmMapIoSpace(physAddr, pReq->Size, MmNonCached);
    if (!pMapped) {
        return STATUS_UNSUCCESSFUL;
    }

    RtlCopyMemory(pMapped, pReq->Data, pReq->Size);
    MmUnmapIoSpace(pMapped, pReq->Size);

    Irp->IoStatus.Information = 0;
    return STATUS_SUCCESS;
}

NTSTATUS StdcReadControlRegister(PIRP Irp, PIO_STACK_LOCATION irpSp) {
    if (irpSp->Parameters.DeviceIoControl.InputBufferLength < 1) {
        return STATUS_BUFFER_TOO_SMALL;
    }
    if (irpSp->Parameters.DeviceIoControl.OutputBufferLength < sizeof(ULONG64)) {
        return STATUS_BUFFER_TOO_SMALL;
    }

    PUCHAR pReg = (PUCHAR)Irp->AssociatedIrp.SystemBuffer;
    PULONG64 pOut = (PULONG64)Irp->AssociatedIrp.SystemBuffer;

    *pOut = StdcReadCr(*pReg);
    Irp->IoStatus.Information = sizeof(ULONG64);
    return STATUS_SUCCESS;
}

NTSTATUS StdcWriteControlRegister(PIRP Irp, PIO_STACK_LOCATION irpSp) {
    if (irpSp->Parameters.DeviceIoControl.InputBufferLength < 9) {
        return STATUS_BUFFER_TOO_SMALL;
    }

    PSTDC_CR_REQ pReq = (PSTDC_CR_REQ)Irp->AssociatedIrp.SystemBuffer;
    StdcWriteCr(pReq->RegisterIndex, pReq->Value);

    Irp->IoStatus.Information = 0;
    return STATUS_SUCCESS;
}

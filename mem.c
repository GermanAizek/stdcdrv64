#include "stdcdrv64.h"

#define STDC_MAX_PHYS_TRANSFER  0x1000000 // 16 MB max transfer size
#define STDC_MIN_PHYS_ADDR      0x100000  // 1 MB boundary (protect IVT, BDA, EBDA)

//
// Physical memory address validation
//
BOOLEAN StdcValidatePhysicalAddress(PHYSICAL_ADDRESS PhysicalAddress, SIZE_T Length) {
    if (Length == 0 || Length > STDC_MAX_PHYS_TRANSFER) {
        return FALSE;
    }

    // Check for integer overflow
    if ((ULONG64)PhysicalAddress.QuadPart + Length < (ULONG64)PhysicalAddress.QuadPart) {
        return FALSE;
    }

    // Protect low physical memory (< 1MB)
    if ((ULONG64)PhysicalAddress.QuadPart < STDC_MIN_PHYS_ADDR) {
        return FALSE;
    }

    return TRUE;
}

//
// Control register write validation
//
BOOLEAN StdcValidateControlRegisterWrite(UCHAR RegisterIndex, ULONG64 Value) {
    ULONG64 currentCr = 0;

    switch (RegisterIndex) {
    case 0:
        // Prevent disabling Write Protection (CR0.WP, bit 16) or Paging (CR0.PG, bit 31)
        if (!(Value & 0x00010000) || !(Value & 0x80000000)) {
            return FALSE;
        }
        return TRUE;

    case 3:
        // Arbitrary CR3 modification changes page directory base - block completely
        return FALSE;

    case 4:
        // Prevent disabling SMEP (bit 20) and SMAP (bit 21) if currently active
        currentCr = StdcReadCr(4);
        if ((currentCr & (1ULL << 20)) && !(Value & (1ULL << 20))) {
            return FALSE;
        }
        if ((currentCr & (1ULL << 21)) && !(Value & (1ULL << 21))) {
            return FALSE;
        }
        return TRUE;

    default:
        // Disallow writing to unexpected CR registers
        return FALSE;
    }
}

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

    // FIX: Validate physical address and transfer size
    if (!StdcValidatePhysicalAddress(physAddr, pReq->Size)) {
        return STATUS_ACCESS_DENIED;
    }

    PVOID pMapped = MmMapIoSpace(physAddr, pReq->Size, MmNonCached);
    if (!pMapped) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    __try {
        RtlCopyMemory(Irp->AssociatedIrp.SystemBuffer, pMapped, pReq->Size);
        Irp->IoStatus.Information = pReq->Size;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        MmUnmapIoSpace(pMapped, pReq->Size);
        return GetExceptionCode();
    }

    MmUnmapIoSpace(pMapped, pReq->Size);
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

    // FIX: Validate physical address and transfer size
    if (!StdcValidatePhysicalAddress(physAddr, pReq->Size)) {
        return STATUS_ACCESS_DENIED;
    }

    PVOID pMapped = MmMapIoSpace(physAddr, pReq->Size, MmNonCached);
    if (!pMapped) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    __try {
        RtlCopyMemory(pMapped, pReq->Data, pReq->Size);
        Irp->IoStatus.Information = 0;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        MmUnmapIoSpace(pMapped, pReq->Size);
        return GetExceptionCode();
    }

    MmUnmapIoSpace(pMapped, pReq->Size);
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

    if (*pReg != 0 && *pReg != 2 && *pReg != 3 && *pReg != 4 && *pReg != 8) {
        return STATUS_INVALID_PARAMETER;
    }

    __try {
        *pOut = StdcReadCr(*pReg);
        Irp->IoStatus.Information = sizeof(ULONG64);
        return STATUS_SUCCESS;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return GetExceptionCode();
    }
}

NTSTATUS StdcWriteControlRegister(PIRP Irp, PIO_STACK_LOCATION irpSp) {
    if (irpSp->Parameters.DeviceIoControl.InputBufferLength < sizeof(STDC_CR_REQ)) {
        return STATUS_BUFFER_TOO_SMALL;
    }

    PSTDC_CR_REQ pReq = (PSTDC_CR_REQ)Irp->AssociatedIrp.SystemBuffer;

    // FIX: Enforce control register write validation to prevent security bypass
    if (!StdcValidateControlRegisterWrite(pReq->RegisterIndex, pReq->Value)) {
        return STATUS_ACCESS_DENIED;
    }

    __try {
        StdcWriteCr(pReq->RegisterIndex, pReq->Value);
        Irp->IoStatus.Information = 0;
        return STATUS_SUCCESS;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return GetExceptionCode();
    }
}

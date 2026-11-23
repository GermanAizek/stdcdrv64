#include "stdcdrv64.h"

//
// Validation for PCI configuration space access
//
BOOLEAN StdcValidatePciAccess(UCHAR Bus, UCHAR Device, UCHAR Function, ULONG Offset, BOOLEAN IsWrite) {
    UNREFERENCED_PARAMETER(Bus);

    if (Device >= 32 || Function >= 8) {
        return FALSE;
    }

    if (Offset >= 256) {
        return FALSE;
    }

    if (IsWrite) {
        // Prevent writing to critical standard PCI configuration headers (< 0x40):
        // 0x00-0x03: Device/Vendor ID
        // 0x04-0x07: Command/Status registers
        // 0x08-0x0B: Class code/Revision ID
        // 0x10-0x27: Base Address Registers (BAR0-BAR5) - unmapping BARs crashes hardware
        // 0x2C-0x2F: Subsystem IDs
        if (Offset < 0x40) {
            return FALSE;
        }
    }

    return TRUE;
}

NTSTATUS StdcReadPciConfig(PIRP Irp, PIO_STACK_LOCATION irpSp) {
    if (irpSp->Parameters.DeviceIoControl.InputBufferLength < 8) {
        return STATUS_BUFFER_TOO_SMALL;
    }
    if (irpSp->Parameters.DeviceIoControl.OutputBufferLength < sizeof(ULONG)) {
        return STATUS_BUFFER_TOO_SMALL;
    }

    PSTDC_PCI_REQ pReq = (PSTDC_PCI_REQ)Irp->AssociatedIrp.SystemBuffer;
    PULONG pOut = (PULONG)Irp->AssociatedIrp.SystemBuffer;

    // FIX: Validate PCI device parameters and bounds
    if (!StdcValidatePciAccess(pReq->Bus, pReq->Device, pReq->Function, pReq->Offset, FALSE)) {
        return STATUS_ACCESS_DENIED;
    }

    __try {
        ULONG origCf8 = StdcInDword(0xCF8);
        ULONG addr = 0x80000000 |
                     ((ULONG)pReq->Bus << 16) |
                     (((ULONG)pReq->Device & 0x1F) << 11) |
                     (((ULONG)pReq->Function & 0x07) << 8) |
                     (pReq->Offset & 0xFC);

        StdcOutDword(0xCF8, addr);

        if (pReq->Mode == 0) {
            *pOut = (ULONG)StdcInByte((USHORT)(0xCFC + (pReq->Offset & 3)));
        } else if (pReq->Mode == 1) {
            *pOut = (ULONG)StdcInWord((USHORT)(0xCFC + (pReq->Offset & 3)));
        } else if (pReq->Mode == 2) {
            *pOut = StdcInDword(0xCFC);
        } else {
            StdcOutDword(0xCF8, origCf8);
            return STATUS_INVALID_PARAMETER;
        }

        StdcOutDword(0xCF8, origCf8);
        Irp->IoStatus.Information = sizeof(ULONG);
        return STATUS_SUCCESS;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return GetExceptionCode();
    }
}

NTSTATUS StdcWritePciConfig(PIRP Irp, PIO_STACK_LOCATION irpSp) {
    if (irpSp->Parameters.DeviceIoControl.InputBufferLength < 12) {
        return STATUS_BUFFER_TOO_SMALL;
    }

    PSTDC_PCI_REQ pReq = (PSTDC_PCI_REQ)Irp->AssociatedIrp.SystemBuffer;

    // FIX: Validate PCI device parameters and block writes to critical header fields
    if (!StdcValidatePciAccess(pReq->Bus, pReq->Device, pReq->Function, pReq->Offset, TRUE)) {
        return STATUS_ACCESS_DENIED;
    }

    __try {
        ULONG origCf8 = StdcInDword(0xCF8);
        ULONG addr = 0x80000000 |
                     ((ULONG)pReq->Bus << 16) |
                     (((ULONG)pReq->Device & 0x1F) << 11) |
                     (((ULONG)pReq->Function & 0x07) << 8) |
                     (pReq->Offset & 0xFC);

        StdcOutDword(0xCF8, addr);

        if (pReq->Mode == 0) {
            StdcOutByte((USHORT)(0xCFC + (pReq->Offset & 3)), (UCHAR)pReq->Value);
        } else if (pReq->Mode == 1) {
            StdcOutWord((USHORT)(0xCFC + (pReq->Offset & 3)), (USHORT)pReq->Value);
        } else if (pReq->Mode == 2) {
            StdcOutDword(0xCFC, pReq->Value);
        } else {
            StdcOutDword(0xCF8, origCf8);
            return STATUS_INVALID_PARAMETER;
        }

        StdcOutDword(0xCF8, origCf8);
        Irp->IoStatus.Information = 0;
        return STATUS_SUCCESS;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return GetExceptionCode();
    }
}

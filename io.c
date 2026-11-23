#include "stdcdrv64.h"

//
// Whitelist and validation for x86 hardware I/O ports
//
BOOLEAN StdcValidateIOPortAccess(USHORT Port, BOOLEAN IsWrite) {
    // Block critical legacy controllers that can cause system crashes, DMA attacks, or exploit SMM
    if ((Port <= 0x001F) ||                     // Master DMA 8237
        (Port >= 0x0020 && Port <= 0x0021) ||   // Master PIC 8259
        (Port >= 0x0040 && Port <= 0x0043) ||   // PIT 8254
        (Port == 0x0060 || Port == 0x0064) ||   // Keyboard Controller 8042
        (Port >= 0x00A0 && Port <= 0x00A1) ||   // Slave PIC 8259
        (Port == 0x00B2 || Port == 0x00B3) ||   // APM / Advanced Power Management SMI control
        (Port >= 0x00C0 && Port <= 0x00DF)) {   // Slave DMA 8237
        return FALSE;
    }

    if (IsWrite) {
        // Safe diagnostic and configuration ports:
        // - POST diagnostic ports: 0x80, 0x81, 0xEB
        // - Real-Time Clock / CMOS: 0x70 - 0x71
        // - PCI Configuration Address / Data: 0xCF8 - 0xCFC
        if (Port == 0x0080 || Port == 0x0081 || Port == 0x00EB ||
            (Port >= 0x0070 && Port <= 0x0071) ||
            (Port >= 0x0CF8 && Port <= 0x0CFC)) {
            return TRUE;
        }
        return FALSE;
    }

    return TRUE;
}

NTSTATUS StdcReadPort(PIRP Irp, PIO_STACK_LOCATION irpSp) {
    if (irpSp->Parameters.DeviceIoControl.InputBufferLength < 3) {
        return STATUS_BUFFER_TOO_SMALL;
    }
    if (irpSp->Parameters.DeviceIoControl.OutputBufferLength < sizeof(ULONG)) {
        return STATUS_BUFFER_TOO_SMALL;
    }

    PSTDC_PORT_REQ pReq = (PSTDC_PORT_REQ)Irp->AssociatedIrp.SystemBuffer;
    PULONG pOut = (PULONG)Irp->AssociatedIrp.SystemBuffer;

    // FIX: Validate I/O port read access
    if (!StdcValidateIOPortAccess(pReq->Port, FALSE)) {
        return STATUS_ACCESS_DENIED;
    }

    __try {
        if (pReq->Mode == 0) {
            *pOut = (ULONG)StdcInByte(pReq->Port);
        } else if (pReq->Mode == 1) {
            *pOut = (ULONG)StdcInWord(pReq->Port);
        } else if (pReq->Mode == 2) {
            *pOut = StdcInDword(pReq->Port);
        } else {
            return STATUS_INVALID_PARAMETER;
        }

        Irp->IoStatus.Information = sizeof(ULONG);
        return STATUS_SUCCESS;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return GetExceptionCode();
    }
}

NTSTATUS StdcWritePort(PIRP Irp, PIO_STACK_LOCATION irpSp) {
    if (irpSp->Parameters.DeviceIoControl.InputBufferLength < 4) {
        return STATUS_BUFFER_TOO_SMALL;
    }

    PSTDC_PORT_REQ pReq = (PSTDC_PORT_REQ)Irp->AssociatedIrp.SystemBuffer;

    // FIX: Validate I/O port write access
    if (!StdcValidateIOPortAccess(pReq->Port, TRUE)) {
        return STATUS_ACCESS_DENIED;
    }

    __try {
        if (pReq->Mode == 0) {
            StdcOutByte(pReq->Port, (UCHAR)pReq->Value);
        } else if (pReq->Mode == 1) {
            if (irpSp->Parameters.DeviceIoControl.InputBufferLength < 5) {
                return STATUS_BUFFER_TOO_SMALL;
            }
            StdcOutWord(pReq->Port, (USHORT)pReq->Value);
        } else if (pReq->Mode == 2) {
            if (irpSp->Parameters.DeviceIoControl.InputBufferLength < 7) {
                return STATUS_BUFFER_TOO_SMALL;
            }
            StdcOutDword(pReq->Port, pReq->Value);
        } else {
            return STATUS_INVALID_PARAMETER;
        }

        Irp->IoStatus.Information = 0;
        return STATUS_SUCCESS;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return GetExceptionCode();
    }
}

#include "stdcdrv64.h"

NTSTATUS StdcReadPort(PIRP Irp, PIO_STACK_LOCATION irpSp) {
    if (irpSp->Parameters.DeviceIoControl.InputBufferLength < 3) {
        return STATUS_BUFFER_TOO_SMALL;
    }
    if (irpSp->Parameters.DeviceIoControl.OutputBufferLength < sizeof(ULONG)) {
        return STATUS_BUFFER_TOO_SMALL;
    }

    PSTDC_PORT_REQ pReq = (PSTDC_PORT_REQ)Irp->AssociatedIrp.SystemBuffer;
    PULONG pOut = (PULONG)Irp->AssociatedIrp.SystemBuffer;

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
}

NTSTATUS StdcWritePort(PIRP Irp, PIO_STACK_LOCATION irpSp) {
    if (irpSp->Parameters.DeviceIoControl.InputBufferLength < 4) {
        return STATUS_BUFFER_TOO_SMALL;
    }

    PSTDC_PORT_REQ pReq = (PSTDC_PORT_REQ)Irp->AssociatedIrp.SystemBuffer;

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
}

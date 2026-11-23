#include "stdcdrv64.h"

NTSTATUS StdcReadMsr(PIRP Irp, PIO_STACK_LOCATION irpSp) {
    if (irpSp->Parameters.DeviceIoControl.InputBufferLength < sizeof(ULONG)) {
        return STATUS_BUFFER_TOO_SMALL;
    }
    if (irpSp->Parameters.DeviceIoControl.OutputBufferLength < sizeof(ULONG64)) {
        return STATUS_BUFFER_TOO_SMALL;
    }

    PULONG pMsr = (PULONG)Irp->AssociatedIrp.SystemBuffer;
    PULONG64 pOut = (PULONG64)Irp->AssociatedIrp.SystemBuffer;

    *pOut = StdcReadMsrVal(*pMsr);
    Irp->IoStatus.Information = sizeof(ULONG64);
    return STATUS_SUCCESS;
}

NTSTATUS StdcWriteMsr(PIRP Irp, PIO_STACK_LOCATION irpSp) {
    if (irpSp->Parameters.DeviceIoControl.InputBufferLength < 0xC) {
        return STATUS_BUFFER_TOO_SMALL;
    }

    PSTDC_MSR_WRITE_REQ pReq = (PSTDC_MSR_WRITE_REQ)Irp->AssociatedIrp.SystemBuffer;
    StdcWriteMsrVal(pReq->MsrIndex, pReq->Value);

    Irp->IoStatus.Information = 0;
    return STATUS_SUCCESS;
}

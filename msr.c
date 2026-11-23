#include "stdcdrv64.h"

//
// Whitelist and validation for Model-Specific Registers
//
BOOLEAN StdcIsSafeMsr(ULONG Msr, BOOLEAN IsWrite) {
    // Block critical CPU execution control and system call registers
    switch (Msr) {
    case 0x0000001B: // IA32_APIC_BASE
    case 0x0000003A: // IA32_FEATURE_CONTROL
    case 0x00000079: // IA32_BIOS_UPDT_TRIG
    case 0x00000174: // IA32_SYSENTER_CS
    case 0x00000175: // IA32_SYSENTER_ESP
    case 0x00000176: // IA32_SYSENTER_EIP
    case 0xC0000080: // IA32_EFER
    case 0xC0000081: // IA32_STAR
    case 0xC0000082: // IA32_LSTAR (System call entry RIP - primary BYOVD target)
    case 0xC0000083: // IA32_CSTAR
    case 0xC0000084: // IA32_FMASK
    case 0xC0000100: // IA32_FS_BASE
    case 0xC0000101: // IA32_GS_BASE
    case 0xC0000102: // IA32_KERNEL_GS_BASE
    case 0xC0000103: // IA32_TSC_AUX
        return FALSE;
    default:
        break;
    }

    if (IsWrite) {
        // Only permit writes to safe diagnostic, performance event, and thermal reporting registers
        if ((Msr >= 0x000000C1 && Msr <= 0x000000C8) || // IA32_PMC0 - IA32_PMC7
            (Msr >= 0x00000186 && Msr <= 0x00000189) || // IA32_PERFEVTSEL0 - IA32_PERFEVTSEL3
            (Msr >= 0x00000198 && Msr <= 0x0000019D) || // IA32_PERF_STATUS / IA32_PERF_CTL
            (Msr >= 0x000001A0 && Msr <= 0x000001A4) || // Misc enables / RAPL
            (Msr >= 0x00000606 && Msr <= 0x00000619)) { // RAPL Power / Energy status
            return TRUE;
        }
        return FALSE;
    }

    return TRUE;
}

NTSTATUS StdcReadMsr(PIRP Irp, PIO_STACK_LOCATION irpSp) {
    if (irpSp->Parameters.DeviceIoControl.InputBufferLength < sizeof(ULONG)) {
        return STATUS_BUFFER_TOO_SMALL;
    }
    if (irpSp->Parameters.DeviceIoControl.OutputBufferLength < sizeof(ULONG64)) {
        return STATUS_BUFFER_TOO_SMALL;
    }

    PULONG pMsr = (PULONG)Irp->AssociatedIrp.SystemBuffer;
    PULONG64 pOut = (PULONG64)Irp->AssociatedIrp.SystemBuffer;

    if (!StdcIsSafeMsr(*pMsr, FALSE)) {
        return STATUS_ACCESS_DENIED;
    }

    __try {
        *pOut = StdcReadMsrVal(*pMsr);
        Irp->IoStatus.Information = sizeof(ULONG64);
        return STATUS_SUCCESS;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return GetExceptionCode();
    }
}

NTSTATUS StdcWriteMsr(PIRP Irp, PIO_STACK_LOCATION irpSp) {
    // FIX: Verify proper buffer length (both index and 64-bit value)
    if (irpSp->Parameters.DeviceIoControl.InputBufferLength < sizeof(STDC_MSR_WRITE_REQ)) {
        return STATUS_BUFFER_TOO_SMALL;
    }

    PSTDC_MSR_WRITE_REQ pReq = (PSTDC_MSR_WRITE_REQ)Irp->AssociatedIrp.SystemBuffer;

    // FIX: Enforce MSR whitelist to prevent arbitrary kernel execution hijack
    if (!StdcIsSafeMsr(pReq->MsrIndex, TRUE)) {
        return STATUS_ACCESS_DENIED;
    }

    __try {
        StdcWriteMsrVal(pReq->MsrIndex, pReq->Value);
        Irp->IoStatus.Information = 0;
        return STATUS_SUCCESS;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return GetExceptionCode();
    }
}

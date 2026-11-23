#ifndef _STDCDRV64_H_
#define _STDCDRV64_H_

#include <ntddk.h>

#define STDC_DEVICE_NAME        L"\\Device\\stdcdrv64"
#define STDC_DOS_DEVICE_NAME    L"\\DosDevices\\stdcdrv64"

//
// IOCTL definitions for Intel SelfTest Data Collector (stdcdrv64.sys)
//
#define IOCTL_STDC_READ_MSR             0x222404
#define IOCTL_STDC_READ_PHYSICAL_MEM    0x222408
#define IOCTL_STDC_WRITE_PHYSICAL_MEM   0x22240C
#define IOCTL_STDC_GET_DRIVER_VERSION   0x222410
#define IOCTL_STDC_SET_PROC_AFFINITY    0x222414
#define IOCTL_STDC_GET_PROCESSOR_COUNT  0x222418
#define IOCTL_STDC_WRITE_MSR            0x22241C
#define IOCTL_STDC_READ_PCI_CONFIG      0x222420
#define IOCTL_STDC_WRITE_PCI_CONFIG     0x222424
#define IOCTL_STDC_READ_PORT            0x222428
#define IOCTL_STDC_WRITE_PORT           0x22242C
#define IOCTL_STDC_READ_CONTROL_REG     0x222430
#define IOCTL_STDC_WRITE_CONTROL_REG    0x222434
#define IOCTL_STDC_GET_SYSTEM_AFFINITY  0x222438
#define IOCTL_STDC_GET_GROUP_COUNT      0x22243C

//
// Intrinsics and CPU instruction helpers
//
#if defined(_MSC_VER)
#include <intrin.h>
#define StdcReadMsrVal(msr)           __readmsr(msr)
#define StdcWriteMsrVal(msr, val)     __writemsr((msr), (val))
#define StdcInByte(port)              __inbyte(port)
#define StdcInWord(port)              __inword(port)
#define StdcInDword(port)             __indword(port)
#define StdcOutByte(port, val)        __outbyte((port), (val))
#define StdcOutWord(port, val)        __outword((port), (val))
#define StdcOutDword(port, val)       __outdword((port), (val))
#else
static __inline ULONG64 StdcReadMsrVal(ULONG msr) {
    ULONG64 val = 0;
    __asm__ __volatile__("rdmsr" : "=A"(val) : "c"(msr));
    return val;
}
static __inline void StdcWriteMsrVal(ULONG msr, ULONG64 val) {
    __asm__ __volatile__("wrmsr" : : "c"(msr), "A"(val));
}
static __inline UCHAR StdcInByte(USHORT port) {
    UCHAR val;
    __asm__ __volatile__("inb %w1, %b0" : "=a"(val) : "Nd"(port));
    return val;
}
static __inline USHORT StdcInWord(USHORT port) {
    USHORT val;
    __asm__ __volatile__("inw %w1, %w0" : "=a"(val) : "Nd"(port));
    return val;
}
static __inline ULONG StdcInDword(USHORT port) {
    ULONG val;
    __asm__ __volatile__("inl %w1, %0" : "=a"(val) : "Nd"(port));
    return val;
}
static __inline void StdcOutByte(USHORT port, UCHAR val) {
    __asm__ __volatile__("outb %b0, %w1" : : "a"(val), "Nd"(port));
}
static __inline void StdcOutWord(USHORT port, USHORT val) {
    __asm__ __volatile__("outw %w0, %w1" : : "a"(val), "Nd"(port));
}
static __inline void StdcOutDword(USHORT port, ULONG val) {
    __asm__ __volatile__("outl %0, %w1" : : "a"(val), "Nd"(port));
}
#endif

static __inline ULONG64 StdcReadCr(UCHAR reg) {
    ULONG64 val = 0;
#if defined(_MSC_VER)
    switch (reg) {
    case 0: val = __readcr0(); break;
    case 2: val = __readcr2(); break;
    case 3: val = __readcr3(); break;
    case 4: val = __readcr4(); break;
    case 8: val = __readcr8(); break;
    default: break;
    }
#elif defined(__GNUC__)
    switch (reg) {
    case 0: __asm__ __volatile__("mov %%cr0, %0" : "=r"(val)); break;
    case 2: __asm__ __volatile__("mov %%cr2, %0" : "=r"(val)); break;
    case 3: __asm__ __volatile__("mov %%cr3, %0" : "=r"(val)); break;
    case 4: __asm__ __volatile__("mov %%cr4, %0" : "=r"(val)); break;
    case 8: __asm__ __volatile__("mov %%cr8, %0" : "=r"(val)); break;
    default: break;
    }
#endif
    return val;
}

static __inline void StdcWriteCr(UCHAR reg, ULONG64 val) {
#if defined(_MSC_VER)
    switch (reg) {
    case 0: __writecr0(val); break;
    case 3: __writecr3(val); break;
    case 4: __writecr4(val); break;
    default: break;
    }
#elif defined(__GNUC__)
    switch (reg) {
    case 0: __asm__ __volatile__("mov %0, %%cr0" : : "r"(val)); break;
    case 3: __asm__ __volatile__("mov %0, %%cr3" : : "r"(val)); break;
    case 4: __asm__ __volatile__("mov %0, %%cr4" : : "r"(val)); break;
    default: break;
    }
#endif
}

//
// Request / Response data structures
//
#pragma pack(push, 1)

typedef struct _STDC_PHYS_MEM_REQ {
    UCHAR Mode;        // 0 = byte/stream, 1 = word, 2 = dword, 3 = qword
    ULONG LowPart;     // Physical address low
    ULONG HighPart;    // Physical address high
    ULONG Size;        // Bytes to read/write
    UCHAR Data[1];     // Data payload (for write)
} STDC_PHYS_MEM_REQ, *PSTDC_PHYS_MEM_REQ;

typedef struct _STDC_PCI_REQ {
    UCHAR Mode;        // 0 = byte, 1 = word, 2 = dword
    UCHAR Bus;
    UCHAR Device;      // < 32 (5 bits)
    UCHAR Function;    // < 8 (3 bits)
    ULONG Offset;      // < 256
    ULONG Value;       // (for write)
} STDC_PCI_REQ, *PSTDC_PCI_REQ;

typedef struct _STDC_PORT_REQ {
    UCHAR Mode;        // 0 = byte, 1 = word, 2 = dword
    USHORT Port;
    ULONG Value;       // (for write)
} STDC_PORT_REQ, *PSTDC_PORT_REQ;

typedef struct _STDC_CR_REQ {
    UCHAR RegisterIndex; // 0 = CR0, 2 = CR2, 3 = CR3, 4 = CR4, 8 = CR8
    ULONG64 Value;       // (for write)
} STDC_CR_REQ, *PSTDC_CR_REQ;

typedef struct _STDC_MSR_WRITE_REQ {
    ULONG MsrIndex;
    ULONG Pad[7];        // Original offset 0x20
    ULONG64 Value;
} STDC_MSR_WRITE_REQ, *PSTDC_MSR_WRITE_REQ;

typedef struct _STDC_VERSION_RESP {
    ULONG Major;
    ULONG Minor;
    ULONG Patch;
} STDC_VERSION_RESP, *PSTDC_VERSION_RESP;

#pragma pack(pop)

//
// Module prototypes
//
NTSTATUS StdcReadMsr(PIRP Irp, PIO_STACK_LOCATION irpSp);
NTSTATUS StdcWriteMsr(PIRP Irp, PIO_STACK_LOCATION irpSp);

NTSTATUS StdcReadPhysicalMemory(PIRP Irp, PIO_STACK_LOCATION irpSp);
NTSTATUS StdcWritePhysicalMemory(PIRP Irp, PIO_STACK_LOCATION irpSp);
NTSTATUS StdcReadControlRegister(PIRP Irp, PIO_STACK_LOCATION irpSp);
NTSTATUS StdcWriteControlRegister(PIRP Irp, PIO_STACK_LOCATION irpSp);

NTSTATUS StdcReadPort(PIRP Irp, PIO_STACK_LOCATION irpSp);
NTSTATUS StdcWritePort(PIRP Irp, PIO_STACK_LOCATION irpSp);

NTSTATUS StdcReadPciConfig(PIRP Irp, PIO_STACK_LOCATION irpSp);
NTSTATUS StdcWritePciConfig(PIRP Irp, PIO_STACK_LOCATION irpSp);

NTSTATUS StdcGetDriverVersion(PIRP Irp, PIO_STACK_LOCATION irpSp);
NTSTATUS StdcSetProcAffinity(PIRP Irp, PIO_STACK_LOCATION irpSp);
NTSTATUS StdcGetProcessorCount(PIRP Irp, PIO_STACK_LOCATION irpSp);
NTSTATUS StdcGetSystemAffinity(PIRP Irp, PIO_STACK_LOCATION irpSp);
NTSTATUS StdcGetSystemGroupCount(PIRP Irp, PIO_STACK_LOCATION irpSp);

#endif // _STDCDRV64_H_

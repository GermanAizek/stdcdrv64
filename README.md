# stdcdrv64

stdcdrv64 (stdcdrv64.sys / TNT STDC) - kernel mode driver used by Intel SelfTest Data Collector.

My opensource driver does not use the original source codes and is a reverse-engineered driver using [Ghidra](https://en.wikipedia.org/wiki/Ghidra) and [Radare2](https://en.wikipedia.org/wiki/Radare2). The rights have not been violated, and the new driver cannot be signed due to the EV certificate. I hope that the vulnerabilities can be closed using my security patches.

The main goal of the project is to recreate a secure driver with fixed vulnerabilities and errors made by past developers.

### Functions

Driver stdcdrv64.sys allows diagnostic software to interact with system hardware directly:

- Reading and writing Model-Specific Registers (MSR) via rdmsr / wrmsr
- Physical memory mapping and direct physical memory read / write operations
- Reading and writing CPU Control Registers (CR0, CR2, CR3, CR4, CR8)
- Direct x86 hardware I/O port read and write
- PCI configuration space inspection and modification via ports 0xCF8 / 0xCFC
- Processor group affinity query and thread affinity assignment

Fixed vulnerabilities in opensource driver stdcdrv64.sys:

- [x] (LOLDrivers: stdcdrv64.sys)[https://www.loldrivers.io/drivers/f33c2e80-7b01-416b-821a-ed06db4b6511/]
  - Insufficient Access Control: Device object `\Device\stdcdrv64` opened with world-accessible DACL (fixed using `IoCreateDeviceSecure` with SDDL `D:P(A;;GA;;;SY)(A;;GA;;;BA)`)

Fixed no publish vulnerabilities and bugs which contributors to this project found:

- [x] Device deletion bypassed on symbolic link deletion failure ([view code](main.c))
- [x] Arbitrary Model-Specific Register (MSR) read and write in commands 0x222404, 0x22241C ([view code](msr.c))
- [x] Arbitrary physical memory read and write in commands 0x222408, 0x22240C ([view code](mem.c))
- [x] Arbitrary control register modification (CR0, CR3, CR4) in commands 0x222430, 0x222434 ([view code](mem.c))
- [ ] Unfiltered write to critical x86 I/O ports and controllers in commands 0x222428, 0x22242C ([view code](io.c))
- [ ] Unrestricted PCI configuration modification in commands 0x222420, 0x222424 ([view code](pci.c))
- [x] Device creation with weak permissions ([view code](main.c))

Authors:

- 2026 - Herman Semenoff <<GermanAizek@yandex.ru>> ([germanaizek.github.io](https://germanaizek.github.io))
- Intel Corporation <<secure@intel.com>>

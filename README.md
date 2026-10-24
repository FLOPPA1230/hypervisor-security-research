# Stealth Hypervisor

A bare-metal stealth hypervisor operating at Ring -1 on the x86_64 architecture. Built as a standalone UEFI application to control hardware and conceal its presence from the host operating system.

## Requirements
- Intel processor supporting VT-x (VMX) and EPT technologies.
- Linux build environment (GCC, Binutils, GNU-EFI).
- USB flash drive (FAT32) for physical hardware testing.
- Virtual machine (QEMU) for safe initial testing.

## Installing Dependencies
```bash
sudo apt update
sudo apt install build-essential gnu-efi
```

## Compilation

Ensure that the linker script `elf_x86_64_efi.lds` is present in the project root directory (it can be copied from the `gnu-efi` path), then execute:

```bash
make
```

This will generate an executable named `stealth.efi`. To clean up temporary build files:

```bash
make clean
```

## Booting

1. Format a USB flash drive with the FAT32 file system.
2. Create a directory named `EFI/BOOT` on the flash drive.
3. Copy `stealth.efi` into the directory and rename it to `BOOTX64.EFI`.
4. Boot the machine from the USB drive (ensure Secure Boot is disabled in your BIOS/UEFI settings).
5. You will see the message `"Stealth Hypervisor Active in Ring -1"` displayed on the screen.

## Safe Testing via QEMU

To avoid freezing your physical machine during development, use QEMU:

```bash
qemu-system-x86_64 -enable-kvm -cpu host -bios /usr/share/OVMF/OVMF_CODE.fd -drive format=raw,file=fat:rw:./
```

*(Requires `ovmf` and `qemu-system-x86` packages to be installed).*

## Project Structure

- `include/`: Header files (VMX, EPT, MSR, Hooks, CPU).
- `src/`: Source code modules (Main, VMX, EPT, MSR, Hooks, CPU, Assembly).
- `Makefile`: Build automation.
- `README.md`: Project documentation file.

## Module Breakdown

- **VMX**: Activates VMX mode on the processor and initializes the VMCS structure for guest control.
- **EPT**: Builds Extended Page Tables to enforce memory access permissions.
- **MSR**: Intercepts Model-Specific Registers and spoofs their values to conceal the hypervisor's presence.
- **Hooks**: The stealth engine (EPT Shadowing) which splits memory into dual views (original and shadow).
- **CPU**: Manages multi-core concurrency and initializes host/guest states.

## Security Warning

This tool operates at the kernel and hardware levels. Do not run it on production systems. Use it strictly in isolated virtual machines or on dedicated hardware designated for authorized security testing. The authors assume no liability for any misuse.

## License

MIT License
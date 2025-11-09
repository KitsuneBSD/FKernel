# 🧱 Building FKernel

## Overview

This document explains how to **compile, link, and run** the FKernel operating system from source.  
It assumes you are using a modern Linux-based environment with the necessary build tools installed.

---

## 🧰 Requirements

Before you begin, ensure the following tools are available in your system:

| Tool | Purpose | Package name (Debian/Arch example) |
|------|----------|------------------------------------|
| **xmake** | Build system and build automation | `xmake` |
| **clang / clang++** | C/C++ compiler for freestanding kernel code | `clang` |
| **nasm** | Assembler for x86_64 assembly sources | `nasm` |
| **ld.lld** | Linker (fast and deterministic) | `lld` |
| **qemu-system-x86_64** | Virtual machine emulator | `qemu-system-x86` |
| **grub-mkrescue** | Builds bootable ISO images | `grub-common` / `grub2-common` |
| **lua** | Runs FKernel’s meta-scripts | `lua5.3` or later |

Install all dependencies with:

```bash
# Debian / Ubuntu
sudo apt install xmake clang lld nasm qemu-system-x86 grub-common lua5.3 mtools xorriso dosfstools

# Arch Linux
sudo pacman -S xmake clang lld nasm qemu grub lua mtools xorriso dosfstools
```

## 🧩 Project Structure

A minimal overview of the repository layout:

```
.
├── Config/                 # Build configuration files (linker, GRUB)
├── Docs/                   # Documentation
├── Include/                # Kernel and library headers
├── Src/                    # Source code (C++, C, ASM)
├── Meta/                   # Lua build/run infrastructure
├── build/                  # Output directory (auto-generated)
├── LICENSE
├── README.md
└── xmake.lua               # Root build configuration
```

The kernel build process is completely self-contained — it uses no external libraries or libc.

## ⚙️ Build Configuration (xmake.lua)

FKernel uses xmake to define build rules and architecture targets.
Highlights from the configuration:

- C++20 standard enabled

- Freestanding environment (-ffreestanding, -nostdlib, -nostdinc)

- Custom linker script: Config/linker.ld

- Custom toolchain definition: FKernel_Compiling

- Post-link hook: runs Meta/mounting_mockos.lua to build the ISO

- Run hook: runs Meta/run_mockos.lua to launch QEMU

### Build Modes
| Mode | Description | Flags |
|------|-------------|-------|
| debug |   Includes debug symbols and kernel logging |	-g, -O1, FKERNEL_DEBUG | 
| release   | Optimized build for runtime testing |	-O2, hidden symbols |

## 🧱 Compiling the Kernel

1. Calibrate xmake toolchain with tools installed on system **MANDATORY**

```bash
xmake f --mode=debug --toolchain=FKernel_Compiling
```

2. Build using xmake

```bash
xmake
``` 

This will:

- Assemble and compile all kernel sources (Src/**)

- Link the kernel binary as build/FKernel.bin

- Run the Meta/mounting_mockos.lua script to:

    -   Prepare a GRUB boot structure

    -   Copy the kernel binary

    -   Build a bootable ISO (build/FKernel-MockOS.iso)

    -   Create a test disk image (build/FKernel-HDA.qcow2)

## 💿 Creating the Bootable Image (automated)

The script Meta/mounting_mockos.lua automates the entire ISO generation process.

Steps performed:

- Clean the build/mockos directory

- Copy Config/grub.cfg and FKernel.bin

- Generate a GRUB bootable ISO:

```bash
grub-mkrescue /usr/lib/grub/i386-pc/ -o build/FKernel-MockOS.iso build/mockos
```

### Create a virtual hard disk:

```bash
qemu-img create -f qcow2 build/FKernel-HDA.qcow2 4G
```

If grub-mkrescue is unavailable, the script automatically falls back to grub2-mkrescue.

## 🚀 Running FKernel in QEMU

After compilation, simply run:

```bash
xmake run
```

This will execute Meta/run_mockos.lua, which:

- Verifies QEMU installation

- Automatically rebuilds the ISO if missing

- Launches FKernel with:

```bash
qemu-system-x86_64 \
    -cdrom build/FKernel-MockOS.iso \
    -hda build/FKernel-HDA.qcow2 \
    -m 2G \
    -nographic \
    -serial mon:stdio \
    -smp 2 \
    -boot d
```

You’ll then see the FKernel boot log directly in your terminal.

## 🧹 Cleaning the Build

To remove all generated binaries, use:

```bash
xmake clean
```

This will:

- Delete the build/ directory

- Remove the ISO and disk image

- Prepare the tree for a fresh build

### 🧩 Tips & Notes

To rebuild from scratch:

```bash
rm -rf build && xmake
``` 

To debug inside QEMU with GDB:

```bash
qemu-system-x86_64 -cdrom build/FKernel-MockOS.iso -s -S
```

Then in another terminal:

```bash
gdb build/FKernel.bin
(gdb) target remote localhost:1234
``` 

To change the GRUB boot message or add modules, edit:

Config/grub.cfg

## ✅ Summary

|Step	| Command	| Description|
|-------|-----------|------------|
|Configure build |	xmake f --mode=debug --toolchain=FKernel_Compiling | Optional: ensure correct platform |
|Build kernel	| xmake |	Compiles and links the kernel |
|Create ISO	(automatic) |	Handled by Meta/mounting_mockos.lua |
| Run kernel	| xmake run	| Boots FKernel inside QEMU |
|Clean build	| xmake clean	| Removes build artifacts |

## 🏁 Conclusion

Once you see the early boot logs (MULTIBOOT2, GDT, APIC, etc.),
you’ve successfully compiled and booted FKernel from scratch 🎉.

This setup ensures a fully reproducible and self-contained build flow —
from source to bootable ISO — ideal for low-level kernel development.

## Recent changes (branch: feature/init)

What changed:

- Partition and block-device plumbing: MBR/EBR parsing and a partition-aware block device were added. The ATA driver now registers disk devices and partition nodes (example: `/dev/ada0`, `/dev/ada0p1`).
- Minimal GPT detection was added (protective GPT detection and entry reading).
- A generic memory-map representation (`MemoryMapView`) and early-init adapters (`early_init_from_view`, `early_init_from_uefi`) were introduced to make early boot memory parsing more flexible.

Files touched (high level):

- `Src/Kernel/Block/partition.cpp`, `Include/Kernel/Block/partition.h`
- `Src/Kernel/Block/partition_device.cpp`, `Include/Kernel/Block/partition_device.h`
- `Src/Kernel/Driver/Ata/AtaController.cpp`
- `Include/Kernel/Boot/memory_map.h`, `Include/Kernel/Boot/early_init.h`, `Src/Kernel/Arch/x86_64/Init/early_init.cpp`

How to test the changes quickly:

1. Configure and build (debug mode recommended):

```bash
xmake f --mode=debug --toolchain=FKernel_Compiling
xmake
```

2. Run the built image in QEMU (the existing `xmake run` target will call the run script):

```bash
xmake run
```

3. Watch the serial output. Look for lines mentioning ATA detection and partition registration, for example:

```
Detected ATA device ada0
Registered partition ada0p1 (offset=...)
```

4. (Optional) Inspect the first 512 bytes of the virtual disk to verify the MBR:

```bash
qemu-img convert -O raw build/FKernel-HDA.qcow2 build/FKernel-HDA.raw
hexdump -C -n 512 build/FKernel-HDA.raw
```

Notes / caveats:

- BSD disklabel parsing is currently a stub and will be implemented in a follow-up. GPT support is conservative: protective GPT detection and entry reading are present, but full GUID/attributes handling is incomplete.
- UEFI entrypoint wiring is not yet implemented; `early_init_from_uefi` provides an adapter to consume a generic memory map but a real UEFI boot path remains to be added.

# FKernel

FKernel is an experimental kernel designed to provide a minimal yet functional base
for process management, memory handling and basic hardware interactions.

No frills - just straight to the point.

## Features

- **Cross-Compiling for x86_64**:

  - [Nasm](https://en.wikipedia.org/wiki/Netwide_Assembler)
  - [Lld](https://en.wikipedia.org/wiki/LLVM#Linker)
  - [Clang](https://en.wikipedia.org/wiki/Clang)
  - [Gnu/Make](<https://en.wikipedia.org/wiki/Make_(software)#Variants>)

- **Bootloader:**

  - [Multiboot 1](https://wiki.osdev.org/Multiboot)
  - [Multiboot 2](https://wiki.osdev.org/Multiboot)
  - [UEFI](https://wiki.osdev.org/UEFI)

  - Generic Structure to parse bootloader data.

- **Memory:**

  - Physical Memory Initialization:
    - [ACPI SRAT](https://wiki.osdev.org/SRAT)
    - [UEFI](https://wiki.osdev.org/UEFI)
    - [BIOS](<https://wiki.osdev.org/Detecting_Memory_(x86)>)
  - Paging table configuration and initial heap setup
  - Kernel execution stack configuration

- **Interrupts:**

  - IDT creation with exceptions handling
  - PIC configuration for basic interrupts

- **Processes:**

  - Basic Process Management Structure.
  - Simple Scheduling:
    - [Round-Robin](https://en.wikipedia.org/wiki/Round-robin_scheduling)
    - [Priority-based](<https://en.wikipedia.org/wiki/Scheduling_(computing)>)
  - Basic syscall mechanism for kernel interactions

- **Input/Output**

  - Basic Drivers for keyboard and vídeo.
  - Functions for console output and keyboard input.

- **File System:**

  - Simplified FAT32 implementation.
  - Function to open, read, write files.

- **Task Management:**

  - Process Queue for basic multitasking
  - Background task handling

- **Debugging and Logging:**
  - Basic logging system for Debugging.
  - Fault and exception detection.

## Developer Enviroment Setup

**Required Tools:**

- Nasm
- Lld
- Clang
- Gnu/Make

-> Set up cross-compiling for x86_64

**Testing:**

- Configure Qemu or Bochs for kernel testing

**Build:**

Use Gnu/Make to compile the project.

---

[License](https://github.com/KitsuneBSD/FKernel/blob/main/LICENSE)
[Contribution](https://github.com/KitsuneBSD/FKernel/blob/main/CONTRIBUTING.md)
[Code of Conduct](https://github.com/KitsuneBSD/FKernel/blob/main/CODE_OF_CONDUCT.md)

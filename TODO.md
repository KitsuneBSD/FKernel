# FKernel - List of changes

## Toolchain

- [ ] Create a custom toolchain to FKernel
  - [x] Lua
  - [ ] Clang
  - [x] Nasm
  - [ ] Lld

## FKernel

- [ ] Create a Adapter class to `BlockDevice`

- [x] Create a base class **Hardware Interrupt** to be inherited by ([8259Pic](https://en.wikipedia.org/wiki/Intel_8259) / [APIC](https://en.wikipedia.org/wiki/Advanced_Programmable_Interrupt_Controller) ...)
- [x] Create a base class **Timer** to be inherited by ([PIT](https://en.wikipedia.org/wiki/Programmable_interval_timer), [APIC Timer](https://en.wikipedia.org/wiki/Advanced_Programmable_Interrupt_Controller#APIC_timer) ...)

Use OpTables to configure globally

## LibFK

- [x] Create a class to make [CRTP](https://en.wikipedia.org/wiki/Curiously_recurring_template_pattern)

### FKernel - Core Functionality

- [ ] **Interrupt and Timer Management Refinement:**
  - [x] Implement `HardwareInterrupt` base class (Strategy pattern for PIC/APIC).
  - [x] Implement `Timer` base class (Strategy pattern for PIT/APIC Timer).
  - [ ] Dynamically register/unregister interrupt handlers.
  - [ ] Support for multi-core interrupt distribution (using IOAPIC).

- [ ] **Process and Thread Management:**
  - [ ] Implement basic process control block (PCB) structure.
  - [ ] Implement thread control block (TCB) structure.
  - [ ] Develop a basic scheduler (e.g., round-robin).
  - [ ] Implement context switching mechanism.
  - [ ] Implement process creation (`fork`/`exec`).
  - [ ] Implement thread creation.
  - [ ] Basic inter-process communication (IPC) mechanisms.

- [ ] **System Call Interface:**
  - [ ] Define a system call table.
  - [ ] Implement a system call dispatcher.
  - [ ] Implement basic system calls (e.g., `read`, `write`, `open`, `close`, `exit`).

- [ ] **User Mode Support:**
  - [ ] Implement mechanisms to switch between kernel and user mode.
  - [ ] Load and execute user-mode programs.
  - [ ] Enforce memory protection between user processes and kernel.

- [ ] **Advanced Memory Management:**
  - [ ] Implement kernel heap for dynamic allocations (already has TLSFHeap, but ensure it's robust).
  - [ ] Implement user-space memory allocation (e.g., `sbrk` for processes).

  - [ ] Support for demand paging and swapping (if needed).

- [ ] **File System Enhancements:**
  - [ ] Implement a real disk filesystem (e.g., FAT32, ext2/3/4).
  - [ ] Extend VFS with full POSIX file operations (e.g., `stat`, `link`, `unlink`, `mkdir`, `rmdir`).
  - [ ] Implement file permissions and ownership.

- [ ] **Device Drivers:**
  - [ ] Generic driver framework.
  - [ ] Implement a basic network driver (e.g., for a virtual NIC in QEMU).
  - [ ] Implement a basic USB driver.

### Toolchain

- [ ] Complete custom toolchain:
  - [ ] Clang
  - [ ] Lld

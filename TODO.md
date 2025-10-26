# FKernel - List of changes
 
## Implementation

### General 
- [ ] Increase the number of files but reduce the complexity on 1 unique header
- [ ] The kernel need load with minimum of 64MB

Rule:
    Each Header need be a choose, keep the structure with relatives or separate on a another header multiple structures on FKernel

### Specific

- [ ] LibC:
    - [ ] Apply `Duff-Device` on critical LibC functions

- [ ] LibFK:
    - [ ] Create a `LibFK/Algorithms/Duff-Device`

- [ ] VNode: 
    - [ ] Add a lot of flags to manipulate the files
    - [ ] Rewrite to use the LibFK as base

- [ ] Meta/Test-Images
    - [ ] Rewrite to check the partitions, and boot properly by the hard disk

- [ ] AtaController
    - [ ] Add support to generalist read/write
    - [ ] Add support to `DMA`
    - [ ] Add support to `UDMA`

- [ ] Filesystem
    - [ ] Add support to filesystem `Fat`
    - [ ] Add support to filesystem `Ext2`
    - [ ] Add support to filesystem `UFS`

- [ ] UEFI
    - [ ] Make a complete implementation about how start the kernel by UEFI
    
- [ ] LibFK
    - [ ] All LibFK need use the smart pointers
    - [ ] We need remove the vicious on primitive types and change him to work with class properly
    - [ ] Add more types to structures

- [ ] Partition
    - [ ] Add support to partition `mbr`
    - [ ] Add support to partition `ebr`
    - [ ] Add support to partition `bsd disklabel`
    - [ ] Add support to partition `apm`
    - [ ] Add support to partition `gpt`

## Tests

### 1. Early Init
- [ ] Verify Multiboot2 initialization
- [ ] Validate GDT setup
- [ ] Validate TSS
- [ ] Test enable/disable of interrupts
- [ ] Validate PIC and PIT initialization
- [ ] Check NMI enabling

### 2. Memory Manager
- [ ] Test insertion of physical memory ranges
- [ ] Validate usable vs reserved memory
- [ ] Test virtual memory manager initialization
- [ ] Check APIC mapping
- [ ] Monitor warnings (addresses outside RAM)

### 3. APIC
- [ ] Calibrate timer and measure ticks/ms
- [ ] Test 1ms timer functionality
- [ ] Validate APIC register mapping

### 4. VFS
- [ ] Test root '/' mount
- [ ] Resolve simple paths (`/`, `/dev`)
- [ ] Lookup directories and files
- [ ] Open files non-blocking and verify file descriptors

### 5. DevFS
- [ ] Register basic devices (`null`, `zero`, `ttyS0`, `console`)
- [ ] Open devices multiple times non-blocking
- [ ] Verify unique file descriptors for each device

### 6. ATA Controller
- [ ] Initialize ATA controller
- [ ] Detect primary/secondary drives
- [ ] Register device (`ada0`) in DevFS
- [ ] Test non-blocking sector read

### 7. Console
- [ ] Open `/dev/console` multiple times (stdin/out/err)
- [ ] Test non-blocking read/write
- [ ] Validate open flags and file descriptor assignment

### 8. Integrated Tests (Debug Mode)
- [ ] Run Lua scripts for mount and run MockOS
- [ ] Verify debug logs (colors, detailed messages)
- [ ] Test full boot sequence without blocking
- [ ] Ensure debug mode does not affect release mode

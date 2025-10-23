# FKernel - List of changes
 
## General 
- [ ] Increase the number of files but reduce the complexity on 1 unique header

Rule:
    Each Header need be a choose, keep the structure with relatives or separate on a another header multiple structures on FKernel

## Specific
- [ ] VNode: 
    - [ ] Add a lot of flags to manipulate the files
    - [ ] Rewrite to use the LibFK as base

- [ ] Meta/Test-Images
    - [ ] Rewrite to check the partitions, and boot properly by the hard disk

- [ ] AtaController
    - [ ] Add support to read/write
    - [ ] Add support to DMA
    - [ ] Add support to UDMA

- [ ] Filesystem
    - [ ] Add support to filesystem Fat
    - [ ] Add support to filesystem Ext2
    - [ ] Add support to filesystem Unix Filesystem

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

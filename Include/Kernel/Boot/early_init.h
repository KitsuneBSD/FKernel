#pragma once

#include <Kernel/Boot/multiboot2.h>
#include <Kernel/Boot/memory_map.h>

void early_init(multiboot2::TagMemoryMap const* mmap);

// Generic early init that accepts a MemoryMapView; useful for UEFI boot
// paths where the bootloader provides an EFI memory map instead of
// multiboot2 tags.
void early_init_from_view(MemoryMapView const& view);

// Lightweight adapter for UEFI: caller passes a pointer to the EFI memory
// map and entry count; implementation should translate EFI map entries
// into MemoryMapView and call `early_init_from_view`.
void early_init_from_uefi(void const* efi_mmap, size_t entry_count);

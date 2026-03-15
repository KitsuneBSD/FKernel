#include <Kernel/Loader/Domains/memory_domain.h>
#include <Kernel/Memory/PhysicalMemory/physical_memory_manager.h>
#include <Kernel/Memory/VirtualMemory/virtual_memory_manager.h>
#include <LibFK/Utilities/Memory.h>

namespace fkernel::elf_domains {

MemoryDomain::MemoryDomain(fk::RefPtr<Node> node) : ElfDomain(node) {}

fk::core::Result<void, fk::core::Error>
MemoryDomain::allocate_memory_region(const MemoryRegion& region, bool for_writing) {
  PageFlags flags = for_writing ? get_load_permissions() : region.permissions;

  return map_pages(region.start_vaddr, region.end_vaddr, flags);
}

fk::core::Result<void, fk::core::Error>
MemoryDomain::apply_final_permissions(const MemoryRegion& region) {
  for (uintptr_t vaddr = region.start_vaddr; vaddr < region.end_vaddr; vaddr += 0x1000) {
    auto existing_flags_res = VirtualMemoryManager::the().get_page_flags(vaddr);
    if (existing_flags_res.is_ok()) {
      PageFlags current = existing_flags_res.value();
      PageFlags requested = region.permissions;

      // Merge flags:
      // - If ANY request allows execution (NX bit 0), the result allows execution.
      // - If ANY request is writable, the result is writable.

      uint64_t combined = static_cast<uint64_t>(current) | static_cast<uint64_t>(requested);

      // Special handling for NX (bit 63): it should only be set if BOTH current and requested have
      // it set. (i.e. if either wants execution, clear NX)
      if (!(static_cast<uint64_t>(current) & static_cast<uint64_t>(PageFlags::ExecuteDisable)) ||
          !(static_cast<uint64_t>(requested) & static_cast<uint64_t>(PageFlags::ExecuteDisable))) {
        combined &= ~static_cast<uint64_t>(PageFlags::ExecuteDisable);
      }

      auto remap_res = remap_page_with_permissions(vaddr, static_cast<PageFlags>(combined));
      if (remap_res.is_error())
        return remap_res.error();
    }
  }

  return {};
}

PageFlags MemoryDomain::elf_flags_to_page_flags(uint32_t elf_flags) {
  PageFlags flags = PageFlags::Present | PageFlags::User;

  if (elf_flags & PF_W)
    flags = flags | PageFlags::Writable;
  if (!(elf_flags & PF_X))
    flags = flags | PageFlags::ExecuteDisable;

  return flags;
}

fk::core::Result<void, fk::core::Error>
MemoryDomain::map_pages(uintptr_t start_vaddr, uintptr_t end_vaddr, PageFlags flags) {
  for (uintptr_t vaddr = start_vaddr; vaddr < end_vaddr; vaddr += 0x1000) {
    if (!is_already_mapped(vaddr)) {
      auto map_res = map_single_page(vaddr, flags);
      if (map_res.is_error())
        return map_res.error();
    }
  }
  return {};
}

bool MemoryDomain::is_already_mapped(uintptr_t vaddr) {
  uintptr_t aligned_vaddr = vaddr & ~0xFFFULL;
  uintptr_t existing_phys = VirtualMemoryManager::the().translate(aligned_vaddr);
  return existing_phys != 0;
}

fk::core::Result<void, fk::core::Error> MemoryDomain::map_single_page(uintptr_t vaddr,
                                                                      PageFlags flags) {
  uintptr_t phys = PhysicalMemoryManager::the().alloc_page();
  // Zero out the physical page using its identity mapping (since we are in kernel)
  // or map it temporarily. Assuming phys is identity mapped for simplicity if it's < 1GB
  // or using memset on vaddr AFTER mapping ONLY if it was not mapped.
  // However, if we map it first and THEN memset, we are fine as long as we only do it for NEW
  // mappings.
  VirtualMemoryManager::the().map_page(vaddr, phys, flags);
  fk::memory::set(reinterpret_cast<void*>(vaddr), 0, 0x1000);
  return {};
}

fk::core::Result<void, fk::core::Error> MemoryDomain::remap_page_with_permissions(uintptr_t vaddr,
                                                                                  PageFlags flags) {
  uintptr_t phys = VirtualMemoryManager::the().translate(vaddr);
  if (phys != 0) {
    VirtualMemoryManager::the().map_page(vaddr, phys, flags);
  }
  return {};
}

PageFlags MemoryDomain::get_load_permissions() {
  return PageFlags::Present | PageFlags::User | PageFlags::Writable;
}

PageFlags MemoryDomain::get_final_permissions(uint32_t elf_flags) {
  return elf_flags_to_page_flags(elf_flags);
}

} // namespace fkernel::elf_domains
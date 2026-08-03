#include <Kernel/Loader/Domains/memory_domain.h>
#include <Kernel/Arch/x86_64/Hardware/Cpu/cpu_ops.h>
#include <Kernel/Memory/PhysicalMemory/physical_memory_manager.h>
#include <Kernel/Memory/VirtualMemory/virtual_memory_manager.h>
#include <LibFK/Algorithms/Logging/log.h>
#include <LibFK/Utilities/memory.h>

namespace fkernel::elf_domains {

MemoryDomain::MemoryDomain(fk::RefPtr<Node> node) : ElfDomain(node) {}

fk::core::Result<void, fk::core::Error>
MemoryDomain::allocate_memory_region(const MemoryRegion& region, bool for_writing) {
  PageFlags flags = for_writing ? get_load_permissions() : region.permissions;

  return map_pages(region.start_vaddr, region.end_vaddr, flags);
}

fk::core::Result<void, fk::core::Error>
MemoryDomain::apply_final_permissions(const MemoryRegion& region) {
  uint32_t has_w = (static_cast<uint64_t>(region.permissions) & static_cast<uint64_t>(PageFlags::Writable)) != 0;
  uint32_t has_x = (static_cast<uint64_t>(region.permissions) & static_cast<uint64_t>(PageFlags::ExecuteDisable)) == 0;

  if (has_w && has_x) {
    fk::algorithms::kwarn("ELF", "W^X violation: segment at 0x%lx is both Writable and Executable", region.actual_vaddr);
    return fk::core::Error::PermissionDenied;
  }

  for (uintptr_t vaddr = region.start_vaddr; vaddr < region.end_vaddr; vaddr += 0x1000) {
    auto existing_flags_res = VirtualMemoryManager::the().get_page_flags(vaddr);
    if (existing_flags_res.is_ok()) {
      PageFlags current = existing_flags_res.value();
      PageFlags requested = region.permissions;

      // Merge flags:
      // - If ANY request allows execution (NX bit 0), the result allows execution.
      // - If ANY request is writable, the result is writable.

      uint64_t combined = static_cast<uint64_t>(current) | static_cast<uint64_t>(requested);

      if (!(static_cast<uint64_t>(requested) & static_cast<uint64_t>(PageFlags::Writable)))
        combined &= ~static_cast<uint64_t>(PageFlags::Writable);

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
  auto flags_res = VirtualMemoryManager::the().get_page_flags(aligned_vaddr);
  if (flags_res.is_error())
    return false;
  return static_cast<uint64_t>(flags_res.value()) & static_cast<uint64_t>(PageFlags::User);
}

fk::core::Result<void, fk::core::Error> MemoryDomain::map_single_page(uintptr_t vaddr,
                                                                      PageFlags flags) {
  uintptr_t phys = PhysicalMemoryManager::the().alloc_page();
  if (phys == 0) {
    fk::algorithms::kwarn("DOMAIN", "map_single_page: alloc_page failed at %p", (void*)vaddr);
    return fk::core::Error::OutOfMemory;
  }
  VirtualMemoryManager::the().map_page(vaddr, phys, flags);
  arch_smap_begin();
  fk::memory::set(reinterpret_cast<void*>(vaddr), 0, 0x1000);
  arch_smap_end();
  return {};
}

fk::core::Result<void, fk::core::Error> MemoryDomain::remap_page_with_permissions(uintptr_t vaddr,
                                                                                   PageFlags flags) {
  uintptr_t phys = VirtualMemoryManager::the().translate(vaddr);
  if (phys == 0) {
    fk::algorithms::kwarn("ELF", "remap_page_with_permissions: page not mapped at %p", (void*)vaddr);
    return fk::core::Error::NotFound;
  }
  VirtualMemoryManager::the().map_page(vaddr, phys, flags);
  return {};
}

PageFlags MemoryDomain::get_load_permissions() {
  return PageFlags::Present | PageFlags::User | PageFlags::Writable;
}

PageFlags MemoryDomain::get_final_permissions(uint32_t elf_flags) {
  return elf_flags_to_page_flags(elf_flags);
}

} // namespace fkernel::elf_domains
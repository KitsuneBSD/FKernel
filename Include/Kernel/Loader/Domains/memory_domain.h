#pragma once
#include <Kernel/Loader/elf_types.h>
#include <Kernel/Loader/Domains/Base/elf_domain.h>
#include <Kernel/Loader/Domains/Types/memory_region.h>

namespace fkernel::elf_domains {

class MemoryDomain : public ElfDomain {
public:
    explicit MemoryDomain(fk::RefPtr<Node> node);
    
    fk::core::Result<void, fk::core::Error>
    allocate_memory_region(const MemoryRegion& region, bool for_writing = true);
    
    fk::core::Result<void, fk::core::Error>
    apply_final_permissions(const MemoryRegion& region);
    
    static PageFlags elf_flags_to_page_flags(uint32_t elf_flags);
    
    fk::core::Result<void, fk::core::Error>
    map_pages(uintptr_t start_vaddr, uintptr_t end_vaddr, PageFlags flags);
    
    bool is_already_mapped(uintptr_t vaddr);
    
private:
    fk::core::Result<void, fk::core::Error>
    map_single_page(uintptr_t vaddr, PageFlags flags);
    
    fk::core::Result<void, fk::core::Error>
    remap_page_with_permissions(uintptr_t vaddr, PageFlags flags);
    
    PageFlags get_load_permissions();
    PageFlags get_final_permissions(uint32_t elf_flags);
};

} // namespace fkernel::elf_domains
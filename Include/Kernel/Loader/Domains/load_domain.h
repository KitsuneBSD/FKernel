#pragma once
#include <Kernel/Loader/elf_types.h>
#include <Kernel/Loader/Domains/Base/elf_domain.h>
#include <Kernel/Loader/Domains/Types/memory_region.h>

namespace fkernel::elf_domains {

class LoadDomain : public ElfDomain {
public:
    explicit LoadDomain(fk::RefPtr<Node> node);
    
    fk::core::Result<void, fk::core::Error>
    process_load_segments(const fk::containers::Vector<Elf64_Phdr>& headers, 
                          uintptr_t load_base);
    
    fk::containers::Vector<MemoryRegion> 
    extract_memory_regions(const fk::containers::Vector<Elf64_Phdr>& headers, 
                          uintptr_t load_base);
    
    MemoryRegion calculate_memory_region(const Elf64_Phdr& phdr, uintptr_t load_base);
    
    fk::core::Result<void, fk::core::Error>
    copy_segment_data(const MemoryRegion& region);
    
    void zero_fill_bss(const MemoryRegion& region);
    
    fk::core::Result<bool, fk::core::Error>
    has_load_segments(const fk::containers::Vector<Elf64_Phdr>& headers);
    
private:
    fk::core::Result<void, fk::core::Error>
    process_single_load_segment(const Elf64_Phdr& phdr, uintptr_t load_base);
};

} // namespace fkernel::elf_domains
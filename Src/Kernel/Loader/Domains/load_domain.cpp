#include <Kernel/Loader/Domains/load_domain.h>
#include <Kernel/Loader/Domains/memory_domain.h>
#include <LibFK/Algorithms/log.h>
#include <LibFK/Utilities/Memory.h>

namespace fkernel::elf_domains {

LoadDomain::LoadDomain(fk::RefPtr<Node> node) : ElfDomain(node) {
}

fk::core::Result<void, fk::core::Error>
LoadDomain::process_load_segments(const fk::containers::Vector<Elf64_Phdr>& headers, 
                                  uintptr_t load_base) {
    MemoryDomain memory(m_node);
    
    for (const auto& phdr : headers) {
        if (phdr.p_type == PT_LOAD) {
            fk::algorithms::kdebug("ELF", "Loading segment: VAddr=%p, PAddr=%p, FileSz=%p, MemSz=%p, Flags=%x",
                                   (void*)phdr.p_vaddr, (void*)phdr.p_paddr, (void*)phdr.p_filesz, (void*)phdr.p_memsz, phdr.p_flags);
            auto process_res = process_single_load_segment(phdr, load_base);
            if (process_res.is_error())
                return process_res.error();
        }
    }
    
    return {};
}

fk::containers::Vector<MemoryRegion> 
LoadDomain::extract_memory_regions(const fk::containers::Vector<Elf64_Phdr>& headers, 
                                    uintptr_t load_base) {
    fk::containers::Vector<MemoryRegion> regions;
    
    for (const auto& phdr : headers) {
        if (phdr.p_type == PT_LOAD) {
            regions.push_back(calculate_memory_region(phdr, load_base));
        }
    }
    
    return regions;
}

MemoryRegion LoadDomain::calculate_memory_region(const Elf64_Phdr& phdr, uintptr_t load_base) {
    MemoryRegion region;
    
    region.actual_vaddr = phdr.p_vaddr + load_base;
    region.start_vaddr = region.actual_vaddr & ~0xFFFULL;
    region.end_vaddr = (region.actual_vaddr + phdr.p_memsz + 0xFFF) & ~0xFFFULL;
    region.permissions = MemoryDomain::elf_flags_to_page_flags(phdr.p_flags);
    region.file_offset = phdr.p_offset;
    region.file_size = phdr.p_filesz;
    region.memory_size = phdr.p_memsz;
    
    return region;
}

fk::core::Result<void, fk::core::Error>
LoadDomain::copy_segment_data(const MemoryRegion& region) {
    if (region.file_size == 0) {
        return {};
    }
    
    auto read_res = read_from_node(region.file_offset, region.file_size, 
                                   reinterpret_cast<uint8_t*>(region.actual_vaddr));
    if (read_res.is_error())
        return read_res.error();
    if (read_res.value() < region.file_size)
        return fk::core::Error::InvalidParameter;
    
    return {};
}

void LoadDomain::zero_fill_bss(const MemoryRegion& region) {
    if (region.memory_size > region.file_size) {
        uintptr_t bss_start = region.actual_vaddr + region.file_size;
        size_t bss_size = region.memory_size - region.file_size;
        fk::memory::set(reinterpret_cast<void*>(bss_start), 0, bss_size);
    }
}

fk::core::Result<bool, fk::core::Error>
LoadDomain::has_load_segments(const fk::containers::Vector<Elf64_Phdr>& headers) {
    for (const auto& phdr : headers) {
        if (phdr.p_type == PT_LOAD) {
            return true;
        }
    }
    return false;
}

fk::core::Result<void, fk::core::Error>
LoadDomain::process_single_load_segment(const Elf64_Phdr& phdr, uintptr_t load_base) {
    MemoryDomain memory(m_node);
    MemoryRegion region = calculate_memory_region(phdr, load_base);
    
    auto alloc_res = memory.allocate_memory_region(region, true);
    if (alloc_res.is_error())
        return alloc_res.error();
    
    auto copy_res = copy_segment_data(region);
    if (copy_res.is_error())
        return copy_res.error();
    
    zero_fill_bss(region);
    
    auto perm_res = memory.apply_final_permissions(region);
    if (perm_res.is_error())
        return perm_res.error();
    
    return {};
}

} // namespace fkernel::elf_domains
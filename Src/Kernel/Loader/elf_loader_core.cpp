#include <Kernel/Loader/elf_loader_core.h>
#include <Kernel/Memory/PhysicalMemory/physical_memory_manager.h>
#include <Kernel/Memory/VirtualMemory/virtual_memory_manager.h>
#include <LibFK/Algorithms/log.h>

namespace fkernel {

ElfLoaderCore::ElfLoaderCore(fk::RefPtr<Node> node)
    : m_node(node)
    , m_parser(node)
    , m_interpreter(node)
    , m_loader(node)
    , m_memory(node) {
}

ElfLoadOperationResult ElfLoaderCore::execute_load() {
    return execute_load_with_base(0);
}

ElfLoadOperationResult ElfLoaderCore::execute_load_with_base(uintptr_t load_base) {
    auto init_res = initialize_context(load_base);
    if (init_res.is_error())
        return init_res.error();
    
    auto parse_res = parse_and_validate();
    if (parse_res.is_error())
        return parse_res.error();
    
    auto interp_res = handle_interpreter();
    if (interp_res.is_error())
        return interp_res.error();
    
    auto load_res = load_segments();
    if (load_res.is_error())
        return load_res.error();
    
    return calculate_entry_point();
}

fk::core::Result<void, fk::core::Error> ElfLoaderCore::parse_and_validate() {
    auto header_res = m_parser.validate_header();
    if (header_res.is_error()) {
        fk::algorithms::kerror("ELF", "Invalid ELF header");
        return header_res.error();
    }
    
    m_context.header = header_res.value();
    
    auto headers_res = m_parser.parse_program_headers(m_context.header);
    if (headers_res.is_error())
        return headers_res.error();
    
    m_context.load_base = m_parser.calculate_load_base(m_context.header, m_context.load_base);
    
    auto log_res = m_parser.log_header_info(m_context.header, m_context.load_base);
    if (log_res.is_error())
        return log_res.error();
    
    auto interp_check = m_interpreter.check_interpreter_needed(headers_res.value());
    if (interp_check.is_error())
        return interp_check.error();
    
    m_context.has_interpreter = interp_check.value();
    
    if (m_context.has_interpreter) {
        auto interp_res = m_interpreter.extract_interpreter_path(headers_res.value());
        if (interp_res.is_error())
            return interp_res.error();
        m_context.interpreter_path = interp_res.value();
    }
    
    return {};
}

fk::core::Result<void, fk::core::Error> ElfLoaderCore::handle_interpreter() {
    if (m_context.has_interpreter) {
        auto interp_res = m_interpreter.load_interpreter(m_context.interpreter_path);
        if (interp_res.is_error())
            return interp_res.error();
        m_context.interpreter_entry = interp_res.value();
    } else {
        m_context.interpreter_entry = 0;
    }
    
    return {};
}

fk::core::Result<void, fk::core::Error> ElfLoaderCore::load_segments() {
    auto headers_res = m_parser.parse_program_headers(m_context.header);
    if (headers_res.is_error())
        return headers_res.error();
    
    // Always map the first page of the ELF file (containing the header and PHDRs)
    // Most ELFs have a PT_LOAD that covers this, but we ensure it here just in case.
    uintptr_t first_page_vaddr = m_context.load_base;
    uintptr_t first_page_phys = PhysicalMemoryManager::the().alloc_page();
    VirtualMemoryManager::the().map_page(first_page_vaddr, first_page_phys, 
                                         PageFlags::Present | PageFlags::User | PageFlags::Writable);
    auto read_res = m_node->read(0, 4096, reinterpret_cast<uint8_t*>(first_page_vaddr));
    if (read_res.is_error()) return read_res.error();

    auto load_res = m_loader.process_load_segments(headers_res.value(), m_context.load_base);
    if (load_res.is_error())
        return load_res.error();
    
    return {};
}

ElfLoadOperationResult ElfLoaderCore::calculate_entry_point() {
    uintptr_t entry = 0;
    if (m_context.has_interpreter) {
        entry = m_context.interpreter_entry;
    } else {
        if (m_context.header.e_type == ET_EXEC) {
            entry = (uintptr_t)m_context.header.e_entry;
        } else {
            entry = (uintptr_t)m_context.header.e_entry + m_context.load_base;
        }
    }

    auto headers_res = m_parser.parse_program_headers(m_context.header);
    if (headers_res.is_error())
        return headers_res.error();
    
    auto& headers = headers_res.value();
    uintptr_t ph_addr = 0;
    for (const auto& phdr : headers) {
        if (phdr.p_type == PT_PHDR) {
            ph_addr = phdr.p_vaddr + m_context.load_base;
            break;
        }
    }

    // Fallback: If no PT_PHDR, it usually follows the ELF header
    if (ph_addr == 0) {
        ph_addr = m_context.load_base + m_context.header.e_phoff;
    }

    return ElfLoadResult {
        .entry = entry,
        .ph_addr = ph_addr,
        .ph_num = m_context.header.e_phnum,
        .ph_ent = m_context.header.e_phentsize
    };
}

fk::core::Result<void, fk::core::Error> ElfLoaderCore::initialize_context(uintptr_t load_base) {
    m_context.load_base = load_base;
    m_context.has_interpreter = false;
    m_context.interpreter_entry = 0;
    return {};
}

fk::containers::Vector<elf_domains::MemoryRegion> 
ElfLoaderCore::extract_memory_regions(const fk::containers::Vector<Elf64_Phdr>& headers) {
    return m_loader.extract_memory_regions(headers, m_context.load_base);
}

} // namespace fkernel
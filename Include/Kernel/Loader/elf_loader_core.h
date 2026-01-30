#pragma once

#include <Kernel/Loader/elf_types.h>
#include <Kernel/Loader/Domains/Base/elf_domain.h>
#include <Kernel/Loader/Domains/Types/load_context.h>
#include <Kernel/Loader/Domains/parser_domain.h>
#include <Kernel/Loader/Domains/interpreter_domain.h>
#include <Kernel/Loader/Domains/load_domain.h>
#include <Kernel/Loader/Domains/memory_domain.h>

namespace fkernel {

class ElfLoaderCore {
private:
    fk::RefPtr<Node> m_node;
    elf_domains::LoadContext m_context;
    
    elf_domains::ParserDomain m_parser;
    elf_domains::InterpreterDomain m_interpreter;
    elf_domains::LoadDomain m_loader;
    elf_domains::MemoryDomain m_memory;
    
public:
    explicit ElfLoaderCore(fk::RefPtr<Node> node);
    
    ElfLoadOperationResult execute_load();
    
    ElfLoadOperationResult execute_load_with_base(uintptr_t load_base);
    
private:
    fk::core::Result<void, fk::core::Error> parse_and_validate();
    fk::core::Result<void, fk::core::Error> handle_interpreter();
    fk::core::Result<void, fk::core::Error> load_segments();
    ElfLoadOperationResult calculate_entry_point();
    
    fk::containers::Vector<elf_domains::MemoryRegion> 
    extract_memory_regions(const fk::containers::Vector<Elf64_Phdr>& headers);
    
    fk::core::Result<void, fk::core::Error> initialize_context(uintptr_t load_base);
};

} // namespace fkernel
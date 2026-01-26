#pragma once
#include <Kernel/Loader/elf_types.h>
#include <Kernel/Loader/Domains/Base/elf_domain.h>

namespace fkernel::elf_domains {

class ParserDomain : public ElfDomain {
public:
    explicit ParserDomain(fk::RefPtr<Node> node);
    
    ElfHeaderResult validate_header();
    
    ProgramHeadersResult parse_program_headers(const Elf64_Ehdr& header);
    
    uint16_t identify_executable_type(const Elf64_Ehdr& header);
    
    uintptr_t calculate_load_base(const Elf64_Ehdr& header, uintptr_t preferred_base);
    
    fk::core::Result<void, fk::core::Error> log_header_info(const Elf64_Ehdr& header, uintptr_t load_base);
};

} // namespace fkernel::elf_domains
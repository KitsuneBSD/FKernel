#pragma once
#include <Kernel/Loader/elf_types.h>
#include <Kernel/Loader/Domains/Base/elf_domain.h>

namespace fkernel::elf_domains {

class InterpreterDomain : public ElfDomain {
public:
    explicit InterpreterDomain(fk::RefPtr<Node> node);
    
    fk::core::Result<fk::text::String, fk::core::Error> 
    extract_interpreter_path(const fk::containers::Vector<Elf64_Phdr>& headers);
    
    fk::core::Result<uintptr_t, fk::core::Error> 
    load_interpreter(const fk::text::String& interpreter_path);
    
    fk::core::Result<bool, fk::core::Error>
    check_interpreter_needed(const fk::containers::Vector<Elf64_Phdr>& headers);

    uintptr_t interpreter_base() const { return m_interpreter_base; }
    
private:
    uintptr_t m_interpreter_base{0};

    fk::core::Result<fk::RefPtr<Node>, fk::core::Error>
    resolve_interpreter_path(const fk::text::String& path);
    
    fk::core::Result<void, fk::core::Error>
    log_interpreter_info(const fk::text::String& path);
};

} // namespace fkernel::elf_domains

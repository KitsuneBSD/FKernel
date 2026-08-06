#include <LibFK/Algorithms/Crypto/chacha20.h>
#include <LibFK/Algorithms/Logging/log.h>

#include <Kernel/Loader/Domains/interpreter_domain.h>
#include <Kernel/Loader/Domains/dynamic_domain.h>
#include <Kernel/Loader/Domains/parser_domain.h>
#include <Kernel/Loader/Domains/load_domain.h>
#include <Kernel/Loader/Domains/memory_domain.h>
#include <Kernel/Fs/Vfs/Core/virtual_filesystem.h>
#include <Kernel/Fs/Vfs/Core/dentry.h>

namespace fkernel::elf_domains {

InterpreterDomain::InterpreterDomain(fk::RefPtr<Node> node) : ElfDomain(node) {
}

fk::core::Result<fk::text::String, fk::core::Error> 
InterpreterDomain::extract_interpreter_path(const fk::containers::Vector<Elf64_Phdr>& headers) {
    for (const auto& phdr : headers) {
        if (phdr.p_type == PT_INTERP) {
            char path_buf[256];
            size_t len = phdr.p_filesz < 255 ? phdr.p_filesz : 255;
            
            auto read_res = read_from_node(phdr.p_offset, len, reinterpret_cast<uint8_t*>(path_buf));
            if (read_res.is_error())
                return read_res.error();
            if (read_res.value() < len)
                return fk::core::Error::InvalidParameter;
                
            path_buf[len] = '\0';
            auto log_res = log_interpreter_info(path_buf);
            if (log_res.is_error())
                return log_res.error();
                
            return fk::text::String(path_buf);
        }
    }
    
    return fk::text::String();
}

fk::core::Result<uintptr_t, fk::core::Error> 
InterpreterDomain::load_interpreter(const fk::text::String& interpreter_path) {
    if (interpreter_path.is_empty()) {
        return 0;
    }
    
    auto node_res = resolve_interpreter_path(interpreter_path);
    if (node_res.is_error()) {
        fk::algorithms::kerror("ELF", "Could not find dynamic linker: %s", interpreter_path.c_str());
        return node_res.error();
    }
    
    ParserDomain parser(node_res.value());
    auto header_res = parser.validate_header();
    if (header_res.is_error())
        return header_res.error();
        
    auto headers_res = parser.parse_program_headers(header_res.value());
    if (headers_res.is_error())
        return headers_res.error();
    
    uintptr_t interpreter_base = 0x70000000;
    {
      uint64_t seed;
      fk::algorithms::ChaCha20PRNG::the().fill_buffer(
          reinterpret_cast<uint8_t*>(&seed), sizeof(seed));
      static constexpr uintptr_t ASLR_MIN  = 0x10000000;
      static constexpr size_t    ASLR_RANGE = 0x60000000 / 0x1000;
      uintptr_t offset = ((seed & 0x3FFFFFFFull) % ASLR_RANGE) * 0x1000;
      interpreter_base = ASLR_MIN + offset;
    }
    m_interpreter_base = interpreter_base;
    
    LoadDomain loader(node_res.value());
    auto load_res = loader.process_load_segments(headers_res.value(), interpreter_base);
    if (load_res.is_error())
        return load_res.error();
    
    DynamicDomain dyn_ldr(node_res.value());
    (void)dyn_ldr.apply_relocations(headers_res.value(), fk::VirtualAddress(interpreter_base));
    fk::algorithms::klog("ELF", "ld.so relocations applied at base 0x%lx", interpreter_base);
    
    return reinterpret_cast<uintptr_t>(header_res.value().e_entry + interpreter_base);
}

fk::core::Result<bool, fk::core::Error>
InterpreterDomain::check_interpreter_needed(const fk::containers::Vector<Elf64_Phdr>& headers) {
    for (const auto& phdr : headers) {
        if (phdr.p_type == PT_INTERP) {
            return true;
        }
    }
    return false;
}

fk::core::Result<fk::RefPtr<Node>, fk::core::Error>
InterpreterDomain::resolve_interpreter_path(const fk::text::String& path) {
    auto dentry_res = VirtualFileSystem::the().resolve_path(path.c_str());
    if (dentry_res.is_error()) return dentry_res.error();
    return dentry_res.value()->top_node();
}

fk::core::Result<void, fk::core::Error>
InterpreterDomain::log_interpreter_info(const fk::text::String& path) {
    fk::algorithms::klog("ELF", "Dynamic linker requested: %s", path.c_str());
    return {};
}

} // namespace fkernel::elf_domains

#pragma once

namespace fkernel {
#define EI_NIDENT 16

// Machine types
#define EM_X86_64       62

// Limits
#define ELF_MAX_PHNUM   256

// Standard program header types
#define PT_NULL         0
#define PT_LOAD         1
#define PT_DYNAMIC      2
#define PT_INTERP       3
#define PT_NOTE         4
#define PT_SHLIB        5
#define PT_PHDR         6
#define PT_TLS          7

// ELF flags
#define PF_X            1
#define PF_W            2
#define PF_R            4

// ELF types
#define ET_NONE         0
#define ET_REL          1
#define ET_EXEC         2
#define ET_DYN          3
#define ET_CORE         4

// GNU extensions
#define PT_GNU_STACK    0x6474e551
#define PT_GNU_RELRO    0x6474e552
#define PT_GNU_EH_FRAME 0x6474e550
} // namespace fkernel
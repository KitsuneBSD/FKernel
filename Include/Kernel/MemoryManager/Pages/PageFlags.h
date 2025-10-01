#pragma once
#include <LibC/stdint.h>

enum PageFlags : uint64_t {
    // Presença
    Present         = 1ULL << 0,   // Página está presente na memória
    Writable        = 1ULL << 1,   // Página é gravável
    User            = 1ULL << 2,   // Usuário pode acessar
    WriteThrough    = 1ULL << 3,   // Write-through caching
    CacheDisabled   = 1ULL << 4,   // Cache desabilitado
    Accessed        = 1ULL << 5,   // Página foi acessada
    Dirty           = 1ULL << 6,   // Página foi modificada
    HugePage        = 1ULL << 7,   // Página de 2 MiB ou 1 GiB
    Global          = 1ULL << 8,   // Página global (não invalida TLB)
    ExecuteDisable  = 1ULL << 63   // NX bit, se suportado
};

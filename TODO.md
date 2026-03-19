# FKernel - Plano de Melhorias

> Gerado em 2026-03-19

## Sumário Executivo

**Status Atual**: O FKernel possui arquitetura sólida, build funcional e testes passando, mas precisa de melhorias em:
- Bugs críticos de memória e scheduler
- Funcionalidades POSIX essenciais
- Segurança (SMEP/SMAP, NX bit)
- Infraestrutura de drivers (USB, async I/O)
- Networking stack completo

**Progresso Atual**: ~40-50% (build, testes, storage I/O funcionais)
**Prioridade Imediata**: Corrigir bugs críticos (Fase 1)

---

---

## Fase -1: LibFK - STL-like Library

### -1.1 Files Structure

```
Src/LibFK/
├── Algorithms/     (CRC32, DJB2)
├── Container/      (Vector, List, HashMap, etc.)
├── Memory/        (OwnPtr, RefPtr, etc.)
├── Terminal/      (ANSI parser)
├── Text/          (String, StringBuilder, etc.)
├── Utilities/     (Pair, Aligner, Archive)
└── cxxabi.cpp

Include/LibFK/
├── Algorithms/
├── Container/
├── Core/          (Result, Error, Assertions)
├── Functional/    (Function, Tuple)
├── Memory/
├── Synchronization/ (Spinlock)
├── Terminal/
├── Text/
├── Traits/       (type_traits, CRTP)
├── Tree/         (Red-Black Tree)
├── Types/         (ProcessId, VirtualAddress, etc.)
└── Utilities/
```

### -1.2 Bugs Críticos - Containers

| # | Problema | Arquivo | Solução | Prioridade |
|---|---------|---------|---------|------------|
| 1 | `List::push_front()` usa `m_head` em vez de `m_metadata` | `list.h:88` | Corrigir para `m_metadata.m_head` | CRITICAL |
| 2 | `IntrusiveList::remove()` recursão infinita | `intrusive_list.h:45` | `remove(&obj)` chama a si mesmo | CRITICAL |
| 3 | `HashMap` não faz resize | `hash_map.h:95` | Implementar rehashing quando cheio | CRITICAL |
| 4 | `Stack::push()` off-by-one | `stack.h:27` | `m_stack[++top_index]` → `m_stack[top_index++]` | CRITICAL |
| 5 | `Bitmap::alloc()` só first-fit | `bitmap.h` | Adicionar best-fit/next-fit | MEDIUM |
| 6 | `CircularBuffer::dequeue()` retorna por cópia | `circular_buffer.h:30` | Retornar por move | MEDIUM |

### -1.3 Bugs Críticos - Core/Memory

| # | Problema | Arquivo | Solução | Prioridade |
|---|---------|---------|---------|------------|
| 1 | `ASSERT()` não definido | `Assertions.h` | Definir macro ASSERT | CRITICAL |
| 2 | `RefCounted::~RefCounted()` não virtual | `RefCounted.h` | Adicionar `virtual` ao destrutor | CRITICAL |
| 3 | `optional::value()` retorna `nullptr` | `optional.h:139` | ASSERT em vez de retornar nullptr | CRITICAL |
| 4 | `Result::value()` assertions comentadas | `Result.h:39` | Descomentar assertions | HIGH |
| 5 | `RetainPtr` truncagem `size_t` vs `uint32_t` | `RetainPtr.h:280` | Usar `size_t` consistente | HIGH |
| 6 | `optional::value()` não retorna nada se vazio | `optional.h:125` | UB - não retorna valor | CRITICAL |

### -1.4 Bugs Críticos - Text

| # | Problema | Arquivo | Solução | Prioridade |
|---|---------|---------|---------|------------|
| 1 | `String::operator=` ignora erros | `string.cpp:95` | Propagar erros de alocação | CRITICAL |
| 2 | `String::append()` ignora erros | `string.cpp:148` | Retornar Result<void, Error> | CRITICAL |
| 3 | `StringBuilder::append_decimal()` buffer overflow | `string_builder.cpp:31` | Buffer 22 bytes para INT64_MIN | CRITICAL |
| 4 | `String::push_back()` falha silenciosa | `string.cpp:158` | Retornar bool ou Result | MEDIUM |
| 5 | `FixedString::assign()` truncagem silenciosa | `fixed_string.h:56` | Retornar bool indicando truncagem | MEDIUM |

### -1.5 Bugs Críticos - Sync/Functional

| # | Problema | Arquivo | Solução | Prioridade |
|---|---------|---------|---------|------------|
| 1 | `Spinlock::try_lock()` race condition | `spinlock.h:43` | Usar CAS atômico | CRITICAL |
| 2 | `Function::invoke()` usa `move()` incorretamente | `Function.h:24` | Usar `std::forward` | CRITICAL |
| 3 | `ANSI_parser` variáveis estáticas RGB | `ansi_parser.h:101` | Mover para estado da instância | HIGH |
| 4 | `InterruptDisabler` sem kernel guard | `interrupt_disabler.h` | Adicionar `#ifdef __fkernel__` | MEDIUM |
| 5 | `Spinlock` inclui interrupt_controller | `spinlock.h:5` | Forward declare para early boot | MEDIUM |

### -1.6 Bugs Críticos - Tree/Algorithms

| # | Problema | Arquivo | Solução | Prioridade |
|---|---------|---------|---------|------------|
| 1 | `rb_tree::fix_remove()` null pointer | `rb_tree.h:244` | Adicionar `if (!w)` check | CRITICAL |
| 2 | `rb_tree::set_left()` encapsulation violation | `rb_tree.h:42` | Usar `l->set_parent(this)` | HIGH |
| 3 | `TarParser::octal_to_int()` sem implementação | `tar.h:30` | Implementar função | CRITICAL |
| 4 | `rb_tree::transplant()` parent link issue | `rb_tree.h:79` | Corrigir atualização de parent | MEDIUM |

### -1.7 Features Faltantes - Containers

| Container | Prioridade |
|-----------|------------|
| `deque<T>` | HIGH |
| `set<T>` | HIGH |
| `map<K,V>` | HIGH |
| `multiset<T>` | MEDIUM |
| `multimap<K,V>` | MEDIUM |
| `priority_queue<T>` | MEDIUM |
| `forward_list<T>` | LOW |
| `unordered_set<T>` | MEDIUM |

### -1.8 Features Faltantes - Text

```
String: substr(), find(), rfind(), replace(), insert(), erase(),
        starts_with(), ends_with(), contains(), trim(), to_upper(), to_lower()

StringView: substr(), find(), rfind(), remove_prefix(), remove_suffix(),
            front(), back(), starts_with(), ends_with()

StringBuilder: append_hex(), append_binary(), append_octal(), append_float()
```

### -1.9 Features Faltantes - Core

| Feature | Prioridade |
|---------|------------|
| `ASSERT()` definição | CRITICAL |
| `enable_if`, `remove_pointer`, `is_pointer` | HIGH |
| `is_floating_point`, `is_signed`, `conditional` | HIGH |
| `Tuple` default ctor, move semantics, tuple_size | MEDIUM |
| `Pair` move assignment | LOW |
| Math: `abs()`, `swap()`, `pow()`, `sqrt()` | HIGH |

### -1.10 API Inconsistências

| Container | `is_empty()` vs `empty()` |
|-----------|---------------------------|
| Vector | `is_empty()` ✅ |
| List | `empty()` ❌ |
| Stack | (nenhum) ❌ |
| Queue | `is_empty()` ✅ |
| CircularBuffer | `is_empty()` ✅ |

### -1.11 Métricas LibFK

| Categoria | Bugs Críticos | Features Faltando |
|-----------|--------------|-------------------|
| Container | 6 | ~8 |
| Core/Memory | 6 | ~15 |
| Text | 5 | ~25 |
| Sync/Functional | 5 | ~5 |
| Tree/Algorithms | 4 | ~8 |
| **Total** | **~26** | **~61** |

---

## Fase 0: LibC - Biblioteca C Freestanding

### 0.1 Files Structure

```
Src/LibC/
├── string/           (20 arquivos - funções de string)
├── stdio/            (4 arquivos - printf/format)
├── assert.c
└── ctype.c

Include/LibC/         (13 headers)
```

### 0.2 Bugs Críticos - String Functions

| # | Problema | Arquivo | Solução | Prioridade |
|---|---------|---------|---------|------------|
| 1 | `strcmp()` ineficiente (O(3n)) | `strcmp.c:7-8` | Single-pass comparison | CRITICAL |
| 2 | `strcat()` sem bounds checking | `strcat.c` | Buffer overflow risk | CRITICAL |
| 3 | `strcpy()` sem bounds checking | `strcpy.c` | Buffer overflow risk | CRITICAL |
| 4 | `strnlen()` ASSERT incorreto | `strnlen.c:9` | `maxlen=0` causa crash | CRITICAL |
| 5 | `atoi()` crash em entrada inválida | `atoi.c` | Retornar 0 em vez de ASSERT | CRITICAL |
| 6 | `strchr/strrchr` não padrão | `strchr.c`, `strrchr.c` | Parâmetro extra `maxlen` não padrão | HIGH |
| 7 | `strtok()` lógica POSIX incorreta | `strtok.c` | Busca delimiter como string | HIGH |
| 8 | `memcpy` sem NULL assertions | `memcpy.c` | Adicionar NULL checks | MEDIUM |
| 9 | `memset/memcmp` sem NULL checks | `memset.c`, `memcmp.c` | Adicionar assertions | MEDIUM |

### 0.3 Bugs Críticos - Stdio

| # | Problema | Arquivo | Solução | Prioridade |
|---|---------|---------|---------|------------|
| 1 | Retorno `vsnprintf` incorreto | `vsnprintf.c:133` | Retornar "would have written" | CRITICAL |
| 2 | Violação de camada (C++ em LibC) | `libc_putc.cpp:13` | Remover Spinlock, usar primitives | CRITICAL |
| 3 | Dependência de Kernel na LibC | `libc_putc.cpp:37` | `MemoryManager::the()` viola freestanding | HIGH |
| 4 | Sem suporte a float | `vsnprintf.c` | Implementar `%f`, `%e`, `%g` | HIGH |

### 0.4 Headers Faltantes

| Header | Prioridade | Descrição |
|--------|------------|-----------|
| `errno.h` | CRITICAL | Definições de erros (~120 constantes) |
| `fcntl.h` | CRITICAL | Flags O_* (O_CREAT, O_RDWR, etc.) |
| `sys/stat.h` | CRITICAL | Estruturas stat (já em Kernel/Posix) |
| `dirent.h` | HIGH | Entradas de diretório |
| `ctype.h` | HIGH | Funções de classificação (~27 faltando) |
| `float.h` | HIGH | Limites de ponto flutuante |
| `wchar.h` | MEDIUM | Suporte a caracteres largos |
| `signal.h` | MEDIUM | Manipulação de sinais |
| `time.h` | MEDIUM | Funções de tempo |
| `termios.h` | LOW | Controle de terminal |
| `pthread.h` | LOW | Suporte a threads |
| `alloca.h` | MEDIUM | Alocação em stack |
| `strings.h` | MEDIUM | bcopy, bzero, index, rindex |

### 0.5 Funções Faltantes - ctype.h

```c
// Implementado apenas: isxdigit(), isdigit(), toupper()

// Faltando (~27):
isalnum() isalpha() isblank() iscntrl() isgraph()
islower() isprint() ispunct() isspace() isupper()
tolower()
```

### 0.6 Funções Faltantes - string.h

```c
// Implementado (~20):
strlen, strcpy, strncpy, strcmp, strncmp, strcat, strchr,
strrchr, strnchr, strnlen, strtok_r, memcpy, memmove, memset,
memcmp, atoi, stol, itoa, ultoa

// Faltando (~20):
strdup()       strndup()     strerror()    strstr()
strcasecmp()   strncasecmp() strcoll()     strxfrm()
memchr()       memccpy()     strpbrk()     strspn()
strcspn()      ffs()         stpcpy()      stpncpy()
strncat()      strtok() (padrão, não _r)
```

### 0.7 Funções Faltantes - stdio.h

```c
// Implementado (4):
snprintf(), vsnprintf(), kprintf(), libc_puts()

// Faltando (~66):
printf(), fprintf(), vprintf(), vfprintf()
fopen(), fclose(), fread(), fwrite(), fgets(), fputs()
gets(), putchar(), getchar(), scanf(), sscanf()
FILE (tipo), stdin, stdout, stderr
feof(), ferror(), fflush(), rewind()
fseek(), ftell(), rewind()
```

### 0.8 Funções Faltantes - stdlib.h

```c
// Implementado (4):
atoi(), strtol(), strtoul(), abort()

// Faltando (~56):
strtod(), strtof(), strtold()
strtoll(), strtoull(), strtoimax(), strtouimax()
abs(), labs(), llabs(), div(), ldiv(), lldiv()
bsearch(), qsort()
mblen(), mbtowc(), wctomb(), mbstowcs(), wcstombs()
getenv(), putenv(), setenv(), unsetenv(), system()
mkstemp(), realpath()
exit(), atexit(), getenv()
calloc(), realloc() (já devem existir?)
```

### 0.9 Tipos Faltantes em stddef.h

```c
// Implementado:
NULL, size_t, ptrdiff_t, wchar_t (parcial)

// Faltando:
FILE (stdio)
fpos_t, wint_t
clock_t, time_t, clockid_t, timer_t
rlim_t, blksize_t, blkcnt_t, nlink_t
mode_t, useconds_t, suseconds_t
```

### 0.10 Constantes POSIX Faltantes

```c
// Access modes (em unistd.h):
F_OK, R_OK, W_OK, X_OK

// File types:
S_ISBLK(), S_ISFIFO(), S_ISSOCK()

// Exit codes:
EXIT_SUCCESS, EXIT_FAILURE

// Random:
RAND_MAX, MB_CUR_MAX

// Limits:
CHAR_MIN, CHAR_MAX, LONG_MIN, LONG_MAX
NAME_MAX, PATH_MAX, PIPE_BUF
```

---

## Fase 1: Bugs Críticos e Segurança

### 1.1 Memory - Bugs Críticos

| # | Problema | Arquivo | Solução | Prioridade |
|---|---------|---------|---------|------------|
| 1 | Heap não faz merge com blocos anteriores | `memory_manager.cpp:162` | Implementar backward merging | CRITICAL |
| 2 | Bitmap/buddy dessincronizados | `physical_memory_manager.cpp:218` | Corrigir sincronização | CRITICAL |
| 3 | `get_pte()` ignora parâmetro `create` | `virtual_memory_manager.cpp:271` | Corrigir lógica | CRITICAL |
| 4 | IOMMU é stub | `vtd.cpp` | Implementar ou remover | HIGH |
| 5 | Sem page fault handler para mmap | `virtual_memory_manager.cpp` | Implementar PF handler | HIGH |

### 1.2 Scheduler - Bugs Críticos

| # | Problema | Arquivo | Solução | Prioridade |
|---|---------|---------|---------|------------|
| 1 | `sleep()` usa busy-wait | `tick_manager.cpp:6` | Usar scheduler blocking | CRITICAL |
| 2 | Sleep queue processada só na CPU 0 | `SchedulerLifecycle.cpp:163` | Corrigir para SMP | CRITICAL |
| 3 | PID overflow (`UINT64_MAX`) | `scheduler.cpp:36` | Tratar overflow | MEDIUM |
| 4 | Task state inconsistente (Zombie vs Blocked) | `SchedulerLifecycle.cpp:17` | Usar enum correto | MEDIUM |
| 5 | Prioridade não é usada no scheduling | `SchedulerManager.cpp:40` | Implementar priority queue | HIGH |

### 1.3 VFS - Bugs Críticos

| # | Problema | Arquivo | Solução | Prioridade |
|---|---------|---------|---------|------------|
| 1 | `open()` não trata `O_CREAT` | `vfs_operations.cpp:9` | Implementar criação de arquivos | CRITICAL |
| 2 | Race condition em `FileDescription::read/write` | `file_description.cpp` | Atomic offset update | CRITICAL |
| 3 | Lock ordering em `rename()` | `vfs_operations.cpp:122` | Evitar deadlock | HIGH |
| 4 | Missing `truncate()`/`fsync()` | VFS layer | Implementar | HIGH |

### 1.4 Containers - Bugs Críticos

| # | Problema | Arquivo | Solução | Prioridade |
|---|---------|---------|---------|------------|
| 1 | `List::push_front()` usa `m_head` em vez de `m_metadata` | `list.h:88` | Corrigir uso de metadata | CRITICAL |
| 2 | `IntrusiveList::remove()` recursão infinita | `intrusive_list.h:45` | Corrigir shadowing | CRITICAL |
| 3 | `HashMap` não faz resize | `hash_map.h:95` | Implementar rehashing | CRITICAL |
| 4 | `Stack::push()` off-by-one | `stack.h:23` | Corrigir índice | CRITICAL |

### 1.5 Security - Arquitetura x86_64

| # | Problema | Solução | Prioridade |
|---|---------|---------|------------|
| 1 | NX bit não configurado em page tables | `setup_page_tables.asm` - adicionar NX flag | CRITICAL |
| 2 | SMEP desabilitado | Habilitar CR4.SMEP | CRITICAL |
| 3 | SMAP desabilitado | Habilitar CR4.SMAP | CRITICAL |
| 4 | Sem SSE/AVX context save | Implementar FXSAVE/FXRSTOR em `context_switch.asm` | HIGH |
| 5 | Signal handler address não validado | `signal_delivery.cpp:80` - verificar user space | CRITICAL |
| 6 | MFENCE após INVLPG incorreto | `invalid_tlb.asm` - usar SFENCE ou remover | MEDIUM |

---

## Fase 2: Funcionalidades POSIX Essenciais

### 2.1 Syscalls Faltantes

| Categoria | Syscalls a Implementar | Prioridade |
|-----------|----------------------|------------|
| **FileSystem** | `truncate`, `ftruncate`, `fsync`, `fdatasync`, `flock` | HIGH |
| **Memory** | `mprotect`, `munmap` (completar), `madvise`, `mlock` | MEDIUM |
| **Signals** | `sigreturn` (completar), `sigaltstack`, `sigpending` | HIGH |
| **Time** | `clock_getres`, `clock_settime`, `setitimer`, `getitimer` | MEDIUM |
| **Networking** | `getsockopt`, `setsockopt`, `socketpair` | HIGH |

### 2.2 POSIX Headers Faltantes

```
<sys/wait.h>         - wait() macros
<sys/resource.h>     - rlimit, rusage
<sys/times.h>        - tms structure
<sys/statvfs.h>      - statvfs structure
<sys/socket.h>       - socket options
<netinet/in.h>       - IP addresses
<netinet/tcp.h>      - TCP options
<poll.h>             - poll()
<glob.h>             - glob()
<regex.h>            - regex
<dlfcn.h>            - dlopen
<termios.h>          - terminal control
<utmpx.h>            - extended accounting
<sys/msg.h>          - POSIX msg queues
<sys/sem.h>          - POSIX semaphores
<sys/shm.h>          - POSIX shared memory
```

### 2.3 POSIX Fixes

| Problema | Arquivo | Solução |
|----------|---------|---------|
| errno incompleto | `sys/errno.h` | Adicionar ~80 valores faltantes |
| Timestamps dummy em stat | `fstat.cpp:36` | Implementar timestamps reais |
| Macros S_IS* incompletos | `sys/stat.h` | Adicionar S_ISBLK, S_ISFIFO, S_ISSOCK |
| Time inconsistente | `time.cpp` vs `gettimeofday.cpp` | Unificar lógica de leap years |

---

## Fase 3: Infraestrutura de Drivers

### 3.1 Storage - Completar Async I/O

| Driver | Status | Ação |
|--------|--------|------|
| NVMe (polling) | Implementado | Completar `interrupt_driven_nvme.cpp` |
| AHCI | Implementado | Completar `interrupt_driven_ahci.cpp` |
| Storage Cache | Implementado | Suportar 4KB blocks, write-back, LRU eviction |
| NVMe Identify | Implementado (bug) | `nvme_controller.cpp:299` - parse real size |

### 3.2 USB Stack (CRÍTICO)

```
Src/Kernel/Driver/Usb/
├── xHCI/
│   ├── xhci_controller.h/cpp     - Host controller driver
│   ├── xhci_registers.h          - Register definitions
│   ├── xhci_command.cpp          - Command ring
│   ├── xhci_transfer.cpp         - Transfer ring
│   └── xhci_interrupt.cpp        - Interrupt handling
├── UsbDevice/
│   ├── usb_device.h/cpp           - Device abstraction
│   └── usb_descriptor.cpp        - Descriptor parsing
├── UsbHub/
│   └── usb_hub.cpp               - Hub detection
└── UsbHid/
    ├── usb_hid.h/cpp             - HID base class
    └── usb_keyboard.cpp          - USB keyboard driver
```

### 3.3 Drivers Faltantes

| Driver | Prioridade |
|--------|------------|
| PS/2 Mouse | MEDIUM |
| Serial Terminal | MEDIUM |
| Pseudo-Terminal (PTY) | MEDIUM |
| virtio-net | LOW |
| VESA framebuffer melhorado | MEDIUM |

### 3.4 Driver Architecture Issues

| Issue | Solução |
|-------|---------|
| Classes duplicadas NVMe/AHCI | Unificar em classe única com flag async |
| Serial não integrado ao VFS | Registrar com DriverManager |
| Device naming inconsistente | Unificar esquema (sdX, ttyX) |

---

## Fase 4: Networking Stack

### 4.1 Implementar Protocolos (Do zero)

```
Src/Kernel/Net/
├── Buffer/
│   ├── packet_buffer.h            - mbuf-like abstraction
│   └── buffer_pool.h             - Pre-allocated buffers
├── Protocols/
│   ├── Ethernet/
│   │   ├── ethernet_frame.h
│   │   └── ethernet_protocol.cpp
│   ├── Arp/
│   │   ├── arp_header.h
│   │   └── arp_protocol.cpp
│   ├── Ipv4/
│   │   ├── ipv4_header.h
│   │   ├── ipv4_protocol.cpp
│   │   └── ipv4_fragmentation.cpp
│   ├── Tcp/
│   │   ├── tcp_header.h
│   │   ├── tcp_protocol.cpp
│   │   ├── tcp_state_machine.cpp
│   │   └── tcp_congestion.cpp
│   └── Udp/
│       ├── udp_header.h
│       └── udp_protocol.cpp
├── InetSocket/
│   ├── tcp_socket.h/cpp
│   ├── udp_socket.h/cpp
│   └── inet_socket_manager.cpp
├── NetworkManager/
│   ├── network_manager.h/cpp
│   └── interface.cpp
└── Utilities/
    ├── checksum.h
    └── ip_address.h
```

### 4.2 Socket Syscalls a Implementar

| Syscall | Prioridade |
|---------|------------|
| `sys_send()`, `sys_recv()` | HIGH |
| `sys_sendto()`, `sys_recvfrom()` | HIGH |
| `sys_shutdown()` | HIGH |
| `sys_getpeername()`, `sys_getsockname()` | MEDIUM |

### 4.3 Features Adicionais

| Feature | Prioridade |
|---------|------------|
| DHCP Client | HIGH |
| DNS Resolver | MEDIUM |
| ICMP/Ping | MEDIUM |
| Routing Table | MEDIUM |

---

## Fase 5: ELF Loader - Completar Linking

### 5.1 Features Críticas

| Feature | Status | Prioridade |
|---------|--------|------------|
| PT_DYNAMIC processing | ❌ | HIGH |
| Symbol resolution (PLT/GOT) | ❌ | HIGH |
| TLS (PT_TLS) | ❌ | MEDIUM |
| ASLR | ❌ | HIGH |
| RELRO (Partial/Full) | ❌ | MEDIUM |
| ET_REL loading | ❌ | LOW |

### 5.2 Validações de Segurança

| Validação | Adicionar |
|-----------|-----------|
| `e_machine == EM_X86_64` | `parser_domain.cpp` |
| Bounds checking em `e_phoff/e_phnum` | `parser_domain.cpp` |
| Validação de interpreter path | `interpreter_domain.cpp` |
| PT_GNU_STACK enforcement | `memory_domain.cpp` |

### 5.3 Issues Conhecidos

| Issue | Arquivo | Solução |
|-------|---------|---------|
| ASLR não implementado | `elf_loader_core.cpp:55` | Randomizar load base |
| RELRO ignorado | `memory_domain.cpp` | Processar PT_GNU_RELRO |
| Hardcoded page zeroing | `memory_domain.cpp:85` | Verificar bounds |

---

## Fase 6: IPC - Fortalecer Modelo Capability

### 6.1 Issues de Segurança

| Issue | Solução |
|-------|---------|
| Sem capability rights | Adicionar rights bitmask `{send, receive, manage}` |
| CSpace O(n) lookup | Free list para O(1) allocation |
| Signal handler não validado | Verificar user address em `signal_delivery.cpp` |
| Endpoint TOCTOU race | Atomic compare-exchange matching |

### 6.2 Features Faltantes

| Feature | Prioridade |
|---------|------------|
| Capability transfer (`cspace_insert`) | HIGH |
| Capability revocation | HIGH |
| Large message via SHM | MEDIUM |
| Timeout em send/receive | MEDIUM |
| `sigreturn` completo | HIGH |
| SA_RESTART flag | MEDIUM |

### 6.3 Shared Memory IPC

| Component | Status |
|-----------|--------|
| `CapabilityType::SharedMemory` | Definido mas não usado |
| `shmget`, `shmat`, `shmctl` | ❌ |
| Memory mapping into address space | ❌ |

---

## Fase 7: ACPI/Hardware - Completar

### 7.1 ACPI Tables

| Table | Status | Ação |
|-------|--------|------|
| FADT | Parcial | Completar campos ACPI 6.x |
| DSDT/SSDT | ❌ | Implementar AML interpreter |
| HPET | Header only | Completar timer, integrar ao TimerManager |
| DMAR | ❌ | IOMMU/VT-d setup |
| SRAT | Parcial | NUMA affinity integration |
| MCFG | Parcial | Completar PCIe config |

### 7.2 CPU Features a Detectar

```
HIGH PRIORITY:
- SSE4.1/4.2, AVX, AVX2, AVX-512
- AES-NI, XSAVE, PCLMUL, F16C
- BMI1/BMI2, RDRAND, RDSEED
- FSGSBASE, PCID, UMIP

MEDIUM PRIORITY:
- VMX/SVM (virtualização)
- MPX, PKRU (memory protection)
- CET/IBT (control-flow integrity)
- TSX (transactional memory)
```

### 7.3 PCI Enhancements

| Feature | Status |
|---------|--------|
| BAR reading/writing | ❌ Implementar |
| MSI-X | Parcial |
| PCIe AER | ❌ |
| PCIe ASPM | ❌ |

### 7.4 APIC/Multicore

| Issue | Solução |
|-------|---------|
| IOAPIC address hardcoded | Parse MADT entries |
| MSI dest hardcoded (0xFEE00000) | Ler LAPIC base do MSR |
| APIC only single-core | Per-CPU GDT/TSS |

---

## Fase 8: Refatoração Object Calisthenics

### 8.1 Classes a Refatorar (Violações)

| Classe | Issue | Lines/Variables |
|--------|-------|-----------------|
| `Dentry` | 5 instance vars | max 2 |
| `MemoryManager` | 7 instance vars | max 2 |
| `UnixSocket` | 11 instance vars | max 2 |
| `VirtualFileSystem` | OK (2 vars) | - |
| `PhysicalZone` | 6 instance vars | max 2 |
| `BuddyState` | 3 instance vars | borderline |

### 8.2 Type Wrappers a Criar

```cpp
// Em Include/LibFK/Types/
physical_address.h    - PhysicalAddress wrapper
virtual_address.h     - VirtualAddress wrapper  
buddy_order.h         - BuddyOrder (em vez de raw size_t)
frame_index.h         - FrameIndex
file_offset.h         - FileOffset (em vez de uint64_t)
file_flags.h          - FileFlags (em vez de int)
process_id.h          - ProcessId
thread_id.h           - ThreadId
signal_number.h       - SignalNumber
```

### 8.3 Refatorações Específicas

| Local | Issue | Solução |
|-------|-------|---------|
| `munmap()` | 5 níveis de indentação | Extrair `RegionSplitter` class |
| `resolve_path()` | 6 níveis de indentação | Extrair métodos |
| `pick_next()` | Prioridade ignorada | Implementar priority queue |
| `select_zone()` | ELSE chains | Early returns |
| `get_page_flags()` | Sem lock | Adicionar `ScopedLockIRQ` |
| `find_task()` | Sem lock | Adicionar lock protection |

### 8.4 Dead Code a Remover

| Arquivo | Issue |
|---------|-------|
| `TaskQueueCollection` | Nunca usado |
| `InterruptDrivenNvmeController` | Sem implementação |
| `InterruptDrivenAhciController` | Sem implementação |
| `Src/Kernel/Loader/Domains/elf_domain.cpp` | Duplicado de `Base/elf_domain.cpp` |

---

## Resumo de Esforço

| Fase | Complexity | Impact | Tempo Estimado |
|------|------------|--------|-----------------|
| **-1 - LibFK** | Low | Crítico | 1-2 semanas |
| **0 - LibC** | Low-Medium | Crítico | 2-4 semanas |
| 1 - Bugs Críticos | Low | Alto | 1-2 semanas |
| 2 - POSIX Essencial | Medium | Alto | 2-4 semanas |
| 3 - Drivers USB | High | Crítico | 4-8 semanas |
| 4 - Networking | Very High | Alto | 8-12 semanas |
| 5 - ELF Loader | Medium | Alto | 2-4 semanas |
| 6 - IPC Security | Medium | Alto | 2-3 semanas |
| 7 - ACPI/Hardware | Medium | Médio | 3-4 semanas |
| 8 - Refatoração | Low | Médio | Contínuo |

---

## Prioridade de Execução Recomendada

1. **Imediato**: Fase -1 - LibFK (ASSERT não definido, RefCounted slicing)
2. **Imediato**: Fase 0 - LibC (corrige bugs + adiciona headers críticos)
3. **Imediato**: Fase 1 - Bugs Críticos Kernel (evita crashes e segurança)
4. **Curto prazo**: Fases 2, 6 - POSIX + IPC Security (usabilidade)
5. **Médio prazo**: Fases 5, 7 - ELF Loader + ACPI (estabilidade)
6. **Longo prazo**: Fases 3, 4 - USB + Networking (funcionalidade)
7. **Contínuo**: Fase 8 - Refatoração (qualidade de código)

---

## Métricas Atuais

| Componente | Files | Bugs Críticos | Missing Features |
|------------|-------|--------------|-----------------|
| **LibFK** | ~65 | ~26 | ~61 |
| **LibC** | ~30 | 9 | ~180 |
| Memory | ~15 | 5 | 3 |
| Scheduler | ~12 | 4 | 5 |
| VFS | ~15 | 4 | 6 |
| Containers Kernel | ~12 | 4 | 10+ |
| Drivers | ~53 | 3 | 8+ |
| Networking | ~5 | 1 | 15+ |
| ELF Loader | ~12 | 2 | 6+ |
| IPC | ~8 | 3 | 5+ |
| Syscall | ~83 | 2 | 20+ |
| **Total** | **~310** | **~62** | **~320** |

### LibC Compliance Summary

| Category | Standard | Implemented | Missing |
|----------|----------|-------------|---------|
| ctype functions | ~30 | 3 | ~27 |
| string functions | ~40 | ~20 | ~20 |
| stdio functions | ~70 | 4 | ~66 |
| stdlib functions | ~60 | 4 | ~56 |
| unistd functions | ~120 | 7 | ~113 |
| **Total functions** | **~320** | **~38 (12%)** | **~282** |

---

## Progresso por Categoria

### Fase -1: LibFK ⚠️ 60%

- [x] Containers básicos (Vector, List, HashMap, Stack)
- [x] Smart pointers (OwnPtr, RefPtr)
- [x] String e StringBuilder
- [x] CRC32/DJB2 hash
- [x] Red-Black tree (parcial)
- [ ] ASSERT() definição (CRITICAL)
- [ ] Corrigir bugs críticos (RefCounted, List, Stack)
- [ ] Containers avançados (deque, set, map)
- [ ] Text features (substr, find, replace)
- [ ] Type traits completos

### Fase 0: LibC ⚠️ 15%

- [x] String functions básicas (strlen, memcpy, etc.)
- [x] vsnprintf com formatação básica
- [ ] Headers POSIX completos (errno, fcntl, ctype, etc.)
- [ ] stdio completo (FILE, fopen, etc.)
- [ ] stdlib completo (strtod, getenv, etc.)
- [ ] Corrigir bugs críticos

### Fase 1: Infraestrutura ✅ COMPLETA

- [x] Build system funcional
- [x] Boot em QEMU
- [x] Test framework
- [x] CI/CD pipeline

### Fase 2: Storage ⚠️ 85%

- [x] StorageDevice interface
- [x] ATA implementation
- [x] AHCI controller (polling)
- [x] NVMe controller (polling)
- [ ] Interrupt-driven I/O
- [ ] NCQ support (AHCI)
- [ ] Multi-queue I/O (NVMe)

### Fase 3: Network ⚠️ 40%

- [x] NetworkDevice interface
- [x] E1000 driver
- [ ] TCP/IP stack (IPv4, TCP, UDP, ARP, ICMP)
- [ ] Socket extensions (AF_INET)
- [ ] TCP sliding window, congestion control

### Fase 4: Qualidade ❌ BLOQUEADO

- [ ] Corrigir classes >200 linhas
- [ ] Eliminar 92+ violações "else {"
- [ ] Corrigir 122 violações method chaining
- [ ] Padronizar diretórios PascalCase

---

## Referências

- [AGENTS.md](./AGENTS.md) - Convenções de desenvolvimento
- [README.md](./README.md) - Build system
- [Docs/](./Docs/) - Documentação detalhada por domínio

---

*Atualizado: Março 2026 com análise completa do codebase (LibFK + LibC)*

# FKernel Real Hardware Drivers - TODO List

## Status Atual (Atualizado: Janeiro 2026)

- **Alta Prioridade**: 60% completo (3/5 totalmente concluídos)
- **Média Prioridade**: 40% completo (4/10 parcialmente concluídos)  
- **Baixa Prioridade**: 0% completo (0/6 iniciados)
- **Progresso Total**: ~40% implementado

## Visão Geral

Transformar FKernel de QEMU-centric para suporte de hardware real, **estendendo abstrações existentes** ao invés de criar HAL separado. Esta abordagem mantém coerência com a arquitetura BSD/XNU inspirada e segue os princípios do projeto.

### Estratégia

- **Extender** abstrações HAL-like existentes (HardwareInterruptManager, TimerManager, etc.)
- **Aproveitar** interfaces Device/BlockDevice/StorageDevice
- **Manter** Strategy Pattern e Object Calisthenics
- **Integrar** com PciManager, MemoryManager, IPC system existentes

---

## High Priority - Infraestrutura Crítica

### 4. Estender Storage Abstraction - ⚠️ **PARCIALMENTE CONCLUÍDO (75%)**

**Status**: Framework implementado, controles inicializados, I/O operations básicas implementadas em AHCI/NVMe
**Descrição**: AHCI/SATA/NVMe como novas implementações de StorageDevice interface, mantendo fallback ATA->DMA->UDMA
**Arquivos Chave**:

- ✅ `Include/Kernel/Driver/Storage/storage_device.h` (base interface existente)
- ✅ `Src/Kernel/Driver/Storage/Ata/` (implementação ATA completa)
- ✅ `Src/Kernel/Driver/Storage/Ahci/ahci_controller.cpp` (I/O operations básicas implementadas via DMA)
- ✅ `Src/Kernel/Driver/Storage/Nvme/nvme_controller.cpp` (I/O operations básicas implementadas via Admin/IO queues)
**Dependências**: PCI device discovery, DMA abstraction, interrupt management
**Integração**:

```cpp
// ✅ Storage hierarchy implementada com base em StorageDevice
class AHCIController : public StorageDevice {
    // ✅ I/O operations implementadas (polling DMA)
    Result<size_t, Error> read_sectors(uint64_t start, size_t count, uint8_t* buffer) override;
};

class NVMeController : public StorageDevice {
    // ✅ I/O operations implementadas (polling Queues)
    Result<size_t, Error> read_sectors(uint64_t start, size_t count, uint8_t* buffer) override;
};
```

**Validação**: ✅ Compilação bem-sucedida, I/O framework funcional
**Próximo**: Migrar polling para interrupções em AHCI/NVMe controllers

### 5. Criar Network Device Interface - ✅ **CONCLUÍDO**

**Status**: Interface implementada e driver E1000 funcional integrado
**Descrição**: Nova classe NetworkDevice seguindo padrão BlockDevice para drivers NIC (e1000, rtl8139)
**Arquivos Chave**:

- ✅ `Include/Kernel/Driver/Network/network_device.h` (interface implementada)
- ✅ `Include/Kernel/Driver/Network/mac_address.h` (estrutura MAC implementada)
- ✅ `Src/Kernel/Driver/Network/E1000/` (driver E1000 implementado)
- ✅ `Src/Kernel/Syscall/SyscallList/Networking/socket.cpp` (socket API inicial integrada)
**Dependências**: PCI device management, interrupt handling, memory management
**Integração**:

```cpp
// ✅ Network device interface implementada
class NetworkDevice : public CharacterDevice {
public:
    virtual Result<void, Error> send_packet(const uint8_t* data, size_t size) = 0;
    virtual Result<size_t, Error> receive_packet(uint8_t* buffer, size_t max_size) = 0;
    virtual MACAddress mac_address() const = 0;
};
```

**Validação**: ✅ Compilação bem-sucedida, driver E1000 registra e expõe eth0 no DevFs

### 6. Implementar Unix Domain Sockets - ⚠️ **PARCIALMENTE CONCLUÍDO (60%)**

**Status**: Infraestrutura base e syscalls iniciais implementadas
**Descrição**: Suporte a AF_UNIX para comunicação local eficiente entre processos
**Arquivos Chave**:

- ✅ `Include/Kernel/Net/socket.h` (classe base Socket)
- ✅ `Include/Kernel/Net/unix_socket.h` (implementação UnixSocket)
- ✅ `Src/Kernel/Net/unix_socket.cpp` (lógica de bind/connect/listen/accept)
- ✅ `Src/Kernel/Syscall/SyscallList/Networking/` (syscalls socket, bind, connect, listen, accept integradas)
**Dependências**: VFS integration, spinlocks, physical memory management
**Integração**:

```cpp
// ✅ UnixSocket implementado com buffers circulares
class UnixSocket : public Socket {
    // Implementação de bind(path), connect(path), etc.
};
```

**Validação**: ✅ Compilação bem-sucedida, syscalls registradas e funcionais para AF_UNIX
**Próximo**: Implementar bloqueio (wait queues) para accept/read/write em sockets

### 21. Melhorar Multiboot2 Framebuffer Detection - ❌ **NÃO INICIADO**

**Descrição**: Modificar multiboot2 header para inferir automaticamente a melhor resolução suportada pelo hardware
**Arquivos Chave**:

- ❌ `Src/Kernel/Arch/x86_64/Section/multiboot2.asm` (atualmente com valores fixos)
- ✅ `Include/Kernel/Boot/Multiboot/multiboot2.h` (estruturas TagFramebuffer existentes)
- ✅ `Src/Kernel/Boot/boot_info.cpp` (parse de framebuffer info implementado)
**Dependências**: Multiboot2 specification compliance, GRUB2 bootloader integration
**Integração**:

```cpp
// ❌ Atualmente valores fixos no assembly
dd 1024    ; Width (hardcoded)
dd 768     ; Height (hardcoded)

// ✅ Deveria usar auto-detection
dd 0       ; Width (0 = auto-detect best resolution)
dd 0       ; Height (0 = auto-detect best resolution)
```

**Validação**: ❌ Ainda usando valores fixos, pode causar "no suitable video mode" em hardware real

### 7. Implementar DAL Framework - ⚠️ **BASE IMPLEMENTADA (30%)**

**Status**: IPC/capabilities existem, mas falta interface DAL específica
**Descrição**: Usar IPC/capability system existente para comunicação userspace drivers com kernel
**Arquivos Chave**:

- ✅ `Include/Kernel/Ipc/endpoint.h` (IPC existente)
- ✅ `Include/Kernel/Ipc/capability.h` (capability system implementado)
- ✅ `Src/Kernel/Ipc/` (implementação IPC completa)
- ❌ `Src/Kernel/Syscall/` (falta syscall interface específica para DAL)
**Dependências**: Complete IPC system, security model, FFI layer
**Integração**:

```cpp
// ⚠️ Base IPC pronto, falta DAL communication layer
class DALDriver {
    Endpoint m_kernel_endpoint;      // ✅ Existente
    Capability m_device_capability; // ✅ Existente
public:
    Result<void, Error> register_device(DeviceType type);
    Result<void, Error> request_io(uint16_t port, size_t size);
    Result<void, Error> map_memory(PhysicalAddress phys, size_t size);
};
```

**Validação**: ❌ Falta userspace driver loading e device access control

### 7. Extender HardwareInterruptManager - ⚠️ **PARCIALMENTE CONCLUÍDO (75%)**

**Status**: MSI allocation e basic configuration implementados para APIC/IOAPIC/x2APIC
**Descrição**: Adicionar MSI/MSI-X support e interrupt sharing na arquitetura existente
**Arquivos Chave**:

- ✅ `Src/Kernel/Arch/x86_64/Interrupt/interrupt_controller.cpp` (implementação básica existente)
- ✅ `Include/Kernel/Arch/x86_64/Interrupt/hardware_interrupt.h` (extensão MSI adicionada)
- ✅ `Src/Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/InterruptController/` (MSI implementado em APIC, IOAPIC e x2APIC)
- ✅ `Include/Kernel/Hardware/Pci/pci_device.h` (config space accessors e capability discovery)
**Dependências**: PCI MSI capability detection, interrupt routing
**Integração**:

```cpp
// ✅ HardwareInterrupt interface extendida
class HardwareInterrupt {
public:
    // ...
    virtual Result<uint8_t, Error> allocate_msi_vector(const PciDevice& device) override;
    virtual void enable_msi(uint8_t vector) override;
    virtual void disable_msi(uint8_t vector) override;
};
```

**Validação**: ✅ Compilação bem-sucedida, suporte a alocação de vetores MSI e configuração de PCI devices
**Próximo**: Implementar suporte a MSI-X e interrupt sharing (shared handlers list)

### 8. Criar USB Host Controller Interface - ❌ **NÃO INICIADO**

**Status**: Diretório USB completamente ausente
**Descrição**: Nova abstração seguindo padrão HardwareInterruptManager/TimerManager
**Arquivos Chave**:

- ❌ `Include/Kernel/Driver/Usb/usb_host_controller.h` (interface não existe)
- ❌ `Src/Kernel/Driver/Usb/Ehci/` (diretório não existe)
- ❌ `Src/Kernel/Driver/Usb/Xhci/` (diretório não existe)
- ❌ `Src/Kernel/Driver/Usb/Ohci/` (diretório não existe)
**Dependências**: PCI device discovery, interrupt management, DMA support
**Integração**:

```cpp
// ❌ USB host controller não implementado
class USBHostController {
public:
    virtual Result<void, Error> initialize() = 0;
    virtual Result<USBDevice*, Error> enumerate_device(uint8_t port) = 0;
    virtual Result<void, Error> submit_transfer(USBTransfer* transfer) = 0;
};
```

**Validação**: ❌ Nenhuma funcionalidade USB implementada

### 9. Estender Display Abstraction - ⚠️ **PARCIALMENTE CONCLUÍDO (75%)**

**Status**: VESA VBE 2.0+ implementado com enumeração de modos e GDT infrastructure
**Descrição**: VESA/VBE como nova implementação além do VGA framebuffer existente
**Arquivos Chave**:

- ✅ `Include/Kernel/Driver/Vga/display.h` (VGA existente)
- ✅ `Src/Kernel/Arch/x86_64/Driver/Vga/vesa.cpp` (Enumeração dinâmica de modos VBE implementada)
- ✅ `Src/Kernel/Arch/x86_64/Segments/gdt.cpp` (Adicionados segmentos de 16-bit e 32-bit para BIOS bridge)
- ✅ `Src/Kernel/Arch/x86_64/Driver/Vga/bios_int10h.cpp` (Melhorada lógica de fallback e detecção VBE)
- ⚠️ `Src/Kernel/Arch/x86_64/Driver/Vga/real_mode_bridge.asm` (Bridge protótipo para real mode interrupts)
**Dependências**: VESA BIOS calls, framebuffer management, mode switching
**Integração**:

```cpp
// ✅ Display abstraction extendida para VESA Real
class VESADriver {
public:
    void discover_vbe(); // ✅ Enumera modos suportados pelo hardware
    Result<void, Error> set_resolution(uint32_t w, uint32_t h, uint32_t bpp); // ✅ Busca modo compatível
};
```

**Validação**: ✅ Compilação bem-sucedida, suporte a detecção de múltiplos modos VBE
**Próximo**: Estabilizar real-mode bridge para troca de modos em tempo real

### 10. Enhance Memory Manager - ✅ **CONCLUÍDO**

**Status**: Memory manager agora é topology-aware (NUMA nodes via SRAT)
**Descrição**: Adicionar topology-aware às zones existentes (DMA, Kernel, Userspace)
**Arquivos Chave**:

- ✅ `Include/Kernel/Hardware/Acpi/srat.h` (Estruturas SRAT implementadas)
- ✅ `Include/Kernel/Hardware/Acpi/topology_manager.h` (Interface do TopologyManager)
- ✅ `Src/Kernel/Hardware/Acpi/topology_manager.cpp` (Parsing de SRAT e mapeamento de nós)
- ✅ `Src/Kernel/Memory/PhysicalMemory/physical_memory_manager.cpp` (Alocação física com preferência de nó)
- ✅ `Src/Kernel/Memory/memory_manager.cpp` (Interface de alto nível integrada)
**Dependências**: ACPI SRAT table parsing, CPU topology detection
**Integração**:

```cpp
// ✅ Memory manager agora suporta preferência de nó NUMA
uintptr_t allocate_page(ZoneType preferred = ZoneType::NORMAL, uint32_t preferred_node = 0);
```

**Validação**: ✅ Compilação bem-sucedida, parsing de SRAT funcional em boot log

### 11. Implementar Hotplug Device Detection - ✅ **CONCLUÍDO**

**Status**: PciManager estendido com suporte a detecção dinâmica e callbacks
**Descrição**: Estender PciManager para detectar dispositivos dinamicamente
**Arquivos Chave**:

- ✅ `Src/Kernel/Hardware/Pci/pci.cpp` (hotplug detection e event handling)
- ✅ `Include/Kernel/Hardware/Pci/pci.h` (interface de callbacks e novos métodos)
- ✅ `Include/Kernel/Hardware/Pci/pci_address.h` (adição de operador de igualdade)
**Dependências**: PCI configuration space access, driver registry
**Integração**:

```cpp
// ✅ PciManager estendido para hotplug
class PciManager {
public:
    void enable_hotplug_detection();
    void register_hotplug_callback(HotplugCallback callback);
    void scan_for_new_devices();
private:
    void handle_hotplug_event(uint8_t bus, uint8_t device, uint8_t function);
};
```

**Validação**: ✅ Compilação bem-sucedida, lógica de inserção/remoção implementada com notificação de callbacks

### 12. Adicionar Power Management

**Descrição**: ACPI states integrados com HardwareInterruptManager/TimerManager existentes
**Arquivos Chave**:

- `Src/Kernel/Arch/x86_64/Acpi/` (ACPI implementation)
- `Src/Kernel/Power/` (new power management)
- `Src/Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/TimerController/` (timer integration)
**Dependências**: ACPI table parsing, device power states, sleep states
**Integração**:

```cpp
// Power management integrated with existing systems
class PowerManager {
public:
    Result<void, Error> enter_sleep_state(SleepState state);
    Result<void, Error> set_device_power_state(const Device& device, PowerState state);
    void register_wake_source(WakeSource source);
};
```

**Validação**: S3/S4 sleep states, wake-on-LAN, device power management

### 13. Implementar DMA Abstraction Layer - ⚠️ **PARCIALMENTE CONCLUÍDO (25%)**

**Status**: ATA-specific DMA existe, mas falta camada genérica para todos drivers
**Descrição**: Genérica DMA management integrada com MemoryManager zones
**Arquivos Chave**:

- ❌ `Include/Kernel/Memory/dma_manager.h` (interface genérica não existe)
- ❌ `Src/Kernel/Memory/dma_manager.cpp` (implementação genérica ausente)
- ⚠️ `Src/Kernel/Driver/Storage/Ata/dma_strategy.cpp` (DMA apenas ATA-specific)
**Dependências**: IOMMU support, memory zones, scatter-gather lists
**Integração**:

```cpp
// ⚠️ DMA abstraction - falta camada genérica
class DMAManager {
public:
    Result<DMABuffer, Error> allocate_buffer(size_t size, DMAConstraints constraints); // ❌ Genérico faltando
    Result<void, Error> map_device_memory(PhysicalAddress phys, size_t size);          // ❌ Genérico faltando
    void free_buffer(DMABuffer buffer);                                               // ❌ Genérico faltando
};
```

**Validação**: ❌ Apenas ATA DMA implementado, falta abstração genérica

### 14. Implementar Hotplug Device Detection

**Descrição**: Estender PciManager para detectar dispositivos dinamicamente
**Arquivos Chave**:

- `Src/Kernel/Hardware/Pci/pci_manager.cpp` (hotplug support)
- `Src/Kernel/Arch/x86_64/Interrupt/` (PCI interrupt handling)
- `Src/Kernel/Driver/` (dynamic driver loading)
**Dependências**: PCI hotplug controller support, interrupt management, driver framework
**Integração**:

```cpp
// Extend PciManager for hotplug
class PciManager {
public:
    void enable_hotplug_detection();
    void register_hotplug_callback(HotplugCallback callback);
    void scan_for_new_devices();
private:
    void handle_hotplug_event(uint8_t bus, uint8_t device, uint8_t function);
};
```

**Validação**: Dynamic device insertion/removal, driver loading/unloading

---

## Low Priority - Recursos Avançados

### 15. Criar Audio Device Interface - ❌ **NÃO INICIADO**

**Status**: Nenhuma implementação de áudio encontrada
**Descrição**: Seguindo padrão CharacterDevice existente para HDA/AC97
**Arquivos Chave**:

- ❌ `Include/Kernel/Driver/Audio/audio_device.h` (interface não existe)
- ❌ `Src/Kernel/Driver/Audio/Hda/` (diretório não existe)
- ❌ `Src/Kernel/Driver/Audio/Ac97/` (diretório não existe)
**Dependências**: PCI device management, interrupt handling, DMA support
**Integração**:

```cpp
// ❌ Audio device não implementado
class AudioDevice : public CharacterDevice {
public:
    virtual Result<void, Error> set_sample_rate(uint32_t rate) = 0;
    virtual Result<void, Error> set_buffer_size(size_t size) = 0;
    virtual Result<void, Error> start_playback() = 0;
    virtual Result<void, Error> stop_playback() = 0;
};
```

**Validação**: ❌ Nenhuma funcionalidade de áudio implementada

### 16. Estender SMP Support - ❌ **NÃO INICIADO**

**Status**: Sistema ainda single-CPU, sem suporte multi-processor
**Descrição**: Multi-processor awareness usando HardwareInterruptManager base
**Arquivos Chave**:

- ❌ `Src/Kernel/Arch/x86_64/Cpu/` (falta SMP initialization)
- ❌ `Src/Kernel/Arch/x86_64/Interrupt/` (falta per-CPU interrupt handling)
- ❌ `Src/Kernel/Scheduler/` (falta multi-CPU scheduling)
**Dependências**: ACPI MADT table, per-CPU data structures, load balancing
**Integração**:

```cpp
// ❌ SMP support não implementado
class SMPManager {
public:
    Result<void, Error> initialize_cpus();
    void schedule_on_cpu(CPUId cpu, Thread* thread);
    void send_ipi(CPUId target_cpu, IPIType type);
};
```

**Validação**: ❌ Multi-CPU boot não suportado

### 17. Port BSD Security Features - ❌ **NÃO INICIADO**

**Status**: Nenhuma implementação de segurança BSD encontrada
**Descrição**: pledge(), unveil() integrados com capability system existente
**Arquivos Chave**:

- ❌ `Src/Kernel/Security/` (diretório não existe)
- ❌ `Src/Kernel/Syscall/` (falta security syscalls)
- ⚠️ `Include/Kernel/Ipc/capability.h` (capabilities existem mas não extendidas)
**Dependências**: Capability system, syscall filtering, process isolation
**Integração**:

```cpp
// ❌ BSD security features não implementadas
class SecurityManager {
public:
    Result<void, Error> apply_pledge(Process* process, PledgeMask promises);
    Result<void, Error> apply_unveil(Process* process, const fk::String& path, UnveilPermissions perms);
    bool check_permission(const Process& process, Operation op, const Resource& resource);
};
```

**Validação**: ❌ Nenhuma política de segurança implementada

### 18. Implementar NUMA Zones - ❌ **NÃO INICIADO**

**Status**: Memory manager ainda não é NUMA-aware
**Descrição**: Extender zone-based memory manager para topologia NUMA
**Arquivos Chave**:

- ❌ `Src/Kernel/Memory/physical_memory_manager.cpp` (falta NUMA zones)
- ❌ `Src/Kernel/Arch/x86_64/Acpi/` (falta SRAT table parsing)
- ❌ `Include/Kernel/Memory/numa_manager.h` (interface não existe)
**Dependências**: ACPI SRAT table, CPU topology, memory locality
**Integração**:

```cpp
// ❌ NUMA zones não implementadas
class NUMAManager {
    fk::Vector<NUMANode> m_nodes;
public:
    Result<PhysicalPage, Error> allocate_local_page(NUMANodeId node);
    NUMANodeId get_preferred_node() const;
    void migrate_page(PhysicalPage page, NUMANodeId target_node);
};
```

**Validação**: ❌ NUMA system não suportado

### 19. Add IOMMU Support - ⚠️ **PARCIALMENTE CONCLUÍDO (50%)**

**Status**: Interface base e inicialização Intel VT-d (DMAR) implementadas
**Descrição**: DMA protection integrado com MemoryManager abstractions
**Arquivos Chave**:

- ✅ `Include/Kernel/Memory/iommu.h` (interface genérica implementada)
- ✅ `Include/Kernel/Arch/x86_64/Memory/IntelIOMMU/vtd.h` (Estruturas Intel VT-d)
- ✅ `Src/Kernel/Arch/x86_64/Memory/IntelIOMMU/vtd.cpp` (Parsing de DMAR e inicialização Root Table)
- ✅ `Include/Kernel/Hardware/Acpi/dmar.h` (Estruturas ACPI DMAR)
**Dependências**: PCI device identification, DMA remapping, interrupt remapping
**Integração**:

```cpp
// ✅ Interface IOMMU integrada ao MemoryManager
class IOMMU {
public:
    virtual Result<void, Error> create_domain(DomainId id) = 0;
    virtual Result<void, Error> map_device(DomainId domain, const PciDevice& device) = 0;
};
```

**Validação**: ✅ Compilação bem-sucedida, detecção de tabela DMAR em boot log
**Próximo**: Implementar gerenciamento de domínios e tabelas de páginas de Segundo Nível (SLPT)

### 20. Create IPUK Framework - ❌ **NÃO INICIADO**

**Status**: Nenhuma implementação IPUK encontrada
**Descrição**: Usando VFS/capability system existentes para isolamento
**Arquivos Chave**:

- ❌ `Src/Kernel/IPUK/` (diretório não existe)
- ✅ `Src/Kernel/Vfs/` (VFS integration existente)
- ✅ `Include/Kernel/Ipc/capability.h` (capability-based isolation existente)
**Dependências**: VFS virtualization, capability system, process isolation
**Integração**:

```cpp
// ❌ IPUK framework não implementado
class IPUKManager {
public:
    Result<IPUKInstance*, Error> create_instance(const IPUKConfig& config);
    Result<void, Error> mount_virtual_filesystem(IPUKInstance* instance, const fk::String& path);
    Result<void, Error> grant_capability(IPUKInstance* instance, Capability cap);
};
```

**Validação**: ❌ Application isolation não implementada

---

## Referências e Links

### Documentação do Projeto

- [AGENTS.md](./AGENTS.md) - Convenções de desenvolvimento e padrões
- [README.md](./README.md) - Build system e instruções
- [Docs/](./Docs/) - Documentação técnica detalhada

### Padrões e Inspirações

- **BSD/XNU**: Device driver architecture, VFS design
- **SerenityOS**: C++ kernel patterns, Object Calisthenics
- **MINIX**: Microkernel concepts, driver isolation
- **seL4**: Formal methods, security principles

### Especificações Técnicas

- **PCI Specification**: Device enumeration and configuration
- **ACPI Specification**: Power management and device discovery
- **x86_64 System V ABI**: Calling conventions and system calls
- **Multiboot2 Specification**: Boot protocol requirements

### Ferramentas e Build

- **XMake**: Build system configuration
- **Clang/LLD**: Toolchain C++20 freestanding
- **QEMU**: Emulation and testing environment

---

## Notas de Implementação

### Princípios Orientadores

1. **Extender vs Reescrever**: Aproveitar abstrações existentes
2. **Strategy Pattern**: Manter consistência com código existente
3. **Object Calisthenics**: Seguir regras do projeto (max 200 lines/class, etc.)
4. **BSD/XNU Alignment**: Manter coerência com arquitetura inspirada
5. **C++20 Freestanding**: Sem dependências de runtime C++

### Processo de Validação

- **Unit Tests**: Para cada nova abstração/interface
- **Integration Tests**: Verificar compatibilidade com sistemas existentes
- **Hardware Testing**: Testar em hardware real vs QEMU
- **Performance Benchmarks**: Medir impacto no sistema
- **Security Review**: Validar isolamento e capabilities

### Critérios de Conclusão

- ❌ Boot bem-sucedido em hardware real (hardcoded values remanescentes)
- ⚠️ Drivers modernos funcionando (AHCI/NVMe frameworks prontos, I/O faltando)
- ❌ DAL framework operacional (base IPC existe, interface DAL faltando)
- ⚠️ Testes abrangentes passando (frameworks testados, funcionalidades críticas faltando)
- ✅ Documentação completa atualizada (este TODO reflete status atual)

### Próximos Passos Prioritários

1. **Concluir I/O Operations**: Implementar read/write em AHCI/NVMe controllers
2. **Remover Hardcodes**: Config ports e ATA ports dinâmicos
3. **Iniciar Network Stack**: Criar interface NetworkDevice base
4. **Estender Interrupt Manager**: Implementar suporte MSI/MSI-X

---

*Este documento é um guia vivo e será atualizado conforme o progresso do desenvolvimento.*

---

*Este documento é um guia vivo e será atualizado conforme o progresso do desenvolvimento.*

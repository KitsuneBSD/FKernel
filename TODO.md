# FKernel Real Hardware Drivers - TODO List

## Status Atual (Atualizado: Janeiro 2026)

- **Alta Prioridade**: 40% completo (2/5 totalmente concluídos)
- **Média Prioridade**: 20% completo (1/9 parcialmente concluídos)  
- **Baixa Prioridade**: 0% completo (0/6 iniciados)
- **Progresso Total**: ~25% implementado

## Visão Geral

Transformar FKernel de QEMU-centric para suporte de hardware real, **estendendo abstrações existentes** ao invés de criar HAL separado. Esta abordagem mantém coerência com a arquitetura BSD/XNU inspirada e segue os princípios do projeto.

### Estratégia
- **Extender** abstrações HAL-like existentes (HardwareInterruptManager, TimerManager, etc.)
- **Aproveitar** interfaces Device/BlockDevice/StorageDevice
- **Manter** Strategy Pattern e Object Calisthenics
- **Integrar** com PciManager, MemoryManager, IPC system existentes

---

## High Priority - Infraestrutura Crítica

### 1. Remover hardcoded QEMU values - ⚠️ **PARCIALMENTE CONCLUÍDO (60%)**
**Status**: HPET dinâmico implementado, PCI config e ATA ports ainda hardcoded
**Descrição**: Substituir endereços hardcodes por detecção dinâmica via ACPI/PCI managers existentes
**Arquivos Chave**:
- ✅ `Src/Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/TimerController/hpet.cpp:16-22` (HPET descoberto via ACPI)
- ❌ `Src/Kernel/Driver/Storage/Ata/ata_controller.cpp:48-51` (ATA ports 0x1F0/0x170 ainda hardcoded)
- ❌ `Src/Kernel/Hardware/Pci/pci.cpp:8-9` (PCI config ports 0xCF8/0xCFC ainda hardcoded)
**Dependências**: ACPI table parsing, PciManager device enumeration
**Integração**: Usar `PciManager::the().get_device()` e ACPI FADT para resource discovery
**Validação**: Boot em hardware real vs QEMU, testar com diferentes configurações de hardware

### 2. Estender PciManager para Driver Matching - ✅ **CONCLUÍDO**
**Status**: Sistema completo de registro e match automático implementado
**Descrição**: Adicionar sistema de match automático drivers→dispositivos usando arquitetura PciManager existente
**Arquivos Chave**:
- ✅ `Src/Kernel/Hardware/Pci/pci.cpp:70-73` (register_driver() implementado)
- ✅ `Src/Kernel/Hardware/Pci/pci.cpp:75-94` (instantiate_drivers() implementado)
- ✅ `Src/Kernel/Hardware/Pci/pci.cpp:96-101` (auto_discover() implementado)
- ✅ `Src/Kernel/Driver/driver_registry.cpp:42-56` (template-based registry)
**Dependências**: Driver framework base, device class database
**Integração**: 
```cpp
// ✅ PciManager extension implemented
class PciDriverRegistry {
    fk::Vector<PciDriverEntry> m_drivers;
public:
    void register_driver(PciClass class_code, PciSubclass subclass, DriverFactory factory);
    DriverFactory find_driver(const PciDevice& device);
};
```
**Validação**: ✅ Testado com dispositivos ATA, VFS integration funcional

### 3. Implementar Driver Framework - ✅ **CONCLUÍDO**
**Status**: Framework completo com registro, lifecycle e VFS integration
**Descrição**: Sistema de registro/lifecycle usando interfaces Device/BlockDevice existentes como base
**Arquivos Chave**:
- ✅ `Include/Kernel/Driver/Device/block_device.h` (base interface existente)
- ✅ `Include/Kernel/Driver/Device/character_device.h` (character device base existente)
- ✅ `Src/Kernel/Driver/Device/driver_manager.cpp` (implementação completa)
**Dependências**: PciDriverRegistry, VFS integration, capability system
**Integração**:
```cpp
// ✅ Driver lifecycle management implementado
class DriverManager {
    fk::HashMap<DeviceId, OwnPtr<Device>> m_devices;
    fk::Vector<OwnPtr<Driver>> m_drivers;
public:
    Result<Device*, Error> register_device(OwnPtr<Device> device);
    void unregister_device(DeviceId id);
    void probe_all_devices();
};
```
**Validação**: ✅ Dynamic driver loading, DevFs registration automática funcional

### 4. Estender Storage Abstraction - ⚠️ **PARCIALMENTE CONCLUÍDO (40%)**
**Status**: Framework implementado, controles inicializados, mas I/O operations faltando
**Descrição**: AHCI/SATA/NVMe como novas implementações de StorageDevice interface, mantendo fallback ATA->DMA->UDMA
**Arquivos Chave**:
- ✅ `Include/Kernel/Driver/Storage/storage_device.h` (base interface existente)
- ✅ `Src/Kernel/Driver/Storage/Ata/` (implementação ATA completa)
- ⚠️ `Src/Kernel/Driver/Storage/Ahci/ahci_controller.cpp` (controle inicializado, I/O retorna NotImplemented)
- ⚠️ `Src/Kernel/Driver/Storage/Nvme/nvme_controller.cpp` (controle inicializado, I/O retorna NotImplemented)
**Dependências**: PCI device discovery, DMA abstraction, interrupt management
**Integração**:
```cpp
// ⚠️ Storage hierarchy parcialmente implementada
class AHCIController : public StorageDevice {
    // ✅ Framework pronto, ❌ I/O operations faltam
    Result<size_t, Error> read_sectors(uint64_t start, size_t count, uint8_t* buffer) override;
};

class NVMeController : public StorageDevice {
    // ✅ Framework pronto, ❌ I/O operations faltam  
    Result<size_t, Error> read_sectors(uint64_t start, size_t count, uint8_t* buffer) override;
};
```
**Validação**: ❌ Falta implementação de I/O operations para completar funcionalidade

### 5. Criar Network Device Interface - ❌ **NÃO INICIADO**
**Status**: Nenhuma implementação encontrada, diretório ausente
**Descrição**: Nova classe NetworkDevice seguindo padrão BlockDevice para drivers NIC (e1000, rtl8139)
**Arquivos Chave**:
- ❌ `Include/Kernel/Driver/Network/network_device.h` (interface não existe)
- ❌ `Src/Kernel/Driver/Network/E1000/` (diretório não existe)
- ❌ `Src/Kernel/Driver/Network/Rtl8139/` (diretório não existe)
- ❌ `Src/Kernel/Syscall/SyscallList/socket.cpp:8-12` (socket API integration ausente)
**Dependências**: PCI device management, interrupt handling, memory management
**Integração**:
```cpp
// ❌ Network device interface não implementada
class NetworkDevice : public CharacterDevice {
public:
    virtual Result<void, Error> send_packet(const uint8_t* data, size_t size) = 0;
    virtual Result<size_t, Error> receive_packet(uint8_t* buffer, size_t max_size) = 0;
    virtual MACAddress mac_address() const = 0;
};
```
**Validação**: ❌ Network stack completamente ausente

---

## Medium Priority - Expansão de Capacidades

### 6. Implementar DAL Framework - ⚠️ **BASE IMPLEMENTADA (30%)**
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

### 7. Extender HardwareInterruptManager - ⚠️ **PARCIALMENTE CONCLUÍDO (40%)**
**Status**: Basic interrupt handling existe, MSI/MSI-X e sharing faltando
**Descrição**: Adicionar MSI/MSI-X support e interrupt sharing na arquitetura existente
**Arquivos Chave**:
- ✅ `Src/Kernel/Arch/x86_64/Interrupt/interrupt_controller.cpp` (implementação básica existente)
- ❌ `Include/Kernel/Arch/x86_64/Interrupt/hardware_interrupt.h` (falta extensão MSI)
- ❌ `Src/Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/` (MSI não implementado)
**Dependências**: PCI MSI capability detection, interrupt routing
**Integração**:
```cpp
// ⚠️ Extend HardwareInterrupt interface - falta implementação
class HardwareInterrupt {
public:
    // ✅ Existing methods implementados...
    virtual Result<uint8_t, Error> allocate_msi_vector(const PciDevice& device) = 0; // ❌ Faltando
    virtual void enable_msi(uint8_t vector) = 0;                                   // ❌ Faltando
    virtual void disable_msi(uint8_t vector) = 0;                                  // ❌ Faltando
};
```
**Validação**: ❌ MSI-enabled devices não suportados, interrupt sharing ausente

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

### 9. Estender Display Abstraction - ⚠️ **PARCIALMENTE CONCLUÍDO (50%)**
**Status**: VESA implementado mas apenas como simulação
**Descrição**: VESA/VBE como nova implementação beyond VGA framebuffer existente
**Arquivos Chave**:
- ✅ `Include/Kernel/Driver/Vga/display.h` (VGA existente)
- ⚠️ `Src/Kernel/Arch/x86_64/Driver/Vga/vesa.cpp:19-20` (VESA apenas simulação)
- ❌ `Src/Kernel/Arch/x86_64/Driver/Vga/` (falta VESA BIOS extensions real)
**Dependências**: VESA BIOS calls, framebuffer management, mode switching
**Integração**:
```cpp
// ⚠️ Extend display abstraction - VESA apenas simulação
class Display {
public:
    // ✅ Existing VGA methods implementados...
    virtual Result<void, Error> set_vesa_mode(uint16_t mode) = 0;            // ⚠️ Simulação apenas
    virtual FramebufferInfo get_framebuffer_info() const = 0;               // ⚠️ Limitado
    virtual Result<void, Error> set_resolution(uint32_t width, uint32_t height, uint32_t bpp) = 0; // ⚠️ Não funcional
};
```
**Validação**: ❌ VESA real não implementado, apenas modo simulado

### 10. Enhance Memory Manager - ❌ **NÃO INICIADO**
**Status**: Memory manager ainda usa zones básicas sem topology-aware
**Descrição**: Adicionar topology-aware às zones existentes (DMA, Kernel, Userspace)
**Arquivos Chave**:
- ❌ `Src/Kernel/Memory/physical_memory_manager.cpp` (falta extensão zones)
- ❌ `Include/Kernel/Memory/physical_memory_manager.h` (falta topology info)
- ❌ `Src/Kernel/Memory/memory_manager.cpp` (falta NUMA integration)
**Dependências**: ACPI SRAT table parsing, CPU topology detection
**Integração**:
```cpp
// ❌ Extend zone-based memory - não implementado
enum class MemoryZone {
    DMA32,      // ✅ Existing
    Normal,     // ✅ Existing  
    HighMem,    // ✅ Existing
    NUMA_Node0, // ❌ New topology-aware zones faltando
    NUMA_Node1,
    // ...
};

class TopologyAwareMemoryManager {
    fk::Vector<NUMANode> m_numa_nodes;
public:
    Result<PhysicalPage, Error> allocate_page(MemoryZone zone, NUMANodeId preferred_node = NUMANodeId::Any);
};
```
**Validação**: ❌ NUMA system não suportado

### 11. Implementar Hotplug Device Detection
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

### 19. Add IOMMU Support - ❌ **NÃO INICIADO**
**Status**: Nenhuma implementação IOMMU encontrada
**Descrição**: DMA protection integrado com MemoryManager abstractions
**Arquivos Chave**:
- ❌ `Include/Kernel/Memory/iommu.h` (interface não existe)
- ❌ `Src/Kernel/Arch/x86_64/Memory/IntelIOMMU/` (diretório não existe)
- ❌ `Src/Kernel/Memory/dma_manager.cpp` (falta IOMMU integration)
**Dependências**: PCI device identification, DMA remapping, interrupt remapping
**Integração**:
```cpp
// ❌ IOMMU support não implementado
class IOMMU {
public:
    Result<void, Error> create_domain(DomainId id);
    Result<void, Error> map_device(DomainId domain, const PciDevice& device);
    Result<void, Error> set_translation(DomainId domain, PhysicalAddress input, PhysicalAddress output, size_t size);
};
```
**Validação**: ❌ DMA protection não implementada

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
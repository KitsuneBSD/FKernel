# FKernel Real Hardware Drivers - TODO List

## Visão Geral

Transformar FKernel de QEMU-centric para suporte de hardware real, **estendendo abstrações existentes** ao invés de criar HAL separado. Esta abordagem mantém coerência com a arquitetura BSD/XNU inspirada e segue os princípios do projeto.

### Estratégia
- **Extender** abstrações HAL-like existentes (HardwareInterruptManager, TimerManager, etc.)
- **Aproveitar** interfaces Device/BlockDevice/StorageDevice
- **Manter** Strategy Pattern e Object Calisthenics
- **Integrar** com PciManager, MemoryManager, IPC system existentes

---

## High Priority - Infraestrutura Crítica

### 1. Remover hardcoded QEMU values
**Descrição**: Substituir endereços hardcodes por detecção dinâmica via ACPI/PCI managers existentes
**Arquivos Chave**:
- `Src/Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/TimerController/hpet.cpp:25` (HPET base 0xFED00000)
- `Src/Kernel/Driver/Storage/Ata/ata_controller.cpp:46-47` (ATA ports 0x1F0/0x170)
- `Src/Kernel/Hardware/Pci/pci.cpp:5-6` (PCI config ports 0xCF8/0xCFC)
**Dependências**: ACPI table parsing, PciManager device enumeration
**Integração**: Usar `PciManager::the().get_device()` e ACPI FADT para resource discovery
**Validação**: Boot em hardware real vs QEMU, testar com diferentes configurações de hardware

### 2. Estender PciManager para Driver Matching
**Descrição**: Adicionar sistema de match automático drivers→dispositivos usando arquitetura PciManager existente
**Arquivos Chave**:
- `Src/Kernel/Hardware/Pci/pci_manager.cpp` (extender device scanning)
- `Include/Kernel/Hardware/Pci/pci_manager.h` (adicionar driver registry)
- `Src/Kernel/Init/init.cpp:12-13` (integration point)
**Dependências**: Driver framework base, device class database
**Integração**: 
```cpp
// PciManager extension example
class PciDriverRegistry {
    fk::Vector<PciDriverEntry> m_drivers;
public:
    void register_driver(PciClass class_code, PciSubclass subclass, DriverFactory factory);
    DriverFactory find_driver(const PciDevice& device);
};
```
**Validação**: Testar com dispositivos PCI conhecidos (ATA, network, graphics)

### 3. Implementar Driver Framework
**Descrição**: Sistema de registro/lifecycle usando interfaces Device/BlockDevice existentes como base
**Arquivos Chave**:
- `Include/Kernel/Driver/Device/block_device.h` (base interface)
- `Include/Kernel/Driver/Device/character_device.h` (character device base)
- `Src/Kernel/Init/init.cpp:11-26` (initialization pattern)
**Dependências**: PciDriverRegistry, VFS integration, capability system
**Integração**:
```cpp
// Driver lifecycle management
class DriverManager {
    fk::HashMap<DeviceId, OwnPtr<Device>> m_devices;
    fk::Vector<OwnPtr<Driver>> m_drivers;
public:
    Result<Device*, Error> register_device(OwnPtr<Device> device);
    void unregister_device(DeviceId id);
    void probe_all_devices();
};
```
**Validação**: Dynamic driver loading/unloading, hotplug support

### 4. Estender Storage Abstraction
**Descrição**: AHCI/SATA/NVMe como novas implementações de StorageDevice interface, mantendo fallback ATA->DMA->UDMA
**Arquivos Chave**:
- `Include/Kernel/Driver/Storage/storage_device.h` (base interface)
- `Src/Kernel/Driver/Storage/Ata/` (existing ATA implementation)
- `Src/Kernel/Driver/Storage/Ahci/` (new AHCI implementation)
- `Src/Kernel/Driver/Storage/Nvme/` (new NVMe implementation)
**Dependências**: PCI device discovery, DMA abstraction, interrupt management
**Integração**:
```cpp
// Storage hierarchy extension
class AHCIController : public StorageDevice {
    // AHCI-specific implementation
    Result<size_t, Error> read_sectors(uint64_t start, size_t count, uint8_t* buffer) override;
};

class NVMeController : public StorageDevice {
    // NVMe-specific implementation  
    Result<size_t, Error> read_sectors(uint64_t start, size_t count, uint8_t* buffer) override;
};
```
**Validação**: Testar com diferentes storage controllers, fallback behavior

### 5. Criar Network Device Interface
**Descrição**: Nova classe NetworkDevice seguindo padrão BlockDevice para drivers NIC (e1000, rtl8139)
**Arquivos Chave**:
- `Include/Kernel/Driver/Network/network_device.h` (new interface)
- `Src/Kernel/Driver/Network/E1000/` (Intel e1000 driver)
- `Src/Kernel/Driver/Network/Rtl8139/` (Realtek rtl8139 driver)
- `Src/Kernel/Syscall/SyscallList/socket.cpp:8-12` (socket API integration)
**Dependências**: PCI device management, interrupt handling, memory management
**Integração**:
```cpp
// Network device interface following BlockDevice pattern
class NetworkDevice : public CharacterDevice {
public:
    virtual Result<void, Error> send_packet(const uint8_t* data, size_t size) = 0;
    virtual Result<size_t, Error> receive_packet(uint8_t* buffer, size_t max_size) = 0;
    virtual MACAddress mac_address() const = 0;
};
```
**Validação**: Network packet transmission/reception, multiple NIC support

---

## Medium Priority - Expansão de Capacidades

### 6. Implementar DAL Framework
**Descrição**: Usar IPC/capability system existente para comunicação userspace drivers com kernel
**Arquivos Chave**:
- `Include/Kernel/Ipc/endpoint.h` (existing IPC)
- `Include/Kernel/Ipc/capability.h` (capability system)
- `Src/Kernel/Ipc/` (IPC implementation)
- `Src/Kernel/Syscall/` (syscall interface for DAL)
**Dependências**: Complete IPC system, security model, FFI layer
**Integração**:
```cpp
// DAL communication using existing IPC
class DALDriver {
    Endpoint m_kernel_endpoint;
    Capability m_device_capability;
public:
    Result<void, Error> register_device(DeviceType type);
    Result<void, Error> request_io(uint16_t port, size_t size);
    Result<void, Error> map_memory(PhysicalAddress phys, size_t size);
};
```
**Validação**: Userspace driver loading, device access control, isolation testing

### 7. Extender HardwareInterruptManager
**Descrição**: Adicionar MSI/MSI-X support e interrupt sharing na arquitetura existente
**Arquivos Chave**:
- `Src/Kernel/Arch/x86_64/Interrupt/interrupt_controller.cpp` (extend existing)
- `Include/Kernel/Arch/x86_64/Interrupt/hardware_interrupt.h` (interface extension)
- `Src/Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/` (MSI implementation)
**Dependências**: PCI MSI capability detection, interrupt routing
**Integração**:
```cpp
// Extend HardwareInterrupt interface
class HardwareInterrupt {
public:
    // Existing methods...
    virtual Result<uint8_t, Error> allocate_msi_vector(const PciDevice& device) = 0;
    virtual void enable_msi(uint8_t vector) = 0;
    virtual void disable_msi(uint8_t vector) = 0;
};
```
**Validação**: MSI-enabled devices, interrupt sharing, performance testing

### 8. Criar USB Host Controller Interface
**Descrição**: Nova abstração seguindo padrão HardwareInterruptManager/TimerManager
**Arquivos Chave**:
- `Include/Kernel/Driver/Usb/usb_host_controller.h` (new interface)
- `Src/Kernel/Driver/Usb/Ehci/` (EHCI implementation)
- `Src/Kernel/Driver/Usb/Xhci/` (XHCI implementation)
- `Src/Kernel/Driver/Usb/Ohci/` (OHCI implementation)
**Dependências**: PCI device discovery, interrupt management, DMA support
**Integração**:
```cpp
// USB host controller following existing manager pattern
class USBHostController {
public:
    virtual Result<void, Error> initialize() = 0;
    virtual Result<USBDevice*, Error> enumerate_device(uint8_t port) = 0;
    virtual Result<void, Error> submit_transfer(USBTransfer* transfer) = 0;
};
```
**Validação**: USB device enumeration, multiple host controllers, device compatibility

### 9. Estender Display Abstraction
**Descrição**: VESA/VBE como nova implementação beyond VGA framebuffer existente
**Arquivos Chave**:
- `Include/Kernel/Driver/Vga/display.h` (existing VGA)
- `Src/Kernel/Driver/Vga/` (extend with VESA/VBE)
- `Src/Kernel/Arch/x86_64/Driver/Vga/` (VESA BIOS extensions)
**Dependências**: VESA BIOS calls, framebuffer management, mode switching
**Integração**:
```cpp
// Extend display abstraction
class Display {
public:
    // Existing VGA methods...
    virtual Result<void, Error> set_vesa_mode(uint16_t mode) = 0;
    virtual FramebufferInfo get_framebuffer_info() const = 0;
    virtual Result<void, Error> set_resolution(uint32_t width, uint32_t height, uint32_t bpp) = 0;
};
```
**Validação**: VESA mode switching, framebuffer access, graphics compatibility

### 10. Enhance Memory Manager
**Descrição**: Adicionar topology-aware às zones existentes (DMA, Kernel, Userspace)
**Arquivos Chave**:
- `Src/Kernel/Memory/physical_memory_manager.cpp` (extend zones)
- `Include/Kernel/Memory/physical_memory_manager.h` (topology info)
- `Src/Kernel/Memory/memory_manager.cpp` (NUMA integration)
**Dependências**: ACPI SRAT table parsing, CPU topology detection
**Integração**:
```cpp
// Extend zone-based memory with topology awareness
enum class MemoryZone {
    DMA32,      // Existing
    Normal,     // Existing  
    HighMem,    // Existing
    NUMA_Node0, // New topology-aware zones
    NUMA_Node1,
    // ...
};

class TopologyAwareMemoryManager {
    fk::Vector<NUMANode> m_numa_nodes;
public:
    Result<PhysicalPage, Error> allocate_page(MemoryZone zone, NUMANodeId preferred_node = NUMANodeId::Any);
};
```
**Validação**: NUMA system testing, memory allocation locality, performance benchmarks

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

### 13. Implementar DMA Abstraction Layer
**Descrição**: Genérica DMA management integrada com MemoryManager zones
**Arquivos Chave**:
- `Include/Kernel/Memory/dma_manager.h` (new interface)
- `Src/Kernel/Memory/dma_manager.cpp` (implementation)
- `Src/Kernel/Driver/` (DMA integration for drivers)
**Dependências**: IOMMU support, memory zones, scatter-gather lists
**Integração**:
```cpp
// DMA abstraction integrated with memory management
class DMAManager {
public:
    Result<DMABuffer, Error> allocate_buffer(size_t size, DMAConstraints constraints);
    Result<void, Error> map_device_memory(PhysicalAddress phys, size_t size);
    void free_buffer(DMABuffer buffer);
};
```
**Validação**: DMA transfers, scatter-gather operations, memory coherence

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

### 15. Criar Audio Device Interface
**Descrição**: Seguindo padrão CharacterDevice existente para HDA/AC97
**Arquivos Chave**:
- `Include/Kernel/Driver/Audio/audio_device.h` (new interface)
- `Src/Kernel/Driver/Audio/Hda/` (Intel HDA driver)
- `Src/Kernel/Driver/Audio/Ac97/` (AC97 driver)
**Dependências**: PCI device management, interrupt handling, DMA support
**Integração**:
```cpp
// Audio device following CharacterDevice pattern
class AudioDevice : public CharacterDevice {
public:
    virtual Result<void, Error> set_sample_rate(uint32_t rate) = 0;
    virtual Result<void, Error> set_buffer_size(size_t size) = 0;
    virtual Result<void, Error> start_playback() = 0;
    virtual Result<void, Error> stop_playback() = 0;
};
```
**Validação**: Audio playback/recording, multiple audio devices

### 16. Estender SMP Support
**Descrição**: Multi-processor awareness usando HardwareInterruptManager base
**Arquivos Chave**:
- `Src/Kernel/Arch/x86_64/Cpu/` (SMP initialization)
- `Src/Kernel/Arch/x86_64/Interrupt/` (per-CPU interrupt handling)
- `Src/Kernel/Scheduler/` (multi-CPU scheduling)
**Dependências**: ACPI MADT table, per-CPU data structures, load balancing
**Integração**:
```cpp
// SMP support extending existing interrupt management
class SMPManager {
public:
    Result<void, Error> initialize_cpus();
    void schedule_on_cpu(CPUId cpu, Thread* thread);
    void send_ipi(CPUId target_cpu, IPIType type);
};
```
**Validação**: Multi-CPU boot, load balancing, interrupt handling

### 17. Port BSD Security Features
**Descrição**: pledge(), unveil() integrados com capability system existente
**Arquivos Chave**:
- `Src/Kernel/Security/` (new security subsystem)
- `Src/Kernel/Syscall/` (security syscalls)
- `Include/Kernel/Ipc/capability.h` (extend capabilities)
**Dependências**: Capability system, syscall filtering, process isolation
**Integração**:
```cpp
// BSD security features using existing capability system
class SecurityManager {
public:
    Result<void, Error> apply_pledge(Process* process, PledgeMask promises);
    Result<void, Error> apply_unveil(Process* process, const fk::String& path, UnveilPermissions perms);
    bool check_permission(const Process& process, Operation op, const Resource& resource);
};
```
**Validação**: Security policy enforcement, syscall filtering, process isolation

### 18. Implementar NUMA Zones
**Descrição**: Extender zone-based memory manager para topologia NUMA
**Arquivos Chave**:
- `Src/Kernel/Memory/physical_memory_manager.cpp` (NUMA zones)
- `Src/Kernel/Arch/x86_64/Acpi/` (SRAT table parsing)
- `Include/Kernel/Memory/numa_manager.h` (NUMA interface)
**Dependências**: ACPI SRAT table, CPU topology, memory locality
**Integração**:
```cpp
// NUMA zones extending existing memory management
class NUMAManager {
    fk::Vector<NUMANode> m_nodes;
public:
    Result<PhysicalPage, Error> allocate_local_page(NUMANodeId node);
    NUMANodeId get_preferred_node() const;
    void migrate_page(PhysicalPage page, NUMANodeId target_node);
};
```
**Validação**: NUMA system testing, memory locality, performance optimization

### 19. Add IOMMU Support
**Descrição**: DMA protection integrado com MemoryManager abstractions
**Arquivos Chave**:
- `Include/Kernel/Memory/iommu.h` (IOMMU interface)
- `Src/Kernel/Arch/x86_64/Memory/IntelIOMMU/` (Intel VT-d)
- `Src/Kernel/Memory/dma_manager.cpp` (IOMMU integration)
**Dependências**: PCI device identification, DMA remapping, interrupt remapping
**Integração**:
```cpp
// IOMMU support integrated with DMA management
class IOMMU {
public:
    Result<void, Error> create_domain(DomainId id);
    Result<void, Error> map_device(DomainId domain, const PciDevice& device);
    Result<void, Error> set_translation(DomainId domain, PhysicalAddress input, PhysicalAddress output, size_t size);
};
```
**Validação**: DMA protection, device isolation, security testing

### 20. Create IPUK Framework
**Descrição**: Usando VFS/capability system existentes para isolamento
**Arquivos Chave**:
- `Src/Kernel/IPUK/` (IPUK subsystem)
- `Src/Kernel/Vfs/` (VFS integration)
- `Include/Kernel/Ipc/capability.h` (capability-based isolation)
**Dependências**: VFS virtualization, capability system, process isolation
**Integração**:
```cpp
// IPUK framework using existing VFS and capabilities
class IPUKManager {
public:
    Result<IPUKInstance*, Error> create_instance(const IPUKConfig& config);
    Result<void, Error> mount_virtual_filesystem(IPUKInstance* instance, const fk::String& path);
    Result<void, Error> grant_capability(IPUKInstance* instance, Capability cap);
};
```
**Validação**: Application isolation, anti-cheat integration, security boundaries

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
- [ ] Boot bem-sucedido em hardware real
- [ ] Drivers modernos funcionando (AHCI, NVMe, USB, etc.)
- [ ] DAL framework operacional
- [ ] Testes abrangentes passando
- [ ] Documentação completa atualizada

---

*Este documento é um guia vivo e será atualizado conforme o progresso do desenvolvimento.*
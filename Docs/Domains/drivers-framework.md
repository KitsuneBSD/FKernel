# Driver Framework

## Overview

FKernel's driver framework is inspired by **FreeBSD's Newbus** -- drivers register for device classes, and the PCI subsystem matches devices to drivers automatically. The framework supports storage, network, input, and terminal devices.

## Architecture

```mermaid
flowchart TD
    subgraph "Hardware Discovery"
        ACPI["ACPI<br/>MADT, MCFG, HPET, SRAT"]
        PCI_BUS["PCI Bus<br/>ECAM (MMIO) or Legacy IO"]
    end
    subgraph "Driver Framework"
        DRM["DriverManager<br/>Register drivers + devices"]
        DR["DriverRegistry<br/>Template-based factory registration"]
        PCI_MGR["PciManager<br/>Scan, enumerate, match"]
    end
    subgraph "Drivers"
        ATA["ATA Controller<br/>Class 0x01 Sub 0x01"]
        AHCI["AHCI Controller<br/>Class 0x01 Sub 0x06"]
        NVME["NVMe Controller<br/>Class 0x01 Sub 0x08"]
        E1K["E1000 NIC<br/>Class 0x02 Sub 0x00"]
    end
    subgraph "Storage Stack"
        PART["PartitionManager<br/>GPT + MBR detection"]
        CACHE["StorageCache<br/>64-entry write-through"]
        AUTO["AutoMounter<br/>FAT12/16/32 detection"]
    end
    subgraph "VFS Integration"
        VFS["VirtualFileSystem"]
        DEVFS["DevFs<br/>/dev/* nodes"]
    end

    ACPI --> PCI_BUS
    PCI_BUS --> PCI_MGR
    PCI_MGR -->|"scan_bus()"| DRM
    DR --> PCI_MGR
    PCI_MGR -->|"instantiate_drivers()"| ATA
    PCI_MGR -->|"instantiate_drivers()"| AHCI
    PCI_MGR -->|"instantiate_drivers()"| NVME
    PCI_MGR -->|"instantiate_drivers()"| E1K
    ATA --> PART
    AHCI --> PART
    NVME --> PART
    PART --> CACHE
    CACHE --> AUTO
    AUTO --> VFS
    DRM --> DEVFS
```

## PCI Driver Matching (Newbus-inspired)

### Registration Flow

```mermaid
flowchart TD
    INIT["DriverRegistry::initialize()"]
    REG_ATA["register_pci_driver<ATA>(0x01, 0x01)<br/>Special: singleton pattern"]
    REG_AHCI["register_pci_driver<AHCI>(0x01, 0x06)<br/>Factory pattern"]
    REG_NVME["register_pci_driver<NVMe>(0x01, 0x08)<br/>Factory pattern"]
    REG_E1K["register_pci_driver<E1000>(0x02, 0x00)<br/>Factory pattern"]
    DISCOVER["PciManager::auto_discover()"]
    SCAN["scan_bus()<br/>Enumerate all PCI devices"]
    MATCH["instantiate_drivers()<br/>Match class/subclass → factory lambda"]
    CREATE["Factory creates driver instance"]
    REG_DRV["DriverManager::register_driver()"]
    REG_DEV["DriverManager::register_device()"]
    PART_SCAN["PartitionManager::scan()<br/>if StorageDevice"]
    MOUNT["AutoMounter::try_mount()<br/>if no partitions found"]

    INIT --> REG_ATA
    INIT --> REG_AHCI
    INIT --> REG_NVME
    INIT --> REG_E1K
    DISCOVER --> SCAN --> MATCH --> CREATE
    CREATE --> REG_DRV
    CREATE --> REG_DEV
    REG_DEV --> PART_SCAN --> MOUNT
```

### Device Classes

| Class | Subclass | Driver | Pattern |
|-------|----------|--------|---------|
| 0x01 (Mass Storage) | 0x01 (IDE) | ATA Controller | Singleton |
| 0x01 (Mass Storage) | 0x06 (AHCI) | AHCI Controller | Factory |
| 0x01 (Mass Storage) | 0x08 (NVMe) | NVMe Controller | Factory |
| 0x02 (Network) | 0x00 (Ethernet) | E1000 | Factory |

## Storage Stack

```mermaid
flowchart TD
    subgraph "Controller Layer"
        ATA_C["ATAController<br/>PIO/DMA, ProgIF detection<br/>Native vs Compatibility mode"]
        AHCI_C["AHCIController<br/>HBA registers, command lists<br/>FIS, PRDT, port detection"]
        NVME_C["NVMeController<br/>Admin+IO queue pairs<br/>Submission/Completion rings"]
    end
    subgraph "Partitioning"
        PM["PartitionManager<br/>GPT (LBA 1) + MBR (LBA 0)"]
        P["Partition<br/>Wraps StorageDevice<br/>offset translation"]
    end
    subgraph "Caching"
        SC["StorageCache<br/>64-entry write-through<br/>512 bytes per entry"]
    end
    subgraph "VFS Layer"
        BD["BlockDevice<br/>Extends Node<br/>Sector↔byte offset conversion"]
        FD["FileDescription<br/>Per-open-file state"]
    end

    ATA_C --> PM
    AHCI_C --> PM
    NVME_C --> PM
    PM --> P
    P --> SC
    SC --> BD
    BD --> FD
```

### ATA Strategy Pattern

```mermaid
flowchart TD
    AD["ATADevice"]
    STRAT["ATATransferStrategy<br/>(abstract)"]
    PIO["PIOStrategy<br/>Port I/O based"]
    DMA["DMAStrategy<br/>Bus mastering DMA"]

    AD -->|"owns"| STRAT
    STRAT --> PIO
    STRAT --> DMA
```

### Data Flow (Read from Disk)

```mermaid
sequenceDiagram
    participant APP as Application
    participant VFS as VirtualFileSystem
    participant FD as FileDescription
    participant P as Partition
    participant SC as StorageCache
    participant HBA as AHCI Controller

    APP->>VFS: read(fd, buf, count)
    VFS->>FD: node()->read(offset, size, buffer)
    FD->>P: read(offset, size, buffer)
    P->>P: Adjust sector by m_start_sector
    P->>SC: read_sectors(adjusted, count, buf)
    SC->>SC: Cache lookup (512B entries)
    alt Cache hit
        SC-->>P: Return cached data
    else Cache miss
        SC->>HBA: read_sectors(sector, count, buf)
        HBA->>HBA: Build command list → FIS → DMA
        HBA-->>SC: Data from disk
        SC->>SC: Store in cache
        SC-->>P: Return data
    end
    P-->>FD: Bytes read
    FD-->>VFS: Update offset atomically
    VFS-->>APP: Return to userspace
```

## Network Drivers

### E1000 (Intel Gigabit Ethernet)

- MMIO register access via BAR0
- RX/TX descriptor rings (128 entries each)
- MAC address from RAL/RAH registers (EEPROM)
- PCI bus mastering enabled for DMA
- Polling-based (interrupt handler not registered)

## Input Drivers

| Device | Interface | Device Node | Notes |
|--------|-----------|-------------|-------|
| PS/2 Mouse | IRQ12 | `/dev/mouse` | 3-byte packet decoding |
| Serial Terminal | UART polling | `/dev/ttyS0` | Read via DR bit, write via THR |
| Keyboard | IRQ1 | (internal) | PS/2 scancode set |

## Terminal Drivers

### Pseudo-Terminal (PTY)

- `PtyMaster` / `PtySlave` / `PtyBuffer`
- `SYS_OPENPTY=503` syscall for userspace allocation
- `/dev/ptmx` pseudo-device
- Blocking reads via `Notification::wait()`

### VGATerminal

- Hardware VGA text mode (80x25)
- Terminal I/O control: TCGETS/TCSETS (echo, raw mode)
- Job control: TIOCGPGRP/TIOCSPGRP/TIOCSCTTY
- Foreground process group signal delivery (Ctrl+C/Z/\)
- Window size reporting (TIOCGWINSZ)

## Hardware Discovery

```mermaid
flowchart TD
    ACPI_INIT["ACPI Subsystem"]
    RSDP["RSDP → XSDT"]
    MADT["MADT<br/>IOAPIC, LAPIC, NMIs"]
    MCFG["MCFG<br/>PCI ECAM base"]
    HPET_TBL["HPET Table<br/>Timer address"]
    SRAT["SRAT<br/>NUMA topology"]

    ACPI_INIT --> RSDP
    RSDP --> MADT
    RSDP --> MCFG
    RSDP --> HPET_TBL
    RSDP --> SRAT

    MCFG --> PCI_ECAM["PCI via ECAM (MMIO)"]
    MCFG -->|"fallback"| PCI_LEGACY["PCI via 0xCF8/0xCFC"]

    PCI_ECAM --> DEVICES["Enumerate PCI Devices"]
    PCI_LEGACY --> DEVICES
    DEVICES --> DRIVER_MATCH["Match class/subclass<br/>→ driver factory"]
```

### ACPI Status

| Table | Status | Notes |
|-------|--------|-------|
| FADT | Partial | Complete ACPI 6.x fields pending |
| HPET | ✅ | Address from ACPI table (was hardcoded) |
| MCFG | ✅ | ECAM base, legacy fallback |
| MADT | ✅ | IOAPIC, LAPIC from MSR |
| SRAT | 60% | NUMA topology parsing |
| DSDT/SSDT | ❌ | AML interpreter pending |
| DMAR | ❌ | IOMMU/VT-d pending |

## Key Design Decisions

- **Newbus-style PCI matching** over Linux's `struct pci_driver` — simpler, class-based
- **Strategy pattern**: Abstract driver interfaces with concrete implementations (ATA PIO vs DMA)
- **Dual-inheritance controllers**: AHCI/NVMe are both `Driver` and `StorageDevice`
- **Partition as StorageDevice**: Transparent offset, can be mounted directly in VFS
- **Polling-based storage** currently (interrupt-driven removed for code quality)
- **ACPI-driven discovery** over hardcoded addresses (progressive removal complete)
- **VFS integration**: All devices appear as files in `/dev`

# Hardcoded Values Removal

## Changes Made

### 1. HPET (High Precision Event Timer)
- **File:** `Src/Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/TimerController/hpet.cpp`
- **Change:** Removed hardcoded base address `0xFED00000`. Address is now obtained dynamically from the ACPI "HPET" table.
- **New Dependency:** `Include/Kernel/Hardware/Firmware/Acpi/hpet.h` (created).

### 2. PCI Manager
- **Files:**
    - `Include/Kernel/Hardware/Buses/Pci/pci.h`
    - `Src/Kernel/Hardware/Buses/Pci/pci.cpp`
- **Change:** Added support for the Enhanced Configuration Access Mechanism (ECAM) via the ACPI "MCFG" table.
- **Logic:** If the MCFG table is found, `PciManager` uses MMIO to access PCI configuration space. Otherwise, it falls back to legacy I/O ports (`0xCF8`/`0xCFC`).
- **New Dependency:** `Include/Kernel/Hardware/Firmware/Acpi/mcfg.h` (created).

### 3. ATA Controller
- **Files:**
    - `Include/Kernel/Driver/Storage/Controllers/Ata/ata_controller.h`
    - `Src/Kernel/Driver/Storage/Controllers/Ata/ata_controller.cpp`
- **Change:** Refactored device detection to prioritize PCI-discovered IDE controllers.
- **Logic:**
    - The controller now reads the PCI `ProgIF` to determine whether channels are in "Native" or "Compatibility" mode.
    - In Native mode, it uses BAR addresses.
    - In Compatibility mode (or if no PCI controller is found), it uses legacy ports (`0x1F0`, `0x3F6`, etc.).

### 4. PCI Driver Matching System
- **Files:**
    - `Include/Kernel/Hardware/Buses/Pci/pci.h`
    - `Src/Kernel/Hardware/Buses/Pci/pci.cpp`
    - `Src/Kernel/Init/init.cpp`
- **Change:** Implemented a driver registration system based on `Class Code` and `Subclass`.
- **Logic:**
    - `PciManager` now allows registering lambdas or functions as driver factories via `register_driver`.
    - The `instantiate_drivers()` method iterates all detected devices and executes matching factories.
    - This decouples `init.cpp` from driver-specific logic, enabling more modular initialization.
- **Integration:** `ATAController` was migrated to this system, registered as the driver for class `0x01` (Mass Storage) and subclass `0x01` (IDE).

## Verification

- Boot in QEMU with and without `-machine q35` (to test MCFG vs Legacy).
- Verify kernel logs for "Found HPET at physical address", "MCFG found", and ATA detection messages.

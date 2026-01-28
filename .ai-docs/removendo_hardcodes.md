# Remoção de Valores Hardcoded

## Alterações Realizadas

### 1. HPET (High Precision Event Timer)
- **Arquivo:** `Src/Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/TimerController/hpet.cpp`
- **Mudança:** Removido o endereço base hardcoded `0xFED00000`. Agora o endereço é obtido dinamicamente através da tabela ACPI "HPET".
- **Nova Dependência:** `Include/Kernel/Hardware/Acpi/hpet.h` (criado).

### 2. PCI Manager
- **Arquivos:** 
    - `Include/Kernel/Hardware/Pci/pci.h`
    - `Src/Kernel/Hardware/Pci/pci.cpp`
- **Mudança:** Adicionado suporte para o Enhanced Configuration Access Mechanism (ECAM) via tabela ACPI "MCFG".
- **Lógica:** Se a tabela MCFG for encontrada, o `PciManager` utiliza MMIO para acessar o espaço de configuração PCI. Caso contrário, mantém o fallback para as portas de IO legadas (`0xCF8`/`0xCFC`).
- **Nova Dependência:** `Include/Kernel/Hardware/Acpi/mcfg.h` (criado).

### 3. ATA Controller
- **Arquivos:**
    - `Include/Kernel/Driver/Storage/Ata/ata_controller.h`
    - `Src/Kernel/Driver/Storage/Ata/ata_controller.cpp`
- **Mudança:** Refatorada a detecção de dispositivos para priorizar controladores IDE encontrados via PCI.
- **Lógica:** 
    - O controlador agora lê o `ProgIF` do dispositivo PCI para determinar se os canais estão em modo "Native" ou "Compatibility".
    - Em modo Native, utiliza os endereços dos BARs.
    - Em modo Compatibility (ou se nenhum controlador PCI for encontrado), utiliza as portas legadas (`0x1F0`, `0x3F6`, etc.).
- **Object Calisthenics:** Removido o uso da palavra-chave `else` em conformidade com as regras do projeto.

## Validação Recomendada
- Boot em QEMU com e sem `-machine q35` (para testar MCFG vs Legado).
- Verificação de logs do kernel para mensagens "Found HPET at physical address", "MCFG found", e logs de detecção de ATA.

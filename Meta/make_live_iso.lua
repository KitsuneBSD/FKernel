-- Adicionar caminho para as bibliotecas Meta
package.path = package.path .. ";./?.lua;./Meta/Lib/?.lua"

local RunCommand = require("Meta.Lib.run_command")
local PrintMessage = require("Meta.Lib.print_message")
local OSInteract = require("Meta.Lib.os_interact")
local IsoBuilder = require("Meta.Lib.iso_builder")

-- Configurações de Caminhos
local ARTIFACTS_DIR = "build"
local KERNEL_BIN = ARTIFACTS_DIR .. "/FKernel.bin"
local INITRD_TAR = ARTIFACTS_DIR .. "/initrd.tar"
local GRUB_CFG = "Config/grub.cfg"
local ISO_ROOT = ARTIFACTS_DIR .. "/live_iso_root"
local OUTPUT_ISO = "FKernel-Live.iso"

function main()
    print("\n" .. string.rep("=", 60))
    print("          FKERNEL LIVE ISO GENERATOR (Standalone)")
    print(string.rep("=", 60) .. "\n")

    -- 1. Validar Artefatos
    if not OSInteract.FileExists(KERNEL_BIN) then
        PrintMessage(true, "Kernel binary not found at " .. KERNEL_BIN)
        print("   Hint: Run 'xmake' first to compile the kernel.")
        os.exit(1)
    end

    if not OSInteract.FileExists(INITRD_TAR) then
        PrintMessage(true, "InitRD tarball not found at " .. INITRD_TAR)
        print("   Hint: Run 'xmake build-initrd' or 'xmake' first.")
        os.exit(1)
    end

    -- 2. Limpar e Preparar Estrutura
    PrintMessage(false, "Preparing ISO directory structure...")
    RunCommand("rm -rf " .. ISO_ROOT)
    RunCommand("mkdir -p " .. ISO_ROOT .. "/boot/grub")

    -- 3. Preparar GRUB e Kernel usando a Lib do projeto
    -- Isso copia o grub.cfg para /boot/grub/ e o kernel para /boot/FKernel.bin
    if not IsoBuilder.prepare_grub(ISO_ROOT, GRUB_CFG, KERNEL_BIN) then
        PrintMessage(true, "Failed to prepare GRUB/Kernel structure.")
        os.exit(1)
    end

    -- 4. Copiar InitRD para o local esperado pelo grub.cfg
    PrintMessage(false, "Adding InitRD to ISO...")
    if not OSInteract.CopyFile(INITRD_TAR, ISO_ROOT .. "/boot/initrd.tar") then
        PrintMessage(true, "Failed to copy InitRD to ISO root.")
        os.exit(1)
    end

    -- 5. Gerar a ISO Híbrida
    PrintMessage(false, "Invoking grub-mkrescue for hybrid image generation...")
    if IsoBuilder.create_iso(ISO_ROOT, OUTPUT_ISO) then
        print("\n" .. string.rep("-", 60))
        PrintMessage(false, "SUCCESS! ISO Created: " .. OUTPUT_ISO)
        print("You can now copy this file directly to your Ventoy drive.")
        print(string.rep("-", 60) .. "\n")
    else
        PrintMessage(true, "Failed to generate ISO.")
        print("Ensure 'grub-common', 'xorriso' and 'mtools' are installed on your host.")
        os.exit(1)
    end
end

main()

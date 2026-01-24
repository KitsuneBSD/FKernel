local RunCommand = require("Meta.Lib.run_command")
local PrintMessage = require("Meta.Lib.print_message")
local OSInteract = require("Meta.Lib.os_interact")

local IsoBuilder = {}

function IsoBuilder.prepare_grub(mockos_path, grub_cfg, kernel_bin)
    local grub_dir = mockos_path .. "/boot/grub"
    RunCommand("mkdir -p " .. grub_dir)
    
    if not OSInteract.CopyFile(grub_cfg, grub_dir .. "/grub.cfg") then return false end
    if not OSInteract.CopyFile(kernel_bin, mockos_path .. "/boot/FKernel.bin") then return false end
    
    return true
end

function IsoBuilder.create_iso(mockos_path, output_iso)
    local grub_cmd = "grub-mkrescue"
    if not OSInteract.CommandExists(grub_cmd) then
        if OSInteract.CommandExists("grub2-mkrescue") then
            grub_cmd = "grub2-mkrescue"
        else
            PrintMessage(true, "grub-mkrescue not found.")
            return false
        end
    end

    local cmd = string.format("%s %s -o %s", grub_cmd, mockos_path, output_iso)
    return RunCommand(cmd)
end

return IsoBuilder

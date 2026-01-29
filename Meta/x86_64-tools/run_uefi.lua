#!/usr/bin/env lua

require("Meta.Lib.os_interact")
require("Meta.Lib.print_message")
require("Meta.Lib.run_command")

local function RunUEFI()
  if not OSInteract.CommandExists("qemu-system-x86_64") then
    PrintMessage(true, "qemu-system-x86_64 not found.")
    return false
  end

  local iso_src = "build/FKernel-MockOS.iso"
  
  -- Ensure build
  PrintMessage(false, "Ensuring FKernel ISO is built...")
  RunCommand("xmake build FKernel")

  local ovmf = "/usr/share/edk2/x64/OVMF.4m.fd"
  if not OSInteract.FileExists(ovmf) then ovmf = "/usr/share/ovmf/OVMF.fd" end

  -- Run QEMU with the ISO using UEFI firmware
  local qemu_cmd = {
    "qemu-system-x86_64",
    "-machine q35",
    "-bios " .. ovmf,
    "-cdrom " .. iso_src,
    "-m 1G",
    "-serial stdio",
    "-no-reboot"
  }

  PrintMessage(false, "Starting FKernel UEFI via GRUB ISO...")
  return RunCommand(table.concat(qemu_cmd, " "))
end

if not ... then RunUEFI() end
return RunUEFI

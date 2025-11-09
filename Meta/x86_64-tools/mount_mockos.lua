#!/usr/bin/env lua

require("Meta.Lib.run_command")
require("Meta.Lib.print_message")
require("Meta.Lib.os_interact")
require("Meta.x86_64-tools.check-kernel")

-- Path's
local grub_dir = "build/mockos/boot/grub"
local grub_cfg = "Config/grub.cfg"
local kernel_bin = "build/FKernel.bin"

-- Cleaning build directory
RunCommand("rm -rf build/mockos")

-- Ensure grub directory exists, cleaning if necessary
if DirExists(grub_dir) then
	RunCommand("rm -rf " .. grub_dir)
end

RunCommand("mkdir -p " .. grub_dir)
PrintMessage(false, "Prepared grub directory")

-- Build the MockOS infrastructure
if RunCommand(string.format("cp %s %s", grub_cfg, grub_dir)) then
	PrintMessage(false, "Copied grub config")
else
	PrintMessage(true, "Failed to copy grub config")
end

if RunCommand(string.format("cp %s build/mockos/boot", kernel_bin)) then
	PrintMessage(false, "Copied kernel binary")
else
	PrintMessage(true, "Failed to copy kernel binary")
end

if not RunCommand("command -v grub-mkrescue >/dev/null 2>&1") then
	if not RunCommand("command -v grub2-mkrescue >/dev/null 2>&1") then
		PrintMessage(true, "grub-mkrescue not found. Please install it before running this script.")
		os.exit(1)
	else
		RunCommand("grub2-mkrescue /usr/lib/grub/i386-pc/ -o build/FKernel-MockOS.iso build/mockos >/dev/null 2>&1 ")
	end
else
	RunCommand("grub-mkrescue /usr/lib/grub/i386-pc/ -o build/FKernel-MockOS.iso build/mockos  >/dev/null 2>&1")
end

if not FileExists("build/FKernel-HDA.qcow2") then
	RunCommand("qemu-img create FKernel-HDA.qcow2 4G -f qcow2 >/dev/null 2>&1 ")
	RunCommand("mv FKernel-HDA.qcow2 build/FKernel-HDA.qcow2")
	PrintMessage(false, "Disk image created")
end

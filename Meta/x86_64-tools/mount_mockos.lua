#!/usr/bin/env lua

require("Meta.Lib.run_command")
require("Meta.Lib.print_message")
require("Meta.Lib.os_interact")
require("Meta.x86_64-tools.check-kernel")

local grub_dir = "build/mockos/boot/grub"
local grub_cfg = "Config/grub.cfg"
local kernel_bin = "build/FKernel.bin"
local fkernel_mockos = "build/FKernel-MockOS.iso"
local hda_path = "build/FKernel-HDA.raw"

local function InitDiskGPTFAT32(disk)
	PrintMessage(false, "Initializing GPT + FAT32...")

	RunCommand("parted -s " .. disk .. " mklabel gpt")

	RunCommand("parted -s " .. disk .. " mkpart EFI fat32 1MiB 512MiB")
	RunCommand("parted -s " .. disk .. " set 1 esp on")

	RunCommand("mkfs.fat -F32 " .. disk)

	PrintMessage(false, "RAW disk formatted with GPT + FAT32")
end

RunCommand("rm -rf build/mockos")
RunCommand("mkdir -p " .. grub_dir)
PrintMessage(false, "Prepared grub directory")

if CopyFile(grub_cfg, grub_dir .. "/grub.cfg") then
	PrintMessage(false, "Copied grub config")
else
	PrintMessage(true, "Failed to copy grub config")
end

if CopyFile(kernel_bin, "build/mockos/boot/FKernel.bin") then
	PrintMessage(false, "Copied kernel binary")
else
	PrintMessage(true, "Failed to copy kernel binary")
end

local grub_cmd = "grub-mkrescue"
if not CommandExists(grub_cmd) then
	if CommandExists("grub2-mkrescue") then
		grub_cmd = "grub2-mkrescue"
	else
		PrintMessage(true, "grub-mkrescue not found. Please install it.")
		os.exit(1)
	end
end

RunCommand(string.format("%s build/mockos -o %s", grub_cmd, fkernel_mockos))

if FileExists(fkernel_mockos) then
	PrintMessage(false, "ISO created successfully")
else
	PrintMessage(true, "Failed to create ISO")
	os.exit(1)
end

if not FileExists(hda_path) then
	RunCommand("qemu-img create -f raw " .. hda_path .. " 4G >/dev/null 2>&1")
	PrintMessage(false, "Disk image (RAW) created")
	InitDiskGPTFAT32(hda_path)
end

PrintMessage(false, "FKernel build completed")

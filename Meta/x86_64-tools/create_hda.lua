#!/usr/bin/env lua

local RunCommand = require("Meta.Lib.run_command")
local PrintMessage = require("Meta.Lib.print_message")

local IMG_RAW = "build/FKernel-HDA.img"
local IMG_QCOW2 = "build/FKernel-HDA.qcow2"
local SIZE = "512M"

PrintMessage(false, "Creating raw disk image (" .. SIZE .. ")...")
RunCommand("rm -f " .. IMG_RAW .. " " .. IMG_QCOW2)
if not RunCommand("qemu-img create -f raw " .. IMG_RAW .. " " .. SIZE) then
    PrintMessage(true, "Failed to create raw image.")
    os.exit(1)
end

PrintMessage(false, "Partitioning with MBR...")
-- Create one FAT32 partition starting at 1MB (sector 2048)
-- type=c (W95 FAT32 LBA)
local sfdisk_input = "label: dos\nlabel-id: 0xdeadbeef\ndevice: " .. IMG_RAW .. "\nunit: sectors\n\n1: start=2048, type=c, bootable"
local f = io.popen("sfdisk " .. IMG_RAW, "w")
f:write(sfdisk_input)
f:close()

PrintMessage(false, "Formatting partition as FAT32...")
-- mformat -i <file>@@<offset>
-- -F: FAT32
-- -v: Label
if not RunCommand("mformat -i " .. IMG_RAW .. "@@1M -F -v FKERNEL") then
    PrintMessage(true, "Failed to format partition. Ensure 'mtools' is installed.")
    os.exit(1)
end

-- Just for testing, let's put a dummy file there using mcopy
PrintMessage(false, "Adding a test file to the image...")
local tmp_file = "build/hello.txt"
local tf = io.open(tmp_file, "w")
tf:write("Hello from FKernel FAT32 Image!\n")
tf:close()
RunCommand("mcopy -i " .. IMG_RAW .. "@@1M " .. tmp_file .. " ::hello.txt")

PrintMessage(false, "Converting to QCOW2...")
if not RunCommand("qemu-img convert -f raw -O qcow2 " .. IMG_RAW .. " " .. IMG_QCOW2) then
    PrintMessage(true, "Failed to convert to qcow2.")
    os.exit(1)
end

RunCommand("rm -f " .. IMG_RAW .. " " .. tmp_file)
PrintMessage(false, "Disk image " .. IMG_QCOW2 .. " created and formatted successfully.")


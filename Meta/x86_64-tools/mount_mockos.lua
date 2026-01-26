#!/usr/bin/env lua

local RunCommand = require("Meta.Lib.run_command")
local PrintMessage = require("Meta.Lib.print_message")
local OSInteract = require("Meta.Lib.os_interact")
local Compiler = require("Meta.Lib.userland_compiler")
local Initrd = require("Meta.Lib.initrd_builder")
local Iso = require("Meta.Lib.iso_builder")
local Components = require("Meta.Lib.component_manager")
require("Meta.x86_64-tools.check-kernel")

local STAGING = "build/initrd_root"
local MOCKOS = "build/mockos"
local CONFIG_FILE = "build/initrd_config.txt"
local ONLY_INITRD = arg[1] == "--only-initrd"

Initrd.setup_staging(STAGING)
RunCommand("mkdir -p build/userland/obj")

Compiler.compile_asm("Src/Userland/lib/syscalls.asm", "build/userland/obj/syscalls.o")
Compiler.compile_asm("Src/Userland/lib/crt0.asm", "build/userland/obj/crt0.o")

local config = { COMPONENTS = "init,shell,ls,cat,uname,clear", SYSTEM_TYPE = "minimal" }
local f = io.open(CONFIG_FILE, "r")
if f then
  for line in f:lines() do
    local k, v = line:match("([^=]+)=(.+)")
    if k then config[k] = v end
  end
  f:close()
end

if config.SYSTEM_TYPE == "standard" or config.SYSTEM_TYPE == "advanced" then
  PrintMessage(false, "Building Standard System (Musl + BusyBox)...")
  if not RunCommand("lua Meta/UserTools/musl/build.lua") then
    PrintMessage(true, "Failed to build Musl LibC.")
    os.exit(1)
  end
  
  -- Copy Musl libraries to initrd
  RunCommand("mkdir -p " .. STAGING .. "/lib")
  RunCommand("mkdir -p " .. STAGING .. "/usr/lib")
  RunCommand("cp -r build/sysroot/lib/* " .. STAGING .. "/lib/ 2>/dev/null || true")
  RunCommand("cp -r build/sysroot/include " .. STAGING .. "/usr/ 2>/dev/null || true")

  if not RunCommand("lua Meta/UserTools/busybox/build.lua") then
    PrintMessage(true, "Failed to build BusyBox.")
    os.exit(1)
  end

  if config.SYSTEM_TYPE == "advanced" then
    PrintMessage(false, "Building Advanced Components (OpenRC)...")
    if RunCommand("lua Meta/UserTools/openrc/build.lua") then
      -- Verify OpenRC was built successfully
      if OSInteract.FileExists(STAGING .. "/sbin/init.openrc") then
        PrintMessage(false, "Setting OpenRC as default init...")
        RunCommand("rm -f " .. STAGING .. "/sbin/init")  -- Remove old symlink
        RunCommand("ln -sf /sbin/init.openrc " .. STAGING .. "/sbin/init")
        PrintMessage(false, "OpenRC successfully configured as init")
      else
        PrintMessage(true, "OpenRC build succeeded but init.openrc not found")
        -- Fall back to BusyBox
        RunCommand("ln -sf /bin/busybox " .. STAGING .. "/sbin/init")
      end
    else
      PrintMessage(true, "Failed to build OpenRC. Falling back to BusyBox init.")
      -- Ensure BusyBox init is available
      RunCommand("ln -sf /bin/busybox " .. STAGING .. "/sbin/init")
    end
  end
end

for comp in (config.COMPONENTS or ""):gmatch("([^,]+)") do
  if comp ~= "" then
    local skip = false
    if (config.SYSTEM_TYPE == "standard" or config.SYSTEM_TYPE == "advanced") and 
       (comp == "init" or comp == "ash") then
      skip = true
      PrintMessage(false, "Skipping native '" .. comp .. "' (provided by BusyBox).")
    end

    if not skip then
      Components.build(comp, config.SYSTEM_TYPE, STAGING)
    end
  end
end

local tar_output = "build/initrd.tar"
RunCommand("ln -sf shell " .. STAGING .. "/bin/sh")
Initrd.pack_tar(STAGING, tar_output)

if ONLY_INITRD then
  PrintMessage(false, "Standalone initrd.tar generated at build/initrd.tar")
  os.exit(0)
end

RunCommand("rm -rf " .. MOCKOS)
if Iso.prepare_grub(MOCKOS, "Config/grub.cfg", "build/FKernel.bin") then
  -- Move the already generated tar to the iso folder
  RunCommand("cp " .. tar_output .. " " .. MOCKOS .. "/boot/initrd.tar")
  if Iso.create_iso(MOCKOS, "build/FKernel-MockOS.iso") then
    PrintMessage(false, "System image created successfully.")
  end
end

if not OSInteract.FileExists("build/FKernel-HDA.qcow2") then
  RunCommand("qemu-img create -f qcow2 build/FKernel-HDA.qcow2 4G >/dev/null 2>&1")
end

PrintMessage(false, "FKernel build process completed.")

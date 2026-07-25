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
local TAR_OUTPUT = "build/initrd.tar"
local ONLY_INITRD = arg[1] == "--only-initrd"

-- Read config before the staleness check (system type affects what to track)
local config = { COMPONENTS = "init,shell,ls,cat,uname,clear", SYSTEM_TYPE = "minimal" }
local f = io.open(CONFIG_FILE, "r")
if f then
  for line in f:lines() do
    local k, v = line:match("([^=]+)=(.+)")
    if k then config[k] = v end
  end
  f:close()
end

local function exec_ok(cmd)
  local rc = os.execute(cmd)
  return rc == true or rc == 0
end

local function initrd_needs_rebuild()
  if not OSInteract.FileExists(TAR_OUTPUT) then return true end
  -- Any userland source file newer than the tar?
  if exec_ok(
    "find Src/Userland \\( -name '*.c' -o -name '*.asm' \\) -newer " ..
    TAR_OUTPUT .. " -print -quit 2>/dev/null | grep -q ."
  ) then return true end
  -- Config file changed?
  if OSInteract.FileExists(CONFIG_FILE) and
     exec_ok("[ '" .. CONFIG_FILE .. "' -nt '" .. TAR_OUTPUT .. "' ]") then
    return true
  end
  -- For standard/advanced builds, also track the BusyBox binary
  if config.SYSTEM_TYPE == "standard" or config.SYSTEM_TYPE == "advanced" then
    local bb = "build/sysroot/bin/busybox"
    if OSInteract.FileExists(bb) and exec_ok("[ '" .. bb .. "' -nt '" .. TAR_OUTPUT .. "' ]") then
      return true
    end
  end
  return false
end

if initrd_needs_rebuild() then
  Initrd.setup_staging(STAGING)
  RunCommand("mkdir -p build/userland/obj")

  Compiler.compile_asm("Src/Userland/lib/syscalls.asm", "build/userland/obj/syscalls.o")
  Compiler.compile_asm("Src/Userland/lib/crt0.asm", "build/userland/obj/crt0.o")

  if config.SYSTEM_TYPE == "standard" or config.SYSTEM_TYPE == "advanced" then
    PrintMessage(false, "Building Standard System (Musl + BusyBox)...")
    if not RunCommand("lua Meta/UserTools/musl/build.lua") then
      PrintMessage(true, "Failed to build Musl LibC.")
      os.exit(1)
    end

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
        if OSInteract.FileExists(STAGING .. "/sbin/init.openrc") then
          PrintMessage(false, "Setting OpenRC as default init...")
          RunCommand("rm -f " .. STAGING .. "/sbin/init")
          RunCommand("ln -sf /sbin/init.openrc " .. STAGING .. "/sbin/init")
          PrintMessage(false, "OpenRC successfully configured as init")
        else
          PrintMessage(true, "OpenRC build succeeded but init.openrc not found")
          RunCommand("ln -sf /bin/busybox " .. STAGING .. "/sbin/init")
        end
      else
        PrintMessage(true, "Failed to build OpenRC. Falling back to BusyBox init.")
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

  if config.SYSTEM_TYPE ~= "standard" and config.SYSTEM_TYPE ~= "advanced" then
    RunCommand("ln -sf shell " .. STAGING .. "/bin/sh")
  end

  -- Always build native test suite if source exists
  if io.open("Src/Userland/ktest/main.c", "r") then
    PrintMessage(false, "Building ktest (native kernel test suite)...")
    Components.build("ktest", config.SYSTEM_TYPE, STAGING)
  end

  Initrd.pack_tar(STAGING, TAR_OUTPUT)
else
  PrintMessage(false, "initrd is up-to-date, skipping rebuild.")
end

if ONLY_INITRD then
  PrintMessage(false, "Standalone initrd.tar generated at build/initrd.tar")
  os.exit(0)
end

RunCommand("rm -rf " .. MOCKOS)
if Iso.prepare_grub(MOCKOS, "Config/grub.cfg", "build/FKernel.bin") then
  RunCommand("cp " .. TAR_OUTPUT .. " " .. MOCKOS .. "/boot/initrd.tar")
  if Iso.create_iso(MOCKOS, "build/FKernel-MockOS.iso") then
    PrintMessage(false, "System image created successfully.")
  end
end

if not OSInteract.FileExists("build/FKernel-HDA.qcow2") then
  PrintMessage(false, "Creating partitioned disk image...")
  if not RunCommand("lua Meta/x86_64-tools/create_hda.lua") then
    PrintMessage(true, "create_hda.lua failed — creating blank fallback image")
    RunCommand("qemu-img create -f qcow2 build/FKernel-HDA.qcow2 4G >/dev/null 2>&1")
  end
end

PrintMessage(false, "FKernel build process completed.")

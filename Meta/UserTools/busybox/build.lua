#!/usr/bin/env lua

-- Script de construção do BusyBox para FKernel
local ArchiveUtils = require("Meta.Lib.archive_utils")
local BuildUtils = require("Meta.Lib.build_utils")
local Toolchain = require("Meta.Lib.toolchain")
local OSInteract = require("Meta.Lib.os_interact")
local PrintMessage = require("Meta.Lib.print_message")

local BB_VERSION = "1.36.1"
local BB_NAME = "busybox-" .. BB_VERSION
local BB_URL = "https://busybox.net/downloads/" .. BB_NAME .. ".tar.bz2"
local ROOT_DIR = os.getenv("PWD")
local BUILD_DIR = ROOT_DIR .. "/build/userland/busybox"
local SYSROOT = ROOT_DIR .. "/build/sysroot"
local INITRD_DIR = ROOT_DIR .. "/build/initrd_root"

print("---" .. " FKernel BusyBox Toolchain ---")

local function mlink(target, link)
  os.execute(string.format("ln -sf %s %s/%s", target, INITRD_DIR, link))
end

local function create_etc_configs()
  print("Creating basic /etc configurations...")
  os.execute("mkdir -p " .. INITRD_DIR .. "/etc/init.d")

  -- Create /etc/inittab
  local inittab = io.open(INITRD_DIR .. "/etc/inittab", "w")
  if inittab then
      inittab:write("::sysinit:/etc/init.d/rcS\n")
      inittab:write("::respawn:-/bin/sh\n")
      inittab:close()
  end

  -- Create /etc/init.d/rcS
  local rcs = io.open(INITRD_DIR .. "/etc/init.d/rcS", "w")
  if rcs then
    rcs:write("#!/bin/sh\n")
    rcs:write("export PATH=/bin:/sbin:/usr/bin:/usr/sbin\n")
    rcs:write("echo \"FKernel standard system booted successfully.\"\n")
    rcs:close()
    os.execute("chmod +x " .. INITRD_DIR .. "/etc/init.d/rcS")
  end

  -- Create /etc/profile
  local profile = io.open(INITRD_DIR .. "/etc/profile", "w")
  if profile then
    profile:write("export PATH=/bin:/sbin:/usr/bin:/usr/sbin\n")
    profile:write("export PS1='# ' \n")
    profile:write("export TERM=vt100\n")
    profile:close()
  end
end

local function create_all_links()
  print("Creating safe symlinks for initrd...")
  create_etc_configs()
  
  local src_dir = BUILD_DIR .. "/" .. BB_NAME
  -- Use the freshly built binary to list applets
  os.execute(src_dir .. "/busybox --list > /tmp/bb_applets.txt")
  
  local f = io.open("/tmp/bb_applets.txt", "r")
  if f then
    for line in f:lines() do
      if line ~= "busybox" then
        mlink("../usr/bin/busybox", "bin/" .. line)
      end
    end
    f:close()
  end
  
  -- Extra safety links
  mlink("../usr/bin/busybox", "bin/sh")
  mlink("../usr/bin/busybox", "sbin/init")
end

local src_dir = BUILD_DIR .. "/" .. BB_NAME
local lock_file = BUILD_DIR .. "/.build.lock"

-- Prevent simultaneous builds
if OSInteract.FileExists(lock_file) then
  PrintMessage(true, "BusyBox build is already in progress (lock found). Skipping.")
  os.exit(0)
end

-- Create lock
os.execute("mkdir -p " .. BUILD_DIR)
os.execute("touch " .. lock_file)

os.execute("mkdir -p " .. INITRD_DIR .. "/usr/bin")
os.execute("mkdir -p " .. INITRD_DIR .. "/bin")
os.execute("mkdir -p " .. INITRD_DIR .. "/sbin")

local tarball = BUILD_DIR .. "/busybox.tar.bz2"
ArchiveUtils.download(BB_URL, tarball)
ArchiveUtils.extract(tarball, BUILD_DIR)

local function ensure_config_set(config_file, key)
  local sed_cmd1 = string.format("sed -i 's/^# %s is not set/%s=y/' %s", key, key, config_file)
  os.execute(sed_cmd1)
  local sed_cmd2 = string.format("sed -i 's/^%s=n/%s=y/' %s", key, key, config_file)
  os.execute(sed_cmd2)
end

-- Start from allnoconfig
print("Generating minimal configuration...")
os.execute("make -C " .. src_dir .. " allnoconfig")

local config_file = src_dir .. "/.config"
print("Enabling essential applets and installer...")

local enable_list = {
  "CONFIG_BUSYBOX",
  "CONFIG_FEATURE_INSTALLER",
  "CONFIG_ASH",
  "CONFIG_SH_IS_ASH",
  "CONFIG_INIT",
  "CONFIG_FEATURE_USE_INITTAB",
  "CONFIG_CAT",
  "CONFIG_RM",
  "CONFIG_LS",
  "CONFIG_CP",
  "CONFIG_MV",
  "CONFIG_MKDIR",
  "CONFIG_CHMOD",
  "CONFIG_CHOWN",
  "CONFIG_LN",
  "CONFIG_SLEEP",
  "CONFIG_UNAME",
  "CONFIG_ID",
  "CONFIG_WHOAMI",
  "CONFIG_ENV",
  "CONFIG_TOUCH",
  "CONFIG_TAIL",
  "CONFIG_HEAD",
  "CONFIG_WC",
  "CONFIG_BASENAME",
  "CONFIG_DIRNAME",
  "CONFIG_TRUE",
  "CONFIG_FALSE",
  "CONFIG_ECHO",
  "CONFIG_CLEAR",
  "CONFIG_SYNC",
  "CONFIG_KILL",
  "CONFIG_PIE",
  -- Features
  "CONFIG_FEATURE_LS_FILETYPES",
  "CONFIG_FEATURE_LS_FOLLOWLINKS",
  "CONFIG_FEATURE_LS_RECURSIVE",
  "CONFIG_FEATURE_LS_WIDTH",
  "CONFIG_FEATURE_LS_SORTFILES",
  "CONFIG_FEATURE_LS_TIMESTAMPS",
  "CONFIG_FEATURE_LS_USERNAME",
  "CONFIG_FEATURE_LS_COLOR",
  "CONFIG_SHOW_USAGE"
}

for _, cfg in ipairs(enable_list) do
  ensure_config_set(config_file, cfg)
end

os.execute("make -C " .. src_dir .. " oldconfig")

print("Compiling BusyBox...")
local CC = Toolchain.get_clang()
local cflags = string.format("--sysroot=%s -isystem %s/include --target=%s", SYSROOT, SYSROOT,
  Toolchain.TRIPLE)
local ldflags = string.format("-static --sysroot=%s -L%s/lib", SYSROOT, SYSROOT)

local make_args = {
  string.format("CC='%s'", CC),
  string.format("EXTRA_CFLAGS='%s'", cflags),
  string.format("EXTRA_LDFLAGS='%s'", ldflags)
}

if BuildUtils.make(src_dir, make_args) then
  os.execute("cp " .. src_dir .. "/busybox " .. INITRD_DIR .. "/usr/bin/")
  create_all_links()
  os.execute("rm -f " .. lock_file)
  PrintMessage(false, "BusyBox installed safely.")
else
  os.execute("rm -f " .. lock_file)
  os.exit(1)
end
